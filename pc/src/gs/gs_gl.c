/*
 * GS rasterization on OpenGL 3.3.
 *
 * Render targets: one per framebuffer (FBP/FBW/PSM) the game draws to, rendered
 * at `scale` x native resolution. Colors are stored as RGBA16F with rgb = C/255
 * and a = A/128, so GS blend factors (A/128, FIX/128, up to ~2.0) map directly
 * to GL blend factors without clamping.
 *
 * Textures: decoded from emulated GS memory (gs_mem.c) and cached per TEX0/CLUT,
 * invalidated by page write generations. A texture whose base pointer is a render
 * target samples that render target directly (render-to-texture effects, the
 * frame-blend copy in PS2_swap, ...).
 */
#include "gs.h"
#include "native.h"
#include "textures.h"
#include "texreplace.h"
#include "../host/prof.h"
#include "stream.h"
#include "../ps2_platform.h"
#include "../render/gl_loader.h"
#include "../host/xr.h"
#include "../movie/movie.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern SDL_Window* g_window;

/* ---------------- configuration ---------------- */
static float g_scale = 2.0f;        /* internal resolution multiplier */
static int   g_margin_x = 0;        /* extra GS pixels on each side (widescreen) */

/* ---------------- render targets ---------------- */
static int psm_is16(uint32_t psm) { return psm == PSMCT16 || psm == PSMCT16S || psm == PSMZ16 || psm == PSMZ16S; }

#define MAX_RT 16
#ifdef __ANDROID__
static void and_foveate(GLuint tex);   /* (foveated render targets, below) */
static int and_fov_next;
#endif
typedef struct {
    int used;
    uint32_t fbp, fbw, psm;
    int vw, vh;            /* virtual GS size covered */
    int pw, ph;            /* physical texture size */
    GLuint tex, fbo, depth;
    uint32_t zbp;
    uint64_t last_use;
    int stale;             /* GS memory under it was overwritten by a transfer */
    int pending_reload;    /* reload from GS memory before the next draw unless it is fully overwritten */
    int has_box, bx0, by0, bx1, by1;   /* area drawn since it last mirrored GS memory (GS pixels);
                                          outside it GS memory is the authoritative copy */
} RenderTarget;
static RenderTarget rts[MAX_RT];
/* what the executed frame did first to each target (replay targets, below) */
enum { FIRST_NONE, FIRST_COVER, FIRST_READ };
static struct { int how, x0, y0, x1, y1; } first[MAX_RT];   /* COVER: area overwritten (physical pixels, x0..x1-1) */
static uint64_t frame_no;
static uint32_t stat_prims, stat_flushes, stat_reloads, stat_decodes, stat_decode_px;
#ifdef __ANDROID__
AndStall g_and_stall;
/* render.c: this game frame's drawing is timed part by part (1 in 30) - reading the clock is a kernel call on the
   Quest: ~13 reads a flush were 12-13% of the render thread */
int g_and_time_frame;
extern volatile int g_and_sub;   /* android_sampler.c: the stage of gs_flush_ a profiler sample falls in */
typedef void* (GLAPIENTRY *PFN_and_map)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);   /* GLES 3: glMapBufferRange */
typedef GLboolean (GLAPIENTRY *PFN_and_unmap)(GLenum target);
/* Android (Quest): the vertex stream in a buffer mapped once for good (GL_EXT_buffer_storage, persistent + coherent) -
   the uploads are plain copies, no driver call (glBufferSubData and per-batch mapping both waited in the driver:
   20-110 ms a game frame in the log). The ring is used in quarters; entering one waits for the fence left when the
   GPU's draws from it were last issued (a full turn earlier: tens of frames). */
static uint8_t* and_pmap;
#define AND_QV (VBO_VERTS / 4)
static void* and_fence[4];
static void* (GLAPIENTRY *and_fencesync)(GLenum, GLbitfield);
static GLenum (GLAPIENTRY *and_clientwait)(void*, GLbitfield, uint64_t);
static void (GLAPIENTRY *and_deletesync)(void*);
static unsigned and_waits; static uint64_t and_wait_ns;   /* (the ring's waits: logged when one is long) */
static void and_persistent_init(void);
volatile int g_and_fps_headset, g_and_fps_game;   /* render.c, once a second: images to the headset, game frames */
int g_and_ab;   /* render.c: A/B test mode - bit 0 no window swap, bit 1 no per-eye glFlush */
int g_vr_one_replay = 0;   /* off: on the Quest it was not faster (38.7 vs 44 images/s) and grates/glass went missing - the right eye re-issues the left eye's recorded GL calls (gl_record.c); 0: both eyes replay */
/* Android (Quest): the frame rates in the picture (the owner follows them in the headset) - two numbers drawn with
   scissored clears (no shader, no font): images the headset got per second, game frames per second. Green: full
   rate, yellow: some lost, red: slow. x, y: lower left in the bound framebuffer; h: its height (the size) */
static void and_text(int x, int y, int px, const char* str, float r, float g, float b)   /* 3x5 glyphs by scissored clears */
{
    static const char chars[] = "0123456789.CG%";
    static const uint16_t font[] = { 0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9, 0x79CF, 0x79EF, 0x7249, 0x7BEF, 0x7BCF,
                                     0x0002, 0x7927, 0x796F, 0x52A5 };
    glClearColor(r, g, b, 1);
    for (int cx = x; *str; str++, cx += 4 * px) {
        const char* f = strchr(chars, *str);
        if (!f || !*str) continue;
        for (int rr = 0; rr < 5; rr++)
            for (int c = 0; c < 3; c++)
                if (font[f - chars] >> (14 - rr * 3 - c) & 1) { glScissor(cx + c * px, y + (4 - rr) * px, px, px); glClear(GL_COLOR_BUFFER_BIT); }
    }
}
static void and_fps_draw(int x, int y, int h)   /* (the CPU/GPU lines were taken out again: the owner felt them cost) */
{
    int px = h / 170 > 2 ? h / 170 : 2;
    char l0[16], l1[16];
    int hv = g_and_fps_headset, gv = g_and_fps_game, n0, n1;
    snprintf(l0, sizeof(l0), "%d", hv);
    snprintf(l1, sizeof(l1), "%d", gv);
    n0 = (int)strlen(l0); n1 = (int)strlen(l1);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x - px, y - px, (n0 + 1 + n1) * 4 * px + px, 7 * px);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    {   /* frame rates: green full, yellow some lost, red slow */
        int good = hv >= 65, ok = hv >= 45;
        and_text(x, y, px, l0, good ? 0.2f : 1.0f, good || ok ? 1.0f : 0.25f, 0.2f);
        good = gv >= 28; ok = gv >= 20;
        and_text(x + (n0 + 1) * 4 * px, y, px, l1, good ? 0.2f : 1.0f, good || ok ? 1.0f : 0.25f, 0.2f);
    }
    glDisable(GL_SCISSOR_TEST);
}
#endif
uint64_t gs_frame_count(void) { extern uint64_t stream_game_frames(void); return stream_game_frames(); }

/* ---------------- texture cache ---------------- */
#define MAX_TEX 512
typedef struct {
    int used;
    uint64_t tex0_key, texa;
    uint32_t clut_hash;
    uint32_t page0, page1;
    uint64_t content;   /* hash of the GS pages the texture spans */
    int w, h;
    GLuint tex;
    uint64_t last_use;
} TexEntry;
static TexEntry texcache[MAX_TEX];
/* texture cache is content-addressed: games stream different textures through the same
   VRAM slots, so entries are keyed on a hash of the pages they cover */
static uint64_t page_hash[512];
static uint8_t page_dirty[512];
static int page_hash_init;

static uint64_t page_hash_get(uint32_t p)
{
    if (!page_hash_init) { memset(page_dirty, 1, sizeof(page_dirty)); page_hash_init = 1; }
    if (page_dirty[p]) {
#ifdef __ANDROID__
        extern uint64_t gs_page_hash(uint32_t p);
        uint64_t h = gs_page_hash(p);   /* (gs_mem.c: from the 256-byte blocks' hashes, made where uploads assemble them) */
#elif 0
        /* Android (Quest): 32-bit multiplies in 4 independent lanes - the 64-bit multiply per word is several instructions
           on 32-bit ARM, and every page an upload changed is hashed again at its next texture lookup (the texture stage:
           33-42% of a game frame). 1.5x faster on ARM (qemu); every single-byte change of a page changes it. */
        const uint32_t* w = (const uint32_t*)(gs_mem + (size_t)p * 8192);
        uint32_t ha = 0x9E3779B1u ^ p, hb = 0x85EBCA77u, hc = 0xC2B2AE3Du, hd = 0x27D4EB2Fu ^ (p * 0x01000193u);
#define AROT(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
        for (int i = 0; i < 2048; i += 4) {
            ha = (ha ^ w[i]) * 0x9E3779B1u; ha = AROT(ha, 13);
            hb = (hb ^ w[i + 1]) * 0x85EBCA77u; hb = AROT(hb, 13);
            hc = (hc ^ w[i + 2]) * 0xC2B2AE3Du; hc = AROT(hc, 13);
            hd = (hd ^ w[i + 3]) * 0x27D4EB2Fu; hd = AROT(hd, 13);
        }
        ha ^= AROT(hc, 7) * 0x9E3779B1u; hb ^= AROT(hd, 11) * 0x85EBCA77u;
#undef AROT
        ha ^= ha >> 15; ha *= 0x2C1B3C6Du; ha ^= ha >> 12;
        hb ^= hb >> 15; hb *= 0x297A2D39u; hb ^= hb >> 12;
        uint64_t h = ((uint64_t)ha << 32 | hb) ^ ((uint64_t)hc << 17) ^ hd;
#else
        const uint64_t* w = (const uint64_t*)(gs_mem + (size_t)p * 8192);
        uint64_t h = 0x9E3779B97F4A7C15ull ^ p;
        for (int i = 0; i < 1024; i++) { h ^= w[i]; h *= 0x100000001B3ull; h ^= h >> 29; }
#endif
        page_hash[p] = h;
        page_dirty[p] = 0;
    }
    return page_hash[p];
}

/* ---------------- batching ---------------- */
#define GROW_TO(ptr, need, cap) do { if ((need) > (cap)) { (cap) = (need) * 2 + 1024; (ptr) = realloc((ptr), (cap) * sizeof(*(ptr))); \
                                       if (!(ptr)) host_fatal("gs: out of memory (%zu entries)", (size_t)(cap)); } } while (0)
typedef struct {
    float x, y, z, w;     /* GS pixel position and depth; homogeneous (times w) for native 3D, else w = 1 */
    float s, t, q;
    float r, g, b, a;
    float fog;
    float clip[2];        /* native 3D: near/far clip distances (others: always inside) */
    float vr;             /* 1: 3D in the game's view projection, VR re-projects it per eye */
} GlVertex;

#define MAX_VERTS 65536
static GlVertex vbuf[MAX_VERTS];
static int vcount;
/* vertex stream: batches are appended and the buffer orphaned when full, so a batch never
   overwrites data the GPU may still be drawing from (no implicit sync per batch) */
#define VBO_VERTS (MAX_VERTS * 8)
static int vbo_first;
/* parts of the batch drawn from the executed frame's vertices on the GPU (svbo, a replay's
   unblended native strips, gs_draw_native_cached) instead of vbuf: batch positions
   start..start+count-1 (vbuf has a gap there), static vertex first */
typedef struct { int start, count, first; } StaticRun;
static StaticRun* sruns; static int nsruns, csruns;
static GLenum batch_mode = GL_TRIANGLES;
static int batch_full_cover; /* batch is a single opaque sprite covering x 0..fbw*64, y 0..>=448 */
static int batch_sprite_first; /* batch starts with a sprite, no alpha test, no FBMSK (rt_first_draw) */

typedef struct {
    uint64_t prim;      /* attribute bits of PRIM */
    GsContext ctx;
    uint64_t texa, fogcol, pabe, colclamp;
    uint32_t epoch;     /* gs_tex_epoch when set up: a batch never spans a change (replay resources) */
} DrawState;
static DrawState batch_state;
static int batch_valid;

static GLuint prog, vao, vbo, present_prog, present_vao, svao, svbo;
/* the overlay (ImGui) sets GL state behind the GL state cache's back (Android: gl_cache.c) */
#ifdef __ANDROID__
#define overlay_render_c() do { overlay_render(); { extern void and_glcache_reset(void); and_glcache_reset(); } } while (0)
#else
#define overlay_render_c() overlay_render()
#endif
static GLint u_vr_on, u_vr_src, u_vr_dst, u_vr_m0, u_vr_m1, u_vr_m2, u_vr_m3, u_vr_zab, u_vr_zs, u_vr_hud;
/* VR re-projection of the draws being replayed for an eye (gs_present_vr); off otherwise */
static struct { int on; float src[4], dst[4], m[16], zab[2], zs, hud[3]; } vr_u;
int g_vr_hud_dist = 200;   /* ini vr_hud_dist: the panel of the game's texts and 2D, cm in front of the head */
int g_vr_hud_y = 0;        /* ini vr_hud_y: the panel moved up (+) or down (-), cm */
int g_vr_hud_angle = 42;   /* ini vr_hud_angle: its width seen from the head (degrees); 0 = drawn over the whole view as before */
/* VR diagnosis: what the first eye's replay draws, by path (logged now and then while stereo is on) */
/* path: 'N' native triangles, 'S' sprites/lines (gs_draw_raw), 'R' GS register writes (VU1 / packets) */
static struct { int on; unsigned n3d, nraw, n2d, nbig; int ns; float s[12][3]; char path[12]; } vr_st;
static void vr_stat_vertex_p(const GlVertex* o, char path)
{
    if (!vr_st.on) return;
    if (o->vr > 1.5f) vr_st.nraw++;
    else if (o->vr > 0.5f) vr_st.n3d++;
    else {
        vr_st.n2d++;
        if (vr_st.ns < 12 && (vr_st.n2d % 5) == 1) {
            vr_st.s[vr_st.ns][0] = o->x; vr_st.s[vr_st.ns][1] = o->y; vr_st.s[vr_st.ns][2] = o->z; vr_st.path[vr_st.ns] = path; vr_st.ns++;
        }
    }
}
static void vr_stat_vertex(const GlVertex* o) { vr_stat_vertex_p(o, 'N'); }
static GLint u_rt_size, u_rt_margin, u_tme, u_tfx, u_tcc, u_fst, u_texsize, u_fge, u_fogcol, u_ate, u_atst, u_aref,
             u_tex, u_tex_alpha_scale, u_texofs, u_premul, u_zscale, u_clamp_mode, u_clamp_vals, u_afail_pass, u_rt16, u_dbg;
static GLint up_tex, up_rect, up_pic;

static const char* vs_src =
    "#version 330 core\n"
    "layout(location=0) in vec4 a_pos;\n"
    "layout(location=1) in vec3 a_stq;\n"
    "layout(location=2) in vec4 a_col;\n"
    "layout(location=3) in float a_fog;\n"
    "layout(location=4) in vec2 a_clip;\n"
    "layout(location=5) in float a_vr;\n"
    "uniform vec2 u_rt_size; uniform float u_rt_margin; uniform float u_zscale;\n"
    /* VR (gs_present_vr): 3D vertices are re-projected for an eye. The game's homogeneous screen
       position (c * Vz + k * V.xy, depth * Vz, Vz) gives back its view position V; u_vr_m*
       (columns) take V to the eye's view (same axes: x right, y down, z forward), which is
       projected with the eye's frustum onto the display area (u_vr_dst: c', k'). The GS depth
       is kept, so the game's depth tests between draws behave as before. */
    "uniform int u_vr_on; uniform vec4 u_vr_src, u_vr_dst, u_vr_m0, u_vr_m1, u_vr_m2, u_vr_m3; uniform vec2 u_vr_zab; uniform float u_vr_zs; uniform vec4 u_vr_hud;\n"
    /* everything perspective-correct: for GS vertices w = 1, so that is the GS's screen-space
       interpolation; native 3D triangles stay whole where VU1 would have cut them at the guard
       band, and screen-linear colour/fog across a triangle reaching far off-screen near the
       camera is wrong (walls came out fully fogged in third-person view) */
    "out vec3 v_stq; out vec4 v_col; out float v_fog; flat out float v_path;\n"
    "void main(){\n"
    "  vec4 P = a_pos;\n"
    "  vec2 clip = a_clip;\n"
    "  float vrm = (a_vr > 2.5 && u_vr_hud.y <= 0.0) ? 0.0 : a_vr;\n"
    "  if (u_vr_on != 0 && vrm > 0.5) {\n"
    "    vec3 V; float zo = P.z / P.w;\n"
    /* a_vr 3: the game's own 2D (texts, message window, icons; not the screen layers): on a
       panel u_vr_hud.y units in front of the frame's head, u_vr_hud.x units per pixel from the
       screen centre - re-projected like the 3D, so it sits still in front of the eyes */
    "    if (vrm > 2.5) V = vec3((P.x / P.w - u_vr_src.x) * u_vr_hud.x, (P.y / P.w - u_vr_src.y) * u_vr_hud.x - u_vr_hud.z, u_vr_hud.y);\n"
    "    else {\n"
    /* a_vr 2: sprite/line corners of 3D points (w = 1): the view depth from the GS depth, z = -b / Vz - a */
    "      if (vrm > 1.5) P = vec4(P.xy, P.z, 1.0) * (-u_vr_zab.y / (P.z + u_vr_zab.x));\n"
    "      float vz = P.w;\n"
    "      V = vec3((P.x - u_vr_src.x * vz) / u_vr_src.z, (P.y - u_vr_src.y * vz) / u_vr_src.w, vz);\n"
    "    }\n"
    "    vec4 E = u_vr_m0 * V.x + u_vr_m1 * V.y + u_vr_m2 * V.z + u_vr_m3;\n"
    /* depth: the GS depth formula applied to the EYE's distance (E.z metres * u_vr_zs units per
       metre), times E.z - affine in the vertex, so it interpolates exactly across the eye's
       screen. (The game camera's depth, kept before, bends across large triangles seen from the
       eye: surfaces close to others - frames on walls, pipes, table parts - fought and flickered.) */
    "    P = vec4(u_vr_dst.x * E.z + u_vr_dst.z * E.x, u_vr_dst.y * E.z + u_vr_dst.w * E.y, vrm > 2.5 ? zo * E.z : -u_vr_zab.y / u_vr_zs - u_vr_zab.x * E.z, E.z);\n"
    "    clip = vec2(E.z - 0.03, vrm > 1.5 ? 1.0 : clip.y);\n"   /* 3 cm in front of the eye (E is in metres) */
    "  }\n"
    "  float w = P.w;\n"
    "  vec2 p = (vec2(P.x + u_rt_margin * w, P.y) / u_rt_size) * 2.0 - w;\n"
    "  gl_Position = vec4(p, P.z * u_zscale * 2.0 - w, w);\n"
    "  gl_ClipDistance[0] = clip.x; gl_ClipDistance[1] = clip.y;\n"
    "  v_stq = a_stq; v_col = a_col; v_fog = a_fog; v_path = a_vr;\n"
    "}\n";

static const char* fs_src =
    "#version 330 core\n"
    "in vec3 v_stq; in vec4 v_col; in float v_fog; flat in float v_path; uniform int u_dbg;\n"
    "uniform sampler2D u_tex;\n"
    "uniform int u_tme, u_tfx, u_tcc, u_fst, u_fge, u_ate, u_atst, u_premul, u_afail_pass;\n"
    "uniform float u_aref, u_tex_alpha_scale;\n"
    "uniform vec2 u_texsize; uniform vec4 u_texofs; uniform vec4 u_fogcol;\n"
    "uniform ivec4 u_clamp_mode; uniform vec4 u_clamp_vals; uniform vec4 u_rt16;\n"
    "layout(location=0) out vec4 o_col;\n"
    "vec2 wrap(vec2 tc){\n"
    "  vec2 t = tc;\n"
    "  if (u_clamp_mode.x == 2) t.x = clamp(t.x, u_clamp_vals.x, u_clamp_vals.y + 0.999);\n"
    "  if (u_clamp_mode.y == 2) t.y = clamp(t.y, u_clamp_vals.z, u_clamp_vals.w + 0.999);\n"
    "  if (u_clamp_mode.x == 3) t.x = float((int(floor(t.x)) & int(u_clamp_vals.x)) | int(u_clamp_vals.y)) + fract(t.x);\n"
    "  if (u_clamp_mode.y == 3) t.y = float((int(floor(t.y)) & int(u_clamp_vals.z)) | int(u_clamp_vals.w)) + fract(t.y);\n"
    "  return t;\n"
    "}\n"
    "void main(){\n"
    "  vec4 c = v_col * 255.0;\n"     /* GS 0..255 integer domain */
    "  if (u_tme != 0) {\n"
    "    vec2 tc = (u_fst != 0) ? v_stq.xy : (v_stq.xy / v_stq.z) * u_texsize;\n"
    "    tc = wrap(tc);\n"
    "    vec4 t = texture(u_tex, tc * u_texofs.xy + u_texofs.zw);\n"
    "    t.rgb *= 255.0; t.a *= abs(u_tex_alpha_scale);\n"
    "    if (u_tex_alpha_scale < 0.0) t.a = min(t.a, 128.0);\n"   /* vr195: an HD pack image: its alpha capped at the GS opaque 0x80 */
    "    if (u_rt16.x != 0.0) {\n"                     /* render target read as 16-bit texels */
    "      t.rgb = floor(clamp(t.rgb, 0.0, 255.0) / 8.0) * 8.0;\n"
    "      t.a = (t.a >= 127.5) ? u_rt16.z : ((u_rt16.w != 0.0 && t.r + t.g + t.b == 0.0) ? 0.0 : u_rt16.y);\n"
    "    }\n"
    "    vec4 r;\n"
    "    if (u_tfx == 0) { r.rgb = t.rgb * c.rgb / 128.0; r.a = (u_tcc != 0) ? t.a * c.a / 128.0 : c.a; }\n"
    "    else if (u_tfx == 1) { r.rgb = t.rgb; r.a = (u_tcc != 0) ? t.a : c.a; }\n"
    "    else if (u_tfx == 2) { r.rgb = t.rgb * c.rgb / 128.0 + c.a; r.a = (u_tcc != 0) ? t.a + c.a : c.a; }\n"
    "    else { r.rgb = t.rgb * c.rgb / 128.0 + c.a; r.a = (u_tcc != 0) ? t.a : c.a; }\n"
    "    c = min(r, vec4(255.0));\n"
    "  }\n"
    "  if (u_fge != 0) { float f = clamp(v_fog, 0.0, 255.0); c.rgb = (f * c.rgb + (255.0 - f) * u_fogcol.rgb) / 256.0; }\n"
    "  c = floor(c + 0.0001);\n"
    "  if (u_ate != 0) {\n"
    "    bool pass = true;\n"
    "    if (u_atst == 0) pass = false;\n"
    "    else if (u_atst == 2) pass = c.a < u_aref;\n"
    "    else if (u_atst == 3) pass = c.a <= u_aref;\n"
    "    else if (u_atst == 4) pass = c.a == u_aref;\n"
    "    else if (u_atst == 5) pass = c.a >= u_aref;\n"
    "    else if (u_atst == 6) pass = c.a > u_aref;\n"
    "    else if (u_atst == 7) pass = c.a != u_aref;\n"
    "    if (u_afail_pass == 0 && !pass) discard;\n"
    "    if (u_afail_pass == 1 && pass) discard;\n"
    "  }\n"
    "  float as = c.a / 128.0;\n"
    "  vec3 rgb = c.rgb / 255.0;\n"
    "  if (u_premul == 1) rgb *= (1.0 + as);\n"
    "  if (u_dbg != 0) {\n"   /* vr_debug_cores: the path of each draw tinted (diagnosis) */
    "    vec3 k = v_path > 2.5 ? vec3(1.0, 0.9, 0.0) : v_path > 2.1 ? vec3(0.0, 1.0, 0.0) : v_path > 1.5 ? vec3(0.1, 0.3, 1.0) : v_path < 0.5 ? vec3(1.0, 0.0, 1.0) : rgb;\n"
    "    rgb = mix(rgb, k, 0.7);\n"
    "  }\n"
    "  o_col = vec4(rgb, as);\n"
    "}\n";

static const char* present_vs =
    "#version 330 core\n"
    "uniform vec4 u_rect;\n"
    "out vec2 v_uv;\n"
    "void main(){\n"
    "  vec2 c = vec2((gl_VertexID & 1) != 0 ? 1.0 : 0.0, (gl_VertexID & 2) != 0 ? 1.0 : 0.0);\n"
    "  v_uv = vec2(mix(u_rect.x, u_rect.z, c.x), mix(u_rect.y, u_rect.w, c.y));\n"
    "  gl_Position = vec4(c.x * 2.0 - 1.0, 1.0 - c.y * 2.0, 0.0, 1.0);\n"
    "}\n";
/* u_pic: picture calibration (brightness offset, contrast, 1 / gamma, on); off at the neutral
   values, so the image is the same as without it. gs_picture is the same on the CPU. */
static const char* present_fs =
    "#version 330 core\n"
    "in vec2 v_uv; uniform sampler2D u_tex; uniform vec4 u_pic; out vec4 o;\n"
    "void main(){\n"
    "  vec3 c = clamp(texture(u_tex, v_uv).rgb, 0.0, 1.0);\n"
    "  if (u_pic.w != 0.0) c = clamp((pow(c, vec3(u_pic.z)) - 0.5) * u_pic.y + 0.5 + u_pic.x, 0.0, 1.0);\n"
    "  o = vec4(c, 1.0);\n"
    "}\n";

static int picture(float* b, float* k, float* ig)
{
    *b = g_config.brightness / 200.0f; *k = g_config.contrast / 100.0f; *ig = 10.0f / (g_config.gamma > 0 ? g_config.gamma : 10);
    return g_config.brightness != 0 || g_config.contrast != 100 || g_config.gamma != 10;
}
float gs_picture(float c)
{
    float b, k, ig;
    if (!picture(&b, &k, &ig)) return c;
    c = (SDL_powf(c, ig) - 0.5f) * k + 0.5f + b;
    return c < 0 ? 0 : c > 1 ? 1 : c;
}

#ifdef __ANDROID__
/* Android (Quest): the GLSL 3.30 core sources as GLSL ES 3.20 - their "#version 330 core" line replaced; default
   precisions (ES has none for floats in fragment shaders); gl_ClipDistance from EXT_clip_cull_distance */
static const char* es_source(const char* src, int frag, char* buf, size_t n)
{
    static const char* vh = "#version 320 es\n#extension GL_EXT_clip_cull_distance : enable\nprecision highp float;\nprecision highp int;\n";
    static const char* fh = "#version 320 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n";
    const char* body = src;
    if (!strncmp(src, "#version 330 core\n", 18)) body = src + 18;
    snprintf(buf, n, "%s%s", frag ? fh : vh, body);
    return buf;
}
#endif

static GLuint compile(const char* vs, const char* fs)
{
    GLuint p = glCreateProgram(), s[2];
#ifdef __ANDROID__
    static char ebuf[2][32768];
    const char* src[2] = { es_source(vs, 0, ebuf[0], sizeof(ebuf[0])), es_source(fs, 1, ebuf[1], sizeof(ebuf[1])) };
    uint64_t and_t0 = SDL_GetTicksNS();
#else
    const char* src[2] = { vs, fs };
#endif
    GLenum type[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    for (int i = 0; i < 2; i++) {
        GLint ok;
        s[i] = glCreateShader(type[i]);
        glShaderSource(s[i], 1, &src[i], NULL);
        glCompileShader(s[i]);
        glGetShaderiv(s[i], GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[2048]; glGetShaderInfoLog(s[i], sizeof(log), NULL, log); host_fatal("shader: %s", log); }
        glAttachShader(p, s[i]);
    }
    glLinkProgram(p);
    {
        GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) { char log[2048]; glGetProgramInfoLog(p, sizeof(log), NULL, log); host_fatal("link: %s", log); }
    }
    glDeleteShader(s[0]); glDeleteShader(s[1]);
#ifdef __ANDROID__
    {
        uint64_t d = SDL_GetTicksNS() - and_t0;
        g_and_stall.compile_ns += d; g_and_stall.compile_n++;
        host_log("android: shader compile %.1f ms\n", d / 1e6);
    }
#endif
    return p;
}

static void* gl_getproc(const char* name) { return (void*)SDL_GL_GetProcAddress(name); }

