/*
 * Render thread (the main thread: it owns the window, OpenGL and the event pump).
 *
 * Game frames arrive from the game thread as recorded GS streams (gs/stream.c). Each is
 * executed once (the real image) and then presented in step with the display: with
 * interpolation, every display refresh during the following game-frame period shows the
 * frame replayed at t = elapsed / period between the previous and the new frame, so any
 * refresh rate gets evenly spaced in-between images; game logic stays at its original rate.
 */
#include "../ps2_platform.h"
#include "../gs/gs.h"
#include "../gs/stream.h"
#include "prof.h"
#include "xr.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>

uint64_t gs_frame_count(void);

extern SDL_Window* g_window;

int render_vsync_setting(void) { return g_config.vsync; }

static double display_hz(void)
{
    static double test_hz = -1;   /* RECVX_DISPLAY_HZ (test aid): the refresh rate to assume, e.g. 1000 with vsync off */
    if (test_hz < 0) test_hz = SDL_getenv("RECVX_DISPLAY_HZ") ? SDL_atof(SDL_getenv("RECVX_DISPLAY_HZ")) : 0;
    if (test_hz > 1) return test_hz;
    if (xr_display_hz() > 1.0) return xr_display_hz();   /* VR: the headset's refresh */
    const SDL_DisplayMode* m = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(g_window));
    return m && m->refresh_rate > 1.0f ? m->refresh_rate : 60.0;
}

/* images per second: the display's refresh, or the "Frame rate limit" below it */
static double present_hz(void)
{
    static const int fps_limits[] = { 0, 30, 60, 90, 120, 144 };   /* settings.c names */
    double hz = display_hz();
    int lim = g_config.fps_limit > 0 && g_config.fps_limit < 6 ? fps_limits[g_config.fps_limit] : 0;
    return lim && lim < hz ? lim : hz;
}

/* vr123: the game frames' arrival on an even grid (a frame arriving a few ms early or late does not move it),
   for the VR in-between images by the clock (gs_gl.c) */
uint64_t g_vr_frame_seq;   /* game frames executed (gs_gl.c: the game time by the headset's display times is pulled to their arrival) */
double g_vr_frame_ref, g_vr_frame_period = 1001.0 / 30000.0 * 1e9;

/* VR heartbeat (once a second, xr_active, in the log): what the render thread does - images to the headset, game frames, how long a game frame takes
   to execute (of which the interpolation matching), how many draws a game frame and an image make. Every number that told what was slow on the Quest. */
extern uint64_t g_pc_draws, g_pc_prims_total;
static struct { uint64_t t0; unsigned presents, frames; uint64_t exec_ns, exec_max, match_ns, match_max, present_ns, present_max, exec_draws, img_draws, exec_prims, img_prims; } hb;
static void hb_log(uint64_t now)
{
    extern void host_log(const char*, ...);
    extern int gs_lazy_flush(void);
    extern int g_vr_debug_log;
    if (!g_vr_debug_log) return;   /* (the log's heartbeat: ini vr_debug_log) */
    if (!hb.t0) { hb.t0 = now; return; }
    if (now - hb.t0 < 1000000000ull) return;
    host_log("pc: last second: %u images to the headset, %u game frames | a game frame: exec avg %.2f max %.2f ms (matching avg %.2f max %.2f), %llu draws, %llu prims | an image: %.2f max %.2f ms wall, %llu draws, %llu prims | lazy flush %s\n",
             hb.presents, hb.frames, hb.frames ? hb.exec_ns / 1e6 / hb.frames : 0.0, hb.exec_max / 1e6, hb.frames ? hb.match_ns / 1e6 / hb.frames : 0.0, hb.match_max / 1e6,
             (unsigned long long)(hb.frames ? hb.exec_draws / hb.frames : 0), (unsigned long long)(hb.frames ? hb.exec_prims / hb.frames : 0),
             hb.presents ? hb.present_ns / 1e6 / hb.presents : 0.0, hb.present_max / 1e6,
             (unsigned long long)(hb.presents ? hb.img_draws / hb.presents : 0), (unsigned long long)(hb.presents ? hb.img_prims / hb.presents : 0), gs_lazy_flush() ? "on" : "off");
    {   /* why the draws ended: reasons 0 full 1 mode 2 prim 3 tex0 4 epoch 5 alpha 6 test 7 frame 8 zbuf 9 scissor 10 xyoffset 11 tex1/clamp 12 texa/fog 13 other */
        extern unsigned g_pc_why[2][14], g_pc_why_epoch[2], g_pc_epoch_merged;
        host_log("pc: batches kept going over a texture upload (same texture): %u\n", g_pc_epoch_merged); g_pc_epoch_merged = 0;
        for (int r = 0; r < 2; r++) {
            char b[200]; int o = 0;
            for (int k = 0; k < 14; k++) if (g_pc_why[r][k]) o += snprintf(b + o, sizeof(b) - (size_t)o, " %d:%u", k, g_pc_why[r][k]);
            host_log("pc: draws ended (%s):%s | epoch changed and nothing else: %u\n", r ? "image replays" : "game frames", b, g_pc_why_epoch[r]);
        }
        memset(g_pc_why, 0, sizeof(g_pc_why)); memset(g_pc_why_epoch, 0, sizeof(g_pc_why_epoch));
    }
    memset(&hb, 0, sizeof(hb)); hb.t0 = now;
}

