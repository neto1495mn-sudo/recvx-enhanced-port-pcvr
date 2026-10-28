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
#include "android_prof.h"
static unsigned hb_slow_logged;   /* slow frame lines this second (at most 5: android_prof.c has the summary) */
/* Android (Quest) diagnosis: once a second, the headset images presented and the loops that skipped the present - a
   partial frame (download sync point) or an in-between image dropped because the next frame was waiting */
static unsigned hb_presents, hb_skip_partial, hb_skip_queued, hb_forced;
/* motion smoothness (the owner: micro stutters, "walks and stops"): per headset image, the game time it shows (frame +
   t) against when the headset shows it (OpenXR display time); the ideal is the game's 30 frames a second of game time
   spread evenly. stops: no game time advanced; slow/fast: under 0.6 / over 1.5 of the ideal; gaps: a headset refresh
   with no new image (the previous one shown again) */
static unsigned mo_stop, mo_slow, mo_ok, mo_fast, mo_gap, mo_n;
static unsigned mo_nointerp, mo_tclamp, mo_tmid;   /* images: no interpolation (the frame itself), t held at 1, t in between */
static int and_trace_exec;   /* this game frame's GPU passes are traced (gl_cache.c) */
/* game frame pacing: intervals between game frames as the game thread finished them (sub) and as the render thread
   finished executing them (exe): count, late (> 1.5 periods), bunched (< 0.5 period), max */
static unsigned pc_n, pc_sub_late, pc_sub_bunch, pc_exe_late, pc_exe_bunch;
static double pc_sub_max, pc_exe_max;
static double mo_rmin = 1e9, mo_rmax;
/* the last headset image's time: frames drawn slower than the game's rate (40-55 ms, the message before the lighter)
   always found the next one waiting and skipped every present - the headset got no image and the controllers were
   no longer read (both in xr_frame_begin), the game looked frozen. Over 30 ms without one: shown anyway */
static uint64_t hb_last_present;
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
#ifdef __ANDROID__
    { void and_sampler_thread(int); and_sampler_thread(0); }   /* android_sampler.c */
#endif
#ifdef __ANDROID__
    host_log("android: render loop built " __DATE__ " " __TIME__ "\n");   /* (render.c changes with most commits: the build's age) */