#ifdef __ANDROID__
static void and_persistent_init(void)   /* the vertex stream's buffer replaced by a persistent mapped one (see and_pmap) */
{
    typedef void (GLAPIENTRY *PFN_storage)(GLenum, GLsizeiptr, const void*, GLbitfield);
    PFN_storage storage = (PFN_storage)SDL_GL_GetProcAddress("glBufferStorageEXT");
    PFN_and_map map = (PFN_and_map)SDL_GL_GetProcAddress("glMapBufferRange");
    const char* ext = (const char*)glGetString(0x1F03 /* GL_EXTENSIONS */);
    const GLbitfield fl = 0x0002 /* MAP_WRITE */ | 0x0040 /* MAP_PERSISTENT */ | 0x0080 /* MAP_COHERENT */;
    void (GLAPIENTRY *delbuf)(GLsizei, const GLuint*) = (void (GLAPIENTRY *)(GLsizei, const GLuint*))SDL_GL_GetProcAddress("glDeleteBuffers");
    GLuint b = 0;
    and_fencesync = (void* (GLAPIENTRY *)(GLenum, GLbitfield))SDL_GL_GetProcAddress("glFenceSync");
    and_clientwait = (GLenum (GLAPIENTRY *)(void*, GLbitfield, uint64_t))SDL_GL_GetProcAddress("glClientWaitSync");
    and_deletesync = (void (GLAPIENTRY *)(void*))SDL_GL_GetProcAddress("glDeleteSync");
    if (!storage || !map || !and_fencesync || !and_clientwait || !and_deletesync || !ext || !strstr(ext, "GL_EXT_buffer_storage")) {
        host_log("android: vertex ring: no persistent mapping (GL_EXT_buffer_storage missing) - per-batch uploads\n");
        return;
    }
    while (glGetError()) {}
    glGenBuffers(1, &b);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, b);
    storage(GL_ARRAY_BUFFER, VBO_VERTS * sizeof(GlVertex), NULL, fl);
    and_pmap = (uint8_t*)map(GL_ARRAY_BUFFER, 0, VBO_VERTS * sizeof(GlVertex), fl);
    if (!and_pmap) {
        host_log("android: vertex ring: persistent mapping failed (GL error %x) - per-batch uploads\n", glGetError());
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (delbuf) delbuf(1, &b);
        return;
    }
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, x));
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, s));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, r));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, fog));
    glEnableVertexAttribArray(4); glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, clip));
    glEnableVertexAttribArray(5); glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, vr));
    if (delbuf) delbuf(1, &vbo);   /* (the old stream buffer's 32 MB) */
    vbo = b;   /* (every later bind of the stream's buffer binds this one) */
    host_log("android: vertex ring: persistent mapped buffer, %u MB\n", (unsigned)(VBO_VERTS * sizeof(GlVertex) >> 20));
}
#endif

void gs_renderer_init(void)
{
    if (!gl_load(gl_getproc)) host_fatal("OpenGL 3.3 entry points missing");
    host_log("gl: %s / %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));
#ifdef __ANDROID__
    host_log("gl: extensions %s\n", glGetString(0x1F03 /* GL_EXTENSIONS */));
#endif
    if (!gs_mem_selftest()) host_fatal("GS swizzle tables failed self test");
    gs_reset();

    prog = compile(vs_src, fs_src);
#define U(n) u_##n = glGetUniformLocation(prog, "u_" #n)
    U(rt_size); U(rt_margin); U(tme); U(tfx); U(tcc); U(fst); U(texsize); U(fge); U(fogcol); U(ate); U(atst); U(aref);
    U(tex); U(tex_alpha_scale); U(texofs); U(premul); U(zscale); U(clamp_mode); U(clamp_vals); U(afail_pass); U(rt16); U(dbg);
    U(vr_on); U(vr_src); U(vr_dst); U(vr_m0); U(vr_m1); U(vr_m2); U(vr_m3); U(vr_zab); U(vr_zs); U(vr_hud);
#undef U
    present_prog = compile(present_vs, present_fs);
    up_tex = glGetUniformLocation(present_prog, "u_tex");
    up_rect = glGetUniformLocation(present_prog, "u_rect");
    up_pic = glGetUniformLocation(present_prog, "u_pic");

    glGenVertexArrays(1, &vao);
    glGenVertexArrays(1, &svao);
    glGenVertexArrays(1, &present_vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &svbo);
    for (int i = 0; i < 2; i++) {   /* the vertex stream, the executed frame's native strips */
        glBindVertexArray(i ? svao : vao);
        glBindBuffer(GL_ARRAY_BUFFER, i ? svbo : vbo);
        if (!i) glBufferData(GL_ARRAY_BUFFER, VBO_VERTS * sizeof(GlVertex), NULL, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, x));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, s));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, r));
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, fog));
        glEnableVertexAttribArray(4); glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, clip));
        glEnableVertexAttribArray(5); glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)offsetof(GlVertex, vr));
    }
    glBindVertexArray(vao);
#ifdef __ANDROID__
    and_persistent_init();
    { extern void and_glcache_install(void); and_glcache_install(); }   /* gl_cache.c (after the setup above) */
#endif

    g_scale = gs_wanted_scale();
}
/* native 3D: near/far clipping by clip distances (the clip volume of the VU1 programs).
   Not GL_DEPTH_CLAMP: it disables near-plane clipping, and some drivers then drop
   triangles reaching behind the camera instead of clipping them. On for game draws only:
   the present quad and the overlay (ImGui) write no gl_ClipDistance, and enabled distances
   a shader does not write are undefined. */
static int clip_on;
static void clip_distances(int on)
{
    if (clip_on == on) return;
    if (on) { glEnable(GL_CLIP_DISTANCE0); glEnable(GL_CLIP_DISTANCE1); }
    else { glDisable(GL_CLIP_DISTANCE0); glDisable(GL_CLIP_DISTANCE1); }
    clip_on = on;
}

/* ---------------- render target management ---------------- */

static struct { uint32_t zbp; int pw, ph; GLuint tex; uint64_t last; } dz[8];
static uint64_t dz_clock;
/* the depth texture attached to each FBO: attaching it again on every flush made the driver
   revalidate the framebuffer per draw. Forgotten whenever an FBO or depth texture is deleted
   (GL reuses names). */
static struct { GLuint fbo, dt; } att[32];
static int natt;
static void fb_forget(void) { natt = 0; }
static void attach_depth(GLuint fbo, GLuint dt)
{
    int i;
    for (i = 0; i < natt && att[i].fbo != fbo; i++) {}
    if (i < natt && att[i].dt == dt) return;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dt, 0);
#ifdef __ANDROID__
    {   /* foveated colour with a plain depth texture: checked the first times (logged if the driver refuses it) */
        static int nchk;
        if (nchk < 30) {
            GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            nchk++;
            if (st != 0x8CD5) host_log("android: render target framebuffer incomplete (0x%x) - foveation?\n", (unsigned)st);
            else if (nchk == 30) host_log("android: render target framebuffers complete (first 30 depth attachments checked)\n");
        }
    }
#endif
    if (i == natt) { if (natt == 32) { natt = 0; i = 0; } natt++; }
    att[i].fbo = fbo; att[i].dt = dt;
}
static GLuint depth_for(uint32_t zbp, int pw, int ph)
{
    int e = 0;
    dz_clock++;
    for (int i = 0; i < 8; i++) if (dz[i].tex && dz[i].zbp == zbp && dz[i].pw == pw && dz[i].ph == ph) { dz[i].last = dz_clock; return dz[i].tex; }
    for (int i = 0; i < 8; i++)   /* a free one, else the least recently used */
        if (!dz[i].tex) { e = i; break; }
        else if (dz[i].last < dz[e].last) e = i;
    if (dz[e].tex) { glDeleteTextures(1, &dz[e].tex); fb_forget(); }   /* (a depth of another size would cut the draws) */
    glGenTextures(1, &dz[e].tex);
    glBindTexture(GL_TEXTURE_2D, dz[e].tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, pw, ph, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    dz[e].zbp = zbp; dz[e].pw = pw; dz[e].ph = ph; dz[e].last = dz_clock;
    return dz[e].tex;
}

#define GL_RGBA16F 0x881A
#define GL_HALF_FLOAT 0x140B

static RenderTarget* rt_find(uint32_t fbp, uint32_t fbw)
{
    for (int i = 0; i < MAX_RT; i++) if (rts[i].used && rts[i].fbp == fbp && rts[i].fbw == fbw) return &rts[i];
    return NULL;
}

/* initial contents from emulated GS memory (e.g. data uploaded before the first draw) */
static void rt_load_from_mem(RenderTarget* rt)
{
    rt->has_box = 0;   /* mirrors GS memory again */
#ifdef __ANDROID__
    uint64_t and_t0 = SDL_GetTicksNS();
#endif
    ntex_before_access(0, 511);
    int w = rt->vw - 2 * g_margin_x, h = rt->vh;
    float* fx = malloc((size_t)w * h * 16);
    GLuint tmp;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float* o = &fx[((size_t)y * w + x) * 4];
            if (psm_is16(rt->psm)) {
                uint32_t c = gs_read_pixel(rt->psm, rt->fbp * 32, rt->fbw, (uint32_t)x, (uint32_t)y);
                o[0] = ((c & 0x1F) << 3) / 255.0f; o[1] = (((c >> 5) & 0x1F) << 3) / 255.0f; o[2] = (((c >> 10) & 0x1F) << 3) / 255.0f;
                o[3] = (c & 0x8000) ? 1.0f : 0.0f;
            } else {
                uint32_t c = gs_read_pixel(rt->psm == PSMCT24 ? PSMCT24 : PSMCT32, rt->fbp * 32, rt->fbw, (uint32_t)x, (uint32_t)y);
                o[0] = (c & 0xFF) / 255.0f; o[1] = ((c >> 8) & 0xFF) / 255.0f; o[2] = ((c >> 16) & 0xFF) / 255.0f; o[3] = (c >> 24) / 128.0f;
            }
        }
#ifdef __ANDROID__
    g_and_stall.reload_cpu_ns += SDL_GetTicksNS() - and_t0;
    {
        static int nl;
        if (nl++ < 20) host_log("android: reload from GS memory %dx%d psm %u fbp %u: CPU loop %.1f ms\n", w, h, rt->psm, rt->fbp, (SDL_GetTicksNS() - and_t0) / 1e6);
    }
#endif
    glGenTextures(1, &tmp);
    glBindTexture(GL_TEXTURE_2D, tmp);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, fx);
    {
        GLuint fb;
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmp, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->fbo);
        glBlitFramebuffer(0, 0, w, h, (GLint)(g_margin_x * g_scale), 0, (GLint)((g_margin_x + w) * g_scale), (GLint)(h * g_scale),
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glDeleteFramebuffers(1, &fb); fb_forget();
    }
    glDeleteTextures(1, &tmp);
    free(fx);
#ifdef __ANDROID__
    g_and_stall.reload_ns += SDL_GetTicksNS() - and_t0; g_and_stall.reload_n++;
#endif
}

static RenderTarget* rt_get(uint32_t fbp, uint32_t fbw, uint32_t psm)
{
    RenderTarget* rt = rt_find(fbp, fbw);
    if (rt) {
        rt->last_use = frame_no;
        if (rt->stale) { rt->stale = 0; rt->psm = psm; rt->pending_reload = 1; }
        return rt;
    }
    for (int i = 0; i < MAX_RT; i++) if (!rts[i].used) { rt = &rts[i]; break; }
    if (!rt) { /* evict LRU */
        rt = &rts[0];
        for (int i = 1; i < MAX_RT; i++) if (rts[i].last_use < rt->last_use) rt = &rts[i];
        glDeleteTextures(1, &rt->tex); glDeleteFramebuffers(1, &rt->fbo); fb_forget();
    }
    memset(rt, 0, sizeof(*rt));
    first[rt - rts].how = FIRST_NONE;
    rt->used = 1; rt->fbp = fbp; rt->fbw = fbw ? fbw : 1; rt->psm = psm;
    rt->vw = (int)rt->fbw * 64 + 2 * g_margin_x;
    rt->vh = 512;
    rt->pw = (int)(rt->vw * g_scale); rt->ph = (int)(rt->vh * g_scale);
    rt->last_use = frame_no;
    glGenTextures(1, &rt->tex);
    glBindTexture(GL_TEXTURE_2D, rt->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, rt->pw, rt->ph, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
#ifdef __ANDROID__
    and_foveate(rt->tex);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &rt->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->tex, 0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    rt_load_from_mem(rt);
    host_log("gs: new render target FBP=%u FBW=%u PSM=%u (%dx%d)\n", fbp, rt->fbw, psm, rt->pw, rt->ph);
    return rt;
}

/* vr162: dynamic resolution - in heavy places the eyes' scene drawn one step lower (4x -> 3x), back when light */
int g_vr_dyn_res = 0;     /* ini vr_dyn_res: 1 on, 0 off (vr176: off by default, fixed resolution) */
static int vr_dyn_down;   /* (render thread) the step down is on */

#ifdef __ANDROID__
/* Android (Quest): foveated rendering into the render targets (GL_QCOM_texture_foveated) - the GPU was the limit (the
   runtime held the headset at half rate: an eye's replay 3-4 ms, a game frame's drawing 3-4 ms on the GPU). Full detail
   in the middle (the 4:3 picture: |x| < ~0.7), less towards the sides and edges, at least half (the margins seen at the
   edge of the lenses) */
#define AND_FOV_BITS 0x8BFB       /* GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM */
#define AND_FOV_MIN 0x8BFC        /* GL_TEXTURE_FOVEATED_MIN_PIXEL_DENSITY_QCOM */
typedef void (GLAPIENTRY *PFN_AndFovParams)(GLuint, GLuint, GLuint, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat);
static int and_fov = -1;
static PFN_AndFovParams and_fovp;
static void and_foveate(GLuint tex)   /* (tex bound to GL_TEXTURE_2D, before it is attached) */
{
    if (and_fov < 0) {
        const char* ext = (const char*)glGetString(0x1F03 /* GL_EXTENSIONS */);
        and_fovp = (PFN_AndFovParams)SDL_GL_GetProcAddress("glTextureFoveationParametersQCOM");
        and_fov = ext && SDL_strstr(ext, "GL_QCOM_texture_foveated") && and_fovp;
        host_log("android: foveated render targets %s\n", and_fov ? "on (GL_QCOM_texture_foveated)" : "not available");
    }
    if (!and_fov) return;
    glTexParameteri(GL_TEXTURE_2D, AND_FOV_BITS, 0x1 /* ENABLE */ | 0x2 /* SCALED_BIN_METHOD */);
    glTexParameterf(GL_TEXTURE_2D, AND_FOV_MIN, 0.5f);
    and_fovp(tex, 0, 0, 0.0f, 0.0f, 1.5f, 1.2f, 0.25f);
}
/* (and_fov_next: ensure_target's next new target is drawn into - a shadow -, foveate it) */
#endif
/* internal resolution: the configured multiplier, or (auto) enough to cover the window */
float gs_wanted_scale(void)
{
    int ww, wh;
    float s;
    if (xr_active() && g_vr_stereo && xr_recommended_eye_height() > 0) {
        /* VR: the eye images are the 480-line display area: enough lines for the headset's
           recommended eye height (Quest 2: 1824 -> 4x), whatever the desktop setting */
        s = ceilf((float)xr_recommended_eye_height() * (float)g_vr_res / 100.0f / 480.0f);
        if (vr_dyn_down && s > 2.0f) s -= 1.0f;   /* vr162 */
#ifdef __ANDROID__
        {   /* Quest (on the headset's own GPU): at most 2x, or recvx.ini's scale when set - 4x took over 100 ms a frame */
            float cap = g_config.scale > 0 ? (float)g_config.scale : 2.0f;
            if (s > cap) s = cap;
        }
#endif
        return s < 1.0f ? 1.0f : s > 8.0f ? 8.0f : s;
    }
    if (g_config.scale > 0) return (float)g_config.scale;
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    s = floorf((float)wh / 448.0f + 0.5f);
    return s < 1.0f ? 1.0f : s > 8.0f ? 8.0f : s;
}

/* change the internal resolution; existing render targets are resampled, not lost */
void gs_set_scale(float scale)
{
    if (scale == g_scale) return;
    gs_flush();
    for (int i = 0; i < MAX_RT; i++) {
        RenderTarget* rt = &rts[i];
        GLuint tex, fbo;
        int pw = (int)(rt->vw * scale), ph = (int)(rt->vh * scale);
        if (!rt->used) continue;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, pw, ph, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
#ifdef __ANDROID__
        and_foveate(tex);
#endif
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rt->fbo);
        glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0, 0, rt->pw, rt->ph, 0, 0, pw, ph, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glDeleteFramebuffers(1, &rt->fbo); fb_forget();
        glDeleteTextures(1, &rt->tex);
        rt->tex = tex; rt->fbo = fbo; rt->pw = pw; rt->ph = ph;
    }
    for (int i = 0; i < 8; i++) if (dz[i].tex) { glDeleteTextures(1, &dz[i].tex); dz[i].tex = 0; }
    fb_forget();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g_scale = scale;
}

/* ---------------- hor+ widescreen ---------------- */
/* The view is widened by GS margins on both sides of every render target; the game's
   clip volume and culling cone are widened to match (pc_hor_scale), the projection is
   unchanged, so 2D drawn at 0..639 stays 4:3 in the middle. The render thread changes the
   margin and the guard band (below) between frames and publishes them; the game thread takes
   them at its frame hand-off (gs_take_view_change), so a game frame sees one value. */
static SDL_AtomicInt view_change, view_margin, view_guard;
static int game_margin, game_guard;   /* game thread */
int gs_take_view_change(void)
{
    int c = SDL_SetAtomicInt(&view_change, 0);
    if (c) { game_margin = SDL_GetAtomicInt(&view_margin); game_guard = SDL_GetAtomicInt(&view_guard); }
    return c;
}
float pc_hor_scale(void) { return (640.0f + 2.0f * game_margin) / 640.0f; }   /* game thread */
float gs_hor_scale(void) { return (640.0f + 2.0f * g_margin_x) / 640.0f; }    /* render thread */

int g_vr_eye_shape = 1;   /* ini vr_eye_shape: 1 = eye images at the headset's own size and shape, 0 = 16:9 */
static int wanted_margin(void)
{
    int ww, wh;
    float k;
    if (g_config.aspect != ASPECT_WIDE) return 0;
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    k = (ww * 3.0f) / (wh * 4.0f);
    if (k <= 1.0f) return 0;
    if (k > 3.0f) k = 3.0f;
    return (int)(320.0f * (k - 1.0f) + 0.5f);
}

/* Enhanced mode: the game's clip volume reaches into the GS guard band (the whole drawing
   area is +-2048 pixels around the screen centre), so geometry crossing a screen edge is
   no longer cut into differently shaped strips every frame, which keeps it matchable for
   interpolation; OpenGL clips it instead. Stays inside +-1900 pixels in any aspect. */
static int guard_on;
float pc_clip_guard(void)   /* game thread */
{
    float g = 1900.0f / (320.0f * pc_hor_scale());
    return game_guard ? (g > 2.5f ? 2.5f : g) : 1.0f;
}

void gs_update_widescreen(void)
{
    int m = wanted_margin();
    static int no_guard = -1;
    if (no_guard < 0) no_guard = SDL_getenv("RECVX_NO_GUARD") != NULL;   /* diagnosis: enhanced without the widened clip volume */
    if (guard_on != (g_config.interpolate != 0 && !no_guard)) {
        guard_on = g_config.interpolate != 0 && !no_guard;
        SDL_SetAtomicInt(&view_guard, guard_on);
        SDL_SetAtomicInt(&view_change, 1);
    }
    if (m == g_margin_x) return;
    gs_flush();
    for (int i = 0; i < MAX_RT; i++)   /* recreated (and reloaded from GS memory) at the new width */
        if (rts[i].used) { glDeleteTextures(1, &rts[i].tex); glDeleteFramebuffers(1, &rts[i].fbo); fb_forget(); rts[i].used = 0; }
    for (int i = 0; i < 8; i++) if (dz[i].tex) { glDeleteTextures(1, &dz[i].tex); dz[i].tex = 0; }
    fb_forget();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g_margin_x = m;
    SDL_SetAtomicInt(&view_margin, m);
    SDL_SetAtomicInt(&view_change, 1);   /* the game thread re-derives its culling volumes */
}

/* render target containing block address tbp (for render-to-texture) */
static uint64_t dbg_tex0;


/* GS pages covered by a render target: [fbp, fbp + count) */
static uint32_t rt_page_count(const RenderTarget* rt)
{
    uint32_t ph = psm_is16(rt->psm) ? 64 : 32;
    return rt->fbw * (uint32_t)((rt->vh + (int)ph - 1) / (int)ph);
}

/* Does a page range touch what was drawn into the render target? A write elsewhere (e.g. a
   CLUT stored beside a small render-to-texture image in the same pages) leaves it valid. */
static int rt_pages_drawn(const RenderTarget* rt, uint32_t p0, uint32_t p1)
{
    uint32_t ph = psm_is16(rt->psm) ? 64 : 32, end = rt->fbp + rt_page_count(rt);
    if (!rt->has_box) return 1;   /* mirrors GS memory: any write makes it out of date */
    if (p0 < rt->fbp) p0 = rt->fbp;
    for (uint32_t p = p0; p <= p1 && p < end; p++) {
        int x = (int)((p - rt->fbp) % rt->fbw) * 64, y = (int)((p - rt->fbp) / rt->fbw * ph);
        if (x <= rt->bx1 && x + 63 >= rt->bx0 && y <= rt->by1 && y + (int)ph - 1 >= rt->by0) return 1;
    }
    return 0;
}

/* Render target that a texture at block address tbp reads from (render-to-texture). */
static RenderTarget* rt_for_texture(uint32_t tbp, uint32_t psm, float* ox, float* oy, int* is16)
{
    for (int i = 0; i < MAX_RT; i++) {
        RenderTarget* rt = &rts[i];
        uint32_t base = rt->fbp * 32, page, ph;
        if (!rt->used || rt->stale || tbp < base || tbp >= base + rt_page_count(rt) * 32) continue;
        if (!rt_pages_drawn(rt, tbp / 32, tbp / 32)) continue;   /* not drawn there: GS memory holds it */
        /* same pixel family only; indexed formats read the GS memory image */
        if (psm_is16(psm) != psm_is16(rt->psm) || psm == PSMT8 || psm == PSMT4 || psm == PSMT8H || psm == PSMT4HL || psm == PSMT4HH) return NULL;
        if ((tbp - base) % 32) { static int n; if (n++ < 10) host_log("gs: texture TBP=%u inside RT FBP=%u at non-page offset (tex0=%016llx)\n", tbp, rt->fbp, (unsigned long long)dbg_tex0); return NULL; }
        ph = psm_is16(rt->psm) ? 64 : 32;
        page = (tbp - base) / 32;
        *ox = (float)((page % rt->fbw) * 64);
        *oy = (float)((page / rt->fbw) * ph);
        *is16 = psm_is16(psm);
        return rt;
    }
    return NULL;
}

/* A transfer overwrote GS memory: render targets over that memory no longer hold it. */
static void rt_invalidate_pages(uint32_t p0, uint32_t p1)
{
    for (int i = 0; i < MAX_RT; i++) {
        RenderTarget* rt = &rts[i];
        if (rt->used && !rt->stale && p0 < rt->fbp + rt_page_count(rt) && p1 >= rt->fbp && rt_pages_drawn(rt, p0, p1)) {
            rt->stale = 1;
        }
    }
}

/* ---------------- texture cache ---------------- */

void gs_notify_mem_write(uint32_t bp, uint32_t bw, int psm, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    /* conservative: mark the pages spanned by the rectangle's rows */
    uint32_t a0 = gs_pixel_addr(psm, bp, bw, x, y), a1 = gs_pixel_addr(psm, bp, bw, x + w - 1, y + h - 1);
    uint32_t div = (psm == PSMT4) ? 2 : 1;
    uint32_t p0 = (a0 / div) / 8192, p1 = (a1 / div) / 8192;
    uint32_t pb = bp / 32;
    if (p0 > pb) p0 = pb;
    if (p1 < p0) p1 = p0;
    p1 = p1 + (bw ? bw : 1) + 1; /* swizzled rows can reach further */
    for (uint32_t p = p0; p <= p1 && p < 512; p++) page_dirty[p] = 1;
    rt_invalidate_pages(p0, p1);
}

void gs_notify_pages(const uint64_t* pages, int changed)
{
#ifdef __ANDROID__
    for (uint32_t w = 0; w < 8; w++)   /* Android: only the set bits (the 512-page loop ran on every upload) */
        for (uint64_t m = pages[w]; m; m &= m - 1) {
            uint32_t p = w * 64 + (uint32_t)__builtin_ctzll(m);
            if (changed) page_dirty[p] = 1;
            rt_invalidate_pages(p, p);
        }
    return;
#endif
    for (uint32_t p = 0; p < 512; p++) {
        if (!((pages[p >> 6] >> (p & 63)) & 1)) continue;
        if (changed) page_dirty[p] = 1;
        rt_invalidate_pages(p, p);   /* render targets do not mirror GS memory: always reload */
    }
}

static uint32_t clut_hash(uint64_t tex0)
{
    uint32_t psm = (tex0 >> 20) & 0x3F, n, h = 2166136261u;
    uint32_t tmp[256];
    if (psm == PSMT8 || psm == PSMT8H) n = 256;
    else if (psm == PSMT4 || psm == PSMT4HL || psm == PSMT4HH) n = 16;
    else return 0;
#ifdef __ANDROID__
    /* Android (Quest): remembered while the CLUT has not changed (gs_clut_gen) - each draw decoded and hashed its 256-entry
       palette (part of gs_flush_, 26-33% of a game frame's execution on the render thread) */
    {
        extern uint32_t gs_clut_gen(void);
        static struct { uint32_t gen, h; uint64_t tex0, texa; } cmemo[256];
        uint32_t gen = gs_clut_gen(), k = (uint32_t)((tex0 ^ (tex0 >> 29) ^ gs.texa * 31) * 0x9E3779B1u) >> 24;
        if (cmemo[k].gen == gen && cmemo[k].tex0 == tex0 && cmemo[k].texa == gs.texa) return cmemo[k].h;
        {
            extern void gs_get_palette(uint64_t tex0, uint64_t texa, uint32_t* out, int n);
            gs_get_palette(tex0, gs.texa, tmp, (int)n);
            for (uint32_t i = 0; i < n; i++) { h ^= tmp[i]; h *= 16777619u; }
        }
        cmemo[k].gen = gen; cmemo[k].tex0 = tex0; cmemo[k].texa = gs.texa; cmemo[k].h = h;
        return h;
    }
#endif
    /* hash the decoded palette by decoding a 1-pixel-wide strip is overkill; decode via a fake 16x16 index ramp */
    extern void gs_get_palette(uint64_t tex0, uint64_t texa, uint32_t* out, int n);
    gs_get_palette(tex0, gs.texa, tmp, (int)n);
    for (uint32_t i = 0; i < n; i++) { h ^= tmp[i]; h *= 16777619u; }
    return h;
}

static uint32_t mv_tbp = UINT32_MAX;   /* gs_movie_bind */
static GLuint mv_rgb;

static GLuint tex_get(uint64_t tex0, uint64_t texa, int* out_w, int* out_h, float* ascale)
{
    *ascale = 255.0f;
    if ((tex0 & 0x3FFF) == mv_tbp && mv_rgb) { *out_w = *out_h = 512; return mv_rgb; }   /* the movie frame */
    {   /* native texture bound to this slot (textures.c) */
        unsigned int nt;
        int r = ntex_lookup(tex0, texa, &nt, out_w, out_h);
        if (r == 2) *ascale = 128.0f;   /* a capture: render-target alpha (A/128) */
        if (r) return nt;
    }
    uint32_t tw = (tex0 >> 26) & 0xF, th = (tex0 >> 30) & 0xF;
    uint32_t w = 1u << (tw > 10 ? 10 : tw), h = 1u << (th > 10 ? 10 : th);
    uint32_t tbp = tex0 & 0x3FFF, psm = (tex0 >> 20) & 0x3F;
    uint64_t key = tex0 & 0x1FFFFFFFFFFFFFull; /* TBP0..CSA; CLD excluded */
    uint32_t ch = clut_hash(tex0);
    uint32_t page0 = tbp / 32, page1 = gs_pixel_addr(psm, tbp, (tex0 >> 14) & 0x3F, w - 1, h - 1) / ((psm == PSMT4) ? 2 : 1) / 8192 + 1;
    TexEntry* e = NULL;
    uint64_t content = 0;
    if (page1 < page0) page1 = page0;
    if (page1 >= 512) page1 = 511;
    ntex_before_access(page0, page1);   /* a texture overlapping a bound native slot */
    ntex_before_access(((tex0 >> 37) & 0x3FFF) / 32, ((tex0 >> 37) & 0x3FFF) / 32 + 1);
#ifdef __ANDROID__
    g_and_sub = 8;   /* (profiler: page hashing) */
#endif
    for (uint32_t p = page0; p <= page1; p++) content = (content ^ page_hash_get(p)) * 0x9E3779B97F4A7C15ull + p;
#ifdef __ANDROID__
    g_and_sub = 9;   /* (profiler: texture cache lookup) */
#endif

#ifdef __ANDROID__
    /* Android (Quest): a direct index into texcache by the lookup's key, checked - the 512 entries were scanned each draw */
    static int16_t tidx[1024];
    static int tidx_init;
    uint32_t tk = (uint32_t)(((key * 0x9E3779B97F4A7C15ull) ^ content ^ ((uint64_t)ch << 17) ^ (psm != PSMCT16 && psm != PSMCT24 ? 0 : texa * 31)) >> 22) & 1023;
    if (!tidx_init) { memset(tidx, -1, sizeof(tidx)); tidx_init = 1; }
    if (tidx[tk] >= 0) {
        TexEntry* t = &texcache[tidx[tk]];
        if (t->used && t->tex0_key == key && t->content == content && t->clut_hash == ch && (psm != PSMCT16 && psm != PSMCT24 ? 1 : t->texa == texa)) {
            t->last_use = frame_no; *out_w = t->w; *out_h = t->h; return t->tex;
        }
    }
#endif
    for (int i = 0; i < MAX_TEX; i++) {
        TexEntry* t = &texcache[i];
        if (t->used && t->tex0_key == key && t->content == content && t->clut_hash == ch && (psm != PSMCT16 && psm != PSMCT24 ? 1 : t->texa == texa)) {
            t->last_use = frame_no; *out_w = t->w; *out_h = t->h;
#ifdef __ANDROID__
            tidx[tk] = (int16_t)i;
#endif
            return t->tex;
        }
    }
    {
        for (int i = 0; i < MAX_TEX; i++) if (!texcache[i].used) { e = &texcache[i]; break; }
        if (!e) {
            e = &texcache[0];
            for (int i = 1; i < MAX_TEX; i++) if (texcache[i].last_use < e->last_use) e = &texcache[i];
        }
        if (e->tex && !trep_kind(e->tex)) gs_tex_delete(e->tex);   /* (a replacement belongs to texreplace.c) */
        memset(e, 0, sizeof(*e));
        if (trep_active() && (e->tex = trep_lookup(tex0, texa)) != 0) goto filled;   /* an HD pack's image (texreplace.c) */
        glGenTextures(1, &e->tex);
    }
    {
        uint32_t* px = malloc(w * h * 4);
#ifdef __ANDROID__
        uint64_t and_t0 = SDL_GetTicksNS();
        g_and_sub = 10;   /* (profiler: texture decode and upload) */
#endif
        stat_decodes++; stat_decode_px += w * h;
        gs_decode_texture(tex0, texa, w, h, px);
        {
            static const char* dd; static int chk, ndump;
            if (!chk++) dd = SDL_getenv("RECVX_DUMP_TEX");
            if (dd && ndump < 40) {
                char path[1024];
                SDL_Surface* sf = SDL_CreateSurfaceFrom((int)w, (int)h, SDL_PIXELFORMAT_ABGR8888, px, (int)w * 4);
                SDL_snprintf(path, sizeof(path), "%s/tex%03d_%llx.bmp", dd, ndump++, (unsigned long long)tex0);
                SDL_SaveBMP(sf, path);
                SDL_DestroySurface(sf);
            }
        }
        glBindTexture(GL_TEXTURE_2D, e->tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        free(px);
#ifdef __ANDROID__
        g_and_stall.tex_ns += SDL_GetTicksNS() - and_t0; g_and_stall.tex_n++;
#endif
    }
filled:
#ifdef __ANDROID__
    tidx[tk] = (int16_t)(e - texcache);
#endif
    e->used = 1; e->tex0_key = key; e->texa = texa; e->clut_hash = ch; e->page0 = page0; e->page1 = page1;
    e->content = content; e->w = (int)w; e->h = (int)h; e->last_use = frame_no;
    *out_w = e->w; *out_h = e->h;
    return e->tex;
}

/* texreplace.c: a replacement redrawn as RGBA8 at dw x dh with mipmaps (block-compressed packs
   come without them; far away they would shimmer in VR). Called at the frame start. */
static const char* copy_fs =
    "#version 330 core\n"
    "in vec2 v_uv; uniform sampler2D u_tex; out vec4 o;\n"
    "void main(){ o = texture(u_tex, v_uv); }\n";
unsigned int gs_tex_to_rgba_mips(unsigned int src, int dw, int dh)
{
    static GLuint cprog, cfbo;
    static GLint cu_tex, cu_rect;
    GLuint t;
    if (!glGenerateMipmap) return 0;
    if (!cprog) {
        cprog = compile(present_vs, copy_fs);
        cu_tex = glGetUniformLocation(cprog, "u_tex"); cu_rect = glGetUniformLocation(cprog, "u_rect");
        glGenFramebuffers(1, &cfbo);
    }
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dw, dh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, cfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != 0x8CD5 /* GL_FRAMEBUFFER_COMPLETE */) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); glDeleteTextures(1, &t); return 0;
    }
    glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, dw, dh);
    glUseProgram(cprog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUniform1i(cu_tex, 0);
    glUniform4f(cu_rect, 0.0f, 1.0f, 1.0f, 0.0f);   /* (present_vs puts v0 at the top: unflipped into the target) */
    glBindVertexArray(present_vao);
    glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, t);
    glGenerateMipmap(GL_TEXTURE_2D);
    glUseProgram(prog);
    return t;
}

