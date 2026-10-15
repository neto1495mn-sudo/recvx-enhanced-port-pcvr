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
        const uint64_t* w = (const uint64_t*)(gs_mem + (size_t)p * 8192);
        uint64_t h = 0x9E3779B97F4A7C15ull ^ p;
        for (int i = 0; i < 1024; i++) { h ^= w[i]; h *= 0x100000001B3ull; h ^= h >> 29; }
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
             u_tex, u_tex_alpha_scale, u_texofs, u_premul, u_zscale, u_clamp_mode, u_clamp_vals, u_afail_pass, u_rt16;
static GLint up_tex, up_rect;

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
    "out vec3 v_stq; out vec4 v_col; out float v_fog;\n"
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
    "  v_stq = a_stq; v_col = a_col; v_fog = a_fog;\n"
    "}\n";

static const char* fs_src =
    "#version 330 core\n"
    "in vec3 v_stq; in vec4 v_col; in float v_fog;\n"
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
    "    t.rgb *= 255.0; t.a *= u_tex_alpha_scale;\n"
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
static const char* present_fs =
    "#version 330 core\n"
    "in vec2 v_uv; uniform sampler2D u_tex; out vec4 o;\n"
    "void main(){ o = vec4(clamp(texture(u_tex, v_uv).rgb, 0.0, 1.0), 1.0); }\n";

static GLuint compile(const char* vs, const char* fs)
{
    GLuint p = glCreateProgram(), s[2];
    const char* src[2] = { vs, fs };
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
    return p;
}

static void* gl_getproc(const char* name) { return (void*)SDL_GL_GetProcAddress(name); }