#endif
    for (;;) {
        Frame* f;
        uint64_t now;
        host_pump_events();
        /* nothing to show yet: wait for the game; otherwise take what has arrived, waiting for
           it until the next refresh is due (a frame arriving meanwhile is executed at once) */
        now = SDL_GetTicksNS();
#ifdef __ANDROID__
        /* Android (Quest): the headset wants an image now (its xrWaitFrame returned on the waiter thread, xr.c): that first,
           the game frame waiting is executed right after, while the waiter waits for the next one - executed first,
           it (6-17 ms) and the image (7-8 ms) missed the headset's 13.9 ms */
        int and_hold = xr_active() && cur && xr_frame_ready();
        {   /* ... but a game frame waits at most 5 ms (the log: holding it for every image took the game from 30 to 22-27; 16 ms
               made the game frames run in bunches once the headset images filled the thread - its frames now take 3-5 ms) */
            static uint64_t queued_since;
            uint64_t tn = SDL_GetTicksNS();
            if (!stream_queued()) queued_since = 0;
            else if (!queued_since) queued_since = tn;
            if (and_hold && queued_since && tn - queued_since > 5000000ull) and_hold = 0;
        }
        f = and_hold ? NULL : stream_pop(!cur ? 20 : pace_until > now ? (int)((pace_until - now) / 1000000) : 0);
#else
        f = stream_pop(!cur ? 20 : pace_until > now ? (int)((pace_until - now) / 1000000) : 0);   /* the rest: SDL_DelayPrecise */
#endif
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
                host_log("android: last second %u presents (%u forced: next frame waiting), skipped %u partial, %u queued\n",
                         hb_presents, hb_forced, hb_skip_partial, hb_skip_queued);
                { extern volatile int g_and_fps_headset, g_and_fps_game; g_and_fps_headset = (int)hb_presents; g_and_fps_game = (int)hb_frames; }   /* (gs_gl.c draws them) */
                {   /* A/B test, automatic: every 20 s the next of 4 modes (window swap, per-eye flush: on or off); each mode's
                       headset images and game frames per second logged, so the faster one shows without anyone's help */
                    extern int g_and_ab;
                    static unsigned ab_secs, ab_pres, ab_game;
                    static const char* const ab_names[4] = { "window swap on, eye flush on", "window swap OFF, eye flush on",
                                                             "window swap on, eye flush OFF", "window swap OFF, eye flush OFF" };
                    ab_pres += hb_presents; ab_game += hb_frames;
                    if (++ab_secs >= 20) {
                        host_log("android: A/B mode %d (%s): %.1f headset images/s, %.1f game frames/s over %u s\n",
                                 g_and_ab, ab_names[g_and_ab], ab_pres / (double)ab_secs, ab_game / (double)ab_secs, ab_secs);
                        /* (the A/B run showed no clear winner - scene-dependent, eye flush off looked worse: mode 0 kept) */
                        ab_secs = ab_pres = ab_game = 0;
                    }
                }
                if (mo_n) host_log("android: motion last second: %u images; game time per image vs display time: %u stopped, %u slow, %u even, %u fast "
                                   "(ratio %.2f..%.2f); %u headset refreshes skipped before an image\n",
                                   mo_n, mo_stop, mo_slow, mo_ok, mo_fast, mo_rmin, mo_rmax, mo_gap);
                host_log("android: motion images last second: %u without interpolation (the game frame itself), %u interpolated but at t = 1 (the next frame "
                         "not there yet), %u in between\n", mo_nointerp, mo_tclamp, mo_tmid);
                mo_nointerp = mo_tclamp = mo_tmid = 0;
                mo_n = mo_stop = mo_slow = mo_ok = mo_fast = mo_gap = 0; mo_rmin = 1e9; mo_rmax = 0;
                xr_timing_log();
                if (pc_n) host_log("android: game frame pacing last second: %u frames; from the game thread %u late %u bunched (max %.1f ms), "
                                   "executed on the render thread %u late %u bunched (max %.1f ms)\n",
                                   pc_n, pc_sub_late, pc_sub_bunch, pc_sub_max / 1e6, pc_exe_late, pc_exe_bunch, pc_exe_max / 1e6);
                pc_n = pc_sub_late = pc_sub_bunch = pc_exe_late = pc_exe_bunch = 0; pc_sub_max = pc_exe_max = 0;
                hb_presents = hb_skip_partial = hb_skip_queued = hb_forced = 0;
                hb_slow_logged = 0;
                and_prof_report();
                { void and_sampler_report(void); and_sampler_report(); }
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
            unsigned and_d0 = g_and_stall.draws, and_p0 = g_and_stall.prims;
            uint64_t and_c0 = and_thread_cpu_ns();
            memset(&g_and_stall, 0, sizeof(g_and_stall));
            g_and_stall.draws = and_d0; g_and_stall.prims = and_p0;
            { extern void and_glcache_reset(void); and_glcache_reset(); }   /* (gl_cache.c: what others changed since is forgotten) */
            { extern int g_and_time_frame; static unsigned nexec; g_and_time_frame = (nexec++ % 30) == 0; }   /* (gs_gl.c: 1 in 30 timed part by part) */
            and_gpu_begin(AND_EXEC);
#endif
#ifdef __ANDROID__
            { extern volatile int g_and_phase; g_and_phase = 1; }
#endif
#ifdef __ANDROID__
            { extern void and_gq_stage(int); and_gq_stage(3); }
            { extern int and_pass_trace(int); static unsigned k; if ((k++ % 150) == 75) and_trace_exec = and_pass_trace(1); }
#endif
#ifdef __ANDROID__
            {   /* option 1 (gs_gl.c): in VR, 3 game frames in 4 executed without their draws */
                extern int g_and_exec_nodraw; extern int gs_exec_nodraw_ok(void);
                static unsigned k;
                const VrView* vv = stream_frame_vr(f);
                g_and_exec_nodraw = 0;
                if (xr_active() && g_vr_stereo && vv && vv->valid && gs_exec_nodraw_ok()) g_and_exec_nodraw = (k++ & 3) != 0;
            }
#endif
            gs_frame_begin();
            stream_exec(f);
#ifdef __ANDROID__
            { extern void and_gq_stage(int); and_gq_stage(-1); }
            if (and_trace_exec) { extern int and_pass_trace(int); extern void and_pass_trace_log(const char*); and_pass_trace(0); and_pass_trace_log("a game frame"); and_trace_exec = 0; }
#endif
            prof_pop();
#ifdef __ANDROID__
            { extern volatile int g_and_phase; g_and_phase = 0; }
#endif
#ifdef __ANDROID__
            and_gpu_end(AND_EXEC);
            {   /* a slow frame's drawing by cause: GS textures, native textures, RT reloads from GS memory (CPU),
                   shader compiles, StoreImage; "other" is the draws themselves */
                void host_log(const char*, ...);
                const AndStall* s = &g_and_stall;
                uint64_t tx = SDL_GetTicksNS() - tx0, cpu = and_thread_cpu_ns() - and_c0,
                         known = s->tex_ns + s->ntex_ns + s->reload_ns + s->compile_ns + s->store_ns;
                and_render_add(AND_EXEC, tx, cpu, s->draws - and_d0, s->prims - and_p0);
                { extern int g_and_time_frame; if (g_and_time_frame) and_exec_gl(s->flush_ns, s->upload_ns, s->draw_ns, s->fbo_ns, s->reload_ns + s->tex_ns + s->ntex_ns); g_and_time_frame = 0; }
                extern int g_and_diag;
                if (g_and_diag && tx >= 20000000 && hb_slow_logged < 5) {
                    hb_slow_logged++;
                    host_log("android: slow frame %.1f ms (cpu %.1f ms, %u draws, %u prims): tex %.1f ms/%u, ntex %.1f ms/%u, reload %.1f ms/%u (cpu loop %.1f), "
                             "compile %.1f ms/%u, storeimage %.1f ms/%u, other %.1f ms; GL: flushes %.1f ms (uploads %.1f, draw calls %.1f, framebuffers %.1f)\n",
                             tx / 1e6, cpu / 1e6, s->draws - and_d0, s->prims - and_p0, s->tex_ns / 1e6, s->tex_n, s->ntex_ns / 1e6, s->ntex_n,
                             s->reload_ns / 1e6, s->reload_n, s->reload_cpu_ns / 1e6,
                             s->compile_ns / 1e6, s->compile_n, s->store_ns / 1e6, s->store_n, (tx > known ? tx - known : 0) / 1e6,
                             s->flush_ns / 1e6, s->upload_ns / 1e6, s->draw_ns / 1e6, s->fbo_ns / 1e6);
                }
            }
#endif
            if (xr_active()) {   /* vr74 diagnosis (vr_debug_log): a game frame's drawing that takes long holds the next headset frame */
                extern int g_vr_debug_log; void host_log(const char*, ...);
                static unsigned nslow;
                uint64_t tx = SDL_GetTicksNS() - tx0;
                if (g_vr_debug_log && tx > 8000000 && nslow < 80) { nslow++; host_log("vr-diag: slow game frame drawing: %.1f ms\n", tx / 1e6); }
            }
#ifdef __ANDROID__
            if (stream_is_partial(f)) { extern int g_and_exec_nodraw; g_and_exec_nodraw = 0; }   /* (a download: the rest of the frame drawn) */
#endif
            if (stream_is_partial(f)) {   /* download sync point */
#ifdef __ANDROID__
                hb_skip_partial++;
#endif
                stream_release(f); continue;
            }
            now = SDL_GetTicksNS();
            if (arrived) {
                double d = (double)(now - arrived);
                if (d > 10e6 && d < 100e6) period += 0.1 * (d - period);
            }
#ifdef __ANDROID__
            {
                extern uint64_t stream_frame_submit_ns(const Frame*);
                static uint64_t pc_sub0, pc_exe0;
                uint64_t sub = stream_frame_submit_ns(f);
                if (pc_exe0 && pc_sub0 && sub > pc_sub0) {
                    double ds = (double)(sub - pc_sub0), de = (double)(now - pc_exe0);
                    pc_n++;
                    if (ds > period * 1.5) pc_sub_late++; else if (ds < period * 0.5) pc_sub_bunch++;
                    if (de > period * 1.5) pc_exe_late++; else if (de < period * 0.5) pc_exe_bunch++;
                    if (ds > pc_sub_max) pc_sub_max = ds;
                    if (de > pc_exe_max) pc_exe_max = de;
                }
                pc_sub0 = sub; pc_exe0 = now;
            }
#endif
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
#ifdef __ANDROID__
            { extern int g_and_exec_nodraw; g_and_exec_nodraw = 0; }   /* (the frame's last batch was flushed above: draws after it are drawn) */
#endif
            if (interp && check_replay) gs_replay_check(f);
            stream_release(prev);
            prev = cur;
            cur = f;
        }
        if (!cur) continue;