/* texreplace.c deletes a replacement: entries that use it decode again */
void gs_texcache_drop_gl(unsigned int gl_tex)
{
    for (int i = 0; i < MAX_TEX; i++) if (texcache[i].used && texcache[i].tex == gl_tex) { texcache[i].used = 0; texcache[i].tex = 0; }
}

/* ---------------- state application ---------------- */

static RenderTarget* cur_rt;

/* once per ALPHA value (it was a line per flush) */
static void blend_log(const char* what, uint64_t al)
{
    static uint8_t seen[256];
    if (!seen[al & 0xFF]) { seen[al & 0xFF] = 1; host_log("gs: %s blend %02x\n", what, (unsigned)(al & 0xFF)); }
}

static void apply_blend(const DrawState* st, int* premul)
{
    uint64_t al = st->ctx.alpha;
    int a = al & 3, b = (al >> 2) & 3, c = (al >> 4) & 3, d = (al >> 6) & 3;
    float fix = (float)((al >> 32) & 0xFF) / 128.0f;
    GLenum cf = c == 0 ? GL_SRC_ALPHA : c == 1 ? GL_DST_ALPHA : GL_CONSTANT_COLOR;
    GLenum cf1 = c == 0 ? GL_ONE_MINUS_SRC_ALPHA : c == 1 ? GL_ONE_MINUS_DST_ALPHA : GL_ONE_MINUS_CONSTANT_COLOR;
    GLenum sf = GL_ONE, df = GL_ZERO, eq = GL_FUNC_ADD;
    *premul = 0;
    if (!((st->prim >> 6) & 1)) { glDisable(GL_BLEND); return; }
    glEnable(GL_BLEND);
    glBlendColor(fix, fix, fix, fix);
    if (a == b) {
        if (d == 0) { sf = GL_ONE; df = GL_ZERO; }
        else if (d == 1) { sf = GL_ZERO; df = GL_ONE; }
        else { sf = GL_ZERO; df = GL_ZERO; }
    } else if (a == 0 && b == 1) {           /* (Cs-Cd)*C + D */
        if (d == 1) { sf = cf; df = cf1; }
        else if (d == 0) { sf = GL_ONE; df = cf; eq = GL_FUNC_SUBTRACT; *premul = c == 0 ? 1 : 0; if (c != 0) blend_log("unhandled", al); }
        else { sf = cf; df = cf; eq = GL_FUNC_SUBTRACT; }
    } else if (a == 0 && b == 2) {           /* Cs*C + D */
        if (d == 1) { sf = cf; df = GL_ONE; }
        else if (d == 0) { sf = GL_ONE; df = GL_ZERO; *premul = c == 0 ? 1 : 0; }
        else { sf = cf; df = GL_ZERO; }
    } else if (a == 1 && b == 0) {           /* (Cd-Cs)*C + D */
        if (d == 0) { sf = cf1; df = cf; }
        else if (d == 2) { sf = cf; df = cf; eq = GL_FUNC_REVERSE_SUBTRACT; }
        else { sf = cf; df = GL_ONE; eq = GL_FUNC_REVERSE_SUBTRACT; blend_log("approximated", al); }
    } else if (a == 1 && b == 2) {           /* Cd*C + D */
        if (d == 0) { sf = GL_ONE; df = cf; }
        else if (d == 2) { sf = GL_ZERO; df = cf; }
        else { sf = GL_ZERO; df = GL_ONE; blend_log("approximated", al); }
    } else if (a == 2 && b == 0) {           /* D - Cs*C */
        if (d == 1) { sf = cf; df = GL_ONE; eq = GL_FUNC_REVERSE_SUBTRACT; }
        else if (d == 0) { sf = cf1; df = GL_ZERO; }
        else { sf = GL_ZERO; df = GL_ZERO; }
    } else if (a == 2 && b == 1) {           /* D - Cd*C */
        if (d == 0) { sf = GL_ONE; df = cf; eq = GL_FUNC_SUBTRACT; }
        else if (d == 1) { sf = GL_ZERO; df = cf1; }
        else { sf = GL_ZERO; df = GL_ZERO; }
    }
    glBlendEquationSeparate(eq, GL_FUNC_ADD);
    glBlendFuncSeparate(sf, df, GL_ONE, GL_ZERO);
}

/* prog's uniforms are set only when their value changes (program object state: kept across
   program switches, and only gs_flush sets prog's) */
static uint32_t ucache[64][4];
static uint8_t uknown[64];
static int uni_changed(GLint loc, const void* v, int n)
{
    if (loc < 0) return 0;
    if (loc >= 64) return 1;
    if (uknown[loc] && !memcmp(ucache[loc], v, (size_t)n * 4)) return 0;
    memcpy(ucache[loc], v, (size_t)n * 4);
    uknown[loc] = 1;
    return 1;
}
static void uni1i(GLint l, int a) { if (uni_changed(l, &a, 1)) glUniform1i(l, a); }
static void uni1f(GLint l, float a) { if (uni_changed(l, &a, 1)) glUniform1f(l, a); }
static void uni2f(GLint l, float a, float b) { float v[2] = { a, b }; if (uni_changed(l, v, 2)) glUniform2f(l, a, b); }
static void uni4f(GLint l, float a, float b, float c, float d) { float v[4] = { a, b, c, d }; if (uni_changed(l, v, 4)) glUniform4f(l, a, b, c, d); }
static void uni4i(GLint l, int a, int b, int c, int d) { int v[4] = { a, b, c, d }; if (uni_changed(l, v, 4)) glUniform4i(l, a, b, c, d); }

/* the pending batch is drawn in two passes (alpha test with a fail mode that still writes) */
static int two_pass(uint64_t test) { return (test & 1) && ((test >> 12) & 3) != 0 && ((test >> 1) & 7) != 1; }
int gs_batch_two_pass(void) { return batch_valid && vcount && two_pass(batch_state.ctx.test); }

/* the batch: vbuf's vertices from vbo_first on, with the static runs in their places (one draw
   call each part, in order: the same primitives in the same order as one call) */
#ifdef __ANDROID__
static void draw_batch_(int afail_pass);
static void draw_batch(int afail_pass)   /* Android diagnosis: the draw calls' wall time */
{
    uint64_t t0;
    if (!g_and_time_frame) { draw_batch_(afail_pass); return; }
    t0 = SDL_GetTicksNS();
    draw_batch_(afail_pass);
    g_and_stall.draw_ns += SDL_GetTicksNS() - t0;
}
static void draw_batch_(int afail_pass)
#else
static void draw_batch(int afail_pass)
#endif
{
    int at = 0;
    uni1i(u_afail_pass, afail_pass);
    if (!nsruns) { glDrawArrays(batch_mode, vbo_first, vcount); return; }
    for (int r = 0; r <= nsruns; r++) {
        int end = r < nsruns ? sruns[r].start : vcount;
        if (end > at) { glBindVertexArray(vao); glDrawArrays(batch_mode, vbo_first + at, end - at); }
        if (r == nsruns) break;
        glBindVertexArray(svao);
        glDrawArrays(batch_mode, sruns[r].first, sruns[r].count);
        at = sruns[r].start + sruns[r].count;
    }
    glBindVertexArray(vao);
}

/* ---------------- RECVX_GPU_TIME (test aid) ----------------
   GPU timestamps (glQueryCounter) around each executed frame and each presented image, read
   back a few images later (no stall); every 2 s one log line with the CPU (render thread,
   wall time without the swap) and GPU milliseconds per executed frame, per in-between image
   and per plain present, and the images per second. */
enum { GT_EXEC0, GT_EXEC1, GT_PRES0, GT_RB, GT_RD, GT_DD, GT_PRES1, GT_N };   /* RB/RD/DD: in-between only */
#define GT_RING 16
typedef void (GLAPIENTRY *PFN_QueryCounter)(GLuint, GLenum);
typedef void (GLAPIENTRY *PFN_GetQueryObjectui64v)(GLuint, GLenum, uint64_t*);
typedef void (GLAPIENTRY *PFN_GetQueryObjectiv)(GLuint, GLenum, GLint*);
typedef void (GLAPIENTRY *PFN_GenQueries)(GLsizei, GLuint*);
static struct {
    int on;
    PFN_QueryCounter counter; PFN_GetQueryObjectui64v get64; PFN_GetQueryObjectiv geti; PFN_GenQueries gen;
    GLuint q[GT_RING][GT_N];
    int kind[GT_RING];          /* 0 free, 1 present (in-between), 2 present (frame), +4 with exec */
    int slot;
    uint64_t cpu_exec0, cpu_exec, cpu_pres0;
    double sum_cpu[3], sum_gpu[3];   /* exec, in-between, frame image */
    double part[4];                  /* in-between: start copy, replay draws, display + overlay, restore */
    int n_cpu[3], n_gpu[3];
    uint64_t last_log;
} gt;
static void gt_init(void)
{
    static int done;
    if (done++) return;
    gt.on = SDL_getenv("RECVX_GPU_TIME") && SDL_atoi(SDL_getenv("RECVX_GPU_TIME"));
    if (!gt.on) return;
    gt.counter = (PFN_QueryCounter)SDL_GL_GetProcAddress("glQueryCounter");
    gt.get64 = (PFN_GetQueryObjectui64v)SDL_GL_GetProcAddress("glGetQueryObjectui64v");
    gt.geti = (PFN_GetQueryObjectiv)SDL_GL_GetProcAddress("glGetQueryObjectiv");
    gt.gen = (PFN_GenQueries)SDL_GL_GetProcAddress("glGenQueries");
    if (!gt.counter || !gt.get64 || !gt.geti || !gt.gen) { gt.on = 0; return; }
    for (int i = 0; i < GT_RING; i++) gt.gen(GT_N, gt.q[i]);
}
#define GL_TIMESTAMP 0x8E28
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
static void gt_mark(int which) { if (gt.on) gt.counter(gt.q[gt.slot][which], GL_TIMESTAMP); }
static void gt_collect(int s, int wait)
{
    GLint avail = 1;
    uint64_t t[GT_N];
    int k = gt.kind[s];
    if (!k) return;
    if (!wait) gt.geti(gt.q[s][GT_PRES1], GL_QUERY_RESULT_AVAILABLE, &avail);
    if (!avail) return;
    for (int i = (k & 4) ? GT_EXEC0 : GT_PRES0; i < GT_N; i++)
        if ((k & 3) == 1 || i < GT_RB || i == GT_PRES1) gt.get64(gt.q[s][i], GL_QUERY_RESULT, &t[i]);
    if ((k & 3) == 1) for (int i = 0; i < 4; i++) gt.part[i] += (t[GT_RB + i] - t[GT_PRES0 + i]) / 1e6;
    if (k & 4) { gt.sum_gpu[0] += (t[GT_EXEC1] - t[GT_EXEC0]) / 1e6; gt.n_gpu[0]++; }
    gt.sum_gpu[k & 3] += (t[GT_PRES1] - t[GT_PRES0]) / 1e6; gt.n_gpu[k & 3]++;
    gt.kind[s] = 0;
}
static void gt_exec_begin(void)
{
    gt_init();
    if (!gt.on) return;
    if (gt.kind[gt.slot] == 4) gt.kind[gt.slot] = 0;   /* executed, never presented (skipped): no GPU time */
    gt_collect(gt.slot, 1);
    gt_mark(GT_EXEC0);
    gt.cpu_exec0 = SDL_GetTicksNS();
}
static void gt_exec_end(void)
{
    if (!gt.on) return;
    gt_mark(GT_EXEC1);
    gt.kind[gt.slot] = 4;
    gt.sum_cpu[0] += (SDL_GetTicksNS() - gt.cpu_exec0) / 1e6; gt.n_cpu[0]++;
}
static void gt_present_begin(void)
{
    gt_init();
    if (!gt.on) return;
    if (!(gt.kind[gt.slot] & 4)) gt_collect(gt.slot, 1);
    gt_mark(GT_PRES0);
    gt.cpu_pres0 = SDL_GetTicksNS();
}
static void gt_present_end(int inbetween)
{
    uint64_t now;
    if (!gt.on) return;
    now = SDL_GetTicksNS();
    gt_mark(GT_PRES1);
    gt.kind[gt.slot] = (gt.kind[gt.slot] & 4) | (inbetween ? 1 : 2);
    gt.sum_cpu[inbetween ? 1 : 2] += (now - gt.cpu_pres0) / 1e6; gt.n_cpu[inbetween ? 1 : 2]++;
    gt.slot = (gt.slot + 1) % GT_RING;
    for (int i = 1; i < GT_RING / 2; i++) gt_collect((gt.slot + i) % GT_RING, 0);
    if (!gt.last_log) gt.last_log = now;
    if (now - gt.last_log >= 2000000000ull) {
        double s = (now - gt.last_log) / 1e9;
#define A(x, n) ((n) ? (x) / (n) : 0.0)
        host_log("gputime: exec %.1f/s cpu %.2f gpu %.2f ms | in-between %.1f/s cpu %.2f gpu %.2f ms (start %.2f draw %.2f display %.2f restore %.2f) | frame image %.1f/s cpu %.2f gpu %.2f ms\n",
                 gt.n_cpu[0] / s, A(gt.sum_cpu[0], gt.n_cpu[0]), A(gt.sum_gpu[0], gt.n_gpu[0]),
                 gt.n_cpu[1] / s, A(gt.sum_cpu[1], gt.n_cpu[1]), A(gt.sum_gpu[1], gt.n_gpu[1]),
                 A(gt.part[0], gt.n_gpu[1]), A(gt.part[1], gt.n_gpu[1]), A(gt.part[2], gt.n_gpu[1]), A(gt.part[3], gt.n_gpu[1]),
                 gt.n_cpu[2] / s, A(gt.sum_cpu[2], gt.n_cpu[2]), A(gt.sum_gpu[2], gt.n_gpu[2]));
#undef A
        memset(gt.sum_cpu, 0, sizeof(gt.sum_cpu)); memset(gt.sum_gpu, 0, sizeof(gt.sum_gpu));
        memset(gt.n_cpu, 0, sizeof(gt.n_cpu)); memset(gt.n_gpu, 0, sizeof(gt.n_gpu)); memset(gt.part, 0, sizeof(gt.part));
        gt.last_log = now;
    }
}

int gs_flush_blocked;
   /* native texture decode/materialize, which runs inside a flush (tex_get) */

/* ---------------- replay resources ----------------
   GS memory, the CLUT and native texture slots change during a frame (a page can hold several
   textures and a render target in turn), and an in-between frame replays the events after the
   frame has run. So the executed frame resolves what each draw reads, and its replays reuse
   that instead of resolving again:
   - textures: kept under (epoch, TEX0, TEXA): the GL texture, or the render target (by slot)
     that was sampled; a replay binds the same texture, or that render target as it is now (the
     replay redraws it);
   - render-target reloads from GS memory: the reloaded image is kept and put back when the
     replay reaches the same draw; replays never reload from GS memory nor write it. */
#define MEMO_SIZE 16384
typedef struct { uint64_t tex0, texa; uint32_t epoch, gen; int rt; uint32_t fbp, fbw; GLuint tex; int w, h; float ox, oy, ascale; int is16; } TexMemo;
static TexMemo memo[MEMO_SIZE];
static uint32_t memo_gen = 1;
#define MAX_RELOADS 8
typedef struct { uint32_t epoch, fbp; int pw, ph, applied; GLuint tex, fbo; int tw, th; } Reload;
static Reload reloads[MAX_RELOADS];
static int nreloads;
static void blit_color(GLuint src_fbo, GLuint dst_fbo, int w, int h);
static void ensure_target(GLuint* tex, GLuint* fbo, int* cw, int* ch, int w, int h);

/* textures deleted while a frame executes stay alive until the next one: its replays still
   bind them (a definition used early in a frame can be forgotten later in it) */
static GLuint* doomed; static size_t ndoomed, cdoomed;
void gs_tex_delete(unsigned int gl_tex)
{
    if (ndoomed == cdoomed) { GROW_TO(doomed, ndoomed + 1, cdoomed); }
    doomed[ndoomed++] = gl_tex;
}
static TexMemo* memo_slot(uint64_t tex0, uint64_t texa, uint32_t epoch, int create)
{
    uint64_t h = (tex0 ^ (texa * 0x94D049BB133111EBull) ^ ((uint64_t)epoch * 0x9E3779B97F4A7C15ull)) * 0xBF58476D1CE4E5B9ull;
    for (int k = 0; k < 32; k++) {
        TexMemo* m = &memo[((uint32_t)(h >> 40) + (uint32_t)k) & (MEMO_SIZE - 1)];
        if (m->gen != memo_gen) {
            if (!create) return NULL;
            m->gen = memo_gen; m->tex0 = tex0; m->texa = texa; m->epoch = epoch; m->rt = -1; m->tex = 0;
            return m;
        }
        if (m->tex0 == tex0 && m->texa == texa && m->epoch == epoch) return m;
    }
    return NULL;
}
/* the memo's render target is still in its slot (not evicted and reused for another one) */
static int memo_rt_ok(const TexMemo* m) { return rts[m->rt].used && rts[m->rt].fbp == m->fbp && rts[m->rt].fbw == m->fbw; }
/* executed frame: a render target was reloaded from GS memory for a draw in this epoch */
static void reload_record(const RenderTarget* rt, uint32_t epoch)
{
    Reload* r;
    if (nreloads == MAX_RELOADS) return;
    r = &reloads[nreloads++];
    ensure_target(&r->tex, &r->fbo, &r->tw, &r->th, rt->pw, rt->ph);
    blit_color(rt->fbo, r->fbo, rt->pw, rt->ph);
    r->epoch = epoch; r->fbp = rt->fbp; r->pw = rt->pw; r->ph = rt->ph; r->applied = 0;
}
/* replay: put back what the executed frame reloaded up to this point */
static void reload_replay(RenderTarget* rt, uint32_t epoch)
{
    for (int i = 0; i < nreloads; i++) {
        Reload* r = &reloads[i];
        if (r->applied || r->fbp != rt->fbp || r->epoch > epoch || r->pw != rt->pw || r->ph != rt->ph) continue;
        blit_color(r->fbo, rt->fbo, rt->pw, rt->ph);
        r->applied = 1;
    }
}
/* the display target as the frame found it: PS2_swap blends the new image over it, so a
   replay starts from it, not from the executed frame's result (that would add a pre-echo of
   the executed frame). RECVX_REPLAY_RTSTART=0: the old behaviour. */
/* Two images: the executed frame's final image (presented) and the start image. The display
   target as the next frame finds it is the previous frame's final image (nothing draws into
   it in between: replays draw into their own targets), so that image becomes the start image
   and the next final image goes into the other one. A copy only when that does not hold (a
   partial frame drew into it, the scale, the margins or the display buffer changed). */
static struct { GLuint tex, fbo; int pw, ph; } img[2];
static int fin, sti, fin_fresh;    /* final and start image; fin_fresh: img[fin] is the display target */
static uint32_t fin_fbp, fin_fbw;
static int start_valid;
static uint32_t start_fbp, start_fbw;
static int rtstart_enabled(void)
{
    static int v = -1;
    if (v < 0) v = !SDL_getenv("RECVX_REPLAY_RTSTART") || SDL_atoi(SDL_getenv("RECVX_REPLAY_RTSTART")) != 0;
    return v;
}
/* ---------------- replay targets ----------------
   A replay draws into a second set of targets, swapped in for its duration (shadow[]), so the
   executed frame's targets are never drawn over and need no copy back. What a replay finds in
   a target must still be what the frame found there: the display target's start image (above),
   for every other target the executed frame's result (what replays drawing over the targets
   and undoing it saw), but only where the frame reads it before overwriting it. The executed
   frame notes per target whether its first access was an opaque write over a whole area (the
   clear after the swap, Ps2ShadowStart's) and which area (first[]); only the rest is copied (a
   few rows and columns), otherwise the whole target. Saves the copy back of every target per
   replay and the copy of every target per executed frame. */
typedef struct { GLuint tex, fbo; int pw, ph; } Shadow;
static Shadow shadow[MAX_RT];
static RenderTarget rts_snap[MAX_RT];
static int disp_end = -1;   /* the target presented after the executed frame */

static void blit_area(GLuint src_fbo, GLuint dst_fbo, int x0, int y0, int x1, int y1)
{
    if (x1 <= x0 || y1 <= y0) return;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);
    glDisable(GL_SCISSOR_TEST);
    glBlitFramebuffer(x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

/* executed frame: the first draw into rt; COVER if it overwrites an area whatever was there:
   a sprite first in the batch, no alpha test, no FBMSK, the depth test passes, all channels
   written, and no blending or one whose result is the source (A = B, D = Cs; or the frame copy
   of PS2_swap with frame blend off, (Cs - Cd) * 128/128 + Cd: GL's Cs * 1 + Cd * 0); the area
   shrunk by a pixel against rounding */
static void rt_first_draw(const RenderTarget* rt, const DrawState* st, uint32_t fbw, uint32_t fpsm)
{
    int i = (int)(rt - rts), zte = (st->ctx.test >> 16) & 1, ztst = (st->ctx.test >> 17) & 3;
    uint64_t sc = st->ctx.scissor;
    int x0 = sc & 0x7FF, x1 = (sc >> 16) & 0x7FF, y0 = (sc >> 32) & 0x7FF, y1 = (sc >> 48) & 0x7FF;
    float sx0 = vbuf[0].x, sx1 = vbuf[0].x, sy0 = vbuf[0].y, sy1 = vbuf[0].y, ax, ay, bx, by;
    uint64_t al = st->ctx.alpha;
    int a = al & 3, b = (al >> 2) & 3, c = (al >> 4) & 3, d = (al >> 6) & 3;
    int opaque = !((st->prim >> 6) & 1) || (a == b && d == 0) || (a == 0 && b == 1 && c == 2 && d == 1 && ((al >> 32) & 0xFF) == 128);
    first[i].how = FIRST_READ;
    if (!batch_sprite_first || !opaque || vcount < 6 || (zte && ztst != 1) || fpsm == PSMCT24) return;
    for (int k = 1; k < 6; k++) {
        sx0 = fminf(sx0, vbuf[k].x); sx1 = fmaxf(sx1, vbuf[k].x);
        sy0 = fminf(sy0, vbuf[k].y); sy1 = fmaxf(sy1, vbuf[k].y);
    }
    if (g_margin_x && x0 == 0 && x1 >= (int)fbw * 64 - 1) { x0 = -g_margin_x; x1 += g_margin_x; }   /* as gs_flush */
    ax = fmaxf((sx0 + g_margin_x) * g_scale, (float)(int)((x0 + g_margin_x) * g_scale));
    bx = fminf((sx1 + g_margin_x) * g_scale, (float)((int)((x0 + g_margin_x) * g_scale) + (int)((x1 - x0 + 1) * g_scale)));
    ay = fmaxf(sy0 * g_scale, (float)(int)(y0 * g_scale));
    by = fminf(sy1 * g_scale, (float)((int)(y0 * g_scale) + (int)((y1 - y0 + 1) * g_scale)));
    first[i].x0 = (int)ceilf(ax) + 1; first[i].x1 = (int)floorf(bx) - 1;
    first[i].y0 = (int)ceilf(ay) + 1; first[i].y1 = (int)floorf(by) - 1;
    if (first[i].x0 < 0) first[i].x0 = 0;
    if (first[i].y0 < 0) first[i].y0 = 0;
    if (first[i].x1 > rt->pw) first[i].x1 = rt->pw;
    if (first[i].y1 > rt->ph) first[i].y1 = rt->ph;
    if (first[i].x1 > first[i].x0 && first[i].y1 > first[i].y0) first[i].how = FIRST_COVER;
}
/* a replay target for rts[i], cleared (a blend that ignores the destination still multiplies
   it by 0: no NaN from uninitialized memory) */
static void shadow_ensure(int i)
{
    Shadow* sh = &shadow[i];
#ifdef __ANDROID__
    and_fov_next = 1;
#endif
    ensure_target(&sh->tex, &sh->fbo, &sh->pw, &sh->ph, rts[i].pw, rts[i].ph);
#ifdef __ANDROID__
    and_fov_next = 0;
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, sh->fbo);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}
/* executed frame: rt is read (sampled, reloaded, captured) */
static void rt_touch(const RenderTarget* rt)
{
    if (!gs_replaying && rt && first[rt - rts].how == FIRST_NONE) first[rt - rts].how = FIRST_READ;
}

