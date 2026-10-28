/* Android (Quest) diagnosis: where the time goes, once a second - the render thread's wall, CPU and GPU time for the
   game frames' drawing (stream_exec) and the headset images (gs_present), the draws in them, and the game thread's
   CPU time and its wait for the render thread. CPU much less than wall: waiting (for the GPU, the headset, a lock);
   GPU near the wall: the GPU is the limit. */
#ifdef __ANDROID__
#include "android_prof.h"
#include <SDL3/SDL.h>
#include <time.h>

void host_log(const char* fmt, ...);

volatile unsigned g_and_fx_split;
int g_and_diag = 0;   /* the graphics diagnosis logs (bad vertices, panel/layer primitives, slow frames): paused - performance only now */

uint64_t and_thread_cpu_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts)) return 0;
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t process_cpu_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts)) return 0;
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* ---- GPU timer queries (GLES 3 query objects, target GL_TIME_ELAPSED_EXT) */
#define GL_TIME_ELAPSED_EXT 0x88BF
#define GL_QUERY_RESULT_EXT 0x8866
#define GL_QUERY_RESULT_AVAILABLE_EXT 0x8867
#define GL_GPU_DISJOINT_EXT 0x8FBB
typedef void (*PFN_GenQ)(int, unsigned*);
typedef void (*PFN_BeginQ)(unsigned, unsigned);
typedef void (*PFN_EndQ)(unsigned);
typedef void (*PFN_GetQOuiv)(unsigned, unsigned, unsigned*);
typedef void (*PFN_GetIv)(unsigned, int*);
static PFN_GenQ genq; static PFN_BeginQ beginq; static PFN_EndQ endq; static PFN_GetQOuiv getq; static PFN_GetIv getiv;
static int gpu_ok = -1;   /* -1 not checked, 0 no timer queries, 1 on */
#define NQ 6
static unsigned q[AND_N][NQ];
static int q_pending[AND_N][NQ], q_active[AND_N] = { -1, -1 };

typedef struct { uint64_t wall, wall_max, cpu, cpu_max, gpu, gpu_max; unsigned n, ngpu, gpu_lost, draws, prims; } Acc;
static Acc acc[AND_N];
static struct { uint64_t wall, cpu, cpu_max, wait; unsigned n; } game;   /* game thread (written there, read here: diagnosis only) */
static SDL_SpinLock game_lock;
static struct { uint64_t flush, upload, draw, fbo, texrl; unsigned n; } gl;   /* game frames' drawing: the GL parts (wall) */

void and_exec_gl(uint64_t flush_ns, uint64_t upload_ns, uint64_t draw_ns, uint64_t fbo_ns, uint64_t tex_reload_ns)
{
    gl.n++; gl.flush += flush_ns; gl.upload += upload_ns; gl.draw += draw_ns; gl.fbo += fbo_ns; gl.texrl += tex_reload_ns;
}

static void gpu_init(void)
{
    typedef const unsigned char* (*PFN_GetStr)(unsigned);
    PFN_GetStr getstr = (PFN_GetStr)SDL_GL_GetProcAddress("glGetString");
    const char* ext = getstr ? (const char*)getstr(0x1F03 /* GL_EXTENSIONS */) : NULL;
    genq = (PFN_GenQ)SDL_GL_GetProcAddress("glGenQueries");
    beginq = (PFN_BeginQ)SDL_GL_GetProcAddress("glBeginQuery");
    endq = (PFN_EndQ)SDL_GL_GetProcAddress("glEndQuery");
    getq = (PFN_GetQOuiv)SDL_GL_GetProcAddress("glGetQueryObjectuiv");
    getiv = (PFN_GetIv)SDL_GL_GetProcAddress("glGetIntegerv");
    gpu_ok = genq && beginq && endq && getq && getiv && ext && SDL_strstr(ext, "GL_EXT_disjoint_timer_query");
    if (gpu_ok) for (int k = 0; k < AND_N; k++) genq(NQ, q[k]);
    host_log("android: GPU timer queries %s\n", gpu_ok ? "on (GL_EXT_disjoint_timer_query)" : "not available - GPU times not measured");
}