#ifdef __ANDROID__
        if (xr_active() && !and_hold && !xr_frame_ready()) {   /* the headset does not want an image yet: game frames meanwhile */
            if (!stream_queued()) xr_frame_wait_ready(2);
            continue;
        }
#endif
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
#ifdef __ANDROID__
            if (t < 1.0f && fixed_t <= 0.0f && stream_queued() && !and_hold) {   /* (the headset waiting: its image first) */
#else
            if (t < 1.0f && fixed_t <= 0.0f && stream_queued()) {
#endif
#ifdef __ANDROID__
                if (xr_active() && SDL_GetTicksNS() - hb_last_present > 30000000ull) { hb_forced++; t = 1.0f; }   /* the frame itself, no replay */
                else
#endif
                {
                    if (xr_active()) vr_skip++;
#ifdef __ANDROID__
                    hb_skip_queued++;
#endif
                    continue;
                }
            }
            if (xr_active() && ++vr_np >= 900) {   /* vr72 diagnosis (vr_debug_log): headset images skipped because the next game frame was already waiting */
                extern int g_vr_debug_log; void host_log(const char*, ...);
                if (g_vr_debug_log) host_log("vr-diag: render loop: game frame %.1f ms (avg), in-between images skipped (next frame waiting) %u of %u\n", period * 1e-6, vr_skip, vr_np);
                vr_np = vr_skip = 0;
            }
            {   /* the name tag moves with the 3D */
                float x, y;
                int fl = stream_point_at(interp ? prev : NULL, cur, t, &x, &y);
                overlay_point(fl, x, y);            }
#ifdef __ANDROID__
            uint64_t and_pt0 = SDL_GetTicksNS(), and_pc0 = and_thread_cpu_ns();
            unsigned and_pd0 = g_and_stall.draws, and_pp0 = g_and_stall.prims;
            { extern void and_glcache_reset(void); and_glcache_reset(); }
            and_gpu_begin(AND_PRESENT);
#endif
#ifdef __ANDROID__
            { extern volatile int g_and_phase; g_and_phase = 2; }
#endif
            swap = gs_present(cur, t);
#ifdef __ANDROID__
            { extern volatile int g_and_phase; g_and_phase = 0; }
            and_gpu_end(AND_PRESENT);
            hb_presents++;
            hb_last_present = SDL_GetTicksNS();
            and_render_add(AND_PRESENT, hb_last_present - and_pt0, and_thread_cpu_ns() - and_pc0, g_and_stall.draws - and_pd0, g_and_stall.prims - and_pp0);
            if (xr_active()) {
                static int64_t mo_d0;
                static double mo_g0;
                int64_t disp = xr_display_time_ns();
                double g = (double)gs_frame_count() - 1.0 + (interp ? (t < 0.0f ? 0.0 : t > 1.0f ? 1.0 : t) : 1.0), xp = xr_display_hz() > 1.0 ? 1e9 / xr_display_hz() : 13.9e6;
                if (!interp) mo_nointerp++;
                else if (t >= 0.999f) mo_tclamp++;
                else mo_tmid++;
                if (mo_d0 && disp > mo_d0 && period > 1e6) {
                    double dd = (double)(disp - mo_d0), r = (g - mo_g0) / (dd / period);
                    mo_n++;
                    if (dd > xp * 1.5) mo_gap++;
                    if (g - mo_g0 <= 1e-6) mo_stop++;
                    else if (r < 0.6) mo_slow++;
                    else if (r > 1.5) mo_fast++;
                    else mo_ok++;
                    if (r < mo_rmin) mo_rmin = r;
                    if (r > mo_rmax) mo_rmax = r;
                }
                mo_d0 = disp; mo_g0 = g;
            }
#endif
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