void gs_replay_begin(void)
{
#ifdef __ANDROID__
    extern void and_gq_stage(int); extern volatile int g_and_phase;
    if (g_and_phase == 2) and_gq_stage(0);
#endif
    RenderTarget* disp = start_valid ? rt_find(start_fbp, start_fbw) : NULL;
    if (disp && (disp->pw != img[sti].pw || disp->ph != img[sti].ph)) disp = NULL;
    for (int i = 0; i < nreloads; i++) reloads[i].applied = 0;
    for (int i = 0; i < MAX_RT; i++) {
        RenderTarget* rt = &rts[i];
        Shadow* sh = &shadow[i];
        GLuint tex, fbo, src;
        int how = first[i].how;
        if (!rt->used) continue;
        if (sh->pw != rt->pw || sh->ph != rt->ph || !sh->tex) shadow_ensure(i);
        tex = rt->tex; fbo = rt->fbo;   /* swap: the executed frame's target waits in shadow[] */
        rt->tex = sh->tex; rt->fbo = sh->fbo;
        sh->tex = tex; sh->fbo = fbo;
        src = rt == disp ? img[sti].fbo : fbo;
        if (how == FIRST_NONE && (rt == disp || i == disp_end)) how = FIRST_READ;   /* presented */
        if (how == FIRST_READ) blit_color(src, rt->fbo, rt->pw, rt->ph);
        else if (how == FIRST_COVER) {
            int x0 = first[i].x0, y0 = first[i].y0, x1 = first[i].x1, y1 = first[i].y1;
            blit_area(src, rt->fbo, 0, 0, rt->pw, y0);
            blit_area(src, rt->fbo, 0, y1, rt->pw, rt->ph);
            blit_area(src, rt->fbo, 0, y0, x0, y1);
            blit_area(src, rt->fbo, x1, y0, rt->pw, y1);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gt_mark(GT_RB);
#ifdef __ANDROID__
    if (g_and_phase == 2) and_gq_stage(1);
#endif
}

#ifdef __ANDROID__
static void gs_flush_(void);
void gs_flush(void)   /* Android diagnosis: the flushes' wall time */
{
    uint64_t t0;
    if (!g_and_time_frame) { gs_flush_(); g_and_sub = 0; return; }
    t0 = SDL_GetTicksNS();
    gs_flush_();
    g_and_sub = 0;
    g_and_stall.flush_ns += SDL_GetTicksNS() - t0;
}
static void gs_flush_(void)
#else
void gs_flush(void)
#endif
{
    const DrawState* st = &batch_state;
    TexMemo *replay_memo = NULL, *record_memo = NULL;
    uint32_t fbp, fbw, fpsm, fbmsk;
    int premul = 0, tme, afail;
    if (gs_flush_blocked) return;
    uint64_t test;
    if (!batch_valid || vcount == 0) { vcount = 0; nsruns = 0; return; }
    stat_flushes++;
#ifdef __ANDROID__
    g_and_stall.draws++;
#endif

    fbp = st->ctx.frame & 0x1FF; fbw = (st->ctx.frame >> 16) & 0x3F; fpsm = (st->ctx.frame >> 24) & 0x3F;
    fbmsk = (uint32_t)(st->ctx.frame >> 32);
#ifdef __ANDROID__
    g_and_sub = 1;   /* (profiler stage: targets) */
#endif
    cur_rt = rt_get(fbp, fbw, fpsm);

    if (gs_replaying) {   /* never from GS memory: what the executed frame loaded here */
        cur_rt->pending_reload = 0;
        reload_replay(cur_rt, st->epoch);
    } else if (cur_rt->pending_reload) {
        cur_rt->pending_reload = 0;
        if (!batch_full_cover) { rt_load_from_mem(cur_rt); stat_reloads++; reload_record(cur_rt, st->epoch); rt_touch(cur_rt); }
    }
#ifdef __ANDROID__
    uint64_t and_f0 = g_and_time_frame ? SDL_GetTicksNS() : 0;
#endif
    if (!gs_replaying && first[cur_rt - rts].how == FIRST_NONE) rt_first_draw(cur_rt, st, fbw, fpsm);
    glBindFramebuffer(GL_FRAMEBUFFER, cur_rt->fbo);
    {
        uint32_t zbp = st->ctx.zbuf & 0x1FF;
        GLuint dt = depth_for(zbp, cur_rt->pw, cur_rt->ph);
        attach_depth(cur_rt->fbo, dt);
    }
#ifdef __ANDROID__
    if (g_and_time_frame) g_and_stall.fbo_ns += SDL_GetTicksNS() - and_f0;
#ifdef __ANDROID__
    g_and_sub = 2;   /* (profiler stage: scissor/box) */
#endif
#endif
    glViewport(0, 0, cur_rt->pw, cur_rt->ph);
    {
        uint64_t sc = st->ctx.scissor;
        int x0 = sc & 0x7FF, x1 = (sc >> 16) & 0x7FF, y0 = (sc >> 32) & 0x7FF, y1 = (sc >> 48) & 0x7FF;
        if (!cur_rt->has_box || x0 < cur_rt->bx0 || y0 < cur_rt->by0 || x1 > cur_rt->bx1 || y1 > cur_rt->by1) {
            /* the drawn area: the scissor, cut to the vertices of a 2D batch. A copy into a small
               target under a full-screen scissor (PS2_Render_Tex_Sub: 320x240 into FBP 476) must
               not claim the pages below it, where a CLUT upload would mark it stale. Native 3D
               (homogeneous positions) and static runs keep the scissor. */
            int b0 = x0, b1 = x1, c0 = y0, c1 = y1;
            if (!nsruns) {
                float mx0 = 1e9f, mx1 = -1e9f, my0 = 1e9f, my1 = -1e9f;
                int k;
                for (k = 0; k < vcount && vbuf[k].w == 1.0f; k++) {
                    mx0 = fminf(mx0, vbuf[k].x); mx1 = fmaxf(mx1, vbuf[k].x);
                    my0 = fminf(my0, vbuf[k].y); my1 = fmaxf(my1, vbuf[k].y);
                }
                if (k == vcount) {
                    if (mx0 > b0) b0 = (int)floorf(mx0);
                    if (mx1 < b1) b1 = (int)ceilf(mx1);
                    if (my0 > c0) c0 = (int)floorf(my0);
                    if (my1 < c1) c1 = (int)ceilf(my1);
                }
            }
            if (!cur_rt->has_box) { cur_rt->has_box = 1; cur_rt->bx0 = b0; cur_rt->by0 = c0; cur_rt->bx1 = b1; cur_rt->by1 = c1; }
            else {
                if (b0 < cur_rt->bx0) cur_rt->bx0 = b0;
                if (c0 < cur_rt->by0) cur_rt->by0 = c0;
                if (b1 > cur_rt->bx1) cur_rt->bx1 = b1;
                if (c1 > cur_rt->by1) cur_rt->by1 = c1;
            }
        }
        if (g_margin_x && x0 == 0 && x1 >= (int)fbw * 64 - 1) { x0 = -g_margin_x; x1 += g_margin_x; }   /* full width -> widened view */
        glEnable(GL_SCISSOR_TEST);
        glScissor((GLint)((x0 + g_margin_x) * g_scale), (GLint)(y0 * g_scale),
                  (GLsizei)((x1 - x0 + 1) * g_scale), (GLsizei)((y1 - y0 + 1) * g_scale));
    }
#ifdef __ANDROID__
    g_and_sub = 3;   /* (profiler stage: program, uniforms) */
#endif
    glUseProgram(prog);
    clip_distances(1);
    uni2f(u_rt_size, (float)cur_rt->vw, (float)cur_rt->vh);
    uni1f(u_rt_margin, (float)g_margin_x);
    uni1i(u_vr_on, vr_u.on);
    if (vr_u.on) {
        uni4f(u_vr_src, vr_u.src[0], vr_u.src[1], vr_u.src[2], vr_u.src[3]);
        uni4f(u_vr_dst, vr_u.dst[0], vr_u.dst[1], vr_u.dst[2], vr_u.dst[3]);
        uni4f(u_vr_m0, vr_u.m[0], vr_u.m[1], vr_u.m[2], vr_u.m[3]);
        uni4f(u_vr_m1, vr_u.m[4], vr_u.m[5], vr_u.m[6], vr_u.m[7]);
        uni4f(u_vr_m2, vr_u.m[8], vr_u.m[9], vr_u.m[10], vr_u.m[11]);
        uni4f(u_vr_m3, vr_u.m[12], vr_u.m[13], vr_u.m[14], vr_u.m[15]);
        uni2f(u_vr_zab, vr_u.zab[0], vr_u.zab[1]);
        uni1f(u_vr_zs, vr_u.zs);
        uni4f(u_vr_hud, vr_u.hud[0], vr_u.hud[1], vr_u.hud[2], 0.0f);
    }
    {
        uint32_t zpsm = (st->ctx.zbuf >> 24) & 0xF;
        float zmax = zpsm == 0 ? 4294967296.0f : zpsm == 1 ? 16777216.0f : 65536.0f;
        uni1f(u_zscale, 1.0f / zmax);
    }

#ifdef __ANDROID__
    g_and_sub = 4;   /* (profiler stage: depth, mask, blend) */
#endif
    /* depth */
    test = st->ctx.test;
    {
        int zte = (test >> 16) & 1, ztst = (test >> 17) & 3, zmsk = (st->ctx.zbuf >> 32) & 1;
        static const GLenum zf[4] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(zte ? zf[ztst] : GL_ALWAYS);
        glDepthMask(zmsk ? GL_FALSE : GL_TRUE);
    }
    /* color mask from FBMSK (whole channels only) */
    glColorMask((fbmsk & 0xFF) != 0xFF, (fbmsk & 0xFF00) != 0xFF00, (fbmsk & 0xFF0000) != 0xFF0000,
                ((fbmsk >> 24) != 0xFF) && fpsm != PSMCT24);

    apply_blend(st, &premul);
    uni1i(u_premul, premul);
    { extern int g_vr_debug_cores; uni1i(u_dbg, g_vr_debug_cores && vr_u.on); }

#ifdef __ANDROID__
    g_and_sub = 5;   /* (profiler stage: texture) */
#endif
    /* texture */
    tme = (st->prim >> 4) & 1;
    {
        static int notex = -1;   /* read once: an environment lookup per draw */
        if (notex < 0) notex = SDL_getenv("RECVX_DBG_NOTEX") != NULL;
        if (notex) { tme = 0; glDisable(GL_BLEND); }
    }
    uni1i(u_tme, tme);
    if (tme) {
        uint64_t tex0 = st->ctx.tex0;
        uint32_t tbp = tex0 & 0x3FFF, tpsm = (tex0 >> 20) & 0x3F;
        uint32_t tw = 1u << ((tex0 >> 26) & 0xF), th = 1u << ((tex0 >> 30) & 0xF);
        float ox = 0, oy = 0;
        RenderTarget* srt;
        int is16 = 0;
        dbg_tex0 = tex0;
        {   /* replay resources: the executed frame resolves, its replays reuse */
            TexMemo* m = memo_slot(tex0, st->texa, st->epoch, !gs_replaying);
            if (gs_replaying && (!m || (m->rt >= 0 ? !memo_rt_ok(m) : !m->tex))) { vcount = 0; nsruns = 0; return; }
                /* the executed frame did not draw this (or what it sampled is gone): neither does its
                   replay (it would resolve against the state the frame left; replays load no CLUT) */
            if (gs_replaying && m) {
                srt = m->rt >= 0 ? &rts[m->rt] : NULL;
                ox = m->ox; oy = m->oy; is16 = m->is16;
                replay_memo = m;
            } else {
                srt = tbp == mv_tbp ? NULL : rt_for_texture(tbp, tpsm, &ox, &oy, &is16);
                if (m && !gs_replaying) { m->rt = srt && srt != cur_rt ? (int)(srt - rts) : -1; m->fbp = srt ? srt->fbp : 0; m->fbw = srt ? srt->fbw : 0; m->ox = ox; m->oy = oy; m->is16 = is16; record_memo = m; }
            }
        }
        rt_touch(srt);
        GLint filt = ((st->ctx.tex1 >> 5) & 1) ? GL_LINEAR : GL_NEAREST;
        int wms = st->ctx.clamp & 3, wmt = (st->ctx.clamp >> 2) & 3, hdk = 0;
        glActiveTexture(GL_TEXTURE0);
        if (srt && srt != cur_rt) {
            glBindTexture(GL_TEXTURE_2D, srt->tex);
            /* texel (GS units) -> RT normalized */
            uni4f(u_texofs, 1.0f / srt->vw, 1.0f / srt->vh, (ox + g_margin_x) / srt->vw, oy / srt->vh);
            uni1f(u_tex_alpha_scale, 128.0f);
            uni4f(u_rt16, (float)is16, (float)(st->texa & 0xFF), (float)((st->texa >> 32) & 0xFF), (float)((st->texa >> 15) & 1));
        } else {
            int w, h;
            float ascale;
            if (srt == cur_rt) host_log("gs: feedback texture (RT samples itself) at TBP=%u\n", tbp);
            GLuint t;
            if (replay_memo && replay_memo->rt < 0 && replay_memo->tex) { t = replay_memo->tex; w = replay_memo->w; h = replay_memo->h; ascale = replay_memo->ascale; }
            else {
                t = tex_get(tex0, st->texa, &w, &h, &ascale);
                if (record_memo && record_memo->rt < 0) { record_memo->tex = t; record_memo->w = w; record_memo->h = h; record_memo->ascale = ascale; }
            }
            glBindTexture(GL_TEXTURE_2D, t);
            hdk = trep_kind(t);
            if (hdk && !gs_replaying) {   /* vr130 diagnosis: the HD textures drawn while the inventory shows an item (its 3D) */
                extern volatile int g_vr_inv_item; extern volatile uint64_t g_vr_inv_ms; extern void trep_note(unsigned int, int);
                if (g_vr_inv_item >= 0 && SDL_GetTicks() - g_vr_inv_ms < 300) trep_note(t, g_vr_inv_item);
            }
            uni4f(u_texofs, 1.0f / (float)w, 1.0f / (float)h, 0.0f, 0.0f);
            /* vr195: an HD image's alpha at most 0x80 (GS opaque) - the pack's item icons are 255 where the originals
               are 0x80, and the blend (Cs - Cd) * As / 128 + Cd then gave 2Cs - Cd: the command letters behind the
               icons showed through them (measured: icon atlas 79dd8d5b... alpha 255, original decode 128) */
            uni1f(u_tex_alpha_scale, hdk ? -ascale : ascale);
            uni4f(u_rt16, 0, 0, 0, 0);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hdk == 2 ? 0x2703 /* GL_LINEAR_MIPMAP_LINEAR */ : hdk ? GL_LINEAR : filt);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, hdk ? GL_LINEAR : filt);
        if (hdk == 2 && glTexParameterf) glTexParameterf(GL_TEXTURE_2D, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY */, 8.0f);   /* HD pack: smooth at a distance */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (wms == 0 && !srt) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (wmt == 0 && !srt) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        uni1i(u_tex, 0);
        uni2f(u_texsize, (float)tw, (float)th);
        uni1i(u_tfx, (int)((tex0 >> 35) & 3));
        uni1i(u_tcc, (int)((tex0 >> 34) & 1));
        uni1i(u_fst, (int)((st->prim >> 8) & 1));
        uni4i(u_clamp_mode, wms, wmt, 0, 0);
        uni4f(u_clamp_vals, (float)((st->ctx.clamp >> 4) & 0x3FF), (float)((st->ctx.clamp >> 14) & 0x3FF),
                    (float)((st->ctx.clamp >> 24) & 0x3FF), (float)((st->ctx.clamp >> 34) & 0x3FF));
    }
    uni1i(u_fge, (int)((st->prim >> 5) & 1));
    {
        float fr = (float)(st->fogcol & 0xFF), fg = (float)((st->fogcol >> 8) & 0xFF), fb = (float)((st->fogcol >> 16) & 0xFF);
        uni4f(u_fogcol, fr, fg, fb, 0.0f);
    }
    uni1i(u_ate, (int)(test & 1));
    uni1i(u_atst, (int)((test >> 1) & 7));
    uni1f(u_aref, (float)((test >> 4) & 0xFF));

#ifdef __ANDROID__
    g_and_sub = 6;   /* (profiler stage: vertex upload) */
#endif
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
#ifdef __ANDROID__
    uint64_t and_u0 = g_and_time_frame ? SDL_GetTicksNS() : 0;
#endif
    {
        static int vbo_next;
#ifdef __ANDROID__
        if (and_pmap) {
            int q0 = vbo_next / AND_QV, q1 = (vbo_next + vcount - 1) / AND_QV;
            if (vbo_next + vcount > VBO_VERTS || q1 != q0) {   /* into the next quarter, from its start */
                int nq = (q0 + 1) % 4;
                if (and_fence[q0]) and_deletesync(and_fence[q0]);
                and_fence[q0] = and_fencesync(0x9117 /* GL_SYNC_GPU_COMMANDS_COMPLETE */, 0);   /* the draws from q0 issued so far */
                if (and_fence[nq]) {
                    uint64_t w0 = SDL_GetTicksNS();
                    GLenum r = and_clientwait(and_fence[nq], 0x0001 /* GL_SYNC_FLUSH_COMMANDS_BIT */, 2000000000ull);
                    and_waits++; and_wait_ns += SDL_GetTicksNS() - w0;
                    if (SDL_GetTicksNS() - w0 > 5000000) {
                        static int nw;
                        if (nw++ < 20) host_log("android: vertex ring: waited %.1f ms for the GPU (quarter %d, wait %u)\n", (SDL_GetTicksNS() - w0) / 1e6, nq, and_waits);
                    }
                    if (r == 0x911D /* GL_WAIT_FAILED */ || r == 0x911B /* GL_TIMEOUT_EXPIRED */) {
                        static int nl;
                        if (nl++ < 5) host_log("android: vertex ring: waiting for the GPU %s\n", r == 0x911B ? "timed out (2 s)" : "failed");
                    }
                    and_deletesync(and_fence[nq]); and_fence[nq] = NULL;
                }
                vbo_next = nq * AND_QV;
            }
            vbo_first = vbo_next;
            if (!nsruns) memcpy(and_pmap + (size_t)vbo_first * sizeof(GlVertex), vbuf, (size_t)vcount * sizeof(GlVertex));
            else for (int r = 0, at = 0; r <= nsruns; r++) {   /* the parts between the static runs */
                int end = r < nsruns ? sruns[r].start : vcount;
                if (end > at) memcpy(and_pmap + (size_t)(vbo_first + at) * sizeof(GlVertex), vbuf + at, (size_t)(end - at) * sizeof(GlVertex));
                if (r < nsruns) at = sruns[r].start + sruns[r].count;
            }
            vbo_next += vcount;
            goto and_uploaded;
        }
#endif
        if (vbo_next + vcount > VBO_VERTS) {
            glBufferData(GL_ARRAY_BUFFER, VBO_VERTS * sizeof(GlVertex), NULL, GL_STREAM_DRAW);   /* orphan */
            vbo_next = 0;
        }
        vbo_first = vbo_next;
#ifdef __ANDROID__
        /* Android (Quest, Adreno): glBufferSubData into the buffer the GPU is still drawing from waited for it - the
           log measured 17-20 ms of every game frame in these uploads (300+ ms with the native 3D off). The range is
           mapped unsynchronized instead: it is new space in the ring (never reused before the orphan above), so the
           GPU is not using it. glBufferSubData stays as the fallback. */
        static PFN_and_map and_map;
        static PFN_and_unmap and_unmap;
        static int and_map_state;   /* 0 not tried, 1 mapping, -1 glBufferSubData */
        uint8_t* and_dst = NULL;
        if (!and_map_state) {
            and_map = (PFN_and_map)SDL_GL_GetProcAddress("glMapBufferRange");
            and_unmap = (PFN_and_unmap)SDL_GL_GetProcAddress("glUnmapBuffer");
            and_map_state = and_map && and_unmap ? 1 : -1;
            host_log("android: vertex uploads by %s\n", and_map_state > 0 ? "unsynchronized buffer mapping" : "glBufferSubData (no glMapBufferRange)");
        }
        if (and_map_state > 0 && vcount > 0) {
            and_dst = (uint8_t*)and_map(GL_ARRAY_BUFFER, (GLintptr)vbo_first * (GLintptr)sizeof(GlVertex), (GLsizeiptr)(vcount * sizeof(GlVertex)),
                                        0x0002 /* GL_MAP_WRITE_BIT */ | 0x0004 /* GL_MAP_INVALIDATE_RANGE_BIT */ | 0x0020 /* GL_MAP_UNSYNCHRONIZED_BIT */);
            if (!and_dst) {
                static int nf;
                if (nf++ < 5) host_log("android: glMapBufferRange failed (GL error %x) - this upload by glBufferSubData\n", glGetError());
            }
        }
        if (and_dst) {
            if (!nsruns) memcpy(and_dst, vbuf, (size_t)vcount * sizeof(GlVertex));
            else for (int r = 0, at = 0; r <= nsruns; r++) {   /* the parts between the static runs (the gaps are not drawn from here) */
                int end = r < nsruns ? sruns[r].start : vcount;
                if (end > at) memcpy(and_dst + (size_t)at * sizeof(GlVertex), vbuf + at, (size_t)(end - at) * sizeof(GlVertex));
                if (r < nsruns) at = sruns[r].start + sruns[r].count;
            }
            if (!and_unmap(GL_ARRAY_BUFFER)) {   /* (the store was lost: rare, the batch is drawn with whatever is there) */
                static int nu;
                if (nu++ < 5) host_log("android: glUnmapBuffer reported the data lost\n");
            }
        } else
#endif
        if (!nsruns) glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)vbo_first * (GLintptr)sizeof(GlVertex), (GLsizeiptr)(vcount * sizeof(GlVertex)), vbuf);
        else for (int r = 0, at = 0; r <= nsruns; r++) {   /* the parts between the static runs */
            int end = r < nsruns ? sruns[r].start : vcount;
            if (end > at)
                glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(vbo_first + at) * (GLintptr)sizeof(GlVertex), (GLsizeiptr)((end - at) * sizeof(GlVertex)), vbuf + at);
            if (r < nsruns) at = sruns[r].start + sruns[r].count;
        }
        vbo_next += vcount;
    }
#ifdef __ANDROID__
and_uploaded:
    if (g_and_time_frame) g_and_stall.upload_ns += SDL_GetTicksNS() - and_u0;
#endif
#ifdef __ANDROID__
    g_and_sub = 7;   /* (profiler stage: draw) */
#endif
    afail = (test >> 12) & 3;
    if (two_pass(test)) {
        /* alpha-test fail still writes some buffers: second pass for failing pixels */
        draw_batch(0);
        glDepthMask(GL_FALSE);
        if (afail == 2) glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        if (afail == 3) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        if (afail == 2) glDepthMask(GL_TRUE);
        draw_batch(1);
    } else {
        draw_batch(0);
    }
    vcount = 0;
    nsruns = 0;
#ifdef __ANDROID__
    g_and_sub = 0;
#endif
}

/* ---------------- primitive intake ---------------- */
/* 0..255 -> 0..1 (the same division as before, looked up) */
static float u8f[256];
static const float* u8_unit(void)
{
    if (u8f[255] == 0.0f) for (int i = 0; i < 256; i++) u8f[i] = i / 255.0f;
    return u8f;
}


/* VR: the primitive being pushed covers more than half the screen either way (full-screen
   layers, copies, fades): never taken for a projected effect (vr_fx_fix) */
static int prim_big;
static int prim_layer;  /* VR: big and covering the whole screen or flat at one depth: a screen layer (big receding effects stay in the world) */
static int prim_tall;   /* VR: covers more than half the screen top to bottom - a screen layer (fades, filters, copies); wide and low (message bands) goes on the panel */

static void push(const GsVertex* v, const GsVertex* attr, float s, float t)
{
    GlVertex* o = &vbuf[vcount++];
    o->x = v->x; o->y = v->y; o->z = attr->z; o->w = 1.0f;
    o->clip[0] = o->clip[1] = 1.0f;
    {   /* 2: a 3D point drawn through the GS registers (sprites/lines of 3D points, VU1 program output,
           and - vr_fx_fix - anything else written there with a GS depth, as effects the game projects
           itself are; overlays have depth 0): the view depth comes back from its GS depth */
        extern int gs_raw_vr, gs_vu1_vr, gs_in_raw, gs_raw_ui, g_vr_fx_fix;
        int fx = g_vr_fx_fix && o->z > 100.0f && !gs_raw_ui;
        if (fx && prim_layer) { fx = 0; if (vr_st.on) vr_st.nbig++; }   /* a screen layer, not an effect */
        o->vr = (gs_raw_vr || gs_vu1_vr || fx) ? 2.0f : prim_tall ? 0.0f : 3.0f;   /* 3: the game's 2D, on the VR panel; 0: screen layers */
        vr_stat_vertex_p(o, gs_in_raw ? 'S' : 'R');
    }
    o->s = s; o->t = t; o->q = attr->q;
    const float* u = u8_unit();
    o->r = u[attr->r]; o->g = u[attr->g]; o->b = u[attr->b]; o->a = u[attr->a];
    o->fog = attr->f;
}

static void push_vertex(const GsVertex* v, int fst)
{
    if (fst) push(v, v, v->u, v->v);
    else push(v, v, v->s, v->t);
}

/* gl_triangles: the batch is set up for native triangles of these PRIM bits, and no
   register, texture epoch or other batch changed since (a later batch_for would compare equal) */
static struct { int valid; uint32_t serial, epoch; unsigned int prim; } batch_native;

/* the batch for a primitive: flushed and started anew when the state differs */
static void batch_for(int type, const GsVertex* v, int count, const GsContext* c, uint64_t prim)
{
    DrawState st;
    batch_native.valid = 0;
    stat_prims++;
#ifdef __ANDROID__
    g_and_stall.prims++;
#endif
    GLenum mode = (type == 0) ? GL_POINTS : (type == 1) ? GL_LINES : GL_TRIANGLES;
    memset(&st, 0, sizeof(st));
    st.prim = prim & 0x7F8; st.ctx = *c; st.texa = gs.texa; st.fogcol = gs.fogcol; st.pabe = gs.pabe; st.colclamp = gs.colclamp;
    if (!(st.prim & (1 << 4))) { st.ctx.tex0 = 0; st.ctx.tex1 = 0; st.ctx.clamp = 0; }
    st.epoch = gs_tex_epoch;
    if (!batch_valid || memcmp(&st, &batch_state, sizeof(st)) != 0 || mode != batch_mode || vcount + 6 > MAX_VERTS) {
        gs_flush();
        batch_state = st; batch_valid = 1; batch_mode = mode;
        batch_sprite_first = type == 6 && !(st.ctx.test & 1) && !(st.ctx.frame >> 32);
        batch_full_cover = type == 6 && !((prim >> 6) & 1) && !(st.ctx.test & 1) && !(st.ctx.frame >> 32) &&
                           fminf(v[0].x, v[1].x) <= 0.0f && fminf(v[0].y, v[1].y) <= 0.0f &&
                           fmaxf(v[0].x, v[1].x) >= (float)(((st.ctx.frame >> 16) & 0x3F) * 64) && fmaxf(v[0].y, v[1].y) >= 448.0f;
    } else {
        batch_full_cover = 0;
    }
}

/* vr188 diagnosis: the 3 s after the cinema screen goes back to 3D: every big draw (screen layers) - for the flat
   screen's edges seen fading in the 3D. Original draws only (not the eyes' replays). (vr204: the menu part - the
   gold weapon and the COMMAND letters, fixed in vr194/195 - taken out) */
int g_vr_watch_menu;        /* set by present_vr_stereo: the cinema screen is on */
uint64_t g_vr_watch_back;   /* ... the time (ms) the 3D came back */
static void vr_watch(int type, const GsVertex* v, int count, const GsContext* c, uint64_t prim)
{
    extern int g_vr_debug_log, gs_replaying;
    float x0 = v[0].x, x1 = v[0].x, y0 = v[0].y, y1 = v[0].y;
    int tex = (prim >> 4) & 1, abe = (prim >> 6) & 1;
    if (!g_vr_debug_log || gs_replaying || !xr_active()) return;
    for (int i = 1; i < count; i++) { x0 = fminf(x0, v[i].x); x1 = fmaxf(x1, v[i].x); y0 = fminf(y0, v[i].y); y1 = fmaxf(y1, v[i].y); }
    if (g_vr_watch_back && SDL_GetTicks() - g_vr_watch_back < 3000 && ((x1 - x0) > 320.0f || (y1 - y0) > 240.0f)) {
        static int nlog;
        if (nlog++ < 400)
            host_log("vr-watch back to 3D +%u ms: type %d at %.0f,%.0f %.0fx%.0f z %.0f tex %d tbp %u rgba %u %u %u %u blend %s %x frame fbp %u\n",
                     (unsigned)(SDL_GetTicks() - g_vr_watch_back), type, x0, y0, x1 - x0, y1 - y0, v[0].z, tex, (unsigned)(c->tex0 & 0x3FFF),
                     v[count - 1].r, v[count - 1].g, v[count - 1].b, v[count - 1].a, abe ? "on" : "off", (unsigned)c->alpha, (unsigned)(c->frame & 0x1FF));
    }
}