static void gpu_poll(int k)
{
    int disjoint = 0;
    for (int i = 0; i < NQ; i++) {
        unsigned avail = 0, ns = 0;
        if (!q_pending[k][i]) continue;
        getq(q[k][i], GL_QUERY_RESULT_AVAILABLE_EXT, &avail);
        if (!avail) continue;
        getq(q[k][i], GL_QUERY_RESULT_EXT, &ns);
        q_pending[k][i] = 0;
        getiv(GL_GPU_DISJOINT_EXT, &disjoint);   /* (the GPU clock jumped: the result is meaningless) */
        if (disjoint) { acc[k].gpu_lost++; continue; }
        acc[k].gpu += ns; acc[k].ngpu++;
        if (ns > acc[k].gpu_max) acc[k].gpu_max = ns;
    }
}

void and_gpu_begin(int k)
{
    if (gpu_ok < 0) {   /* off: measured once (headset image 7-10 ms on the GPU, a game frame's drawing 3-4 ms) - with them on the
                           headset got ~3-4 fewer images a second; RECVX_GPU_TIMERS=1 (env.txt) turns them on */
        if (SDL_getenv("RECVX_GPU_TIMERS")) gpu_init();
        else { gpu_ok = 0; host_log("android: GPU timer queries off\n"); }
    }
    if (!gpu_ok || q_active[k] >= 0) return;
    gpu_poll(k);
    for (int i = 0; i < NQ; i++)
        if (!q_pending[k][i]) { beginq(GL_TIME_ELAPSED_EXT, q[k][i]); q_active[k] = i; return; }
    acc[k].gpu_lost++;   /* all still in flight: this one not measured */
}

void and_gpu_end(int k)
{
    if (!gpu_ok || q_active[k] < 0) return;
    endq(GL_TIME_ELAPSED_EXT);
    q_pending[k][q_active[k]] = 1;
    q_active[k] = -1;
}

void and_render_add(int k, uint64_t wall_ns, uint64_t cpu_ns, unsigned draws, unsigned prims)
{
    Acc* a = &acc[k];
    a->n++; a->wall += wall_ns; a->cpu += cpu_ns; a->draws += draws; a->prims += prims;
    if (wall_ns > a->wall_max) a->wall_max = wall_ns;
    if (cpu_ns > a->cpu_max) a->cpu_max = cpu_ns;
}

void and_game_frame(uint64_t wall_ns, uint64_t cpu_ns, uint64_t wait_ns)
{
    SDL_LockSpinlock(&game_lock);
    game.n++; game.wall += wall_ns; game.cpu += cpu_ns; game.wait += wait_ns;
    if (cpu_ns > game.cpu_max) game.cpu_max = cpu_ns;
    SDL_UnlockSpinlock(&game_lock);
}

/* gs_gl.c draws these in the headset: CPU % of one core (the process), ms a game frame / a headset image; GPU busy %,
   ms a game frame / a headset image */
volatile float g_and_hud[6];

/* GPU time by stage (one log, to see which part of an eye's drawing to cut): sequential timer queries, never nested,
   results read when available. 0 the targets restored before an eye's replay, 1 the eye's replay, 2 the eye image
   (resample, lasers, vignette, kept copy), 3 a game frame's drawing */
