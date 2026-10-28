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
    static unsigned every[AND_N];
    if (gpu_ok < 0) gpu_init();
    if ((every[k]++ & 7) != 0) return;   /* 1 in 8 measured: reading queries back each frame can make the driver submit */
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

void and_prof_report(void)
{
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
        { void gs_upload_stats(unsigned*, unsigned*); unsigned uh, um; gs_upload_stats(&uh, &um);
          host_log("android: texture upload chunks last second: %u skipped (same data, memory untouched), %u run\n", uh, um); }
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