void render_loop(void)
{
    Frame *prev = NULL, *cur = NULL;
    uint64_t arrived = 0, pace_until = 0;
    unsigned vr_np = 0, vr_skip = 0;   /* VR diagnosis */
    double swap_avg = 0.0;   /* how long the buffer swap blocks, smoothed */
    double period = 1001.0 / 30000.0 * 1e9;   /* game frame period, measured below */
    int interp = 0, shown = 0;   /* refreshes presented since the frame arrived */
    int check_replay = SDL_getenv("RECVX_REPLAY_CHECK") && SDL_atoi(SDL_getenv("RECVX_REPLAY_CHECK"));
    float fixed_t = SDL_getenv("RECVX_INTERP_T") ? (float)SDL_atof(SDL_getenv("RECVX_INTERP_T")) : 0.0f;   /* test aid: first refresh shows t */
    FILE* trace = SDL_getenv("RECVX_TRACE_PRESENT") ? fopen(SDL_getenv("RECVX_TRACE_PRESENT"), "w") : NULL;   /* test aid */
    prof_thread(PROF_RIDLE);
    for (;;) {
        Frame* f;
        uint64_t now;
        host_pump_events();
        /* nothing to show yet: wait for the game; otherwise take what has arrived, waiting for
           it until the next refresh is due (a frame arriving meanwhile is executed at once) */
        now = SDL_GetTicksNS();
        f = stream_pop(!cur ? 20 : pace_until > now ? (int)((pace_until - now) / 1000000) : 0);   /* the rest: SDL_DelayPrecise */
        if (f) {
            prof_push(PROF_EXEC);
            uint64_t tx0 = SDL_GetTicksNS(), hd0 = g_pc_draws, hp0 = g_pc_prims_total;
            gs_frame_begin();
            stream_exec(f);
            prof_pop();
            if (xr_active()) { uint64_t tx = SDL_GetTicksNS() - tx0; hb.frames++; hb.exec_ns += tx; if (tx > hb.exec_max) hb.exec_max = tx; hb.exec_draws += g_pc_draws - hd0; hb.exec_prims += g_pc_prims_total - hp0; }
            if (xr_active()) {   /* vr74 diagnosis (vr_debug_log): a game frame's drawing that takes long holds the next headset frame */
                extern int g_vr_debug_log; void host_log(const char*, ...);
                static unsigned nslow;
                uint64_t tx = SDL_GetTicksNS() - tx0;
                if (g_vr_debug_log && tx > 8000000 && nslow < 80) { nslow++; host_log("vr-diag: slow game frame drawing: %.1f ms\n", tx / 1e6); }
            }
            if (stream_is_partial(f)) { stream_release(f); continue; }   /* download sync point */
            now = SDL_GetTicksNS();
            if (arrived) {
                double d = (double)(now - arrived);
                if (d > 10e6 && d < 100e6) period += 0.1 * (d - period);
            }
            arrived = now;
            shown = 0;
            {   /* vr123 */
                double nw = (double)now, d = nw - (g_vr_frame_ref + period);
                if (g_vr_frame_ref <= 0.0 || fabs(d) > period * 0.5) g_vr_frame_ref = nw;
                else g_vr_frame_ref += period + 0.1 * d;
                g_vr_frame_period = period;
                g_vr_frame_seq++;
            }
            prof_push(PROF_INTERP);
            uint64_t tm0 = SDL_GetTicksNS();
            interp = g_config.interpolate && stream_match(cur, f);
            if (xr_active()) { uint64_t tm = SDL_GetTicksNS() - tm0; hb.match_ns += tm; if (tm > hb.match_max) hb.match_max = tm; }
            prof_pop();
            gs_frame_executed(interp);
            if (interp && check_replay) gs_replay_check(f);
            stream_release(prev);
            prev = cur;
            cur = f;
        }
        if (!cur) continue;
        now = SDL_GetTicksNS();
        if (pace_until > now) {   /* the refresh is not due yet: a frame that arrived is executed, its
                                     image waits for the refresh like any other (shown at once, it cut
                                     the previous image short); woken early (ms timeout): the rest */
            SDL_DelayPrecise(pace_until - now);
            if (!f) continue;
            now = SDL_GetTicksNS();
        }
        {
            /* With a display rate that is a whole multiple of the game's (60/120/240 Hz), step by
               refreshes: t = 1/n, 2/n, ... 1, evenly spaced however the game thread's frame
               delivery jitters against vsync. Otherwise by time since arrival. */
            double n = period * 1e-9 * present_hz();
            float t;
            uint64_t swap;
            if (!interp) t = 1.0f;
            else if (fixed_t > 0.0f) t = shown ? 1.0f : fixed_t;
            else if ((g_config.vsync || xr_active()) && n > 1.5 && fabs(n - floor(n + 0.5)) < 0.1) t = (float)((shown + 1) / floor(n + 0.5));
            else t = (float)((double)(now - arrived) / period);
            /* the next game frame is already waiting (the game runs below its rate): an
               in-between image of this one would only delay it (a whole replay of the scene) */
            if (t < 1.0f && fixed_t <= 0.0f && stream_queued()) { if (xr_active()) vr_skip++; continue; }
            if (xr_active() && ++vr_np >= 900) {   /* vr72 diagnosis (vr_debug_log): headset images skipped because the next game frame was already waiting */
                extern int g_vr_debug_log; void host_log(const char*, ...);
                if (g_vr_debug_log) host_log("vr-diag: render loop: game frame %.1f ms (avg), in-between images skipped (next frame waiting) %u of %u\n", period * 1e-6, vr_skip, vr_np);
                vr_np = vr_skip = 0;
            }
            {   /* the name tag moves with the 3D */
                float x, y;
                int fl = stream_point_at(interp ? prev : NULL, cur, t, &x, &y);
                overlay_point(fl, x, y);            }
            { extern int g_vr_interp_now; g_vr_interp_now = interp; }
            uint64_t tp0 = SDL_GetTicksNS(), pd0 = g_pc_draws, pp0 = g_pc_prims_total;
            swap = gs_present(cur, t);
            if (xr_active()) { uint64_t tp = SDL_GetTicksNS() - tp0; hb.presents++; hb.present_ns += tp; if (tp > hb.present_max) hb.present_max = tp; hb.img_draws += g_pc_draws - pd0; hb.img_prims += g_pc_prims_total - pp0; hb_log(SDL_GetTicksNS()); }
            shown++;
            if (trace) fprintf(trace, "%llu %.3f %llu %llu\n", (unsigned long long)gs_frame_count(), t, (unsigned long long)(now / 1000),
                               (unsigned long long)(swap / 1000));
            /* pace to the display refresh instead of spinning: without vsync, and with vsync when
               the swap does not wait (the offscreen driver, some compositors): presenting as fast
               as the GPU allows starved the frames themselves (RM_1310 headless: 197 -> 37 ms a
               game frame in vanilla mode). Whether the swap waits is its own duration: one that
               waits blocks for part of a refresh on average. Presents start one refresh apart
               (the wait is the stream_pop above), so every image, in-between or not, is shown
               for one refresh. */
            {   /* on a fixed grid (a late present does not shift the following ones) */
                uint64_t tick = (uint64_t)(1e9 / present_hz());
                swap_avg += ((double)swap - swap_avg) / 8;
                if (xr_active()) pace_until = 0;   /* VR: xrWaitFrame in gs_present paces to the headset */
                else if (g_config.vsync && swap_avg >= tick / 8 && present_hz() == display_hz()) pace_until = 0;   /* limited: always paced */
                else pace_until = pace_until && pace_until + tick > now ? pace_until + tick : now + tick;
            }
        }
    }
}