void gs_draw_primitive(int type, const GsVertex* v, int count, const GsContext* c, uint64_t prim)
{
    vr_watch(type, v, count, c, prim);   /* vr188 */
    {
        float x0 = v[0].x, x1 = v[0].x, y0 = v[0].y, y1 = v[0].y;
        for (int i = 1; i < count; i++) { x0 = fminf(x0, v[i].x); x1 = fmaxf(x1, v[i].x); y0 = fminf(y0, v[i].y); y1 = fmaxf(y1, v[i].y); }
        float z0 = v[0].z, z1 = v[0].z;
        for (int i = 1; i < count; i++) { z0 = fminf(z0, v[i].z); z1 = fmaxf(z1, v[i].z); }
        prim_big = (x1 - x0) > 320.0f || (y1 - y0) > 240.0f;
#ifdef __ANDROID__
        /* Android (Quest): flat at one depth is a screen layer only at an overlay's depth - window panes seen straight on
           (big triangles at one world depth, 9374..11463 in the log) went onto the VR panel (yellow) as layers */
        prim_layer = prim_big && (((x1 - x0) >= 600.0f && (y1 - y0) >= 400.0f) || ((z1 - z0) <= 0.002f * fabsf(z1) + 1.0f && z1 <= 100.0f));
        {
            static unsigned nl;
            extern int g_and_diag;
            if (g_and_diag && !prim_layer && prim_big && (z1 - z0) <= 0.002f * fabsf(z1) + 1.0f && nl < 10) {
                nl++;
                host_log("android: big flat primitive at a world depth kept in the world (was a screen layer): x %.0f..%.0f y %.0f..%.0f depth %.1f\n", x0, x1, y0, y1, z1);
            }
        }
#else
        prim_layer = prim_big && (((x1 - x0) >= 600.0f && (y1 - y0) >= 400.0f) || (z1 - z0) <= 0.002f * fabsf(z1) + 1.0f);
#endif
        prim_tall = (y1 - y0) > 240.0f && (prim_layer || z1 <= 100.0f);
#ifdef __ANDROID__
        {   /* Android (Quest) diagnosis: GS-register primitives that go onto the VR panel (yellow with vr_debug_cores) -
               a window's lit panes did; logged once per texture address, with depths and the marks deciding it */
            extern int gs_raw_vr, gs_vu1_vr, gs_raw_ui, g_vr_fx_fix, gs_replaying;
            static uint32_t seen[64];
            static int nseen;
            int fx = g_vr_fx_fix && z1 > 100.0f && !gs_raw_ui && !prim_layer;
            (void)gs_replaying;
            extern int g_and_diag;
            if (g_and_diag && vr_u.on && !gs_raw_vr && !gs_vu1_vr && !fx && !prim_tall && nseen < 64) {   /* (vr_u.on: an eye's replay) */
                uint32_t key = (uint32_t)(c->tex0 & 0x3FFF) | ((uint32_t)type << 16) | (((prim >> 4) & 1) << 20);
                int k = 0;
                while (k < nseen && seen[k] != key) k++;
                if (k == nseen) {
                    seen[nseen++] = key;
                    host_log("android: panel GS primitive type %d (%d vertices, prim %llx, tex %s tbp %u, frame fbp %u): x %.0f..%.0f y %.0f..%.0f "
                             "depth %.1f..%.1f; raw_ui %d, layer %d\n", type, count, (unsigned long long)prim, (prim >> 4) & 1 ? "on" : "off",
                             (unsigned)(c->tex0 & 0x3FFF), (unsigned)(c->frame & 0x1FF), x0, x1, y0, y1, z0, z1, gs_raw_ui, prim_layer);
                }
            }
        }
#endif
    }
    {   /* VR (as native_glue.c): a polygon with corners both in the world (GS depth) and not - the
           others behind or too close to the camera - stretched between the world and the panel: left out */
        extern int gs_raw_vr, gs_vu1_vr, gs_raw_ui, g_vr_fx_fix;
        if (vr_u.on && g_vr_fx_fix && !gs_raw_vr && !gs_vu1_vr && !gs_raw_ui && !prim_layer && count > 1) {
            int fx = 0, other = 0;
            for (int i = 0; i < count; i++) { if (v[i].z > 100.0f) fx++; else other++; }
            if (fx && other) return;
        }
    }
    int fst = (prim >> 8) & 1;
    batch_for(type, v, count, c, prim);
    if (type == 6) { /* sprite: two corners, attributes from the 2nd vertex */
        const GsVertex *a = &v[0], *b = &v[1];
        GsVertex q[4] = { *a, *b, *a, *b };
        if (g_margin_x) {
            /* full-width fills (clear, fade) and full-width copies of a render target (frame
               blend, screen effects) cover the widened view; other 2D stays 4:3 */
            float w = (float)(((c->frame >> 16) & 0x3F) * 64), ox, oy;
            int is16, lo_i = a->x <= b->x ? 0 : 1;
            GsVertex* lo = &q[lo_i];
            GsVertex* hi = &q[1 - lo_i];
            int tex = (prim >> 4) & 1;
            if (lo->x <= 0.0f && hi->x >= w &&
                (!tex || (fst && rt_for_texture(c->tex0 & 0x3FFF, (c->tex0 >> 20) & 0x3F, &ox, &oy, &is16)))) {
                if (tex) {
                    float du = (hi->u - lo->u) / (hi->x - lo->x) * g_margin_x;
                    lo->u -= du; hi->u += du;
                }
                lo->x -= g_margin_x; hi->x += g_margin_x;
                q[2].x = q[0].x; q[3].x = q[1].x; q[2].u = q[0].u; q[3].u = q[1].u;
            }
        }
        float sa = fst ? q[0].u : a->s, ta = fst ? a->v : a->t, sb = fst ? q[1].u : b->s, tb = fst ? b->v : b->t;
        q[1].y = a->y; q[2].y = b->y; q[2].x = q[0].x; q[1].x = q[3].x;
        push(&q[0], b, sa, ta); push(&q[1], b, sb, ta); push(&q[2], b, sa, tb);
        push(&q[1], b, sb, ta); push(&q[3], b, sb, tb); push(&q[2], b, sa, tb);
        if (!fst) for (int i = 6; i > 0; i--) vbuf[vcount - i].q = b->q;
        return;
    }
    if (type == 3) {
        int iip = (prim >> 3) & 1;
        for (int i = 0; i < 3; i++) {
            push_vertex(&v[i], fst);
            if (!iip) { /* flat: color of the last vertex */
                GlVertex* o = &vbuf[vcount - 1];
                const float* u = u8_unit();
                o->r = u[v[2].r]; o->g = u[v[2].g]; o->b = u[v[2].b]; o->a = u[v[2].a];
            }
        }
        return;
    }
    for (int i = 0; i < (type == 0 ? 1 : 2); i++) push_vertex(&v[i], fst);
}

/* native 3D triangles (gs/native.h) -> GL vertices, with the current GS context of PRIM.CTXT */
static void native_to_gl(unsigned int prim, const NativeVertex* v, int n, GlVertex* o)
{
    const GsContext* c = &gs.ctx[(prim >> 9) & 1];
    const float* u = u8_unit();
    float ofx = (float)(c->xyoffset & 0xFFFF) / 16.0f, ofy = (float)((c->xyoffset >> 32) & 0xFFFF) / 16.0f;
    int flat = !((prim >> 3) & 1);
    for (int i = 0; i < n; i += 3)
        for (int k = 0; k < 3; k++, o++) {
            const NativeVertex* p = &v[i + (flat ? 2 : k)];   /* flat shading: colour of the last vertex */
            const NativeVertex* q = &v[i + k];
            o->x = q->x - ofx * q->w; o->y = q->y - ofy * q->w; o->z = q->z; o->w = q->w;
            o->s = q->s; o->t = q->t; o->q = 1.0f;
            o->r = u[p->r]; o->g = u[p->g]; o->b = u[p->b]; o->a = u[p->a];
            o->fog = (float)((int32_t)q->fog & 0xFF);   /* XYZF2 F: FTOI4, low 8 bits */
            o->clip[0] = q->near_d; o->clip[1] = q->far_d;
            o->vr = (q->cull & 0x80) ? 1.0f : (q->cull & 0x40) ? 2.25f : (q->cull & 0x20) ? 0.0f : 3.0f;   /* (2.25: a native effect - the same path, told apart by vr_debug_cores) */   /* 2: 2D with a GS depth (vr_fx_fix); 3: 2D on the VR panel; 0: screen layers */
            vr_stat_vertex(o);
#ifdef __ANDROID__
            extern int g_and_diag;
            if (g_and_diag) {   /* Android (Quest) diagnosis: missing arms, glass, parts of objects (only on the Quest) - vertices that
                   cannot draw: not finite, w <= 0 for 3D, far outside the GS area; counted per path, a few logged */
                static unsigned nall[4], nbad[4], nlog;
                static uint64_t t_last;
                int k = (q->cull & 0x80) ? 0 : (q->cull & 0x40) ? 1 : (q->cull & 0x20) ? 2 : 3;
                int bad = !isfinite(o->x) || !isfinite(o->y) || !isfinite(o->z) || !isfinite(o->w) || !isfinite(o->s) || !isfinite(o->t) ||
                          (k == 0 ? o->w <= 0.0f && o->clip[0] >= 0.0f : 0) ||
                          (o->w > 0.0f && (fabsf(o->x / o->w) > 20000.0f || fabsf(o->y / o->w) > 20000.0f));
                static unsigned tick;
                nall[k]++;
                if (bad) {
                    nbad[k]++;
                    if (nlog < 40) {
                        nlog++;
                        host_log("android: bad vertex (%s, prim %x): x %g y %g z %g w %g s %g t %g clip %g %g\n",
                                 k == 0 ? "3D" : k == 1 ? "effect in the world" : k == 2 ? "screen layer" : "2D", prim, o->x, o->y, o->z, o->w, o->s, o->t, o->clip[0], o->clip[1]);
                    }
                }
                if (!(++tick & 4095) && SDL_GetTicksNS() - t_last > 5000000000ull) {
                    if (t_last && (nbad[0] | nbad[1] | nbad[2] | nbad[3]))
                        host_log("android: bad vertices last 5 s: 3D %u of %u, effects %u of %u, screen layers %u of %u, 2D %u of %u\n",
                                 nbad[0], nall[0], nbad[1], nall[1], nbad[2], nall[2], nbad[3], nall[3]);
                    t_last = SDL_GetTicksNS();
                    memset(nall, 0, sizeof(nall)); memset(nbad, 0, sizeof(nbad));
                }
            }
#endif
        }
}

/* GL triangles into the batch: the state is set up once (as for a GS triangle; a flush when
   the batch is full), then the vertices go straight in */
static void gl_triangles(unsigned int prim, const GlVertex* g, int n, int sfirst)
{
    const GsContext* c = &gs.ctx[(prim >> 9) & 1];
    static const GsVertex none[3];   /* (batch_for reads vertices of sprites only) */
    for (int i = 0; i < n;) {
        int k;
        if (i == 0 && batch_native.valid && batch_native.serial == gs_reg_serial && batch_native.epoch == gs_tex_epoch &&
            batch_native.prim == (prim & 0x7F8) && vcount + 6 <= MAX_VERTS) {
            stat_prims++; batch_full_cover = 0;
#ifdef __ANDROID__
            g_and_stall.prims++;
#endif   /* what batch_for does for a primitive of the same state */
        } else {
            batch_for(3, none, 3, c, (uint64_t)(prim & 0x7F8) | 3);
            batch_native.valid = 1; batch_native.serial = gs_reg_serial; batch_native.epoch = gs_tex_epoch; batch_native.prim = prim & 0x7F8;
        }
        k = (MAX_VERTS - vcount) / 3 * 3;
        if (k > n - i) k = n - i;
        if (sfirst < 0) memcpy(&vbuf[vcount], &g[i], (size_t)k * sizeof(GlVertex));
        else if (nsruns && sruns[nsruns - 1].start + sruns[nsruns - 1].count == vcount &&
                 sruns[nsruns - 1].first + sruns[nsruns - 1].count == sfirst + i) sruns[nsruns - 1].count += k;   /* continues the run */
        else {
            GROW_TO(sruns, (size_t)nsruns + 1, csruns);
            sruns[nsruns++] = (StaticRun){ vcount, k, sfirst + i };
        }
        vcount += k;
        i += k;
    }
}

/* The GL vertices of each native strip as the executed frame drew them, by event: a replay
   draws a strip it does not blend from here instead of clipping and converting it again. */
static GlVertex* ncache; static size_t nncache, cncache;
static int ncache_on_gpu;   /* uploaded to svbo (gs_native_cache_upload) */
static uint32_t (*ncache_at)[2]; static size_t cncache_at;   /* per event: first vertex, count (UINT32_MAX: none) */
void gs_native_cache_begin(size_t nev)
{
    nncache = 0;
    ncache_on_gpu = 0;
    GROW_TO(ncache_at, nev, cncache_at);
    memset(ncache_at, 0xFF, nev * sizeof(*ncache_at));
}

/* draw n triangle vertices; ev >= 0: the executed frame's strip of that event (kept) */
void gs_draw_native(unsigned int prim, const NativeVertex* v, int n, long ev)
{
    static GlVertex tmp[64 * 9];
    GlVertex* o = tmp;
    if (ev >= 0) {
        GROW_TO(ncache, nncache + (size_t)n, cncache);
        o = ncache + nncache;
        ncache_at[ev][0] = (uint32_t)nncache; ncache_at[ev][1] = (uint32_t)n;
        nncache += (size_t)n;
    }
    native_to_gl(prim, v, n, o);
    gl_triangles(prim, o, n, -1);
}

/* replay: draw the executed frame's strip of event ev again; 0 if it was not kept */
int gs_draw_native_cached(unsigned int prim, size_t ev)
{
    if (ev >= cncache_at || ncache_at[ev][0] == UINT32_MAX) return 0;
    if (ncache_at[ev][1]) gl_triangles(prim, ncache + ncache_at[ev][0], (int)ncache_at[ev][1], ncache_on_gpu ? (int)ncache_at[ev][0] : -1);
    if (vr_st.on) for (uint32_t k = 0; k < ncache_at[ev][1]; k++) vr_stat_vertex(&ncache[ncache_at[ev][0] + k]);
    return 1;
}

/* executed frame done, replays follow: its native strips to the GPU once */
static void gs_native_cache_upload(void)
{
    if (!nncache || ncache_on_gpu) return;
    glBindBuffer(GL_ARRAY_BUFFER, svbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nncache * sizeof(GlVertex)), ncache, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    ncache_on_gpu = 1;
}

/* ---------------- presentation ---------------- */

/* where the 640x480 display area goes in a ww x wh window: 4:3 pillarboxed, stretched, or
   the whole widened target (dw = 640 + margins wide) for hor+ */
static void display_rect(int ww, int wh, int* vx, int* vy, int* vw, int* vh, float* dw)
{
    *dw = 640.0f;
    *vw = (int)(wh * 4.0f / 3.0f); *vh = wh;
    if (*vw > ww) { *vw = ww; *vh = (int)(ww * 3.0f / 4.0f); }
    if (g_config.aspect == ASPECT_STRETCH) { *vw = ww; *vh = wh; }
    if (g_margin_x) {   /* hor+: the whole widened render target, same pixel aspect as 4:3 */
        float a = (4.0f / 3.0f) * gs_hor_scale();
        *dw = 640.0f + 2.0f * g_margin_x;
        *vw = (int)(wh * a + 0.5f); *vh = wh;
        if (*vw > ww) { *vw = ww; *vh = (int)(ww / a + 0.5f); }
    }
    *vx = (ww - *vw) / 2; *vy = (wh - *vh) / 2;
}

/* window pixel (from the top left) -> the game's 2D screen space (0..640 x 0..480, hor+
   margins outside 0..640); main thread */
void gs_window_to_screen(float px, float py, float* x, float* y)
{
    int ww, wh, vx, vy, vw, vh;
    float dw;
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    display_rect(ww, wh, &vx, &vy, &vw, &vh, &dw);
    *x = vw > 0 ? (px - vx) * dw / vw - (dw - 640.0f) / 2 : 0;
    *y = vh > 0 ? (py - vy) * 480.0f / vh : 0;
}

/* the 640x480 display area of a render-target texture to the window */
static void draw_display(GLuint tex, int rt_vw, int rt_vh)
{
    int ww, wh;
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    clip_distances(0);   /* also for the overlay drawn after it */
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, ww, wh);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (tex) {
        float dw, dh = 480.0f;
        int vx, vy, vw, vh;
        display_rect(ww, wh, &vx, &vy, &vw, &vh, &dw);
        glViewport(vx, vy, vw, vh);
        glUseProgram(present_prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        /* a replay target may have been sampled by a draw with TEX1's NEAREST filter */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glUniform1i(up_tex, 0);
        {
            float b, k, ig;
            int on = picture(&b, &k, &ig);
            glUniform4f(up_pic, b, k, ig, on ? 1.0f : 0.0f);
        }
        {
            float x0 = g_margin_x && dw > 640.0f ? 0.0f : (float)g_margin_x;
            glUniform4f(up_rect, x0 / rt_vw, 0.0f, (x0 + dw) / rt_vw, dh / rt_vh);
        }
        glBindVertexArray(present_vao);
        glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
    }
}

static void save_window(const char* dir, const char* kind)
{
    int ww, wh;
    SDL_Surface* sf;
    char path[1024];
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    sf = SDL_CreateSurface(ww, wh, SDL_PIXELFORMAT_ABGR8888);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, ww, wh, GL_RGBA, GL_UNSIGNED_BYTE, sf->pixels);
    SDL_FlipSurface(sf, SDL_FLIP_VERTICAL);
    SDL_snprintf(path, sizeof(path), "%s/%s_%05llu.bmp", dir, kind, (unsigned long long)frame_no);
    SDL_SaveBMP(sf, path);
    SDL_DestroySurface(sf);
}

static int every = 30, from;   /* RECVX_DUMP_EVERY, RECVX_DUMP_FROM (dump_frame) */
static const char* dump_dir;
/* debugging aid: RECVX_DUMP_FRAMES=<dir> saves every 30th presented frame
   (RECVX_DUMP_EVERY=n: every n-th, the in-between and window images too; render targets and
   statistics stay at every 30th; RECVX_DUMP_FROM=n: none before frame n - each dump stalls the GPU) */
static int dump_on(void)
{
    static int checked;
    if (!checked++) {
        const char* e = SDL_getenv("RECVX_DUMP_EVERY");
        dump_dir = SDL_getenv("RECVX_DUMP_FRAMES");
        if (e && SDL_atoi(e) > 0) every = SDL_atoi(e);
        if (SDL_getenv("RECVX_DUMP_FROM")) from = SDL_atoi(SDL_getenv("RECVX_DUMP_FROM"));
    }
    return dump_dir != NULL;
}
static void dump_frame(int interp)
{
    if (!dump_on() || frame_no < (uint64_t)from) return;
    if (frame_no % every == 0 && interp) {   /* the in-between image of a dumped frame */
        save_window(dump_dir, "interp");
    } else if (frame_no % 30 == 0) {
        static uint64_t last_ns;
        uint64_t now = SDL_GetTicksNS();
        host_log("frame %llu: %u prims, %u flushes, %u rt reloads, %u decodes (%u kpx/30f), %.2f ms/frame avg\n", (unsigned long long)frame_no, stat_prims, stat_flushes,
                 stat_reloads, stat_decodes, stat_decode_px / 1000, last_ns ? (now - last_ns) / 30.0 / 1e6 : 0.0);
        last_ns = now; stat_reloads = 0; stat_decodes = 0; stat_decode_px = 0;
        for (int r = 0; r < MAX_RT; r++) if (rts[r].used) {
            SDL_Surface* rs = SDL_CreateSurface(rts[r].pw, rts[r].ph, SDL_PIXELFORMAT_ABGR8888);
            char rp[1024];
            glBindFramebuffer(GL_READ_FRAMEBUFFER, rts[r].fbo);
            glReadPixels(0, 0, rts[r].pw, rts[r].ph, GL_RGBA, GL_UNSIGNED_BYTE, rs->pixels);
            SDL_snprintf(rp, sizeof(rp), "%s/rt%u_%05llu.bmp", dump_dir, rts[r].fbp, (unsigned long long)frame_no);
            SDL_SaveBMP(rs, rp);
            SDL_DestroySurface(rs);
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        save_window(dump_dir, "frame");
    } else if (frame_no % every == 0 && !interp) {
        save_window(dump_dir, "frame");
    }
}

/* ---------------- frame interpolation: render-target snapshot ---------------- */

static int final_vw, final_vh;

static void blit_color(GLuint src_fbo, GLuint dst_fbo, int w, int h)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);
    glDisable(GL_SCISSOR_TEST);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void ensure_target(GLuint* tex, GLuint* fbo, int* cw, int* ch, int w, int h)
{
    if (*tex && *cw == w && *ch == h) return;
#ifdef __ANDROID__
    int fresh = !*tex;
#endif
    if (!*tex) { glGenTextures(1, tex); glGenFramebuffers(1, fbo); }
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
#ifdef __ANDROID__
    if (and_fov_next && fresh) and_foveate(*tex);   /* (the feature bits are set once a texture) */
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    *cw = w; *ch = h;
}

/* the targets' state after the executed frame (replays draw into shadow[], gs_replay_begin) */
static int rt_snapshot(void)
{
    gs_flush();
    for (int i = 0; i < MAX_RT; i++) {
        rts_snap[i] = rts[i];
        if (rts[i].used && (shadow[i].pw != rts[i].pw || shadow[i].ph != rts[i].ph || !shadow[i].tex)) shadow_ensure(i);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return 1;
}

/* after a replay: the executed frame's targets back; the replay's become the shadows again */
#ifdef __ANDROID__
static int rt_restore_deleted;   /* (gl_record: a target the recorded replay used is gone) */
static int and_rec_ok;           /* the left eye's recording can be issued again for the right eye */
#endif
static void rt_restore(void)
{
    for (int i = 0; i < MAX_RT; i++) {
        RenderTarget* r = &rts[i];
        const RenderTarget* sn = &rts_snap[i];
        if (!sn->used) {   /* created during the replay: lost (recreated from GS memory when next used) */
            if (r->used) { glDeleteTextures(1, &r->tex); glDeleteFramebuffers(1, &r->fbo); fb_forget(); memset(r, 0, sizeof(*r));
#ifdef __ANDROID__
                rt_restore_deleted = 1;
#endif
            }
            continue;
        }
        /* the slot's textures now (the replay's, or another target's if the replay evicted it) */
        if (r->used && r->pw == sn->pw && r->ph == sn->ph) { shadow[i] = (Shadow){ r->tex, r->fbo, r->pw, r->ph }; }
        else {
            if (r->used) { glDeleteTextures(1, &r->tex); glDeleteFramebuffers(1, &r->fbo); fb_forget();
#ifdef __ANDROID__
                rt_restore_deleted = 1;
#endif
            }
            memset(&shadow[i], 0, sizeof(shadow[i]));
        }
        *r = *sn;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* StoreImage: rendered pixels exist only in OpenGL; copy the region of the render target
   covering GS address bp back into GS memory (sampled at native resolution) */
void gs_rt_to_mem(uint32_t bp, uint32_t bw, uint32_t psm, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    RenderTarget* rt = NULL;
    float* px;
    int sx, sy, sw, sh;
    if (w == 0 || h == 0 || (psm != PSMCT32 && psm != PSMCT24 && !psm_is16(psm))) return;
    for (int i = 0; i < MAX_RT; i++)
        if (rts[i].used && rts[i].fbp * 32 == bp && rts[i].fbw == (bw ? bw : 1)) { rt = &rts[i]; break; }   /* bp: 256-byte blocks */
    if (!rt) return;
#ifdef __ANDROID__
    uint32_t t0 = SDL_GetTicks(), t1, t2;
#endif
    gs_flush();
#ifdef __ANDROID__
    t1 = SDL_GetTicks();
#endif
    rt_touch(rt);
    sx = (int)((x + g_margin_x) * g_scale); sy = (int)(y * g_scale);
    sw = (int)(w * g_scale); sh = (int)(h * g_scale);
    if (sx + sw > rt->pw) sw = rt->pw - sx;
    if (sy + sh > rt->ph) sh = rt->ph - sy;
    if (sw <= 0 || sh <= 0) return;
    px = malloc((size_t)sw * sh * 16);   /* up to 335 MB at scale 8 */
    if (!px) { host_log("gs: no memory to read back %dx%d pixels (StoreImage)\n", sw, sh); return; }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, rt->fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(sx, sy, sw, sh, GL_RGBA, GL_FLOAT, px);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#ifdef __ANDROID__
    t2 = SDL_GetTicks();
#endif
    for (uint32_t j = 0; j < h; j++)
        for (uint32_t i = 0; i < w; i++) {
            int qx = (int)((i + 0.5f) * g_scale), qy = (int)((j + 0.5f) * g_scale);
            const float* c;
            uint32_t r, g, b, a;
            if (qx >= sw || qy >= sh) continue;
            c = &px[((size_t)qy * sw + qx) * 4];
#define CL(v, m) (uint32_t)((v) <= 0 ? 0 : (v) * (m) >= 255.0f ? 255 : (v) * (m) + 0.5f)
            r = CL(c[0], 255.0f); g = CL(c[1], 255.0f); b = CL(c[2], 255.0f); a = CL(c[3], 128.0f);
#undef CL
            if (psm_is16(psm)) gs_write_pixel((int)psm, bp, bw, x + i, y + j, (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) | (a >= 128 ? 0x8000 : 0));
            else gs_write_pixel((int)psm, bp, bw, x + i, y + j, r | (g << 8) | (b << 16) | (a << 24));
        }
    free(px);
    for (uint32_t p = rt->fbp; p < rt->fbp + rt_page_count(rt) && p < 512; p++) page_dirty[p] = 1;
#ifdef __ANDROID__
    {
        static int n;
        uint32_t t3 = SDL_GetTicks();
        g_and_stall.store_ns += (uint64_t)(t3 - t0) * 1000000u; g_and_stall.store_n++;
        if (t3 - t0 >= 2 || n < 20) {
            n++;
            host_log("android: StoreImage %ux%u (read %dx%d) flush %ums glReadPixels %ums copy %ums total %ums\n",
                     w, h, sw, sh, t1 - t0, t2 - t1, t3 - t2, t3 - t0);
        }
    }
#endif
}

/* StoreRenderTex on the GPU (RECVX_NATIVE_RTEX): GS pixels (0,0)-(w,h) of a render target
   copied at render resolution (margin and scale as in gs_rt_to_mem) into a new texture, 0 if
   there is no such target (transparent black). wide (hor+): GS x w/2 -+ w/2 * gs_hor_scale(),
   the view the game drew with its widened clip volume, into a texture as many times wider
   (the full-screen render texture, drawn back over the widened view) */
unsigned int gs_capture_rt(uint32_t fbp, uint32_t fbw, uint32_t w, uint32_t h, int wide)
{
    RenderTarget* rt = rt_find(fbp, fbw ? fbw : 1);
    GLuint tex = 0, fbo = 0;
    float gw = wide && g_margin_x ? w * gs_hor_scale() : (float)w;   /* GS pixels captured across */
    int cw = 0, ch = 0, pw = (int)(gw * g_scale), ph = (int)(h * g_scale);
    gs_flush();
    rt_touch(rt);
    ensure_target(&tex, &fbo, &cw, &ch, pw, ph);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (rt) {
        int sx = (int)((g_margin_x + (w - gw) / 2) * g_scale), sw = pw < rt->pw - sx ? pw : rt->pw - sx, sh = ph < rt->ph ? ph : rt->ph;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rt->fbo);
        glBlitFramebuffer(sx, 0, sx + sw, sh, 0, 0, sw, sh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo); fb_forget();
    return tex;
}

/* ---------------- movies (pc/src/movie) ---------------- */

static const char* movie_vs =
    "#version 330 core\n"
    "void main(){\n"
    "  vec2 c = vec2((gl_VertexID & 1) != 0 ? 1.0 : 0.0, (gl_VertexID & 2) != 0 ? 1.0 : 0.0);\n"
    "  gl_Position = vec4(c * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";
/* YCbCr (ITU-R BT.601, video range) -> RGB, one fragment per image pixel (row 0 = the top
   row, as uploaded). 4:2:0 chroma as MPEG-2 places it: on the even luma columns, between
   two luma rows; interpolated. Alpha 0x80, as the IPU's output. */
static const char* movie_fs =
    "#version 330 core\n"
    "uniform sampler2D u_y, u_cb, u_cr; out vec4 o;\n"
    "void main(){\n"
    "  vec2 p = gl_FragCoord.xy;\n"
    "  vec2 c = vec2(p.x * 0.5 + 0.25, p.y * 0.5) / vec2(textureSize(u_cb, 0));\n"
    "  float y = (texelFetch(u_y, ivec2(p), 0).r - 16.0 / 255.0) * (255.0 / 219.0);\n"
    "  float cb = (texture(u_cb, c).r - 128.0 / 255.0) * (255.0 / 224.0), cr = (texture(u_cr, c).r - 128.0 / 255.0) * (255.0 / 224.0);\n"
    "  o = vec4(clamp(vec3(y + 1.402 * cr, y - 0.344136 * cb - 0.714136 * cr, y + 1.772 * cb), 0.0, 1.0), 128.0 / 255.0);\n"
    "}\n";

/* Textures at TBP tbp are movie frame index from now until the next executed frame: the
   frame's planes are uploaded once and converted into mv_rgb (512 x 512, the TEX0 size the
   game's sprite uses, image at the top left). Replays bind the same frame again. */
void gs_movie_bind(uint32_t tbp, int index)
{
    static GLuint prog_yuv, planes[3], fbo;
    static int pw, ph;
    static uint32_t shown;
    uint32_t serial;
    int w, h, stride;
    const uint8_t* yuv;
    gs_flush();   /* draws before this sample what was bound before */
    mv_tbp = tbp;
    if (!prog_yuv) {
        prog_yuv = compile(movie_vs, movie_fs);
        glUseProgram(prog_yuv);
        glUniform1i(glGetUniformLocation(prog_yuv, "u_y"), 1);
        glUniform1i(glGetUniformLocation(prog_yuv, "u_cb"), 2);
        glUniform1i(glGetUniformLocation(prog_yuv, "u_cr"), 3);
        glGenTextures(3, planes);
        glGenTextures(1, &mv_rgb);
        glBindTexture(GL_TEXTURE_2D, mv_rgb);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mv_rgb, 0);
    }
    yuv = movie_lock_frame(index, &serial, &w, &h, &stride);
    if (yuv && serial == shown) { movie_unlock_frame(); return; }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    fb_forget();
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    clip_distances(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (!yuv || w > 512 || h > 512) {   /* nothing decoded: black */
        if (yuv) movie_unlock_frame();
        glClearColor(0, 0, 0, 128.0f / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shown = 0;
        return;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < 3; i++) {
        int cw = i ? w / 2 : w, ch = i ? h / 2 : h;
        const uint8_t* src = yuv + (i ? (size_t)stride * h + (size_t)(i - 1) * (stride / 2) * (h / 2) : 0);
        glActiveTexture(GL_TEXTURE0 + 1 + i);
        glBindTexture(GL_TEXTURE_2D, planes[i]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, i ? stride / 2 : stride);
        if (w != pw || h != ph) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, cw, ch, 0, GL_RED, GL_UNSIGNED_BYTE, src);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cw, ch, GL_RED, GL_UNSIGNED_BYTE, src);
        }
    }
    movie_unlock_frame();
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    pw = w; ph = h; shown = serial;
    glViewport(0, 0, w, h);
    glUseProgram(prog_yuv);
    glBindVertexArray(present_vao);
    glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
}

/* ---------------- presentation (render thread) ---------------- */

static int snapshot_valid, final_dumped, interp_dumped;

/* frame boundary: resolution / widescreen changes are applied between frames */
void gs_frame_begin(void)
{
    gt_exec_begin();
    if (++memo_gen == 0) memo_gen = 1;   /* resources of the frame about to execute */
    if (ndoomed) { glDeleteTextures((GLsizei)ndoomed, doomed); ndoomed = 0; }
    trep_frame_begin();   /* HD replacements get their mipmaps (texreplace.c) */
    nreloads = 0;
    mv_tbp = UINT32_MAX;   /* a movie frame binds it again (EV_MOVIE) */
    snapshot_valid = 0;   /* no replays of the previous frame while this one is half executed (download) */
    memset(first, 0, sizeof(first));
    gs_set_scale(gs_wanted_scale());
    gs_update_widescreen();
    /* ponytail: only the display target gets a start copy. Every other target is cleared or
       fully redrawn each frame before it is read (FBP 0 and depth by the libgraph clear after
       the swap, 380 by Ps2ShadowStart, 476 by PS2_Render_Tex_Sub). A target that kept pixels
       across frames would need its own start copy (or a full rt_snapshot here). */
    start_valid = 0;
    if (g_config.interpolate && rtstart_enabled()) {
        RenderTarget* rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
        if (rt && fin_fresh && fin_fbp == rt->fbp && fin_fbw == rt->fbw && img[fin].pw == rt->pw && img[fin].ph == rt->ph) sti = fin;
        else if (rt) {
            sti = fin ^ 1;   /* img[fin] may still be presented (a partial frame) */
            ensure_target(&img[sti].tex, &img[sti].fbo, &img[sti].pw, &img[sti].ph, rt->pw, rt->ph);
            blit_color(rt->fbo, img[sti].fbo, rt->pw, rt->ph);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        if (rt) { start_fbp = rt->fbp; start_fbw = rt->fbw; start_valid = 1; }
    }
    fin_fresh = 0;
}

/* a game frame was executed: keep its image; with interpolation, snapshot the render
   targets so in-between replays can be undone */
void gs_frame_executed(int interpolate)
{
    RenderTarget* rt;
    gs_flush();
    if (!(frame_no & 63)) {   /* every 64th frame: glGetError waits for the driver thread (Mesa glthread),
                                 which cost ~0.2 ms per flush (250 ms a frame at 1,200 flushes); errors stay set until read */
        GLenum e = glGetError();
        static int nerr;
        if (e && nerr++ < 10) host_log("gl error 0x%x in frames %llu..%llu\n", e, (unsigned long long)(frame_no > 63 ? frame_no - 63 : 1), (unsigned long long)frame_no + 1);
    }
    frame_no++;
    rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
    disp_end = rt ? (int)(rt - rts) : -1;
    snapshot_valid = 0;
    final_dumped = interp_dumped = 0;
    if (rt) {
        if (start_valid && sti == fin) fin ^= 1;   /* the start image stays for the replays */
        ensure_target(&img[fin].tex, &img[fin].fbo, &img[fin].pw, &img[fin].ph, rt->pw, rt->ph);
        blit_color(rt->fbo, img[fin].fbo, rt->pw, rt->ph);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        fin_fresh = 1; fin_fbp = rt->fbp; fin_fbw = rt->fbw;
        final_vw = rt->vw; final_vh = rt->vh;
        if (dump_on()) { draw_display(img[fin].tex, final_vw, final_vh); dump_frame(0); }   /* RECVX_DUMP_FRAMES: the real image */
        final_dumped = 1;
        if (interpolate || (xr_active() && g_vr_stereo)) { snapshot_valid = rt_snapshot(); gs_native_cache_upload(); }   /* VR: eye replays */
    } else {
        final_vw = 0;
    }
    stat_prims = stat_flushes = 0;
    gt_exec_end();
}

/* debugging aid: RECVX_DUMP_FRAMES with RECVX_DUMP_OVERLAY=1 also saves every 30th (RECVX_DUMP_EVERY) presented
   frame as shown, with the overlay (name tags, windows): win_<frame>.bmp */
static void dump_window(void)
{
    static const char* dir; static int checked; static uint64_t last;
    if (!checked++) dir = SDL_getenv("RECVX_DUMP_OVERLAY") ? SDL_getenv("RECVX_DUMP_FRAMES") : NULL;
    if (!dir || frame_no % every || frame_no < (uint64_t)from || frame_no == last) return;
    last = frame_no;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    save_window(dir, "win");
}

/* RECVX_REPLAY_CHECK=1: replay the executed frame at t = 1, which must reproduce it, and
   compare (replay_check.c); everything is restored, the shown image is unaffected */
void gs_replay_check(const struct Frame* cur)
{
    extern void replay_check_compare(uint64_t frame, uint8_t* exec, uint8_t* replay, int w, int h);
    RenderTarget* rt;
    uint8_t *a, *b;
    int w = img[fin].pw, h = final_vh ? img[fin].ph * 480 / final_vh : 0;   /* the displayed 480 lines */
    if (!snapshot_valid || !final_vw) return;
    a = malloc((size_t)w * h * 4); b = calloc((size_t)w * h, 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, img[fin].fbo);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, a);
    stream_replay(cur, 1.0f);
    rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
    if (rt) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rt->fbo);
        glReadPixels(0, 0, rt->pw < w ? rt->pw : w, rt->ph < h ? rt->ph : h, GL_RGBA, GL_UNSIGNED_BYTE, b);   /* ponytail: assumes same size (scale fixed within a frame) */
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    rt_restore();
    replay_check_compare(frame_no, a, b, w, h);
    free(a); free(b);
}


