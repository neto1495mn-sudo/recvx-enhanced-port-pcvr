/* Android (Quest): GL state calls that set what is already set are not passed to the driver. The profiler showed
   the GL driver and its kernel calls (__ioctl) at 35-50% of the render thread, and an eye's replay made ~10000 GL
   calls for ~500 draws - most of them state the draw before had set the same way (each flush sets its whole
   state). The GL entry points (gl_loader.h pointers) are wrapped once; only the render thread's calls are filtered
   (the HD texture uploader has its own context). What is not known is passed on and remembered: and_glcache_reset
   (each game frame, each headset image, after ImGui) forgets everything. */
#ifdef __ANDROID__
#include "../render/gl_loader.h"
#include <SDL3/SDL.h>
#include <string.h>
#include <math.h>

void host_log(const char* fmt, ...);

static SDL_ThreadID owner;
static unsigned skipped, passed;

#define NCAP 10
static const GLenum caps[NCAP] = { 0x0B71 /* DEPTH_TEST */, 0x0BE2 /* BLEND */, 0x0C11 /* SCISSOR_TEST */, 0x0B44 /* CULL_FACE */,
                                   0x3000 /* CLIP_DISTANCE0 */, 0x3001, 0x0B90 /* STENCIL_TEST */, 0x0BD0 /* DITHER */,
                                   0x8DB9 /* FRAMEBUFFER_SRGB */, 0x8037 /* POLYGON_OFFSET_FILL */ };
#define NUNIT 8
#define TP 2048   /* texture parameters: (texture, pname) -> value */
static struct {
    int cap[NCAP];                        /* -1 unknown, 0, 1 */
    int unit;                             /* active texture unit (0..), -1 unknown */
    int64_t tex[NUNIT];                   /* 2D texture bound per unit, -1 unknown */
    int64_t dfb, rfb, prog, vao, abuf;    /* -1 unknown */
    int vp_k, sc_k; GLint vp[4], sc[4];
    int dfunc_k, dmask_k, cmask_k, bfunc_k, beq_k, bcol_k, ccol_k;
    GLenum dfunc; GLboolean dmask, cmask[4]; GLenum bfunc[4], beq[2]; GLfloat bcol[4], ccol[4];
    int pack_k, unpack_k; GLint pack, unpack;
} s;
static struct { uint32_t tex; uint16_t pname; uint8_t used; GLint v; } tp[TP];

static PFN_glEnable r_Enable; static PFN_glDisable r_Disable; static PFN_glActiveTexture r_ActiveTexture; static PFN_glBindTexture r_BindTexture;
static PFN_glTexParameteri r_TexParameteri; static PFN_glGenTextures r_GenTextures; static PFN_glDeleteTextures r_DeleteTextures;
static PFN_glBindFramebuffer r_BindFramebuffer; static PFN_glDeleteFramebuffers r_DeleteFramebuffers; static PFN_glUseProgram r_UseProgram;
static PFN_glBindVertexArray r_BindVertexArray; static PFN_glBindBuffer r_BindBuffer; static PFN_glViewport r_Viewport; static PFN_glScissor r_Scissor;
static PFN_glDepthFunc r_DepthFunc; static PFN_glDepthMask r_DepthMask; static PFN_glColorMask r_ColorMask; static PFN_glBlendFuncSeparate r_BlendFuncSeparate;
static PFN_glBlendEquationSeparate r_BlendEquationSeparate; static PFN_glBlendColor r_BlendColor; static PFN_glClearColor r_ClearColor;
static PFN_glPixelStorei r_PixelStorei; static PFN_glBlitFramebuffer r_BlitFramebuffer;
/* GPU passes (on the Quest's tiled GPU each target switch stores and loads a whole target): per phase (0 other, 1 game
   frames, 2 headset images) the draw-target switches passed to the driver, the blits and their pixels */