#define GQ_N 4
#define GQ_RING 12
static unsigned gq[GQ_N][GQ_RING];
static int gq_pend[GQ_N][GQ_RING], gq_cur = -1, gq_curi, gq_ok = -1;
static uint64_t gq_sum[GQ_N];
static unsigned gq_cnt[GQ_N], gq_lost[GQ_N];
static void gq_poll(int s)
{
    for (int i = 0; i < GQ_RING; i++) {
        unsigned avail = 0, ns = 0;
        int disjoint = 0;
        if (!gq_pend[s][i]) continue;
        getq(gq[s][i], GL_QUERY_RESULT_AVAILABLE_EXT, &avail);
        if (!avail) continue;
        getq(gq[s][i], GL_QUERY_RESULT_EXT, &ns);
        gq_pend[s][i] = 0;
        getiv(GL_GPU_DISJOINT_EXT, &disjoint);
        if (disjoint) { gq_lost[s]++; continue; }
        gq_sum[s] += ns; gq_cnt[s]++;
    }
}
void and_gq_stage(int s)
{
    if (gq_ok < 0) {
        typedef const unsigned char* (*PFN_GetStr)(unsigned);
        PFN_GetStr getstr = (PFN_GetStr)SDL_GL_GetProcAddress("glGetString");
        const char* ext = getstr ? (const char*)getstr(0x1F03) : NULL;
        genq = (PFN_GenQ)SDL_GL_GetProcAddress("glGenQueries"); beginq = (PFN_BeginQ)SDL_GL_GetProcAddress("glBeginQuery");
        endq = (PFN_EndQ)SDL_GL_GetProcAddress("glEndQuery"); getq = (PFN_GetQOuiv)SDL_GL_GetProcAddress("glGetQueryObjectuiv");
        getiv = (PFN_GetIv)SDL_GL_GetProcAddress("glGetIntegerv");
        /* (measured once - eye replay 3-4 ms, eye image ~0.85 ms, restores ~0, game frame 3-4 ms - and it cost ~8 headset
           images a second: off; RECVX_GPU_STAGES=1 in env.txt turns it on) */
        gq_ok = SDL_getenv("RECVX_GPU_STAGES") && genq && beginq && endq && getq && getiv && ext && SDL_strstr(ext, "GL_EXT_disjoint_timer_query") && gpu_ok <= 0;
        if (gq_ok) for (int k = 0; k < GQ_N; k++) genq(GQ_RING, gq[k]);
        host_log("android: GPU time by stage %s\n", gq_ok ? "on" : "off");
    }
    if (!gq_ok) return;
    if (gq_cur >= 0) { endq(GL_TIME_ELAPSED_EXT); gq_pend[gq_cur][gq_curi] = 1; gq_cur = -1; }
    if (s < 0 || s >= GQ_N) return;
    gq_poll(s);
    for (int i = 0; i < GQ_RING; i++)
        if (!gq_pend[s][i]) { beginq(GL_TIME_ELAPSED_EXT, gq[s][i]); gq_cur = s; gq_curi = i; return; }
    gq_lost[s]++;
}
static void gq_report(void)
{
    static const char* const nm[GQ_N] = { "targets restored", "eye replay", "eye image", "game frame drawing" };
    char b[512];
    int o = 0;
    if (gq_ok <= 0) return;
    for (int k = 0; k < GQ_N; k++) {
        gq_poll(k);
        o += SDL_snprintf(b + o, sizeof(b) - (size_t)o, "%s%s %.2f ms x %u = %.0f ms (%u lost)", k ? "; " : "", nm[k],
                          gq_cnt[k] ? gq_sum[k] / 1e6 / gq_cnt[k] : 0.0, gq_cnt[k], gq_sum[k] / 1e6, gq_lost[k]);
        gq_sum[k] = 0; gq_cnt[k] = gq_lost[k] = 0;
    }
    host_log("android: GPU by stage last second: %s\n", b);
}