/* ---------------- VR presentation (render thread) ---------------- */

const char* g_vr_status;   /* shown by the overlay: why this frame is on the cinema screen */

/* VR danger vignette (vr_glue.c sets g_vr_danger while the player's life is in Danger): a light
   red edge over each eye, pulsing like a heartbeat, centred on the eye's own view axis */
volatile int g_vr_danger;
int g_vr_danger_fx = 1;   /* ini vr_danger_fx */
/* vr112: the life's colours (vr_glue.c): poisoned - a purple edge pulsing as the red one (red and purple
   by turns, softer, when also in Danger); a hit in Caution - one yellow (life 60-119) or orange (30-59)
   flash; an enemy's body at the eyes - a dark edge (0..1); the heartbeat's sound with the pulse */
volatile int g_vr_poison, g_vr_flash_col;
volatile uint64_t g_vr_flash_ms;
volatile float g_vr_close_dark;
volatile float g_vr_move_dark;   /* vr116: walking with the stick / turning, 0..1 (vr_glue.c) */
int g_vr_comfort = 0;            /* ini vr_comfort: the comfort vignette while moving - 0 off, 1 weak, 2 medium, 3 strong */
int g_vr_heart_sound = 60;   /* ini vr_heart_sound: its volume, 0 = none */
void audio_vr_heart(int volume);
static GLuint vig_prog;
static GLint uv_tan, uv_amt, uv_col, uv_rng;
static const char* vig_vs =
    "#version 330 core\n"
    "uniform vec4 u_tan;\n"   /* l, r, u, d of the eye */
    "out vec2 v_t;\n"
    "void main(){\n"
    "  vec2 c = vec2((gl_VertexID & 1) != 0 ? 1.0 : 0.0, (gl_VertexID & 2) != 0 ? 1.0 : 0.0);\n"
    "  v_t = vec2(mix(u_tan.x, u_tan.y, c.x), mix(u_tan.w, u_tan.z, c.y));\n"
    "  gl_Position = vec4(c.x * 2.0 - 1.0, c.y * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";
static const char* vig_fs =
    "#version 330 core\n"
    "in vec2 v_t; uniform float u_amt; uniform vec4 u_col; uniform vec2 u_rng; out vec4 o;\n"
    "void main(){ float r = length(v_t); float a = smoothstep(u_rng.x, u_rng.y, r) * u_amt; o = vec4(u_col.rgb, a); }\n";

/* VR markers (VrView.mk*, vr_glue.c) over each eye: the aiming dot (a small white dot,
   vr_sight_size hundredths of a degree across) and the holster's ammo count (6 cm high digits on a dark
   rounded plate). Drawn after the eye's picture, facing the eye. */
int g_vr_sight_size = 30;
static GLuint mk_prog;
static GLint um_rect, um_kind, um_val, um_aspect;
static const char* mk_vs =
    "#version 330 core\n"
    "uniform vec4 u_rect;\n"   /* centre (NDC) and half size (NDC) */
    "out vec2 v_c;\n"
    "void main(){\n"
    "  vec2 c = vec2((gl_VertexID & 1) != 0 ? 1.0 : -1.0, (gl_VertexID & 2) != 0 ? 1.0 : -1.0);\n"
    "  v_c = c;\n"
    "  gl_Position = vec4(u_rect.xy + c * u_rect.zw, 0.0, 1.0);\n"
    "}\n";
static const char* mk_fs =
    "#version 330 core\n"
    "in vec2 v_c; out vec4 o;\n"
    "uniform int u_kind; uniform int u_val; uniform float u_aspect;\n"
    "const int F[70] = int[70](14,17,19,21,25,17,14, 4,12,4,4,4,4,14, 14,17,1,2,4,8,31, 31,2,4,2,1,17,14, 2,6,10,18,31,2,2,\n"
    "                          31,16,30,1,1,17,14, 6,8,16,30,17,17,14, 31,1,2,4,8,8,8, 14,17,17,14,17,17,14, 14,17,17,15,1,2,12);\n"
    "float seg(vec2 p, vec2 a, vec2 b){ vec2 pa = p - a, ba = b - a; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); return length(pa - ba * h); }\n"
    /* signed distance-ish to a triangle (negative inside): the largest of the distances outside its three edges */
    "float edge(vec2 p, vec2 a, vec2 b, vec2 c){ vec2 n = normalize(vec2(b.y - a.y, a.x - b.x)); if (dot(c - a, n) > 0.0) n = -n; return dot(p - a, n); }\n"
    "float tri(vec2 p, vec2 a, vec2 b, vec2 c){ return max(max(edge(p, a, b, c), edge(p, b, c, a)), edge(p, c, a, b)); }\n"
    "void main(){\n"
    "  float aa;\n"
    "  if (u_kind == 3) {\n"   /* vr223: the shotgun's reticle, as RE4's - a ring and a centre dot (vr224: white, thin; vr225: two side arcs and the dot, as the owner drew it: ( + ) ) */
    "    float r = length(v_c); aa = fwidth(r);\n"
    "    float ring = 1.0 - smoothstep(0.012 - aa, 0.012 + aa, abs(r - 0.9));\n"
    "    ring *= 1.0 - smoothstep(0.62, 0.72, abs(v_c.y) / max(r, 1e-4));\n"   /* vr225: only the sides - ( . ) (vr227: thinner still) */
    "    float cd = 1.0 - smoothstep(0.035 - aa, 0.035 + aa, r);\n"
    "    float a = max(ring, cd) * 0.9;\n"
    "    if (a <= 0.0) discard; o = vec4(1.0, 1.0, 1.0, a); return;\n"
    "  }\n"
    "  if (u_kind == 0) {\n"   /* the dot */
    "    float r = length(v_c); aa = fwidth(r);\n"
    "    float a = 1.0 - smoothstep(1.0 - aa * 1.5, 1.0, r);\n"
    "    if (a <= 0.0) discard; o = vec4(1.0, 1.0, 1.0, a); return;\n"
    "  }\n"
    "  if (u_kind == 1 && u_val == 1) {\n"   /* vr127: close enough - the A button: a dark disc, a thin white rim, a white A */
    "    float r = length(v_c); aa = fwidth(r);\n"
    "    float disc = 1.0 - smoothstep(0.95 - aa, 0.95 + aa, r);\n"
    "    float rim = smoothstep(0.80 - aa, 0.80 + aa, r) * disc;\n"
    "    vec2 p = v_c;\n"
    "    float d = min(min(seg(p, vec2(-0.34, -0.45), vec2(0.0, 0.47)), seg(p, vec2(0.34, -0.45), vec2(0.0, 0.47))), seg(p, vec2(-0.19, -0.10), vec2(0.19, -0.10)));\n"
    "    float glyph = 1.0 - smoothstep(0.075 - aa, 0.075 + aa, d);\n"
    "    float w = max(rim, glyph * disc);\n"
    "    float a = max(disc * 0.55, w * 0.95);\n"
    "    if (a <= 0.0) discard;\n"
    "    o = vec4(mix(vec3(0.08, 0.07, 0.06), vec3(1.0, 0.97, 0.88), clamp(w / max(a, 1e-3) * min(w * 1.2, 1.0), 0.0, 1.0)), a); return;\n"
    "  }\n"
    "  if (u_kind == 1) {\n"   /* the interaction icon: a dark disc, a thin white rim, a white arrowhead pointing down */
    "    float r = length(v_c); aa = fwidth(r);\n"
    "    float disc = 1.0 - smoothstep(0.95 - aa, 0.95 + aa, r);\n"
    "    float rim = smoothstep(0.86 - aa, 0.86 + aa, r) * disc;\n"
    "    vec2 p = v_c;\n"
    "    float d = min(tri(p, vec2(-0.40, 0.28), vec2(0.0, 0.04), vec2(0.0, -0.42)), tri(p, vec2(0.0, 0.04), vec2(0.40, 0.28), vec2(0.0, -0.42)));\n"
    "    float glyph = 1.0 - smoothstep(-aa, aa, d);\n"
    "    float w = max(rim, glyph * disc);\n"
    "    float a = max(disc * 0.65, w);\n"
    "    if (a <= 0.0) discard;\n"
    "    o = vec4(mix(vec3(0.05, 0.05, 0.055), vec3(0.96, 0.95, 0.93), clamp(w, 0.0, 1.0)), a); return;\n"
    "  }\n"
    /* the number: n digits of 5x7 dots in cells of 6x9, on a plate of n*6+2 x 11 (vr132: or "a/b", u_val 1000000 + a * 1000 + b) */
    "  int two = u_val >= 1000000 ? 1 : 0, va = two != 0 ? (u_val - 1000000) / 1000 : u_val, vb = two != 0 ? u_val - 1000000 - va * 1000 : 0;\n"
    "  int na = va >= 100 ? 3 : va >= 10 ? 2 : 1, nb = vb >= 100 ? 3 : vb >= 10 ? 2 : 1;\n"
    "  int n = two != 0 ? na + 1 + nb : na;\n"
    "  vec2 g = (v_c * 0.5 + 0.5) * vec2(float(n * 6 + 2), 11.0);\n"
    "  g.y = 11.0 - g.y;\n"
    "  vec2 q = abs(v_c) * vec2(u_aspect, 1.0); vec2 lim = vec2(u_aspect, 1.0) - 0.25;\n"
    "  float plate = length(max(q - lim, 0.0)) - 0.25; aa = fwidth(plate);\n"
    "  float inside = 1.0 - smoothstep(-aa, aa, plate), rim = inside * smoothstep(-0.16 - aa, -0.16 + aa, plate);\n"
    "  float pa = inside * 0.82;\n"
    "  float lit = 0.0;\n"
    "  vec2 c = g - vec2(1.0, 2.0);\n"
    "  int di = int(floor(c.x / 6.0));\n"
    "  if (di >= 0 && di < n && c.y >= 0.0 && c.y < 7.0) {\n"
    "    int v = 10, len = na, pos = di, num = va;\n"
    "    if (di > na) { len = nb; pos = di - na - 1; num = vb; }\n"
    "    if (di != na || two == 0) { v = num; for (int k = 0; k < len - 1 - pos; k++) v /= 10; v = v - (v / 10) * 10; }\n"
    "    vec2 cc = vec2(c.x - float(di) * 6.0, c.y);\n"
    "    int col = int(floor(cc.x)), row = int(floor(cc.y));\n"
    "    if (col < 5) {\n"
    "      int bits = v == 10 ? (row < 2 ? 1 : row == 2 ? 2 : row == 3 ? 4 : row == 4 ? 8 : 16) : F[v * 7 + row];\n"
    "      if (((bits >> (4 - col)) & 1) != 0) { vec2 f = fract(cc) - 0.5; float e = length(max(abs(f) - 0.36, 0.0)) - 0.16; float ea = fwidth(e); lit = 1.0 - smoothstep(-ea, ea, e); }\n"
    "    }\n"
    "  }\n"
    "  float a = max(pa, lit);\n"
    "  if (a <= 0.0) discard;\n"
    "  vec3 base = mix(vec3(0.035, 0.045, 0.03), vec3(0.40, 0.48, 0.22), rim);\n"
    "  o = vec4(mix(base, vec3(0.55, 0.95, 0.38), lit), a);\n"
    "}\n";

static float vr_world_L[16];   /* (vr130: here for the markers too; defined again below - a tentative definition) */
static int vr_world_ok;
static void draw_markers(const VrView* v, const float M[16], const VrEye* E, int w, int h)
{
    if (!v || v->mk_n <= 0) return;
    if (!mk_prog) {
        mk_prog = compile(mk_vs, mk_fs);
        um_rect = glGetUniformLocation(mk_prog, "u_rect"); um_kind = glGetUniformLocation(mk_prog, "u_kind");
        um_val = glGetUniformLocation(mk_prog, "u_val"); um_aspect = glGetUniformLocation(mk_prog, "u_aspect");
    }
    glViewport(0, 0, w, h);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glUseProgram(mk_prog);
    glBindVertexArray(present_vao);
    for (int i = 0; i < v->mk_n && i < 12; i++) {
        const float* p = v->mk[i];
        float V[3], e[3], tx, ty, nx, ny, hx, hy, ang;
        int kind = v->mk_kind[i];
        if (kind == 2 && vr_world_ok) for (int k = 0; k < 3; k++) V[k] = vr_world_L[k] * p[0] + vr_world_L[4 + k] * p[1] + vr_world_L[8 + k] * p[2] + vr_world_L[12 + k];   /* vr130: the holster's count moves with the body, as its box */
        else for (int k = 0; k < 3; k++) V[k] = v->G[k] * p[0] + v->G[4 + k] * p[1] + v->G[8 + k] * p[2] + v->G[12 + k];   /* world -> V */
        for (int k = 0; k < 3; k++) e[k] = M[k] * V[0] + M[4 + k] * V[1] + M[8 + k] * V[2] + M[12 + k];               /* V -> eye (x right, y down, z forward) */
        if (e[2] < 0.05f) continue;
        tx = e[0] / e[2]; ty = -e[1] / e[2];                                    /* tangents right, up */
        nx = (tx - E->l) / (E->r - E->l) * 2.0f - 1.0f;
        ny = (ty - E->d) / (E->u - E->d) * 2.0f - 1.0f;
        if (kind == 0) { ang = tanf(g_vr_sight_size / 100.0f * (3.14159265f / 360.0f)); hx = hy = ang; }
        else if (kind == 1) { hx = hy = 0.04f / e[2]; }
        else if (kind == 3) { hx = hy = tanf(1.6f * (3.14159265f / 180.0f)); }   /* vr223: the ring 3.2 degrees across, at any distance */                         /* 8 cm across */
        else {
            int val = v->mk_val[i], two = val >= 1000000, va = two ? (val - 1000000) / 1000 : val, vb = two ? val - 1000000 - va * 1000 : 0;
            int n = (va >= 100 ? 3 : va >= 10 ? 2 : 1) + (two ? 1 + (vb >= 100 ? 3 : vb >= 10 ? 2 : 1) : 0);
            hy = (two ? 0.015f : 0.035f) / e[2];                                /* the plate 7 cm high (vr132: "a/b" 3 cm, in front of the box) */
            hx = hy * (float)(n * 6 + 2) / 11.0f;
            glUniform1f(um_aspect, (float)(n * 6 + 2) / 11.0f);
        }
        glUniform1i(um_kind, kind);
        glUniform1i(um_val, v->mk_val[i]);
        glUniform4f(um_rect, nx, ny, hx / (E->r - E->l) * 2.0f, hy / (E->u - E->d) * 2.0f);
        glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
    }
    glBindVertexArray(vao);
    glDisable(GL_BLEND);
}

/* vr77: the laser sight - a faint red beam from each gun's muzzle, strongest at the gun and gone by
   half way, and a small red dot where the shot would hit; moved with the hand like the gun */
static float vr_laser_L[2][16];
static int vr_laser_on[2];
static GLuint lz_prog;
static GLint ul_a, ul_b, ul_hw, ul_dot, ul_iz, ul_depth, ul_rect, ul_eye, ul_z;
static const char* lz_vs =
    "#version 330 core\n"
    "uniform vec2 u_a; uniform vec2 u_b; uniform vec2 u_hw; uniform int u_dot; uniform vec2 u_iz;\n"
    "out vec2 v_q; out float v_iz;\n"
    "void main(){\n"
    "  float t = (gl_VertexID & 1) != 0 ? 1.0 : 0.0, s = (gl_VertexID & 2) != 0 ? 1.0 : -1.0;\n"
    "  if (u_dot != 0) { vec2 c = vec2(t * 2.0 - 1.0, s); v_q = c; v_iz = u_iz.y; gl_Position = vec4(u_b + c * u_hw, 0.0, 1.0); return; }\n"
    "  vec2 d = (u_b - u_a) / u_hw; vec2 n = normalize(vec2(-d.y, d.x)) * u_hw;\n"
    "  v_q = vec2(t, s); v_iz = mix(u_iz.x, u_iz.y, t);\n"
    "  gl_Position = vec4(mix(u_a, u_b, t) + n * s, 0.0, 1.0);\n"
    "}\n";
/* vr80: hidden behind whatever is nearer in the eye's picture (the gun, the hand, a wall): the replay's
   depth for this eye (GS depth of the eye's distance, z = -zb / (zs * Ez) - za, times u_z.x) read back
   as a distance (u_z: scale, zs, zb, za; u_z.y 0 = no depth to test) */
static const char* lz_fs =
    "#version 330 core\n"
    "in vec2 v_q; in float v_iz; out vec4 o; uniform int u_dot; uniform sampler2D u_depth; uniform vec4 u_rect; uniform vec2 u_eye; uniform vec4 u_z;\n"
    "void main(){\n"
    "  if (u_z.y > 0.0) {\n"
    "    vec2 c = gl_FragCoord.xy / u_eye; vec2 uv = vec2(mix(u_rect.x, u_rect.z, c.x), mix(u_rect.y, u_rect.w, 1.0 - c.y));\n"
    "    float gz = texture(u_depth, uv).r / u_z.x + u_z.w;\n"
    "    if (abs(gz) > 1e-6) { float ez = -u_z.z / (u_z.y * gz); if (ez > 0.0 && ez < 1.0 / v_iz - 0.015) discard; }\n"
    "  }\n"
    "  if (u_dot != 0) { float r = length(v_q); float a = 1.0 - smoothstep(0.55, 1.0, r); if (a <= 0.0) discard; o = vec4(1.0, 0.12, 0.08, 0.85 * a); return; }\n"
    "  float a = 0.55 * clamp(1.0 - v_q.x / 0.65, 0.0, 1.0) * (1.0 - smoothstep(0.3, 1.0, abs(v_q.y)));\n"
    "  if (a <= 0.0) discard; o = vec4(1.0, 0.1, 0.05, a);\n"
    "}\n";
static int laser_ndc(const float L[16], const float M[16], const VrEye* E, const float p[3], float e[3])
{
    float V[3];
    for (int k = 0; k < 3; k++) V[k] = L[k] * p[0] + L[4 + k] * p[1] + L[8 + k] * p[2] + L[12 + k];   /* world -> V (with the hand) */
    for (int k = 0; k < 3; k++) e[k] = M[k] * V[0] + M[4 + k] * V[1] + M[8 + k] * V[2] + M[12 + k];   /* V -> eye */
    (void)E;
    return 1;
}
static void eye_to_ndc(const VrEye* E, const float e[3], float n[2])
{
    float tx = e[0] / e[2], ty = -e[1] / e[2];
    n[0] = (tx - E->l) / (E->r - E->l) * 2.0f - 1.0f;
    n[1] = (ty - E->d) / (E->u - E->d) * 2.0f - 1.0f;
}
static void draw_lasers(const VrView* v, const float M[16], const VrEye* E, int w, int h, const RenderTarget* rt)
{
    GLuint dtex = 0;
    int any = 0;
    for (int s = 0; s < 2; s++) any |= v && v->laser_ok[s] && vr_laser_on[s];
    if (!any) return;
    if (!lz_prog) {
        lz_prog = compile(lz_vs, lz_fs);
        ul_a = glGetUniformLocation(lz_prog, "u_a"); ul_b = glGetUniformLocation(lz_prog, "u_b");
        ul_hw = glGetUniformLocation(lz_prog, "u_hw"); ul_dot = glGetUniformLocation(lz_prog, "u_dot"); ul_iz = glGetUniformLocation(lz_prog, "u_iz");
        ul_depth = glGetUniformLocation(lz_prog, "u_depth"); ul_rect = glGetUniformLocation(lz_prog, "u_rect");
        ul_eye = glGetUniformLocation(lz_prog, "u_eye"); ul_z = glGetUniformLocation(lz_prog, "u_z");
    }
    glViewport(0, 0, w, h);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glUseProgram(lz_prog);
    for (int i = 0; i < 8 && rt; i++) if (dz[i].tex && dz[i].zbp == rt->zbp && dz[i].pw == rt->pw && dz[i].ph == rt->ph) dtex = dz[i].tex;   /* the eye's depth (the replay's) */
    {
        uint32_t zpsm = (uint32_t)((gs.ctx[0].zbuf >> 24) & 0xF);
        float zmax = zpsm == 0 ? 4294967296.0f : zpsm == 1 ? 16777216.0f : 65536.0f;
        float dw = 640.0f + 2.0f * g_margin_x;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dtex);
        glUniform1i(ul_depth, 0);
        glUniform4f(ul_rect, 0.0f, 0.0f, rt ? dw / rt->vw : 1.0f, rt ? 480.0f / rt->vh : 1.0f);
        glUniform2f(ul_eye, (float)w, (float)h);
        glUniform4f(ul_z, 1.0f / zmax, dtex ? vr_u.zs : 0.0f, v->zb, v->za);
        {   /* VR diagnosis (vr_debug_log) */
            extern int g_vr_debug_log;
            static int last = -1;
            if (g_vr_debug_log && (dtex != 0) != last) { last = dtex != 0; host_log("vr-diag: laser hidden behind nearer things: %s (zpsm %u)\n", dtex ? "yes" : "no depth found", zpsm); }
        }
    }
    glBindVertexArray(present_vao);
    for (int s = 0; s < 2; s++) {
        float ea[3], eb[3], na[2], nb[2];
        if (!v->laser_ok[s] || !vr_laser_on[s]) continue;
        laser_ndc(vr_laser_L[s], M, E, v->laser_a[s], ea);
        laser_ndc(vr_laser_L[s], M, E, v->laser_b[s], eb);
        if (ea[2] < 0.05f && eb[2] < 0.05f) continue;
        if (ea[2] < 0.05f || eb[2] < 0.05f) {   /* clipped at the near plane */
            float* in = ea[2] < 0.05f ? ea : eb; const float* o = ea[2] < 0.05f ? eb : ea;
            float k = (0.05f - in[2]) / (o[2] - in[2]);
            for (int q = 0; q < 3; q++) in[q] += (o[q] - in[q]) * k;
        }
        eye_to_ndc(E, ea, na); eye_to_ndc(E, eb, nb);
        glUniform2f(ul_iz, 1.0f / ea[2], 1.0f / eb[2]);
        glUniform1i(ul_dot, 0);
        glUniform2f(ul_a, na[0], na[1]); glUniform2f(ul_b, nb[0], nb[1]);
        glUniform2f(ul_hw, 1.6f / (float)w, 1.6f / (float)h);   /* about 1.6 pixels to each side */
        glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
        if (eb[2] >= 0.05f) {   /* the dot where it hits: about 1 cm across at its distance, at least 3 pixels */
            float r = 0.006f / eb[2], rx = r / (E->r - E->l) * 2.0f, ry = r / (E->u - E->d) * 2.0f;
            if (rx < 3.0f / w) rx = 3.0f / w;
            if (ry < 3.0f / h) ry = 3.0f / h;
            glUniform1i(ul_dot, 1);
            glUniform2f(ul_hw, rx, ry);
            glDrawArrays(0x0005, 0, 4);
        }
    }
    glBindVertexArray(vao);
    glDisable(GL_BLEND);
}

/* vr87: the ammo box at the holster / in the left hand - a small olive box in the game's look (a
   yellow band, three rounds on its sides, worn edges), lit by the room's item light, hidden behind
   what is nearer in the eye's picture (as the laser) */
static float vr_world_L[16];   /* world -> V of this image (the holster's box) */
static int vr_world_ok;
static GLuint bx_prog;
static GLint ub_vis, ub_c[8], ub_fr, ub_depth, ub_rect, ub_eye, ub_z, ub_amb, ub_tr[6], ub_btex, ub_bt;
/* vr131: ammo boxes with their own picture (the HD pack's texture of the item as the inventory shows it, found with
   the vr130 log): the file and where each face is on it (x0 y0 x1 y1 of 512: faces x-, x+ the ends; y-, y+ bottom, top;
   z-, z+ the large sides) */