extern volatile int g_and_phase;
static unsigned fb_sw[3], blit_n[3];
static double blit_px[3];
/* pass trace (one eye / one game frame now and then): each draw-target switch with its draws, vertices, clears, blits */
static PFN_glDrawArrays r_DrawArrays; static PFN_glClear r_Clear;
static int mine(void);
#define TRN 64
static struct { GLuint fbo; unsigned draws, verts, clears, blits; } tr[TRN];
static int tr_n, tr_on, tr_pending;
/* GPU time per pass (the owner: more GPU detail, even if it costs): a timer query from each target switch to the next,
   on the traced eye / game frame only; read when available (and_pass_trace_times), no new trace before */
typedef void (GLAPIENTRY *PFN_TrGenQ)(GLsizei, GLuint*); typedef void (GLAPIENTRY *PFN_TrBeginQ)(GLenum, GLuint);
typedef void (GLAPIENTRY *PFN_TrEndQ)(GLenum); typedef void (GLAPIENTRY *PFN_TrGetQ)(GLuint, GLenum, GLuint*);
static PFN_TrBeginQ trq_begin; static PFN_TrEndQ trq_end; static PFN_TrGetQ trq_get;
static GLuint trq[TRN];
static int trq_ok = -1, trq_active;
static void trq_init(void)
{
    PFN_TrGenQ gen = (PFN_TrGenQ)SDL_GL_GetProcAddress("glGenQueries");
    trq_begin = (PFN_TrBeginQ)SDL_GL_GetProcAddress("glBeginQuery"); trq_end = (PFN_TrEndQ)SDL_GL_GetProcAddress("glEndQuery");
    trq_get = (PFN_TrGetQ)SDL_GL_GetProcAddress("glGetQueryObjectuiv");
    trq_ok = gen && trq_begin && trq_end && trq_get && SDL_GL_ExtensionSupported("GL_EXT_disjoint_timer_query");
    if (trq_ok) gen(TRN, trq);
    host_log("android: GPU time per pass (traced eye / game frame) %s\n", trq_ok ? "on" : "not available");
}
static void trq_stop(void) { if (trq_active) { trq_end(0x88BF /* TIME_ELAPSED */); trq_active = 0; } }
int and_pass_trace(int on)
{
    if (on) {
        if (tr_pending || tr_on) return 0;
        if (trq_ok < 0) trq_init();
        tr_n = 0; tr_on = 1;
        return 1;
    }
    if (!tr_on) return 0;
    if (trq_ok > 0) trq_stop();
    tr_on = 0; tr_pending = tr_n > 0;
    return 1;
}
/* the traced passes' GPU ms: 1 when all are in (the trace is then done), 0 not yet */
int and_pass_trace_times(double* ms, int max)
{
    if (!tr_pending) return 0;
    for (int i = 0; i < tr_n && i < max; i++) {
        GLuint avail = 0, ns = 0;
        if (trq_ok <= 0) { ms[i] = -1.0; continue; }
        trq_get(trq[i], 0x8867 /* RESULT_AVAILABLE */, &avail);
        if (!avail) return 0;
        trq_get(trq[i], 0x8866 /* RESULT */, &ns);
        ms[i] = ns / 1e6;
    }
    tr_pending = 0;
    return 1;
}
int and_pass_trace_take(GLuint* fbo, unsigned (*cnt)[4], int max)
{
    int n = tr_n < max ? tr_n : max;
    for (int i = 0; i < n; i++) { fbo[i] = tr[i].fbo; cnt[i][0] = tr[i].draws; cnt[i][1] = tr[i].verts; cnt[i][2] = tr[i].clears; cnt[i][3] = tr[i].blits; }
    return n;
}
static void tr_new(GLuint f)
{
    if (!tr_on || !mine() || tr_n >= TRN) return;
    if (trq_ok > 0) { trq_stop(); trq_begin(0x88BF, trq[tr_n]); trq_active = 1; }
    tr[tr_n].fbo = f; tr[tr_n].draws = tr[tr_n].verts = tr[tr_n].clears = tr[tr_n].blits = 0; tr_n++;
}
#define TR_CUR (tr_on && tr_n > 0 && mine() ? &tr[tr_n - 1] : NULL)
static void GLAPIENTRY c_DrawArrays(GLenum m, GLint first, GLsizei n) { if (TR_CUR) { TR_CUR->draws++; TR_CUR->verts += (unsigned)n; } r_DrawArrays(m, first, n); }
static void GLAPIENTRY c_Clear(GLbitfield b) { if (TR_CUR) TR_CUR->clears++; r_Clear(b); }
void and_glcache_passes(unsigned* sw, unsigned* bn, double* bpx)
{
    for (int k = 0; k < 3; k++) { sw[k] = fb_sw[k]; bn[k] = blit_n[k]; bpx[k] = blit_px[k]; fb_sw[k] = blit_n[k] = 0; blit_px[k] = 0; }
}