void and_prof_report(void)
{
    gq_report();
    static uint64_t last_t, last_proc;
    static const char* const name[AND_N] = { "game-frame drawing", "headset images" };
    uint64_t now = SDL_GetTicksNS(), proc = process_cpu_ns();
    double secs = last_t ? (now - last_t) / 1e9 : 1.0, exec_avg = 0.0;
    if (gpu_ok > 0) for (int k = 0; k < AND_N; k++) gpu_poll(k);
    for (int k = 0; k < AND_N; k++) {
        Acc* a = &acc[k];
        unsigned n = a->n ? a->n : 1, ng = a->ngpu ? a->ngpu : 1;
        host_log("android: cpu/gpu %s: %u, wall avg %.1f max %.1f ms, cpu avg %.1f max %.1f ms, gpu avg %.1f max %.1f ms (%u measured, %u lost), "
                 "gpu busy %.0f%%, draws avg %u, prims avg %u\n",
                 name[k], a->n, a->wall / 1e6 / n, a->wall_max / 1e6, a->cpu / 1e6 / n, a->cpu_max / 1e6,
                 a->gpu / 1e6 / ng, a->gpu_max / 1e6, a->ngpu, a->gpu_lost,
                 a->ngpu ? 100.0 * (double)a->gpu / ng * a->n / (secs * 1e9) : 0.0, a->draws / n, a->prims / n);
        if (k == AND_EXEC) exec_avg = a->wall / 1e6 / n;
        g_and_hud[1 + k] = a->n ? (float)(a->cpu / 1e6 / n) : 0.0f;
        g_and_hud[4 + k] = a->ngpu ? (float)(a->gpu / 1e6 / ng) : 0.0f;
        if (k == AND_EXEC) g_and_hud[3] = 0.0f;
        if (a->ngpu) g_and_hud[3] += (float)(100.0 * (double)a->gpu / ng * a->n / (secs * 1e9));
        *a = (Acc){ 0 };
    }
    {
        unsigned n = gl.n ? gl.n : 1;
        host_log("android: cpu/gpu game-frame drawing, wall per frame: flushes %.1f ms (vertex uploads %.1f, draw calls %.1f, framebuffers %.1f, "
                 "textures and reloads %.1f, the rest state set-up), outside the flushes %.1f ms\n",
                 gl.flush / 1e6 / n, gl.upload / 1e6 / n, gl.draw / 1e6 / n, gl.fbo / 1e6 / n, gl.texrl / 1e6 / n,
                 exec_avg > gl.flush / 1e6 / n ? exec_avg - gl.flush / 1e6 / n : 0.0);
        gl = (typeof(gl)){ 0 };
    }
    SDL_LockSpinlock(&game_lock);
    {
        unsigned n = game.n ? game.n : 1;
        { void and_glcache_stats(unsigned*, unsigned*); unsigned sk, pa; and_glcache_stats(&sk, &pa);
          host_log("android: GL state calls last second: %u skipped as repeated, %u passed to the driver\n", sk, pa); }
        { void and_glcache_passes(unsigned*, unsigned*, double*); unsigned sw[3], bn[3]; double bp[3]; and_glcache_passes(sw, bn, bp);
          host_log("android: GPU passes last second: target switches game frames %u, headset images %u, other %u; blits game frames %u (%.1f Mpixels), "
                   "headset images %u (%.1f Mpixels), other %u (%.1f Mpixels)\n", sw[1], sw[2], sw[0], bn[1], bp[1] / 1e6, bn[2], bp[2] / 1e6, bn[0], bp[0] / 1e6); }
        { void ntex_stats_log(void); ntex_stats_log(); }
        { void gs_upload_fmt_bytes(unsigned*); unsigned ub[5]; gs_upload_fmt_bytes(ub);
          host_log("android: texture uploads last second in whole blocks: CT32 %u KB (content changed %u KB), CT16 %u KB, T8 %u KB, T4 %u KB\n",
                   ub[0] >> 10, ub[4] >> 10, ub[1] >> 10, ub[2] >> 10, ub[3] >> 10); }
        g_and_hud[0] = last_proc ? (float)(100.0 * (double)(proc - last_proc) / (secs * 1e9)) : 0.0f;
        host_log("android: cpu/gpu game thread: %u frames, cpu avg %.1f max %.1f ms, wall avg %.1f ms, waiting for the render thread avg %.1f ms; "
                 "process cpu %.0f%% of one core; effect polygons split %u\n",
                 game.n, game.cpu / 1e6 / n, game.cpu_max / 1e6, game.wall / 1e6 / n, game.wait / 1e6 / n,
                 last_proc ? 100.0 * (double)(proc - last_proc) / (secs * 1e9) : 0.0, g_and_fx_split);
        game.n = 0; game.wall = game.cpu = game.cpu_max = game.wait = 0;
    }
    SDL_UnlockSpinlock(&game_lock);
    g_and_fx_split = 0;
    last_t = now; last_proc = proc;
}
#else
typedef int android_prof_unused;   /* (PC: nothing) */
#endif