static const struct { int item; const char* file; float r[6][4]; } box_pics[] = {
    { 12, "b773dd6ded67f309-892c799538d422ab-00002213.dds",   /* handgun bullets: "Amerikansky Volk 9 mm" */
      { { 366, 4, 429, 116 }, { 366, 4, 429, 116 }, { 2, 322, 201, 390 }, { 2, 124, 201, 190 }, { 2, 2, 201, 119 }, { 2, 2, 201, 119 } } },
};
static const char* bx_vs =
    "#version 330 core\n"
    "uniform vec4 u_c[8]; uniform vec4 u_fr; uniform int u_vis; uniform vec4 u_tr[6];\n"
    "out vec2 v_uv; out float v_ez; flat out int v_face; out vec2 v_tuv;\n"
    "const int F[24] = int[24](0,4,6,2, 1,3,7,5, 0,1,5,4, 2,6,7,3, 0,2,3,1, 4,5,7,6);\n"
    "const int T[6] = int[6](0,1,2, 0,2,3);\n"
    "const vec2 Q[4] = vec2[4](vec2(0.0,0.0), vec2(1.0,0.0), vec2(1.0,1.0), vec2(0.0,1.0));\n"
    "void main(){\n"
    "  int f = gl_VertexID / 6, k = T[gl_VertexID % 6];\n"
    "  if ((u_vis & (1 << f)) == 0) { gl_Position = vec4(0.0, 0.0, 2.0, 1.0); v_uv = vec2(0.0); v_face = f; v_ez = 1.0; gl_ClipDistance[0] = -1.0; return; }\n"
    "  vec3 e = u_c[F[f * 4 + k]].xyz;\n"
    "  v_uv = Q[k]; v_face = f; v_ez = e.z;\n"
    "  { int ci = F[f * 4 + k]; float bx = float(ci & 1), by = float((ci >> 1) & 1), bz = float((ci >> 2) & 1); vec2 fu;\n"   /* vr131: the face's place on the box's picture */
    "    if (f == 0) fu = vec2(bz, 1.0 - by); else if (f == 1) fu = vec2(1.0 - bz, 1.0 - by);\n"
    "    else if (f == 2) fu = vec2(bx, bz); else if (f == 3) fu = vec2(bx, 1.0 - bz);\n"
    "    else if (f == 4) fu = vec2(1.0 - bx, 1.0 - by); else fu = vec2(bx, 1.0 - by);\n"
    "    v_tuv = mix(u_tr[f].xy, u_tr[f].zw, fu); }\n"
    "  float tx = e.x / e.z, ty = -e.y / e.z;\n"
    "  vec2 n = vec2((tx - u_fr.x) / (u_fr.y - u_fr.x) * 2.0 - 1.0, (ty - u_fr.w) / (u_fr.z - u_fr.w) * 2.0 - 1.0);\n"
    "  gl_Position = vec4(n * e.z, 0.0, e.z);\n"
    "  gl_ClipDistance[0] = e.z - 0.05;\n"
    "}\n";
static const char* bx_fs =
    "#version 330 core\n"
    "in vec2 v_uv; in float v_ez; flat in int v_face; in vec2 v_tuv; out vec4 o;\n"
    "uniform sampler2D u_depth; uniform vec4 u_rect; uniform vec2 u_eye; uniform vec4 u_z; uniform vec4 u_amb; uniform sampler2D u_btex; uniform int u_bt;\n"
    "float h(vec2 p){ return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }\n"
    "void main(){\n"
    "  if (u_z.y > 0.0) {\n"
    "    vec2 c = gl_FragCoord.xy / u_eye; vec2 uv = vec2(mix(u_rect.x, u_rect.z, c.x), mix(u_rect.y, u_rect.w, 1.0 - c.y));\n"
    "    float gz = texture(u_depth, uv).r / u_z.x + u_z.w;\n"
    "    if (abs(gz) > 1e-6) { float ez = -u_z.z / (u_z.y * gz); if (ez > 0.0 && ez < v_ez - 0.01) discard; }\n"
    "  }\n"
    "  vec2 q = v_uv; vec3 col = vec3(0.30, 0.34, 0.16);\n"
    "  if (u_bt != 0) {\n"   /* vr131: the ammo's own box picture */
    "    float sh = v_face == 3 ? 1.0 : v_face == 2 ? 0.45 : v_face >= 4 ? 0.85 : 0.7;\n"
    "    vec3 amb = clamp(u_amb.rgb, vec3(0.35), vec3(1.0));\n"
    "    o = vec4(texture(u_btex, v_tuv).rgb * sh * amb * 1.15, 1.0); return;\n"
    "  }\n"
    "  bool side = v_face >= 4, end = v_face <= 1;\n"
    "  if ((side || end) && q.y > 0.56 && q.y < 0.72) col = vec3(0.72, 0.58, 0.16);\n"
    "  if (side && q.y > 0.14 && q.y < 0.46) { for (int i = 0; i < 3; i++) { float x = 0.3 + 0.2 * float(i);\n"
    "    if (abs(q.x - x) < 0.045 && q.y < 0.38) col = vec3(0.70, 0.52, 0.20);\n"
    "    if (q.y >= 0.38 && abs(q.x - x) < 0.045 * (0.46 - q.y) / 0.08) col = vec3(0.55, 0.32, 0.16); } }\n"
    "  if (v_face == 3 && abs(q.y - 0.5) < 0.03) col *= 0.55;\n"
    "  float d = min(min(q.x, 1.0 - q.x), min(q.y, 1.0 - q.y)); if (d < 0.05) col *= 0.6;\n"
    "  col *= 0.85 + 0.15 * h(floor(q * 24.0) + float(v_face));\n"
    "  float sh = v_face == 3 ? 1.0 : v_face == 2 ? 0.45 : side ? 0.8 : 0.65;\n"
    "  vec3 amb = clamp(u_amb.rgb, vec3(0.35), vec3(1.0));\n"
    "  o = vec4(col * sh * amb * 1.15, 1.0);\n"
    "}\n";
static void draw_box(const VrView* v, const float M[16], const VrEye* E, int w, int h, const RenderTarget* rt)
{
    const float* L;
    float e8[8][3];
    GLuint dtex = 0;
    if (!v || !v->box_mode) return;
    L = v->box_mode == 2 ? (vr_laser_on[1] ? vr_laser_L[1] : NULL) : (vr_world_ok ? vr_world_L : NULL);
    if (!L) return;
    for (int c = 0; c < 8; c++) {
        const float* p = v->box_c[c];
        float V[3];
        for (int k = 0; k < 3; k++) V[k] = L[k] * p[0] + L[4 + k] * p[1] + L[8 + k] * p[2] + L[12 + k];
        for (int k = 0; k < 3; k++) e8[c][k] = M[k] * V[0] + M[4 + k] * V[1] + M[8 + k] * V[2] + M[12 + k];
    }
    if (!bx_prog) {
        bx_prog = compile(bx_vs, bx_fs);
        for (int c = 0; c < 8; c++) { char nm[16]; snprintf(nm, sizeof(nm), "u_c[%d]", c); ub_c[c] = glGetUniformLocation(bx_prog, nm); }
        ub_fr = glGetUniformLocation(bx_prog, "u_fr"); ub_vis = glGetUniformLocation(bx_prog, "u_vis");
        ub_depth = glGetUniformLocation(bx_prog, "u_depth"); ub_rect = glGetUniformLocation(bx_prog, "u_rect");
        ub_eye = glGetUniformLocation(bx_prog, "u_eye"); ub_z = glGetUniformLocation(bx_prog, "u_z"); ub_amb = glGetUniformLocation(bx_prog, "u_amb");
        for (int f = 0; f < 6; f++) { char nm[16]; snprintf(nm, sizeof(nm), "u_tr[%d]", f); ub_tr[f] = glGetUniformLocation(bx_prog, nm); }
        ub_btex = glGetUniformLocation(bx_prog, "u_btex"); ub_bt = glGetUniformLocation(bx_prog, "u_bt");
    }
    for (int i = 0; i < 8 && rt; i++) if (dz[i].tex && dz[i].zbp == rt->zbp && dz[i].pw == rt->pw && dz[i].ph == rt->ph) dtex = dz[i].tex;
    glViewport(0, 0, w, h);
    glDisable(GL_BLEND);
    clip_distances(1);
    glUseProgram(bx_prog);
    {
        uint32_t zpsm = (uint32_t)((gs.ctx[0].zbuf >> 24) & 0xF);
        float zmax = zpsm == 0 ? 4294967296.0f : zpsm == 1 ? 16777216.0f : 65536.0f;
        float dw = 640.0f + 2.0f * g_margin_x;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dtex);
        glUniform1i(ub_depth, 0);
        glUniform4f(ub_rect, 0.0f, 0.0f, rt ? dw / rt->vw : 1.0f, rt ? 480.0f / rt->vh : 1.0f);
        glUniform2f(ub_eye, (float)w, (float)h);
        glUniform4f(ub_z, 1.0f / zmax, dtex ? vr_u.zs : 0.0f, v->zb, v->za);
    }
    for (int c = 0; c < 8; c++) glUniform4f(ub_c[c], e8[c][0], e8[c][1], e8[c][2], 1.0f);
    glUniform4f(ub_fr, E->l, E->r, E->u, E->d);
    {   /* vr90: only the faces turned to the eye (a closed box, the far faces not seen through the near ones) */
        static const int F[24] = { 0,4,6,2, 1,3,7,5, 0,1,5,4, 2,6,7,3, 0,2,3,1, 4,5,7,6 };
        float bc[3] = { 0, 0, 0 };
        int vis = 0;
        for (int c = 0; c < 8; c++) for (int k = 0; k < 3; k++) bc[k] += e8[c][k] / 8.0f;
        for (int f = 0; f < 6; f++) {
            float fc[3] = { 0, 0, 0 }, d = 0.0f;
            for (int q = 0; q < 4; q++) for (int k = 0; k < 3; k++) fc[k] += e8[F[f * 4 + q]][k] / 4.0f;
            for (int k = 0; k < 3; k++) d += (fc[k] - bc[k]) * fc[k];   /* outward normal . (eye -> face): < 0 faces the eye */
            if (d < 0.0f) vis |= 1 << f;
        }
        glUniform1i(ub_vis, vis);
    }
    glUniform4f(ub_amb, v->box_amb[0], v->box_amb[1], v->box_amb[2], 1.0f);
    {   /* vr131: the ammo's own picture, when the pack has it */
        extern unsigned int trep_named(const char* file, int* mips);
        GLuint bt = 0;
        int mips = 0;
        for (size_t i = 0; i < sizeof(box_pics) / sizeof(box_pics[0]) && !bt; i++)
            if (box_pics[i].item == v->box_item) {
                float tr[6][4];
                bt = trep_named(box_pics[i].file, &mips);
                for (int f = 0; f < 6; f++) for (int k = 0; k < 4; k++) tr[f][k] = box_pics[i].r[f][k] / 512.0f;
                for (int f = 0; f < 6; f++) glUniform4f(ub_tr[f], tr[f][0], tr[f][1], tr[f][2], tr[f][3]);
            }
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, bt);
        if (bt) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mips ? 0x2703 /* GL_LINEAR_MIPMAP_LINEAR */ : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(ub_btex, 2);
        glUniform1i(ub_bt, bt != 0);
    }
    glBindVertexArray(present_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(vao);
    clip_distances(0);
}

static void vignette_pass(const VrEye* E, float amt, float r, float g, float b, float r0, float r1)
{
    glUseProgram(vig_prog);
    glUniform4f(uv_tan, E->l, E->r, E->u, E->d);
    glUniform1f(uv_amt, amt);
    glUniform4f(uv_col, r, g, b, 1.0f);
    glUniform2f(uv_rng, r0, r1);   /* where the edge starts and where it is full (the view's tangent) */
    glBindVertexArray(present_vao);
    glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
}

static void draw_vignette(const VrEye* E, int w, int h)
{
    static unsigned last_beat;
    float comfort_amt, comfort_r0 = 0, comfort_r1 = 1;
    uint64_t now = SDL_GetTicks();
    unsigned bi = (unsigned)(now / 900);
    float t, beat, amt = 0, r = 0, g = 0, b = 0, dark = g_vr_close_dark;
    int danger = g_vr_danger && g_vr_danger_fx, poison = g_vr_poison && g_vr_danger_fx;
    t = (float)(now % 900) / 1000.0f;   /* ~67 beats a minute: lub-dub, pause */
    beat = expf(-((t - 0.05f) * (t - 0.05f)) / 0.004f) + 0.6f * expf(-((t - 0.30f) * (t - 0.30f)) / 0.005f);
    if (danger && poison) {   /* red and purple by turns, softer */
        if (bi & 1) { r = 0.45f; g = 0.0f; b = 0.6f; } else { r = 0.75f; g = 0.0f; b = 0.0f; }
        amt = 0.12f + 0.22f * beat;
    } else if (poison) { r = 0.45f; g = 0.0f; b = 0.6f; amt = 0.18f + 0.30f * beat; }
    else if (danger) { r = 0.75f; g = 0.0f; b = 0.0f; amt = 0.18f + 0.30f * beat; }   /* light: at most ~half opaque at the very edge */
    else if (g_vr_flash_col && g_vr_danger_fx && now - g_vr_flash_ms < 500) {   /* one flash a hit */
        float d = (float)(now - g_vr_flash_ms);
        float env = d < 60.0f ? d / 60.0f : expf(-(d - 60.0f) / 130.0f);
        if (g_vr_flash_col == 1) { r = 0.85f; g = 0.75f; b = 0.0f; } else { r = 0.9f; g = 0.4f; b = 0.0f; }
        amt = 0.45f * env;
    }
    if ((danger || poison) && g_vr_heart_sound > 0 && bi != last_beat) audio_vr_heart(g_vr_heart_sound);   /* (once a beat: the eyes share it) */
    last_beat = bi;
    {   /* vr116: the comfort vignette - darker edges, a narrower view, while walking with the stick or turning */
        static const float lv[4][3] = { { 0, 0, 0 }, { 0.80f, 1.25f, 0.75f }, { 0.62f, 1.10f, 0.90f }, { 0.45f, 0.95f, 1.0f } };
        int c = g_vr_comfort > 0 && g_vr_comfort < 4 ? g_vr_comfort : 0;
        float md = g_vr_move_dark;
        if (c && md > 0.0f) comfort_amt = lv[c][2] * (md > 1.0f ? 1.0f : md), comfort_r0 = lv[c][0], comfort_r1 = lv[c][1];
        else comfort_amt = 0.0f;
    }
    if (amt <= 0.0f && dark <= 0.0f && comfort_amt <= 0.0f) return;
    if (!vig_prog) {
        vig_prog = compile(vig_vs, vig_fs);
        uv_tan = glGetUniformLocation(vig_prog, "u_tan");
        uv_amt = glGetUniformLocation(vig_prog, "u_amt");
        uv_col = glGetUniformLocation(vig_prog, "u_col");
        uv_rng = glGetUniformLocation(vig_prog, "u_rng");
    }
    glViewport(0, 0, w, h);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    if (comfort_amt > 0.0f) vignette_pass(E, comfort_amt, 0.0f, 0.0f, 0.0f, comfort_r0, comfort_r1);
    if (amt > 0.0f) vignette_pass(E, amt, r, g, b, 0.45f, 1.25f);
    if (dark > 0.0f) vignette_pass(E, 0.9f * (dark > 1.0f ? 1.0f : dark), 0.0f, 0.0f, 0.0f, 0.45f, 1.25f);   /* an enemy's body at the eyes */
    glBindVertexArray(vao);
    glDisable(GL_BLEND);
}

/* the display area of a render-target texture (the 640x480 area, hor+ margins included) into
   the bound draw framebuffer of w x h, the right way up for OpenXR (row 0 at the bottom) */
static void draw_area_to(GLuint tex, int rt_vw, int rt_vh, int w, int h)
{
    float dw = 640.0f + 2.0f * g_margin_x, x0 = 0.0f;
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    clip_distances(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, w, h);
    glUseProgram(present_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glUniform1i(up_tex, 0);
    glUniform4f(up_rect, x0 / rt_vw, 0.0f, (x0 + dw) / rt_vw, 480.0f / rt_vh);
    glBindVertexArray(present_vao);
    glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
    glBindVertexArray(vao);
}

/* Stereo: the executed frame replayed once per eye with its 3D re-projected from the frame's
   camera to the eye (vr_view.h), each into the eye's swapchain image. 0 if this frame cannot
   (no VR camera, no snapshot to replay from, no eye poses): the caller shows the cinema screen. */
/* room changes in VR (gs_present_vr): the last stereo frame's eye images, kept to be shown at
   their own poses (fixed in the room) while it fades to black and the next room loads; the next
   room then fades in. vr_fade: brightness 1 .. 0. */
int g_vr_door_fade = 1;   /* ini vr_door_fade */
int g_vr_debug_cores = 0; /* ini vr_debug_cores: tint each VR draw path (green: native effects, blue: GS-register effects, yellow: the panel, magenta: screen layers) */
static GLuint hold_tex[2], hold_fbo, fade_prog;
static GLint uf_tex, uf_rect, uf_k, uf_mode;
static int hold_w, hold_h, hold_valid;
static float vr_fade = 1.0f;
static const char* fade_fs =
    "#version 330 core\n"
    "in vec2 v_uv; uniform sampler2D u_tex; uniform float u_k; uniform int u_mode; out vec4 o;\n"
    "void main(){ if (u_mode == 0) o = vec4(texture(u_tex, v_uv).rgb * u_k, 1.0); else o = vec4(0.0, 0.0, 0.0, 1.0 - u_k); }\n";
static void fade_prog_init(void)
{
    if (fade_prog) return;
    fade_prog = compile(present_vs, fade_fs);
    uf_tex = glGetUniformLocation(fade_prog, "u_tex"); uf_rect = glGetUniformLocation(fade_prog, "u_rect");
    uf_k = glGetUniformLocation(fade_prog, "u_k"); uf_mode = glGetUniformLocation(fade_prog, "u_mode");
    glGenFramebuffers(1, &hold_fbo);
}
/* (the eye image attached to cur, bound) keep a copy, then darken it to vr_fade */
static void hold_keep_and_dim(int e, int w, int h, GLuint cur)
{
    if (!g_vr_door_fade) return;
    fade_prog_init();
    if (!hold_tex[e] || hold_w != w || hold_h != h) {
        for (int k = 0; k < 2; k++) {
            if (!hold_tex[k]) glGenTextures(1, &hold_tex[k]);
            glBindTexture(GL_TEXTURE_2D, hold_tex[k]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        hold_w = w; hold_h = h; hold_valid = 0;
    }
#ifdef __ANDROID__
    /* Android (Quest): kept one headset image in 4 - each copy split the eye's pass on the tiled GPU (the image stored,
       copied, loaded again for what is drawn after: 26 blits and 5.3 Mpixels an image, most of them this). The kept
       poses go with the kept images (xr_hold_mark); a room change fades out from up to 3 images (~40 ms) earlier */
    static int hold_copy, hold_n;
    if (e == 0) hold_copy = !hold_valid || (hold_n++ & 3) == 0;
    if (hold_copy && e == 1) { extern void xr_hold_mark(void); xr_hold_mark(); }
    if (hold_copy)
#endif
    {   /* copy: the eye framebuffer (bound) -> hold_tex[e] */
        glBindFramebuffer(GL_READ_FRAMEBUFFER, cur);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hold_fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hold_tex[e], 0);
        glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, cur);
        if (e == 1) hold_valid = 1;
    }
    if (vr_fade < 0.999f) {   /* fading in after a room change */
        glViewport(0, 0, w, h);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
        glUseProgram(fade_prog);
        glUniform1i(uf_mode, 1); glUniform1f(uf_k, vr_fade);
        glUniform4f(uf_rect, 0.0f, 0.0f, 1.0f, 1.0f);
        glBindVertexArray(present_vao);
        glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
        glBindVertexArray(vao);
        glDisable(GL_BLEND);
    }
}
/* a held frame: the kept eye images at brightness vr_fade into the swapchain, shown at the kept poses */
static int present_vr_held(void)
{
    static GLuint fbo;
    if (!hold_valid || !xr_have_held()) return 0;
    if (!fbo) glGenFramebuffers(1, &fbo);
    fade_prog_init();
    for (int e = 0; e < 2; e++) {
        GLuint tex = xr_eye_target(e, hold_w, hold_h);
        if (!tex) { xr_eye_release(e); return 0; }
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glViewport(0, 0, hold_w, hold_h);
        glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glUseProgram(fade_prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hold_tex[e]);
        glUniform1i(uf_tex, 0); glUniform1i(uf_mode, 0); glUniform1f(uf_k, vr_fade);
        glUniform4f(uf_rect, 0.0f, 1.0f, 1.0f, 0.0f);   /* (present_vs puts v0 at the top: unflipped) */
        glBindVertexArray(present_vao);
        glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
        glBindVertexArray(vao);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        xr_eye_release(e);
    }
    glUseProgram(prog);
    xr_frame_end_held();
    return 1;
}

int g_vr_interp_body = 1;   /* ini vr_interp_body: the stick walking smoothed (vr71) */
int g_vr_interpolate = 1;   /* ini vr_interpolate: the characters move at the headset's rate (their strips blended, stream.c) */
/* the game's projection of a frame as a 4x4 (column-major): V (view, homogeneous) -> the
   homogeneous GS position of its native vertices (c * Vz + k * V.xy, -zb - za * Vz, Vz) */
static void vr_proj_matrix(float m[16], const VrView* v, float ofx, float ofy)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = v->kx; m[8] = v->cx - ofx;
    m[5] = v->ky; m[9] = v->cy - ofy;
    m[10] = -v->za; m[14] = -v->zb;
    m[11] = 1.0f;
}
static int present_vr_stereo_(const struct Frame* cur, float t);
/* vr141: the smooth turn by the headset's clock, shared with the game frame (vr_glue.c, which does not see SDL) */
static SDL_AtomicInt g_vr_turn_total;   /* the turn so far (microradians, wraps) - added here per headset image */
static SDL_AtomicInt g_vr_turn_taken;   /* ... of it, what the game frame has taken into its yaw */
static SDL_AtomicInt g_vr_turn_allow;   /* ... it may turn (smooth turning, controllers, not on the stairs) */
int  vr_turn_total(void) { return SDL_GetAtomicInt(&g_vr_turn_total); }
void vr_turn_take(int taken, int allow) { SDL_SetAtomicInt(&g_vr_turn_taken, taken); SDL_SetAtomicInt(&g_vr_turn_allow, allow); }
static float vr_turn_t = 1.0f;   /* vr125: the smooth turn's own t - by the clock, not held at 0..1: a game frame arriving a few ms
                                    late or early no longer stops the turn and then jumps it (the arrivals' even grid, render.c) */