void and_glcache_reset(void)
{
    for (int i = 0; i < NCAP; i++) s.cap[i] = -1;
    s.unit = -1;
    for (int i = 0; i < NUNIT; i++) s.tex[i] = -1;
    s.dfb = s.rfb = s.prog = s.vao = s.abuf = -1;
    s.vp_k = s.sc_k = s.dfunc_k = s.dmask_k = s.cmask_k = s.bfunc_k = s.beq_k = s.bcol_k = s.ccol_k = s.pack_k = s.unpack_k = 0;
    memset(tp, 0, sizeof(tp));
}
void and_glcache_stats(unsigned* skip, unsigned* pass) { *skip = skipped; *pass = passed; skipped = passed = 0; }

static int mine(void) { return SDL_GetCurrentThreadID() == owner; }
static int capi(GLenum c) { for (int i = 0; i < NCAP; i++) if (caps[i] == c) return i; return -1; }
#define SKIP() do { skipped++; return; } while (0)
#define PASS() (passed++)

static void GLAPIENTRY c_Enable(GLenum c)
{
    int i = mine() ? capi(c) : -1;
    if (i >= 0) { if (s.cap[i] == 1) SKIP(); s.cap[i] = 1; }
    PASS(); r_Enable(c);
}
static void GLAPIENTRY c_Disable(GLenum c)
{
    int i = mine() ? capi(c) : -1;
    if (i >= 0) { if (s.cap[i] == 0) SKIP(); s.cap[i] = 0; }
    PASS(); r_Disable(c);
}
static void GLAPIENTRY c_ActiveTexture(GLenum t)
{
    if (mine()) { int u = (int)t - 0x84C0; if (u == s.unit) SKIP(); s.unit = u >= 0 && u < NUNIT ? u : -1; }
    PASS(); r_ActiveTexture(t);
}
static void GLAPIENTRY c_BindTexture(GLenum target, GLuint t)
{
    if (mine()) {
        if (target == 0x0DE1 /* TEXTURE_2D */ && s.unit >= 0) { if (s.tex[s.unit] == (int64_t)t) SKIP(); s.tex[s.unit] = t; }
        else if (target == 0x0DE1) for (int i = 0; i < NUNIT; i++) s.tex[i] = -1;   /* (unit unknown) */
    }
    PASS(); r_BindTexture(target, t);
}
static int tp_slot(uint32_t tex, uint16_t pname, int make)
{
    uint32_t h = (tex * 2654435761u ^ pname * 40503u) & (TP - 1);
    for (int n = 0; n < 16; n++, h = (h + 1) & (TP - 1)) {
        if (tp[h].used && tp[h].tex == tex && tp[h].pname == pname) return (int)h;
        if (!tp[h].used) { if (!make) return -1; tp[h].used = 1; tp[h].tex = tex; tp[h].pname = pname; tp[h].v = (GLint)0x7FFFFFFF; return (int)h; }
    }
    return -1;   /* (crowded: not remembered) */
}
static void tp_forget(uint32_t tex) { for (int i = 0; i < TP; i++) if (tp[i].used && tp[i].tex == tex) tp[i].used = 0; }
static void GLAPIENTRY c_TexParameteri(GLenum target, GLenum pname, GLint v)
{
    if (mine() && target == 0x0DE1 && s.unit >= 0 && s.tex[s.unit] > 0 &&
        (pname == 0x2800 || pname == 0x2801 || pname == 0x2802 || pname == 0x2803)) {   /* MAG/MIN filter, WRAP_S/T */
        int k = tp_slot((uint32_t)s.tex[s.unit], (uint16_t)pname, 1);
        if (k >= 0) { if (tp[k].v == v) SKIP(); tp[k].v = v; }
    }
    PASS(); r_TexParameteri(target, pname, v);
}
static void GLAPIENTRY c_GenTextures(GLsizei n, GLuint* t)
{
    r_GenTextures(n, t);
    if (mine()) for (GLsizei i = 0; i < n; i++) tp_forget(t[i]);   /* (a reused name starts with default parameters) */
}
static void GLAPIENTRY c_DeleteTextures(GLsizei n, const GLuint* t)
{
    if (mine()) for (GLsizei i = 0; i < n; i++) {
        tp_forget(t[i]);
        for (int u = 0; u < NUNIT; u++) if (s.tex[u] == (int64_t)t[i]) s.tex[u] = 0;   /* deleting a bound texture binds 0 */
    }
    r_DeleteTextures(n, t);
}
static void GLAPIENTRY c_BindFramebuffer(GLenum target, GLuint f)
{
    if (mine()) {
        if (target == 0x8D40 /* FRAMEBUFFER */) { if (s.dfb == (int64_t)f && s.rfb == (int64_t)f) SKIP(); s.dfb = s.rfb = f; }
        else if (target == 0x8CA9 /* DRAW */) { if (s.dfb == (int64_t)f) SKIP(); s.dfb = f; }
        else if (target == 0x8CA8 /* READ */) { if (s.rfb == (int64_t)f) SKIP(); s.rfb = f; }
        if (target != 0x8CA8) { fb_sw[(unsigned)g_and_phase < 3 ? g_and_phase : 0]++; tr_new(f); }
    }
    PASS(); r_BindFramebuffer(target, f);
}
static void GLAPIENTRY c_BlitFramebuffer(GLint x0, GLint y0, GLint x1, GLint y1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLbitfield m, GLenum f)
{
    if (TR_CUR) TR_CUR->blits++;
    if (mine()) { int k = (unsigned)g_and_phase < 3 ? g_and_phase : 0; blit_n[k]++; blit_px[k] += fabs((double)(dx1 - dx0) * (double)(dy1 - dy0)); }
    r_BlitFramebuffer(x0, y0, x1, y1, dx0, dy0, dx1, dy1, m, f);
}
static void GLAPIENTRY c_DeleteFramebuffers(GLsizei n, const GLuint* f)
{
    if (mine()) for (GLsizei i = 0; i < n; i++) { if (s.dfb == (int64_t)f[i]) s.dfb = 0; if (s.rfb == (int64_t)f[i]) s.rfb = 0; }
    r_DeleteFramebuffers(n, f);
}
static void GLAPIENTRY c_UseProgram(GLuint p) { if (mine()) { if (s.prog == (int64_t)p) SKIP(); s.prog = p; } PASS(); r_UseProgram(p); }
static void GLAPIENTRY c_BindVertexArray(GLuint a) { if (mine()) { if (s.vao == (int64_t)a) SKIP(); s.vao = a; } PASS(); r_BindVertexArray(a); }
static void GLAPIENTRY c_BindBuffer(GLenum t, GLuint b)
{
    if (mine() && t == 0x8892 /* ARRAY_BUFFER (not VAO state) */) { if (s.abuf == (int64_t)b) SKIP(); s.abuf = b; }
    PASS(); r_BindBuffer(t, b);
}
static void GLAPIENTRY c_Viewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    if (mine()) { GLint v[4] = { x, y, w, h }; if (s.vp_k && !memcmp(v, s.vp, sizeof(v))) SKIP(); memcpy(s.vp, v, sizeof(v)); s.vp_k = 1; }
    PASS(); r_Viewport(x, y, w, h);
}
static void GLAPIENTRY c_Scissor(GLint x, GLint y, GLsizei w, GLsizei h)
{
    if (mine()) { GLint v[4] = { x, y, w, h }; if (s.sc_k && !memcmp(v, s.sc, sizeof(v))) SKIP(); memcpy(s.sc, v, sizeof(v)); s.sc_k = 1; }
    PASS(); r_Scissor(x, y, w, h);
}
static void GLAPIENTRY c_DepthFunc(GLenum f) { if (mine()) { if (s.dfunc_k && s.dfunc == f) SKIP(); s.dfunc = f; s.dfunc_k = 1; } PASS(); r_DepthFunc(f); }
static void GLAPIENTRY c_DepthMask(GLboolean m) { if (mine()) { if (s.dmask_k && s.dmask == m) SKIP(); s.dmask = m; s.dmask_k = 1; } PASS(); r_DepthMask(m); }
static void GLAPIENTRY c_ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    if (mine()) { GLboolean v[4] = { r, g, b, a }; if (s.cmask_k && !memcmp(v, s.cmask, 4)) SKIP(); memcpy(s.cmask, v, 4); s.cmask_k = 1; }
    PASS(); r_ColorMask(r, g, b, a);
}
static void GLAPIENTRY c_BlendFuncSeparate(GLenum a, GLenum b, GLenum c, GLenum d)
{
    if (mine()) { GLenum v[4] = { a, b, c, d }; if (s.bfunc_k && !memcmp(v, s.bfunc, sizeof(v))) SKIP(); memcpy(s.bfunc, v, sizeof(v)); s.bfunc_k = 1; }
    PASS(); r_BlendFuncSeparate(a, b, c, d);
}
static void GLAPIENTRY c_BlendEquationSeparate(GLenum a, GLenum b)
{
    if (mine()) { if (s.beq_k && s.beq[0] == a && s.beq[1] == b) SKIP(); s.beq[0] = a; s.beq[1] = b; s.beq_k = 1; }
    PASS(); r_BlendEquationSeparate(a, b);
}
static void GLAPIENTRY c_BlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    if (mine()) { GLfloat v[4] = { r, g, b, a }; if (s.bcol_k && !memcmp(v, s.bcol, sizeof(v))) SKIP(); memcpy(s.bcol, v, sizeof(v)); s.bcol_k = 1; }
    PASS(); r_BlendColor(r, g, b, a);
}
static void GLAPIENTRY c_ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    if (mine()) { GLfloat v[4] = { r, g, b, a }; if (s.ccol_k && !memcmp(v, s.ccol, sizeof(v))) SKIP(); memcpy(s.ccol, v, sizeof(v)); s.ccol_k = 1; }
    PASS(); r_ClearColor(r, g, b, a);
}
static void GLAPIENTRY c_PixelStorei(GLenum p, GLint v)
{
    if (mine()) {
        if (p == 0x0D05 /* PACK_ALIGNMENT */) { if (s.pack_k && s.pack == v) SKIP(); s.pack = v; s.pack_k = 1; }
        else if (p == 0x0CF5 /* UNPACK_ALIGNMENT */) { if (s.unpack_k && s.unpack == v) SKIP(); s.unpack = v; s.unpack_k = 1; }
    }
    PASS(); r_PixelStorei(p, v);
}

#define WRAP(n) do { r_##n = gl_##n; gl_##n = c_##n; } while (0)
/* render thread, after gl_load: from now on its state calls go through the cache */
void and_glcache_install(void)
{
    owner = SDL_GetCurrentThreadID();
    WRAP(Enable); WRAP(Disable); WRAP(ActiveTexture); WRAP(BindTexture); WRAP(TexParameteri); WRAP(GenTextures); WRAP(DeleteTextures);
    WRAP(BindFramebuffer); WRAP(DeleteFramebuffers); WRAP(UseProgram); WRAP(BindVertexArray); WRAP(BindBuffer); WRAP(Viewport);
    WRAP(Scissor); WRAP(DepthFunc); WRAP(DepthMask); WRAP(ColorMask); WRAP(BlendFuncSeparate); WRAP(BlendEquationSeparate);
    WRAP(BlendColor); WRAP(ClearColor); WRAP(PixelStorei); WRAP(BlitFramebuffer); WRAP(DrawArrays); WRAP(Clear);
    and_glcache_reset();
    host_log("android: GL state cache on (repeated state calls not passed to the driver)\n");
}
#else
typedef int gl_cache_unused;
#endif
