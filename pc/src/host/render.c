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
#include <string.h>

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
double g_vr_frame_ref, g_vr_frame_period = 1001.0 / 30000.0 * 1e9;

#ifdef __ANDROID__
#include "xr.h"
#endif

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
#ifdef __ANDROID__
        {   /* Android (Quest) diagnosis: once a second, the game frames that arrived and the controller buttons - a stop
               shows as 0 frames (the game stuck) or frames with no button reaching it */
            static uint64_t hb_t;
            static unsigned hb_frames, hb_loops;
            extern volatile uint64_t g_and_cd_ns;
            extern volatile unsigned g_and_cd_reads, g_and_cd_sectors;
            void host_log(const char*, ...);
            hb_loops++;
            if (f) hb_frames++;
            if (now - hb_t >= 1000000000ull) {
                VrInput in;
                int have = xr_input(&in);
                host_log("android: last second %u game frames, %u loops; buttons %x sticks %.1f %.1f / %.1f %.1f triggers %.1f %.1f grips %.1f %.1f%s\n",
                         hb_frames, hb_loops, have ? in.buttons : 0, have ? in.lx : 0, have ? in.ly : 0, have ? in.rx : 0, have ? in.ry : 0,
                         have ? in.ltrig : 0, have ? in.rtrig : 0, have ? in.lgrip : 0, have ? in.rgrip : 0, have ? "" : " (no controller input)");
                if (g_and_cd_reads)
                    host_log("android: last second disc %u reads, %u KB, %.1f ms\n", g_and_cd_reads, g_and_cd_sectors * 2, g_and_cd_ns / 1e6);
                g_and_cd_ns = 0; g_and_cd_reads = g_and_cd_sectors = 0;
                hb_t = now; hb_frames = hb_loops = 0;
            }
        }
#endif
        if (f) {
            prof_push(PROF_EXEC);
            uint64_t tx0 = SDL_GetTicksNS();
#ifdef __ANDROID__
            memset(&g_and_stall, 0, sizeof(g_and_stall));
#endif
            gs_frame_begin();
            stream_exec(f);
            prof_pop();
#ifdef __ANDROID__
            {   /* a slow frame's drawing by cause: GS textures, native textures, RT reloads from GS memory (CPU),
                   shader compiles, StoreImage; "other" is the draws themselves */
                void host_log(const char*, ...);
                const AndStall* s = &g_and_stall;
                uint64_t tx = SDL_GetTicksNS() - tx0, known = s->tex_ns + s->ntex_ns + s->reload_ns + s->compile_ns + s->store_ns;
                if (tx >= 20000000)
                    host_log("android: slow frame %.1f ms: tex %.1f ms/%u, ntex %.1f ms/%u, reload %.1f ms/%u, compile %.1f ms/%u, storeimage %.1f ms/%u, other %.1f ms\n",
                             tx / 1e6, s->tex_ns / 1e6, s->tex_n, s->ntex_ns / 1e6, s->ntex_n, s->reload_ns / 1e6, s->reload_n,
                             s->compile_ns / 1e6, s->compile_n, s->store_ns / 1e6, s->store_n, (tx > known ? tx - known : 0) / 1e6);
            }
#endif
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
            }
            prof_push(PROF_INTERP);
            interp = g_config.interpolate && stream_match(cur, f);
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
            swap = gs_present(cur, t);
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