/* vr74 diagnosis: a headset frame's work by step (ns): 0 replay (both eyes), 1 eye image wait, 2 eye copy, 3 mirror window, 4 hand to the headset */
static uint64_t vr_tm[5];
static uint64_t vr_tb[3];   /* vr161: before the picture, the picture's path (all of it), the flat path's frame replay */
static int vr_path;         /* ... 1 held (room change), 2 3D stereo, 3 flat (cinema screen / no VR camera) */
#define VR_TM(k, stmt) do { uint64_t t_ = SDL_GetTicksNS(); stmt; vr_tm[k] += SDL_GetTicksNS() - t_; } while (0)
/* vr71 diagnosis (vr_debug_log): how long the 3D picture takes, and the gaps between headset frames (stutter) */
static int present_vr_stereo(const struct Frame* cur, float t)
{
    extern int g_vr_debug_log;
    static uint64_t last_ns, worst_gap, worst_draw, sum_draw;
    static unsigned n, slow, gaps;
    uint64_t t0 = SDL_GetTicksNS(), t1;
    double hz = xr_display_hz() > 1.0 ? xr_display_hz() : 90.0, budget = 1e9 / hz;
    int r = present_vr_stereo_(cur, t);
    t1 = SDL_GetTicksNS();
    if (last_ns) { uint64_t g = t0 - last_ns; if (g > worst_gap) worst_gap = g; if (g > 1.5 * budget) gaps++; }
    last_ns = t0;
    if (t1 - t0 > worst_draw) worst_draw = t1 - t0;
    if (t1 - t0 > budget * 0.8) slow++;
    sum_draw += t1 - t0;
    if (++n >= 450) {
        if (g_vr_debug_log) host_log("vr-diag: timing: 3D picture avg %.1f ms, worst %.1f ms (budget %.1f, slow %u); headset frames missed %u, worst gap %.1f ms\n",
                                     sum_draw / 1e6 / n, worst_draw / 1e6, budget / 1e6, slow, gaps, worst_gap / 1e6);
        n = slow = gaps = 0; worst_gap = worst_draw = sum_draw = 0;
    }
    return r;
}
static int present_vr_stereo_(const struct Frame* cur, float t)
{
    static GLuint vr_fbo;
    const VrView* v = cur ? stream_frame_vr(cur) : NULL;
    float iG[16], iW[16], A[16];
    float dw = 640.0f + 2.0f * g_margin_x;
    int w = (int)(dw * g_scale + 0.5f), h = (int)(480.0f * g_scale + 0.5f), ok = 1;
    if (g_vr_eye_shape && xr_recommended_eye_width() > 0 && xr_recommended_eye_height() > 0) {
        /* the eye images at the headset's recommended size and shape (Quest 2: 1832 x 1920 at
           100%); the frustum fills the drawn area, which is resampled onto them */
        w = (int)(xr_recommended_eye_width() * (float)g_vr_res / 100.0f + 0.5f);
        h = (int)(xr_recommended_eye_height() * (float)g_vr_res / 100.0f + 0.5f);
    }
    const GsContext* c = &gs.ctx[0];
    float ofx = (float)(c->xyoffset & 0xFFFF) / 16.0f, ofy = (float)((c->xyoffset >> 32) & 0xFFFF) / 16.0f;
    {   /* why a frame stays on the cinema screen: logged when the reason changes */
        static int last = -1, nlog;
        int why = !v ? 1 : !v->valid ? 2 : !snapshot_valid ? 3 : !xr_frame_eye(0) || !xr_frame_eye(1) ? 4 :
                  !vr_mat_invert(iG, v->G) ? 5 : !vr_mat_invert(iW, v->W) ? 6 : 0;
        static const char* names[] = { "ok", "no frame", "frame without VR camera (cutscene/menu/door)", "no snapshot to replay",
                                       "no eye poses", "camera matrix not invertible", "play-space matrix not invertible" };
        if (why != last && nlog < 200) {
            nlog++;
            host_log("vr: stereo %s (frame %llu, scale %d, area %dx%d)\n", names[why], (unsigned long long)frame_no, (int)g_scale, w, h);
            if (why == 0 && v) host_log("vr: proj c %.1f %.1f k %.1f %.1f xyoffset %.1f %.1f\n", v->cx, v->cy, v->kx, v->ky, ofx, ofy);
            last = why;
        }
        g_vr_status = why ? names[why] : NULL;
        {   /* vr188 diagnosis (vr_watch) */
            extern int g_vr_watch_menu; extern uint64_t g_vr_watch_back;
            if (why == 2) g_vr_watch_menu = 1;
            else if (why == 0 && g_vr_watch_menu) { g_vr_watch_menu = 0; g_vr_watch_back = SDL_GetTicks(); host_log("vr-watch: back to 3D from the cinema screen\n"); }
        }
        if (why) return 0;
    }
    {   /* vr67: walking moves the play space (W) once a game frame: in between, the play space at t
           (still scenery and characters move with the body at the headset's rate) */
        const struct Frame* pf = g_vr_interpolate && t < 0.999f ? stream_prev_frame(cur) : NULL;
        const VrView* pv = pf ? stream_frame_vr(pf) : NULL;
        float Wt[16], iWt[16], mv = 0.0f, zs = sqrtf(v->W[0] * v->W[0] + v->W[1] * v->W[1] + v->W[2] * v->W[2]);
        float dl[3];
        int use = 0, steady = 1;
        if (g_vr_interp_body && pv && pv->valid) {   /* vr71: only the body's own move (the stick); the head's part and the room walking as the game made them */
            dl[0] = v->body[0] - pv->body[0] - v->follow[0];
            dl[1] = v->body[1] - pv->body[1];
            dl[2] = v->body[2] - pv->body[2] - v->follow[1];
            mv = sqrtf(dl[0] * dl[0] + dl[1] * dl[1] + dl[2] * dl[2]) / (zs > 1e-3f ? zs : 10.0f);   /* metres */
            {   /* vr72: only a steady move - one turned back or cut short (a wall pushing the body back) is shown as made */
                float a2 = 0.0f, b2 = 0.0f, ab = 0.0f;
                for (int k = 0; k < 3; k++) { a2 += v->step[k] * v->step[k]; b2 += v->step_prev[k] * v->step_prev[k]; ab += v->step[k] * v->step_prev[k]; }
                steady = b2 < 1e-6f ? a2 < 0.25f * zs * zs * 0.01f : (ab > 0.7f * sqrtf(a2 * b2) && a2 > 0.25f * b2);
                if (a2 < 1e-8f) steady = 1;   /* not moving */
            }
            if (mv < 0.5f && steady) {   /* not a jump (a room change), not a wall */
                memcpy(Wt, v->W, sizeof(Wt));
                for (int k = 0; k < 3; k++) Wt[12 + k] -= (1.0f - t) * dl[k];
                if (vr_mat_invert(iWt, Wt)) use = 1;
            }
        }
        {   /* VR diagnosis (vr_debug_log) */
            extern int g_vr_debug_log;
            static unsigned nl, nused, njump, nwall;
            if (use) nused++; else if (pv && pv->valid && g_vr_interp_body) { if (!steady) nwall++; else njump++; }
            if (g_vr_debug_log && (nl++ % 300) == 0)
                host_log("vr-diag: walking smoothed: %s (stick move %.3f m this game frame, room-walk %.3f %.3f; since last: used %u, refused %u, wall/turned back %u)\n",
                         !g_vr_interp_body ? "off (vr_interp_body 0)" : use ? "on" : "not this frame", mv, v->follow[0], v->follow[1], nused, njump, nwall);
        }
        float e = (float)(int)((unsigned)SDL_GetAtomicInt(&g_vr_turn_total) - (unsigned)v->turn_at) * 1e-6f;   /* vr141: turned since the frame */
        if (fabsf(e) > 1e-5f && fabsf(e) < 0.5f) {   /* vr120: a smooth turn shown at the headset's rate (vr125: by its own t,
               also past the frame while the next is a little late, and on frames not interpolated otherwise):
               the play space as the frame made it, turned back by what is left of this frame's turn, about the head
               (W(yaw - e) = W(yaw) * T(head) * Ry(-e) * T(-head) - the head stays where it is) */
            VrPose hp;
            float base[16], R[16], T1[16], T2[16], M1[16], M2[16], c = cosf(e), sn = sinf(e);
            memcpy(base, use ? Wt : v->W, sizeof(base));
            if (xr_head_pose(&hp)) {   /* vr124: the head pose is in the recentred space and W = Wg * recentre: the turn goes
                                          in between (W * inverse(recentre) * T(head) * Ry * T(-head) * recentre); about the
                                          head in the headset's own space it slid sideways and came back each game frame */
                float Rc[16], iRc[16], M3[16], M4[16];
                vr_mat_identity(R); R[0] = c; R[8] = sn; R[2] = -sn; R[10] = c;   /* as make_Wg (vr_glue.c) */
                vr_mat_identity(T1); T1[12] = hp.px; T1[13] = hp.py; T1[14] = hp.pz;
                vr_mat_identity(T2); T2[12] = -hp.px; T2[13] = -hp.py; T2[14] = -hp.pz;
                xr_recenter_matrix(Rc);
                if (vr_mat_invert(iRc, Rc)) {
                    vr_mat_mul(M1, R, T2); vr_mat_mul(M2, T1, M1); vr_mat_mul(M3, M2, Rc); vr_mat_mul(M4, iRc, M3);
                    vr_mat_mul(Wt, base, M4);
                    if (vr_mat_invert(iWt, Wt)) use = 1;
                }
            }
        }
        vr_mat_mul(A, use ? iWt : iW, iG);   /* V of the frame -> local (metres) */
        {   /* vr68: the hand objects follow the controllers now, not as they were when the game frame was made:
               GS -> V -> local (as made) -> D = aim now * inverse(aim then) -> back through this image's A -> GS */
            extern int g_hand_on;
            extern float g_hand_M[2][16];
            VrInput in;
            float P0[16], iP0[16], A0[16], iA[16], B[16], C[16], D[16], Mn[16], iMt[16];
            int have = xr_input(&in);
            g_hand_on = 0;
            vr_laser_on[0] = vr_laser_on[1] = 0;
            vr_proj_matrix(P0, v, 0.0f, 0.0f);
            float Rc[16], iRc[16], dbg_turn = 0.0f;
            xr_recenter_matrix(Rc);   /* local -> recentred */
            {   /* vr87: world -> V of this image (the ammo box at the holster) */
                float iA2[16], iW2[16];
                vr_world_ok = vr_mat_invert(iA2, A) && vr_mat_invert(iW2, v->W);
                if (vr_world_ok) vr_mat_mul(vr_world_L, iA2, iW2);
            }
            if (have && vr_mat_invert(iP0, P0) && vr_mat_invert(iA, A) && vr_mat_invert(iRc, Rc)) {
                vr_mat_mul(A0, iW, iG);
                for (int h = 0; h < 2; h++) {
                    const VrPose* now = h ? &in.aiml : &in.aim;
                    int nv = h ? in.aiml_valid : in.aim_valid;
                    if (v->hand_ok[h] && nv) {
                        float t1[16], t2[16], iWf[16];
                        vr_mat_from_pose(Mn, now);
                        vr_mat_rigid_inverse_pose(iMt, &v->hand[h]);
                        vr_mat_mul(D, Mn, iMt);   /* in the recentred space the controller poses are in */
                        vr_mat_mul(C, D, Rc); vr_mat_mul(D, iRc, C);   /* vr70: to local (eye poses' space): inverse(rec) * D * rec */
                        if (h == 0) { float tr = D[0] + D[5] + D[10], cc = (tr - 1.0f) * 0.5f; dbg_turn = acosf(cc > 1.0f ? 1.0f : cc < -1.0f ? -1.0f : cc) * 57.2958f; }
                        vr_mat_mul(B, A0, iP0);   /* GS -> local as made */
                        vr_mat_mul(C, D, B);
                        vr_mat_mul(B, iA, C);     /* -> V of this image */
                        vr_mat_mul(g_hand_M[h], P0, B);
                        if (vr_mat_invert(iWf, v->W)) { vr_mat_mul(t1, D, iWf); vr_mat_mul(t2, iA, t1); memcpy(vr_laser_L[h], t2, sizeof(t2)); vr_laser_on[h] = 1; }   /* vr77: world -> V, moved with the hand */
                    } else {
                        vr_mat_mul(C, A0, iP0); vr_mat_mul(B, iA, C); vr_mat_mul(g_hand_M[h], P0, B);   /* as made (A to this image's view) */
                    }
                }
                g_hand_on = 1;
            }
            {   /* VR diagnosis (vr_debug_log): the recentring the hands are corrected through */
                extern int g_vr_debug_log;
                static unsigned nl;
                if (g_vr_debug_log && (nl++ % 300) == 0)
                    host_log("vr-diag: hands: %s, recentring turned %.1f deg, right hand turned since the game frame %.1f deg\n",
                             g_hand_on ? "follow the controllers" : "as made", atan2f(Rc[8], Rc[0]) * 57.2958f, dbg_turn);
            }
        }
    }
    float chT[16];
    int chars = 0;
    if (g_vr_interpolate && t < 0.999f) {   /* in between two game frames: the characters at t */
        const struct Frame* pf = stream_prev_frame(cur);
        const VrView* pv = pf ? stream_frame_vr(pf) : NULL;
        if (pv && pv->valid) {
            float Pc[16], Pp[16], iPp[16], iGp[16], A1[16], A2[16];
            vr_proj_matrix(Pc, v, 0.0f, 0.0f);   /* the strips carry no XYOFFSET (measured on the still scenery, vr66) */
            vr_proj_matrix(Pp, pv, 0.0f, 0.0f);
            if (vr_mat_invert(iPp, Pp) && vr_mat_invert(iGp, pv->G)) {   /* previous GS -> previous V -> world -> this V -> this GS */
                vr_mat_mul(A1, iGp, iPp);
                vr_mat_mul(A2, v->G, A1);
                vr_mat_mul(chT, Pc, A2);
                chars = 1;
            }
        }
    }
    if (!vr_fbo) glGenFramebuffers(1, &vr_fbo);
    gs_flush();
    for (int e = 0; e < 2 && ok; e++) {
        const VrEye* E = xr_frame_eye(e);
        float iE[16], M[16], l = E->l, r = E->r, u = E->u, d = E->d;
        RenderTarget* rt;
        GLuint tex;
        vr_mat_rigid_inverse_pose(iE, &E->pose);
        vr_mat_mul(M, iE, A);                                /* V -> eye (OpenGL axes) */
        for (int k = 0; k < 4; k++) { M[k * 4 + 1] = -M[k * 4 + 1]; M[k * 4 + 2] = -M[k * 4 + 2]; }   /* S: y down, z forward */
        vr_u.src[0] = v->cx - ofx; vr_u.src[1] = v->cy - ofy; vr_u.src[2] = v->kx; vr_u.src[3] = v->ky;
        /* the eye's frustum onto the display area: x from -margin (tan l) to 640 + margin (tan r),
           y from 0 (tan u, up) to 480 (tan d); V.y is down, so y = 480 (u + V.y / V.z) / (u - d) */
        vr_u.dst[0] = -(float)g_margin_x + dw * (-l) / (r - l);
        vr_u.dst[1] = 480.0f * u / (u - d);
        vr_u.dst[2] = dw / (r - l);
        vr_u.dst[3] = 480.0f / (u - d);
        memcpy(vr_u.m, M, sizeof(M));
        vr_u.zab[0] = v->za; vr_u.zab[1] = v->zb;
        vr_u.zs = sqrtf(v->W[0] * v->W[0] + v->W[1] * v->W[1] + v->W[2] * v->W[2]);   /* game units per metre */
        if (vr_u.zs < 1e-3f) vr_u.zs = 10.0f;
        if (g_vr_hud_angle > 0) {   /* the 2D panel: distance (units) and size (units per pixel) */
            float d = g_vr_hud_dist / 100.0f * vr_u.zs;
            vr_u.hud[1] = d;
            vr_u.hud[0] = d * tanf(g_vr_hud_angle * (3.14159265f / 360.0f)) / 320.0f;
            vr_u.hud[2] = g_vr_hud_y / 100.0f * vr_u.zs;   /* V.y is down: up is subtracted */
        } else vr_u.hud[0] = vr_u.hud[1] = vr_u.hud[2] = 0.0f;
        vr_u.on = 1;
        {
            static unsigned nlog;
            int logit = e == 0 && nlog < 40 && (frame_no % 90) == 0;
            if (logit) memset(&vr_st, 0, sizeof(vr_st)), vr_st.on = 1;
#ifdef __ANDROID__
            /* Android (Quest): the left eye's replay recorded, the right eye issues it again with its own matrix and
               frustum (gl_record.c) - the game frame's stream runs once a headset image, not twice. Anything the
               recording cannot reproduce: the right eye replays as before. */
            extern int g_vr_one_replay;
            extern void glrec_begin(void); extern int glrec_end(void); extern void glrec_replay(const GLint*, const float (*)[4], int);
            extern const char* glrec_why(void); extern size_t glrec_count(void); extern int glrec_buffers_ok;
            static RenderTarget rec_rts[MAX_RT];
            static Shadow rec_shadow[MAX_RT];
            static unsigned n_reissue, n_full, n_logged;
            if (e == 1 && and_rec_ok && g_vr_one_replay) {
                const GLint locs[5] = { u_vr_dst, u_vr_m0, u_vr_m1, u_vr_m2, u_vr_m3 };
                const float vals[5][4] = { { vr_u.dst[0], vr_u.dst[1], vr_u.dst[2], vr_u.dst[3] }, { vr_u.m[0], vr_u.m[1], vr_u.m[2], vr_u.m[3] },
                                           { vr_u.m[4], vr_u.m[5], vr_u.m[6], vr_u.m[7] }, { vr_u.m[8], vr_u.m[9], vr_u.m[10], vr_u.m[11] },
                                           { vr_u.m[12], vr_u.m[13], vr_u.m[14], vr_u.m[15] } };
                memcpy(rts, rec_rts, sizeof(rts)); memcpy(shadow, rec_shadow, sizeof(shadow));   /* the targets as the left eye's replay left them */
                VR_TM(0, glrec_replay(locs, vals, 5));
                memset(uknown, 0, sizeof(uknown)); clip_on = -1; fb_forget();   /* (GL state now the recording's end, not the caches') */
                n_reissue++;
            } else {
                int rec = e == 0 && g_vr_one_replay;
                and_rec_ok = 0;
                if (rec) {   /* the caches forgotten: the recording sets every state it relies on */
                    memset(uknown, 0, sizeof(uknown)); clip_on = -1; fb_forget();
                    glrec_buffers_ok = and_pmap != NULL;
                    glrec_begin();
                }
                if (chars) VR_TM(0, stream_replay_chars(cur, t, chT));
                else VR_TM(0, stream_replay_exact(cur));
                if (rec) {
                    and_rec_ok = glrec_end();
                    if (and_rec_ok) { memcpy(rec_rts, rts, sizeof(rts)); memcpy(rec_shadow, shadow, sizeof(shadow)); rt_restore_deleted = 0; }
                    else if (n_logged < 20) { n_logged++; host_log("android: one replay for both eyes: not this image (%s)\n", glrec_why()); }
                }
                if (e == 1) n_full++;
            }
            if (e == 1 && (n_reissue + n_full) % 2000 == 1)
                host_log("android: one replay for both eyes: %u images re-issued, %u replayed twice (last recording %u GL calls)\n", n_reissue, n_full, (unsigned)glrec_count());
#else
            if (chars) VR_TM(0, stream_replay_chars(cur, t, chT));
            else VR_TM(0, stream_replay_exact(cur));
#endif
            if (logit) {
                vr_st.on = 0; nlog++;
                host_log("vr: frame %llu vertices: 3D %u, GS-depth (sprites/VU1) %u, flat 2D %u (screen layers kept flat %u)\n",
                         (unsigned long long)frame_no, vr_st.n3d, vr_st.nraw, vr_st.n2d, vr_st.nbig);
                for (int k = 0; k < vr_st.ns; k++)
                    host_log("vr:   flat 2D sample path %c x %.1f y %.1f z %.0f\n", vr_st.path[k], vr_st.s[k][0], vr_st.s[k][1], vr_st.s[k][2]);
            }
        }
        vr_u.on = 0;
        rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
        VR_TM(1, tex = xr_eye_target(e, w, h));
        if (!rt || !tex) {
            static int nfail;
            if (nfail++ < 20) host_log("vr: eye %d failed: %s\n", e, !rt ? "no display target after the replay" : "no swapchain image");
            g_vr_status = !rt ? "eye: no display target after the replay" : "eye: no swapchain image";
        }
        if (rt && tex) {
            glBindFramebuffer(GL_FRAMEBUFFER, vr_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
#ifdef __ANDROID__
            { extern void and_gq_stage(int); and_gq_stage(2); }
#endif
            VR_TM(2, draw_area_to(rt->tex, rt->vw, rt->vh, w, h); draw_lasers(v, M, E, w, h, rt); draw_box(v, M, E, w, h, rt); draw_markers(v, M, E, w, h); draw_vignette(E, w, h); hold_keep_and_dim(e, w, h, vr_fbo));
#ifdef __ANDROID__
            glBindFramebuffer(GL_FRAMEBUFFER, vr_fbo);
            glViewport(0, 0, w, h);
            and_fps_draw(w * 40 / 100 + (e ? -w / 80 : w / 80), h * 30 / 100, h);   /* below the middle; per eye a little inwards (seen ~1.5 m away) */
#endif
#ifdef __ANDROID__
            /* Android (Quest): no mirror - the window is never swapped in VR (gs_present_vr's end), and the clear and
               the scaled copy were GPU work in every headset image (the runtime holds the images at half rate when the
               GPU is late: 7-10 ms an image measured) */
            if (0)
#endif
            if (e == 0) {   /* vr127: the window mirrors the left eye (a copy on the GPU), filling it, the middle kept */
                uint64_t tm0 = SDL_GetTicksNS();
                int ww = 0, wh = 0;
                SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
                if (ww > 0 && wh > 0) {
                    int sw = w, sh = (int)((float)w * wh / ww), sx = 0, sy;
                    if (sh > h) { sh = h; sw = (int)((float)h * ww / wh); sx = (w - sw) / 2; }
                    sy = (h - sh) / 2;
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                    glDisable(GL_SCISSOR_TEST);
                    glViewport(0, 0, ww, wh);
                    glClearColor(0, 0, 0, 1);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, vr_fbo);
                    glBlitFramebuffer(sx, sy, sx + sw, sy + sh, 0, 0, ww, wh, GL_COLOR_BUFFER_BIT, GL_LINEAR);
                    glBindFramebuffer(GL_FRAMEBUFFER, vr_fbo);
                }
                vr_tm[3] += SDL_GetTicksNS() - tm0;
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            fb_forget();
        } else ok = 0;
#ifdef __ANDROID__
        if (!(g_and_ab & 2))   /* A/B test (render.c): the per-eye flush or not (each one a driver submission) */
#endif
#ifdef __ANDROID__
        { extern void and_gq_stage(int); and_gq_stage(-1); }
#endif
        glFlush();   /* vr99: the eye's work to the GPU now, not all at the hand-off (xrEndFrame waits less) */
        xr_eye_release(e);
        rt_restore();
#ifdef __ANDROID__
        if (rt_restore_deleted && and_rec_ok) {
            static unsigned nl;
            if (nl++ < 20) host_log("android: one replay for both eyes: not this image (a render target of the replay deleted)\n");
            and_rec_ok = 0;
        }
        rt_restore_deleted = 0;
#endif
    }
    return ok;
}

/* VR present: one headset frame. Stereo when the frame has a VR camera, else the cinema
   screen (the window image as usual). The window always mirrors. */
static uint64_t gs_present_vr(const struct Frame* cur, float t)
{
    uint64_t swap;
    uint64_t tw0 = SDL_GetTicksNS();
    int ww = 0, wh = 0, fr = xr_frame_begin(), door;
    uint64_t tw = SDL_GetTicksNS() - tw0;   /* vr73 diagnosis: how long the headset made us wait */
    vr_turn_t = 1.0f;
    if (fr == 1) {   /* vr123: an in-between image at the time the headset shows it (after its wait), not by
                        counting images - a headset image shown late no longer holds the turn back */
        extern double g_vr_frame_ref, g_vr_frame_period;
        double tc = g_vr_frame_period > 1e6 ? ((double)SDL_GetTicksNS() - g_vr_frame_ref) / g_vr_frame_period : 1.0;
        if (t < 1.0f) t = tc < 0.0 ? 0.0f : tc > 1.0 ? 1.0f : (float)tc;
        vr_turn_t = tc < -0.5 ? -0.5f : tc > 1.5 ? 1.5f : (float)tc;   /* vr125: at most half a game frame ahead or behind */
    }
    {   /* vr141: the smooth turn by the headset's clock - the stick read each headset image, turned by the time since the last
           (~120 deg/s, as 4 deg a game frame was); the game frame takes it into its yaw, the view shows what it has not yet.
           vr142: the time is the headset's display time (OpenXR), not ours - even steps, and the image after a missed one
           covers the time it missed; the stick eased in and out over ~0.1 s (the top speed as before) */
        static uint64_t last_ns;
        static int64_t last_disp;
        static float rx_s;
        uint64_t now = SDL_GetTicksNS();
        int64_t disp = fr == 1 ? xr_display_time_ns() : 0;
        float tdt;
        VrInput in;
        if (disp > 0 && last_disp > 0 && disp > last_disp) tdt = (float)(disp - last_disp) * 1e-9f;
        else tdt = last_ns ? (float)(now - last_ns) * 1e-9f : 0.0f;
        last_ns = now; if (disp > 0) last_disp = disp;
        if (tdt > 0.05f) tdt = 0.05f;
        {   /* vr156 diagnosis (vr_debug_log): the right stick out, no turn - why */
            extern int g_vr_debug_log;
            static unsigned held, nl;
            VrInput di;
            if (fr == 1 && xr_input(&di) && fabsf(di.rx) > 0.6f && rx_s == 0.0f) held++; else held = 0;
            if (g_vr_debug_log && held == 90 && nl++ < 20)
                host_log("vr-diag: smooth turn stuck: stick %.2f, allowed %d, ahead %.2f rad\n", di.rx, SDL_GetAtomicInt(&g_vr_turn_allow),
                         (float)(int)((unsigned)SDL_GetAtomicInt(&g_vr_turn_total) - (unsigned)SDL_GetAtomicInt(&g_vr_turn_taken)) * 1e-6f);
        }
        if (fr == 1 && SDL_GetAtomicInt(&g_vr_turn_allow) && xr_input(&in)) {
            float want = fabsf(in.rx) > 0.15f ? in.rx : 0.0f, k = tdt / 0.05f;   /* ~95% of the way in 0.15 s, most of it in 0.1 */
            rx_s += (want - rx_s) * (k > 1.0f ? 1.0f : k);
            if (want == 0.0f && fabsf(rx_s) < 0.02f) rx_s = 0.0f;
        } else rx_s = 0.0f;
        if (rx_s != 0.0f) {
            int tot = SDL_GetAtomicInt(&g_vr_turn_total);
            float ahead = (float)(int)((unsigned)tot - (unsigned)SDL_GetAtomicInt(&g_vr_turn_taken)) * 1e-6f;
            if (fabsf(ahead) < 0.35f) {   /* the game stopped taking it (loading, a pause): no more */
                float d = -rx_s * (3.14159265f / 180.0f) * 120.0f * tdt;
                SDL_SetAtomicInt(&g_vr_turn_total, (int)((unsigned)tot + (unsigned)(int)(d * 1e6f)));
            }
        }
    }
    static uint64_t last_ms;
    uint64_t now_ms = SDL_GetTicks();
    float dt = last_ms ? (float)(now_ms - last_ms) / 1000.0f : 0.0f;
    extern int vr_door_transition(void);   /* (vr_glue.c) */
    last_ms = now_ms;
    if (dt > 0.1f) dt = 0.1f;
    door = g_vr_door_fade && vr_door_transition();
    {   /* VR diagnosis (vr_debug_log): which picture the headset gets, logged when it changes */
        extern int g_vr_debug_log;
        static int last = -1;
        int k = fr != 1 ? 0 : (door && hold_valid && xr_have_held()) ? 1 : (cur && stream_frame_vr(cur) && stream_frame_vr(cur)->valid) ? 2 : 3;
        static const char* names[] = { "no headset frame", "room change (held picture)", "3D stereo", "cinema screen" };
        if (g_vr_debug_log && k != last) host_log("vr-diag: headset shows %s (frame %llu%s%s)\n", names[k], (unsigned long long)frame_no,
                                                  g_vr_status ? ", " : "", g_vr_status ? g_vr_status : "");
        last = k;
    }
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    if (fr != 1) g_vr_status = fr == 0 ? "headset asked not to render" : "headset not running";
    uint64_t tb0 = SDL_GetTicksNS();   /* vr161 */
    vr_tb[0] = tb0 - tw0 - tw;
    vr_path = 3;
    if (fr == 1 && door && hold_valid && xr_have_held()) {   /* room change: the last 3D picture fades out, fixed in the room */
        vr_path = 1;
        vr_fade -= dt / 0.5f;
        if (vr_fade < 0.0f) vr_fade = 0.0f;
        draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);   /* mirror */
        overlay_render_c();
        if (!present_vr_held()) xr_frame_end_none();
    } else if (fr == 1 && (vr_fade = vr_fade + dt / 0.5f > 1.0f ? 1.0f : vr_fade + dt / 0.5f, present_vr_stereo(cur, t))) {   /* (fading back in after one) */
        vr_path = 2;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);   /* vr127: the mirror is the left eye (copied while drawing it); the overlay over it */
        { int ww = 0, wh = 0; SDL_GetWindowSizeInPixels(g_window, &ww, &wh); glViewport(0, 0, ww, wh); }
        clip_distances(0);
        VR_TM(3, overlay_render_c());
        VR_TM(4, xr_frame_end_stereo());
    } else {
        int inbetween = cur && snapshot_valid && t < 1.0f;
        if (!door) vr_fade = 1.0f;   /* (the cinema screen: menus, films - no fade) */
        if (inbetween) {
            RenderTarget* rt;
            { uint64_t tr = SDL_GetTicksNS(); stream_replay(cur, t); vr_tb[2] = SDL_GetTicksNS() - tr; }   /* vr161 */
            rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
            draw_display(rt ? rt->tex : 0, rt ? rt->vw : 640, rt ? rt->vh : 512);
            overlay_render_c();
            rt_restore();
        } else {
            draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);
            overlay_render_c();
        }
        if (fr == 1) {   /* the game's picture only (no black bars), at its own aspect */
            int vx, vy, vw, vh;
            float dw;
            display_rect(ww, wh, &vx, &vy, &vw, &vh, &dw);
#ifdef __ANDROID__
            if (vw > 0 && vh > 0) {   /* the frame rates in the game picture's top left corner (the window is the quad) */
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, ww, wh);
                and_fps_draw(vx + vw / 30, wh - vy - vh / 10, vh);
            }
#endif
            if (vw > 0 && vh > 0) xr_frame_end_quad_rect(vx, wh - vy - vh, vw, vh);
            else xr_frame_end_quad(ww, wh);
        }
        else if (fr == 0) xr_frame_end_none();
    }
    vr_tb[1] = SDL_GetTicksNS() - tb0;   /* vr161 */
    swap = SDL_GetTicksNS();
#ifdef __ANDROID__
    /* Android (Quest): the window is not seen in the headset - not swapped (the cinema screen is copied from its back
       buffer before this). With xrWaitFrame on its own thread the swap was the render thread's wait: 1.5-4.7 ms on
       average, 13-32 ms peaks */
    (void)g_and_ab;
    if (0)
#endif
    SDL_GL_SwapWindow(g_window);
    swap = SDL_GetTicksNS() - swap;
    {   /* vr73 diagnosis (vr_debug_log): where a headset frame's time goes - the headset's wait, the
           window mirror's buffer swap (a monitor's vsync can hold it), the rest of the work */
        extern int g_vr_debug_log;
        static uint64_t sw, ss, swmax, ssmax, sall, sallmax, last;
        static unsigned n, nswap_long;
        uint64_t all = SDL_GetTicksNS() - tw0;
        sw += tw; ss += swap; sall += all;
        if (tw > swmax) swmax = tw;
        if (swap > ssmax) ssmax = swap;
        if (all - tw - swap > sallmax) sallmax = all - tw - swap;
        if (swap > 4000000) nswap_long++;
        if (fr == 1) {   /* vr162: dynamic resolution - our work averaged over ~0.5 s */
            static double avg;
            static uint64_t last_change, light_since;
            uint64_t now = SDL_GetTicksNS(), wk = all - tw - swap;
            avg = avg ? avg * 0.978 + wk * 0.022 : (double)wk;
            if (!g_vr_dyn_res) { if (vr_dyn_down) { vr_dyn_down = 0; host_log("vr: dynamic resolution off (vr_dyn_res 0)\n"); } }
            else if (now - last_change > 2000000000ull) {
                if (!vr_dyn_down && avg > 10.0e6) {
                    vr_dyn_down = 1; last_change = now; light_since = 0;
                    host_log("vr: dynamic resolution down (our work %.1f ms average)\n", avg / 1e6);
                } else if (vr_dyn_down) {
                    if (avg < 6.5e6) { if (!light_since) light_since = now; }
                    else light_since = 0;
                    if (light_since && now - light_since > 4000000000ull) {
                        vr_dyn_down = 0; last_change = now; light_since = 0;
                        host_log("vr: dynamic resolution back up (our work %.1f ms average)\n", avg / 1e6);
                    }
                }
            }
        }
        {   /* vr74: a slow headset frame (over 11 ms of our own work), step by step */
            static unsigned nslow;
            uint64_t work = all - tw - swap;
            if (g_vr_debug_log && fr == 1 && work > 11000000 && nslow < 400) {   /* vr161: 400 (was 80: used up at the start) */
                nslow++;
                host_log("vr-diag: slow frame %llu: work %.1f ms = replay %.1f, eye image wait %.1f, eye copy %.1f, mirror %.1f, to headset %.1f, other %.1f (t %.2f)\n",
                         (unsigned long long)frame_no, work / 1e6, vr_tm[0] / 1e6, vr_tm[1] / 1e6, vr_tm[2] / 1e6, vr_tm[3] / 1e6, vr_tm[4] / 1e6,
                         (double)(work - vr_tm[0] - vr_tm[1] - vr_tm[2] - vr_tm[3] - vr_tm[4]) / 1e6, t);
                {   /* vr161: the "other" divided */
                    static const char* const pn[4] = { "?", "held picture (room change)", "3D stereo", "flat (cinema screen / no VR camera)" };
                    host_log("vr-diag: slow frame %llu: before the picture %.1f ms; path %s %.1f ms%s; after (mirror swap excluded) %.1f ms\n",
                             (unsigned long long)frame_no, vr_tb[0] / 1e6, pn[vr_path & 3], vr_tb[1] / 1e6,
                             vr_path == 3 && vr_tb[2] ? " (of it the frame replay)" : "", (double)(work - vr_tb[0] - vr_tb[1]) / 1e6);
                    if (vr_path == 3 && vr_tb[2]) host_log("vr-diag: slow frame %llu: flat path frame replay %.1f ms\n", (unsigned long long)frame_no, vr_tb[2] / 1e6);
                }
            }
            for (int k = 0; k < 5; k++) vr_tm[k] = 0;
            vr_tb[0] = vr_tb[1] = vr_tb[2] = 0;
        }
        (void)last;
        if (++n >= 450) {
            if (g_vr_debug_log) host_log("vr-diag: frame time: headset wait avg %.1f max %.1f ms; window swap avg %.1f max %.1f ms (over 4 ms: %u); our work avg %.1f max %.1f ms\n",
                                         sw / 1e6 / n, swmax / 1e6, ss / 1e6 / n, ssmax / 1e6, nswap_long, (sall - sw - ss) / 1e6 / n, sallmax / 1e6);
            n = nswap_long = 0; sw = ss = swmax = ssmax = sall = sallmax = 0;
        }
    }
    return swap;
}

/* show the frame at position t between the previous and the executed frame (t >= 1 or no
   replay: the frame itself); returns how long the buffer swap blocked (ns) */
uint64_t gs_present(const struct Frame* cur, float t)
{
    uint64_t swap;
    int inbetween = cur && snapshot_valid && t < 1.0f;
    if (xr_active()) {
        prof_push(PROF_PRESENT);
        swap = gs_present_vr(cur, t);
        prof_pop();
        return swap;
    }
    prof_push(PROF_PRESENT);
    gt_present_begin();
    if (inbetween) {
        RenderTarget* rt;
        stream_replay(cur, t);
        gt_mark(GT_RD);
        rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
        draw_display(rt ? rt->tex : 0, rt ? rt->vw : 640, rt ? rt->vh : 512);
        overlay_render_c();
        if (!interp_dumped) { dump_frame(1); interp_dumped = 1; }
        gt_mark(GT_DD);
        rt_restore();
    } else {
        draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);
        overlay_render_c();
        if (!final_dumped) { dump_frame(0); final_dumped = 1; }
        dump_window();
    }
    gt_present_end(inbetween);
    swap = SDL_GetTicksNS();
    SDL_GL_SwapWindow(g_window);
    swap = SDL_GetTicksNS() - swap;
    prof_pop();
    return swap;
}