void gs_renderer_init(void)
{
    if (!gl_load(gl_getproc)) host_fatal("OpenGL 3.3 entry points missing");
    host_log("gl: %s / %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    if (!gs_mem_selftest()) host_fatal("GS swizzle tables failed self test");
    gs_reset();

    prog = compile(vs_src, fs_src);
#define U(n) u_##n = glGetUniformLocation(prog, "u_" #n)
    U(rt_size); U(rt_margin); U(tme); U(tfx); U(tcc); U(fst); U(texsize); U(fge); U(fogcol); U(ate); U(atst); U(aref);
    U(tex); U(tex_alpha_scale); U(texofs); U(premul); U(zscale); U(clamp_mode); U(clamp_vals); U(afail_pass); U(rt16);
    U(vr_on); U(vr_src); U(vr_dst); U(vr_m0); U(vr_m1); U(vr_m2); U(vr_m3); U(vr_zab); U(vr_zs); U(vr_hud);
#undef U
    present_prog = compile(present_vs, present_fs);
    up_tex = glGetUniformLocation(present_prog, "u_tex");
    up_rect = glGetUniformLocation(present_prog, "u_rect");

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

/* internal resolution: the configured multiplier, or (auto) enough to cover the window */
float gs_wanted_scale(void)
{
    int ww, wh;
    float s;
    if (xr_active() && g_vr_stereo && xr_recommended_eye_height() > 0) {
        /* VR: the eye images are the 480-line display area: enough lines for the headset's
           recommended eye height (Quest 2: 1824 -> 4x), whatever the desktop setting */
        s = ceilf((float)xr_recommended_eye_height() * (float)g_vr_res / 100.0f / 480.0f);
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
    /* hash the decoded palette by decoding a 1-pixel-wide strip is overkill; decode via a fake 16x16 index ramp */
    extern void gs_get_palette(uint64_t tex0, uint64_t texa, uint32_t* out, int n);
    gs_get_palette(tex0, gs.texa, tmp, (int)n);
    for (uint32_t i = 0; i < n; i++) { h ^= tmp[i]; h *= 16777619u; }
    return h;
}

static GLuint tex_get(uint64_t tex0, uint64_t texa, int* out_w, int* out_h, float* ascale)
{
    *ascale = 255.0f;
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
    for (uint32_t p = page0; p <= page1; p++) content = (content ^ page_hash_get(p)) * 0x9E3779B97F4A7C15ull + p;

    for (int i = 0; i < MAX_TEX; i++) {
        TexEntry* t = &texcache[i];
        if (t->used && t->tex0_key == key && t->content == content && t->clut_hash == ch && (psm != PSMCT16 && psm != PSMCT24 ? 1 : t->texa == texa)) {
            t->last_use = frame_no; *out_w = t->w; *out_h = t->h; return t->tex;
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
    }
filled:
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
static void draw_batch(int afail_pass)
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
    ensure_target(&sh->tex, &sh->fbo, &sh->pw, &sh->ph, rts[i].pw, rts[i].ph);
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
}

void gs_flush(void)
{
    const DrawState* st = &batch_state;
    TexMemo *replay_memo = NULL, *record_memo = NULL;
    uint32_t fbp, fbw, fpsm, fbmsk;
    int premul = 0, tme, afail;
    if (gs_flush_blocked) return;
    uint64_t test;
    if (!batch_valid || vcount == 0) { vcount = 0; nsruns = 0; return; }
    stat_flushes++;

    fbp = st->ctx.frame & 0x1FF; fbw = (st->ctx.frame >> 16) & 0x3F; fpsm = (st->ctx.frame >> 24) & 0x3F;
    fbmsk = (uint32_t)(st->ctx.frame >> 32);
    cur_rt = rt_get(fbp, fbw, fpsm);

    if (gs_replaying) {   /* never from GS memory: what the executed frame loaded here */
        cur_rt->pending_reload = 0;
        reload_replay(cur_rt, st->epoch);
    } else if (cur_rt->pending_reload) {
        cur_rt->pending_reload = 0;
        if (!batch_full_cover) { rt_load_from_mem(cur_rt); stat_reloads++; reload_record(cur_rt, st->epoch); rt_touch(cur_rt); }
    }
    if (!gs_replaying && first[cur_rt - rts].how == FIRST_NONE) rt_first_draw(cur_rt, st, fbw, fpsm);
    glBindFramebuffer(GL_FRAMEBUFFER, cur_rt->fbo);
    {
        uint32_t zbp = st->ctx.zbuf & 0x1FF;
        GLuint dt = depth_for(zbp, cur_rt->pw, cur_rt->ph);
        attach_depth(cur_rt->fbo, dt);
    }
    glViewport(0, 0, cur_rt->pw, cur_rt->ph);
    {
        uint64_t sc = st->ctx.scissor;
        int x0 = sc & 0x7FF, x1 = (sc >> 16) & 0x7FF, y0 = (sc >> 32) & 0x7FF, y1 = (sc >> 48) & 0x7FF;
        if (!cur_rt->has_box) { cur_rt->has_box = 1; cur_rt->bx0 = x0; cur_rt->by0 = y0; cur_rt->bx1 = x1; cur_rt->by1 = y1; }
        else {
            if (x0 < cur_rt->bx0) cur_rt->bx0 = x0;
            if (y0 < cur_rt->by0) cur_rt->by0 = y0;
            if (x1 > cur_rt->bx1) cur_rt->bx1 = x1;
            if (y1 > cur_rt->by1) cur_rt->by1 = y1;
        }
        if (g_margin_x && x0 == 0 && x1 >= (int)fbw * 64 - 1) { x0 = -g_margin_x; x1 += g_margin_x; }   /* full width -> widened view */
        glEnable(GL_SCISSOR_TEST);
        glScissor((GLint)((x0 + g_margin_x) * g_scale), (GLint)(y0 * g_scale),
                  (GLsizei)((x1 - x0 + 1) * g_scale), (GLsizei)((y1 - y0 + 1) * g_scale));
    }
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
        float ox, oy;
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
                srt = rt_for_texture(tbp, tpsm, &ox, &oy, &is16);
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
            uni4f(u_texofs, 1.0f / (float)w, 1.0f / (float)h, 0.0f, 0.0f);
            uni1f(u_tex_alpha_scale, ascale);
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

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    {
        static int vbo_next;
        if (vbo_next + vcount > VBO_VERTS) {
            glBufferData(GL_ARRAY_BUFFER, VBO_VERTS * sizeof(GlVertex), NULL, GL_STREAM_DRAW);   /* orphan */
            vbo_next = 0;
        }
        vbo_first = vbo_next;
        if (!nsruns) glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)vbo_first * (GLintptr)sizeof(GlVertex), (GLsizeiptr)(vcount * sizeof(GlVertex)), vbuf);
        else for (int r = 0, at = 0; r <= nsruns; r++) {   /* the parts between the static runs */
            int end = r < nsruns ? sruns[r].start : vcount;
            if (end > at)
                glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(vbo_first + at) * (GLintptr)sizeof(GlVertex), (GLsizeiptr)((end - at) * sizeof(GlVertex)), vbuf + at);
            if (r < nsruns) at = sruns[r].start + sruns[r].count;
        }
        vbo_next += vcount;
    }
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
        if (fx && prim_big) { fx = 0; if (vr_st.on) vr_st.nbig++; }   /* a screen layer, not an effect */
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

void gs_draw_primitive(int type, const GsVertex* v, int count, const GsContext* c, uint64_t prim)
{
    {
        float x0 = v[0].x, x1 = v[0].x, y0 = v[0].y, y1 = v[0].y;
        for (int i = 1; i < count; i++) { x0 = fminf(x0, v[i].x); x1 = fmaxf(x1, v[i].x); y0 = fminf(y0, v[i].y); y1 = fmaxf(y1, v[i].y); }
        prim_big = (x1 - x0) > 320.0f || (y1 - y0) > 240.0f;
        prim_tall = (y1 - y0) > 240.0f;
    }
    {   /* VR (as native_glue.c): a polygon with corners both in the world (GS depth) and not - the
           others behind or too close to the camera - stretched between the world and the panel: left out */
        extern int gs_raw_vr, gs_vu1_vr, gs_raw_ui, g_vr_fx_fix;
        if (vr_u.on && g_vr_fx_fix && !gs_raw_vr && !gs_vu1_vr && !gs_raw_ui && !prim_big && count > 1) {
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
            o->vr = (q->cull & 0x80) ? 1.0f : (q->cull & 0x40) ? 2.0f : (q->cull & 0x20) ? 0.0f : 3.0f;   /* 2: 2D with a GS depth (vr_fx_fix); 3: 2D on the VR panel; 0: screen layers */
            vr_stat_vertex(o);
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
            stat_prims++; batch_full_cover = 0;   /* what batch_for does for a primitive of the same state */
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
    if (!*tex) { glGenTextures(1, tex); glGenFramebuffers(1, fbo); }
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
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
static void rt_restore(void)
{
    for (int i = 0; i < MAX_RT; i++) {
        RenderTarget* r = &rts[i];
        const RenderTarget* sn = &rts_snap[i];
        if (!sn->used) {   /* created during the replay: lost (recreated from GS memory when next used) */
            if (r->used) { glDeleteTextures(1, &r->tex); glDeleteFramebuffers(1, &r->fbo); fb_forget(); memset(r, 0, sizeof(*r)); }
            continue;
        }
        /* the slot's textures now (the replay's, or another target's if the replay evicted it) */
        if (r->used && r->pw == sn->pw && r->ph == sn->ph) { shadow[i] = (Shadow){ r->tex, r->fbo, r->pw, r->ph }; }
        else {
            if (r->used) { glDeleteTextures(1, &r->tex); glDeleteFramebuffers(1, &r->fbo); fb_forget(); }
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
    gs_flush();
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
}

/* StoreRenderTex on the GPU (RECVX_NATIVE_RTEX): GS pixels (0,0)-(w,h) of a render target
   copied at render resolution (margin and scale as in gs_rt_to_mem) into a new texture, 0 if
   there is no such target (transparent black) */
unsigned int gs_capture_rt(uint32_t fbp, uint32_t fbw, uint32_t w, uint32_t h)
{
    RenderTarget* rt = rt_find(fbp, fbw ? fbw : 1);
    GLuint tex = 0, fbo = 0;
    int cw = 0, ch = 0, pw = (int)(w * g_scale), ph = (int)(h * g_scale);
    gs_flush();
    rt_touch(rt);
    ensure_target(&tex, &fbo, &cw, &ch, pw, ph);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (rt) {
        int sx = (int)(g_margin_x * g_scale), sw = pw < rt->pw - sx ? pw : rt->pw - sx, sh = ph < rt->ph ? ph : rt->ph;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rt->fbo);
        glBlitFramebuffer(sx, 0, sx + sw, sh, 0, 0, sw, sh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo); fb_forget();
    return tex;
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
static GLuint vig_prog;
static GLint uv_tan, uv_amt;
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
    "in vec2 v_t; uniform float u_amt; out vec4 o;\n"
    "void main(){ float r = length(v_t); float a = smoothstep(0.45, 1.25, r) * u_amt; o = vec4(0.75, 0.0, 0.0, a); }\n";

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
    "  if (u_kind == 0) {\n"   /* the dot */
    "    float r = length(v_c); aa = fwidth(r);\n"
    "    float a = 1.0 - smoothstep(1.0 - aa * 1.5, 1.0, r);\n"
    "    if (a <= 0.0) discard; o = vec4(1.0, 1.0, 1.0, a); return;\n"
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
    /* the number: n digits of 5x7 dots in cells of 6x9, on a plate of n*6+2 x 11 */
    "  int n = u_val >= 100 ? 3 : u_val >= 10 ? 2 : 1;\n"
    "  vec2 g = (v_c * 0.5 + 0.5) * vec2(float(n * 6 + 2), 11.0);\n"
    "  g.y = 11.0 - g.y;\n"
    "  vec2 q = abs(v_c) * vec2(u_aspect, 1.0); vec2 lim = vec2(u_aspect, 1.0) - 0.25;\n"
    "  float plate = length(max(q - lim, 0.0)) - 0.25; aa = fwidth(plate);\n"
    "  float pa = (1.0 - smoothstep(-aa, aa, plate)) * 0.6;\n"
    "  float lit = 0.0;\n"
    "  vec2 c = g - vec2(1.0, 2.0);\n"
    "  int di = int(floor(c.x / 6.0));\n"
    "  if (di >= 0 && di < n && c.y >= 0.0 && c.y < 7.0) {\n"
    "    int v = u_val; for (int k = 0; k < n - 1 - di; k++) v /= 10; v = v - (v / 10) * 10;\n"
    "    vec2 cc = vec2(c.x - float(di) * 6.0, c.y);\n"
    "    int col = int(floor(cc.x)), row = int(floor(cc.y));\n"
    "    if (col < 5) {\n"
    "      int bits = F[v * 7 + row];\n"
    "      if (((bits >> (4 - col)) & 1) != 0) { vec2 f = fract(cc) - 0.5; float e = length(max(abs(f) - 0.36, 0.0)) - 0.16; float ea = fwidth(e); lit = 1.0 - smoothstep(-ea, ea, e); }\n"
    "    }\n"
    "  }\n"
    "  float a = max(pa, lit);\n"
    "  if (a <= 0.0) discard;\n"
    "  o = vec4(mix(vec3(0.05), vec3(1.0, 0.97, 0.9), lit), a);\n"
    "}\n";

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
        for (int k = 0; k < 3; k++) V[k] = v->G[k] * p[0] + v->G[4 + k] * p[1] + v->G[8 + k] * p[2] + v->G[12 + k];   /* world -> V */
        for (int k = 0; k < 3; k++) e[k] = M[k] * V[0] + M[4 + k] * V[1] + M[8 + k] * V[2] + M[12 + k];               /* V -> eye (x right, y down, z forward) */
        if (e[2] < 0.05f) continue;
        tx = e[0] / e[2]; ty = -e[1] / e[2];                                    /* tangents right, up */
        nx = (tx - E->l) / (E->r - E->l) * 2.0f - 1.0f;
        ny = (ty - E->d) / (E->u - E->d) * 2.0f - 1.0f;
        if (kind == 0) { ang = tanf(g_vr_sight_size / 100.0f * (3.14159265f / 360.0f)); hx = hy = ang; }
        else if (kind == 1) { hx = hy = 0.04f / e[2]; }                         /* 8 cm across */
        else {
            int n = v->mk_val[i] >= 100 ? 3 : v->mk_val[i] >= 10 ? 2 : 1;
            hy = 0.035f / e[2];                                                 /* the plate 7 cm high */
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

static void draw_vignette(const VrEye* E, int w, int h)
{
    float t, beat, amt;
    if (!g_vr_danger || !g_vr_danger_fx) return;
    if (!vig_prog) {
        vig_prog = compile(vig_vs, vig_fs);
        uv_tan = glGetUniformLocation(vig_prog, "u_tan");
        uv_amt = glGetUniformLocation(vig_prog, "u_amt");
    }
    t = fmodf((float)(SDL_GetTicks() % 100000) / 1000.0f, 0.9f);   /* ~67 beats a minute: lub-dub, pause */
    beat = expf(-((t - 0.05f) * (t - 0.05f)) / 0.004f) + 0.6f * expf(-((t - 0.30f) * (t - 0.30f)) / 0.005f);
    amt = 0.18f + 0.30f * beat;   /* light: at most ~half opaque at the very edge */
    glViewport(0, 0, w, h);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glUseProgram(vig_prog);
    glUniform4f(uv_tan, E->l, E->r, E->u, E->d);
    glUniform1f(uv_amt, amt);
    glBindVertexArray(present_vao);
    glDrawArrays(0x0005 /* GL_TRIANGLE_STRIP */, 0, 4);
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

static int present_vr_stereo(const struct Frame* cur)
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
        if (why) return 0;
    }
    vr_mat_mul(A, iW, iG);   /* V of the frame -> local (metres) */
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
            stream_replay_exact(cur);
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
        tex = xr_eye_target(e, w, h);
        if (!rt || !tex) {
            static int nfail;
            if (nfail++ < 20) host_log("vr: eye %d failed: %s\n", e, !rt ? "no display target after the replay" : "no swapchain image");
            g_vr_status = !rt ? "eye: no display target after the replay" : "eye: no swapchain image";
        }
        if (rt && tex) {
            glBindFramebuffer(GL_FRAMEBUFFER, vr_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            draw_area_to(rt->tex, rt->vw, rt->vh, w, h);
            draw_markers(v, M, E, w, h);
            draw_vignette(E, w, h);
            hold_keep_and_dim(e, w, h, vr_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            fb_forget();
        } else ok = 0;
        xr_eye_release(e);
        rt_restore();
    }
    return ok;
}

/* VR present: one headset frame. Stereo when the frame has a VR camera, else the cinema
   screen (the window image as usual). The window always mirrors. */
static uint64_t gs_present_vr(const struct Frame* cur, float t)
{
    uint64_t swap;
    int ww = 0, wh = 0, fr = xr_frame_begin(), door;
    static uint64_t last_ms;
    uint64_t now_ms = SDL_GetTicks();
    float dt = last_ms ? (float)(now_ms - last_ms) / 1000.0f : 0.0f;
    extern int vr_door_transition(void);   /* (vr_glue.c) */
    last_ms = now_ms;
    if (dt > 0.1f) dt = 0.1f;
    door = g_vr_door_fade && vr_door_transition();
    SDL_GetWindowSizeInPixels(g_window, &ww, &wh);
    if (fr != 1) g_vr_status = fr == 0 ? "headset asked not to render" : "headset not running";
    if (fr == 1 && door && hold_valid && xr_have_held()) {   /* room change: the last 3D picture fades out, fixed in the room */
        vr_fade -= dt / 0.5f;
        if (vr_fade < 0.0f) vr_fade = 0.0f;
        draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);   /* mirror */
        overlay_render();
        if (!present_vr_held()) xr_frame_end_none();
    } else if (fr == 1 && (vr_fade = vr_fade + dt / 0.5f > 1.0f ? 1.0f : vr_fade + dt / 0.5f, present_vr_stereo(cur))) {   /* (fading back in after one) */
        draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);   /* mirror */
        overlay_render();
        xr_frame_end_stereo();
    } else {
        int inbetween = cur && snapshot_valid && t < 1.0f;
        if (!door) vr_fade = 1.0f;   /* (the cinema screen: menus, films - no fade) */
        if (inbetween) {
            RenderTarget* rt;
            stream_replay(cur, t);
            rt = rt_find(gs.disp_fbp, gs.disp_fbw ? gs.disp_fbw : 10);
            draw_display(rt ? rt->tex : 0, rt ? rt->vw : 640, rt ? rt->vh : 512);
            overlay_render();
            rt_restore();
        } else {
            draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);
            overlay_render();
        }
        if (fr == 1) {   /* the game's picture only (no black bars), at its own aspect */
            int vx, vy, vw, vh;
            float dw;
            display_rect(ww, wh, &vx, &vy, &vw, &vh, &dw);
            if (vw > 0 && vh > 0) xr_frame_end_quad_rect(vx, wh - vy - vh, vw, vh);
            else xr_frame_end_quad(ww, wh);
        }
        else if (fr == 0) xr_frame_end_none();
    }
    swap = SDL_GetTicksNS();
    SDL_GL_SwapWindow(g_window);
    return SDL_GetTicksNS() - swap;
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
        overlay_render();
        if (!interp_dumped) { dump_frame(1); interp_dumped = 1; }
        gt_mark(GT_DD);
        rt_restore();
    } else {
        draw_display(final_vw ? img[fin].tex : 0, final_vw ? final_vw : 640, final_vw ? final_vh : 512);
        overlay_render();
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
