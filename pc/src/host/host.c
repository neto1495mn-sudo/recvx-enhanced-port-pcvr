/*
 * Host entry point: configuration, SDL window/GL context, event pump, and the
 * hand-off into the original game's main() (renamed recvx_game_main).
 */
#include "xr.h"
#include "ps2_platform.h"
#include "../gs/stream.h"
#include "prof.h"
#include "coop.h"
#include "tps.h"
#include "menu_mouse.h"
#include "lobby.h"
#include "bindings.h"
#include "settings.h"
void render_loop(void);
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <execinfo.h>
#if defined(__ANDROID__) && __ANDROID_API__ < 33   /* bionic's backtrace() is API 33+: no backtrace before */
static inline int backtrace(void** f, int n) { (void)f; (void)n; return 0; }
static inline void backtrace_symbols_fd(void* const* f, int n, int fd) { (void)f; (void)n; (void)fd; }
#endif
#include <signal.h>
#include <unistd.h>
#endif

int recvx_game_main(int argc, char* argv[]);
void audio_init(void);
void gs_renderer_init(void);

int pc_screen_filter_mode(void) { return g_config.screen_filter; }
int pc_skip_door_demo(void) { return g_config.skip_doors; }
int pc_quick_knife_on(void) { return g_config.quick_knife; }
int pc_cfg_no_ink_ribbon(void) { return g_config.no_ink_ribbon; }   /* in co-op the host's counts (coop_glue.c pc_no_ink_ribbon) */
/* the game tests a few joints of a model against the screen; with a wide or moving free
   camera that drops models that are partly visible, so third person never culls */
extern int tps_cam_active;
int pc_model_cull(void) { return g_config.model_cull && !tps_cam_active; }
int pc_frame_blend(void) { return g_config.frame_blend < 0 ? !g_config.interpolate : g_config.frame_blend; }
int pc_battle_unlocked(void) { return g_config.battle_unlocked; }
int pc_skip_intro(void) { return g_config.skip_intro; }
int pc_skip_intro_press_start(void)
{
    static int done;
    if (!g_config.skip_intro || done) return 0;
    done = 1;
    return 1;
}
int pc_config_native_tex(void) { return g_config.native_tex; }
int pc_native_3d(void)
{
    static int env = -2;
    if (env == -2) env = getenv("RECVX_NATIVE3D") ? atoi(getenv("RECVX_NATIVE3D")) != 0 : -1;
#ifdef __ANDROID__
    {   /* Android (Quest) diagnosis: the menu's "Native 3D renderer" switched (missing arms / glass test) */
        static int last = -1;
        int now = env >= 0 ? env : g_config.native_3d;
        if (now != last) { host_log("android: native 3D renderer %s\n", now ? "ON" : "OFF"); last = now; }
    }
#endif
    return env >= 0 ? env : g_config.native_3d;
}

/* full-screen 2D overlays: 0 original 4:3, 1 stretched to the (widened) view, 2 off;
   fades always cover the whole view */
enum { PC_OVL_CINEMA, PC_OVL_FADE, PC_OVL_EFFECT };   /* as in pc_prelude.h */
static int overlay_stretch;
int pc_overlay_begin(int kind)
{
    int mode = kind == PC_OVL_CINEMA ? g_config.cinema_bars : kind == PC_OVL_EFFECT ? g_config.screen_effects : 1;
    if (mode == 2) return 0;
    overlay_stretch = mode == 1;
    return 1;
}
void pc_overlay_end(void) { overlay_stretch = 0; }
float pc_overlay_x(float x)
{
    extern float pc_hor_scale(void);
    return overlay_stretch ? 320.0f + (x - 320.0f) * pc_hor_scale() : x;
}

#ifdef __ANDROID__
#define PC_DEF_CINEMA_BARS 2   /* the headset: no cutscene bars (the setting is hidden until the code; config.c moves older saved files) */
#else
#define PC_DEF_CINEMA_BARS 1
#endif
HostConfig g_config = { .window_w = 1280, .window_h = 960, .vsync = 1, .log_level = 1, .music_volume = 100, .sfx_volume = 100, .interpolate = 1, .frame_blend = 0, .aspect = 2, .show_fps = 1, .show_ping = 1,
                       .screen_filter = 1, .cinema_bars = PC_DEF_CINEMA_BARS, .screen_effects = 1, .native_3d = 1, .model_cull = 1, .native_tex = 1, .quick_knife = 1,
                       .skip_doors = 1, .skip_intro = 1, .battle_unlocked = 1, .contrast = 100, .gamma = 10 };   /* first-start defaults; recvx.ini overrides */
SDL_Window* g_window;
SDL_GLContext g_gl;

void host_log(const char* fmt, ...)
{
    va_list ap;
    if (g_config.log_level <= 0) return;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void host_fatal(const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "fatal: %s\n", buf);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "recvx", buf, g_window);
    exit(1);
}

void host_pump_events(void)
{
    SDL_Event ev;
    bind_test_frame();
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_GAMEPAD_ADDED || ev.type == SDL_EVENT_GAMEPAD_REMOVED) { extern void pad_devices_changed(void); pad_devices_changed(); }
        if (bind_event(&ev)) continue;   /* a "press a key" of the settings */
        if (overlay_event(&ev)) continue;
        tps_input_event(&ev);
        menu_mouse_event(&ev);
        lobby_io_event(&ev);
        switch (ev.type) {
        case SDL_EVENT_QUIT:
            host_log("exit: quit asked (window closed or quit)\n");   /* vr144 */
            exit(0);
        case SDL_EVENT_KEY_DOWN:
            if (bind_hotkey(&ev, B_FULLSCREEN)) {
                g_config.fullscreen = !g_config.fullscreen;
                SDL_SetWindowFullscreen(g_window, g_config.fullscreen);
                settings_save_later();
            }
            if (bind_hotkey(&ev, B_TPS_TOGGLE) && !g_lobby_io.text_on) {
                g_tps.on = !g_tps.on;
                settings_save_later();
            }
            if (bind_hotkey(&ev, B_VR_RECENTER) && !g_lobby_io.text_on) xr_recenter();
            break;
        }
    }
    settings_poll();
    lobby_io_frame(g_window);
}

#ifndef _WIN32
/* debugging aid: RECVX_WATCHDOG=<seconds> prints a backtrace and exits (headless runs) */
static void watchdog(int sig)
{
    void* frames[64];
    int n = backtrace(frames, 64);
    (void)sig;
    fprintf(stderr, "watchdog: backtrace\n");
    backtrace_symbols_fd(frames, n, 2);
    if (getenv("RECVX_PROFILE")) exit(3);   /* run atexit: profile dump */
    _exit(3);
}
#endif

static void usage(void)
{
    fprintf(stderr,
        "recvx-enhanced-recomp " RECVX_VERSION "\n"
        "usage: recvx [options] <game disc image (.iso)>\n"
        "  --save-dir <dir>   settings and memory card directory (default: ./save if it exists,\n"
        "                     else save/ next to the program, which also holds mods/ and logs)\n"
        "  --size WxH         window size (default 1280x960)\n"
        "  --fullscreen\n"
        "  --enhanced         display-rate frames (60+ FPS) blended from the 30 FPS game (default)\n"
        "  --vanilla          original 30 FPS presentation\n"
        "  --skip-doors       room transitions (doors, stairs, ladders) without their animation (default on)\n"
        "  --skip-intro       boot straight into the title menu (default on)\n"
        "  --third-person     over-the-shoulder camera, mouse/keyboard controls (toggle: F5)\n"
        "  --vr               play in a VR headset through OpenXR: stereo first person in normal play,\n"
        "                     a cinema screen for cutscenes and menus (F8 recenters)\n"
        "  --coop-host PORT   online co-op: wait for a partner on this UDP port (F3: co-op window)\n"
        "  --coop-join HOST:PORT#CODE | ROOM  online co-op: join a host, or a room through the co-op server\n"
        "  --coop-server ADDR   co-op server (host or host:port, UDP 7141)  --coop-conn auto|direct|server  --coop-pw PASSWORD\n"
        "  --quiet\n"
        "  --version          print the version and exit\n"
        "  --log[=file]       copy all output to a log file (default recvx.log), crash backtraces included\n"
        "  --profile[=file]   per-frame timing CSV (default recvx_profile.csv): logic, vu0, vu1, gs,\n"
        "                     interp, present, wait; also shown live in the F1 overlay\n"
        "  --profile-samples=file  sampling CPU profiler (summarise with pc/tools/profile.py)\n"
        "F1 opens the settings overlay; settings are saved in <save dir>/recvx.ini (options set there,\n"
        "e.g. skip_doors = 0 or skip_intro = 0, override the defaults).\n"
        "The image can also be given with the RECVX_ISO environment variable. Without one, a file\n"
        "selector asks for it on the first start and recvx.ini remembers it (key iso).\n");
}

static char log_path[1024], profile_path[1024];
int g_launcher = 1;      /* ini launcher: the launcher window at start (vr120) */
int g_launcher_lang = 0; /* ini launcher_lang: the launcher's language - 0 English, 1 Portuguese, 2 Spanish (vr134) */
static int no_launcher;  /* --no-launcher */
static int iso_given;   /* image from the command line / RECVX_ISO: no file selector */
static int save_given;  /* --save-dir */
static char start_dir[1024];   /* where the program was started: relative image paths are taken from here */

static void iso_from(const char* s)
{
    int absolute = s[0] == '/' || s[0] == '\\' || (s[0] && s[1] == ':');
    if (absolute || !start_dir[0]) SDL_strlcpy(g_config.iso_path, s, sizeof(g_config.iso_path));
    else SDL_snprintf(g_config.iso_path, sizeof(g_config.iso_path), "%s%s", start_dir, s);
    iso_given = 1;
}

/* Started from somewhere else (a file manager, a shortcut, another directory): the program's own
   folder holds save/, mods/ and the logs, so the release binaries need no start script. A save
   folder in the current directory or an explicit --save-dir keeps the current directory (several
   instances with their own folders, test setups). */
static void home_dir(void)
{
    const char* base;
#ifdef __ANDROID__
    if (save_given) { pc_chdir(g_config.save_dir); return; }   /* Android: the app's folder (RecvxActivity) holds everything - textures/, the logs */
#endif
    if (save_given || SDL_GetPathInfo("save", NULL)) return;
    if (!(base = SDL_GetBasePath())) return;
    pc_chdir(base);
}

static void parse_args(int argc, char** argv)
{
    const char* env = SDL_getenv("RECVX_ISO");   /* UTF-8 on Windows too */
    if (env) iso_from(env);
    SDL_strlcpy(g_config.save_dir, "save", sizeof(g_config.save_dir));
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--save-dir") && i + 1 < argc) { SDL_strlcpy(g_config.save_dir, argv[++i], sizeof(g_config.save_dir)); save_given = 1; }
        else if (!strcmp(argv[i], "--size") && i + 1 < argc) sscanf(argv[++i], "%dx%d", &g_config.window_w, &g_config.window_h);
        else if (!strcmp(argv[i], "--fullscreen")) g_config.fullscreen = 1;
        else if (!strcmp(argv[i], "--enhanced")) g_config.interpolate = 1;
        else if (!strcmp(argv[i], "--vanilla")) g_config.interpolate = 0;
        else if (!strcmp(argv[i], "--skip-doors")) g_config.skip_doors = 1;
        else if (!strcmp(argv[i], "--skip-intro")) g_config.skip_intro = 1;
        else if (!strcmp(argv[i], "--third-person")) g_tps.on = 1;
        else if (!strcmp(argv[i], "--vr")) g_vr_request = 1;
        else if (!strcmp(argv[i], "--no-vr")) g_vr_request = 0;
        else if (!strcmp(argv[i], "--no-launcher")) no_launcher = 1;   /* vr120 */
        else if (!strcmp(argv[i], "--quiet")) g_config.log_level = 0;
        else if (!strcmp(argv[i], "--coop-host") && i + 1 < argc) { g_coop.port = atoi(argv[++i]); g_coop.request = COOP_REQ_HOST; }
        else if (!strcmp(argv[i], "--coop-join") && i + 1 < argc) { SDL_strlcpy(g_coop.join, argv[++i], sizeof(g_coop.join)); g_coop.request = COOP_REQ_JOIN; }
        else if (!strcmp(argv[i], "--coop-server") && i + 1 < argc) SDL_strlcpy(g_coop.server, argv[++i], sizeof(g_coop.server));
        else if (!strcmp(argv[i], "--coop-pw") && i + 1 < argc) SDL_strlcpy(g_coop.password, argv[++i], sizeof(g_coop.password));
        else if (!strcmp(argv[i], "--coop-conn") && i + 1 < argc) { i++; g_coop.conn = !strcmp(argv[i], "direct") ? COOP_CONN_DIRECT : !strcmp(argv[i], "server") ? COOP_CONN_SERVER : COOP_CONN_AUTO; }
        else if (!strncmp(argv[i], "--log", 5) && (argv[i][5] == 0 || argv[i][5] == '='))
            SDL_strlcpy(log_path, argv[i][5] ? argv[i] + 6 : "recvx.log", sizeof(log_path));
        else if (!strncmp(argv[i], "--profile-samples=", 18)) SDL_setenv_unsafe("RECVX_PROFILE", argv[i] + 18, 1);
        else if (!strncmp(argv[i], "--profile", 9) && (argv[i][9] == 0 || argv[i][9] == '='))
            SDL_strlcpy(profile_path, argv[i][9] ? argv[i] + 10 : "recvx_profile.csv", sizeof(profile_path));
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) { usage(); exit(0); }
        else if (!strcmp(argv[i], "--version")) { printf("recvx-enhanced-recomp " RECVX_VERSION "\n"); exit(0); }
        else if (argv[i][0] != '-') iso_from(argv[i]);
        else { usage(); exit(1); }
    }
}

/* First start (or the remembered image is gone): ask for the disc image, remember it in recvx.ini. */
static void SDLCALL iso_picked(void* done, const char* const* files, int filter)
{
    (void)filter;
    if (!files) host_log("file selector: %s\n", SDL_GetError());
    else if (files[0]) SDL_strlcpy(g_config.iso_path, files[0], sizeof(g_config.iso_path));
    SDL_SetAtomicInt(done, 1);
}

static void pick_iso(void)
{
    static const SDL_DialogFileFilter filters[] = { { "PS2 disc image (*.iso)", "iso" }, { "All files", "*" } };
    static SDL_AtomicInt done;
    SDL_PropertiesID props = SDL_CreateProperties();
    g_config.iso_path[0] = 0;
    SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, (void*)filters);
    SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, SDL_arraysize(filters));
    SDL_SetStringProperty(props, SDL_PROP_FILE_DIALOG_TITLE_STRING, "Select the Resident Evil Code: Veronica X disc image (SLUS-20184)");
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, iso_picked, &done, props);
    SDL_DestroyProperties(props);
    while (!SDL_GetAtomicInt(&done)) { SDL_Event ev; SDL_WaitEventTimeout(&ev, 50); }
}

#ifdef __ANDROID__
/* Quest: the disc image is chosen inside the headset launcher (overlay.cpp overlay_launcher_xr): the system's file selector
   in the background, the launcher polls for the answer */
static int disc_ok;
static SDL_AtomicInt disc_pick_done;
static char disc_pick_path[1024];
static void disc_prepare(void)
{
    extern void fontsz_from_iso(void);
    extern void plates_from_iso(void);
    fontsz_from_iso();
    plates_from_iso();
}
int host_disc_ok(void) { return disc_ok; }
static void SDLCALL disc_cb(void* ud, const char* const* files, int filter)
{
    (void)ud; (void)filter;
    disc_pick_path[0] = 0;
    if (!files) host_log("file selector: %s\n", SDL_GetError());
    else if (files[0]) SDL_strlcpy(disc_pick_path, files[0], sizeof(disc_pick_path));
    SDL_SetAtomicInt(&disc_pick_done, 1);
}
void host_disc_pick_start(void)
{
    static const SDL_DialogFileFilter filters[] = { { "PS2 disc image (*.iso)", "iso" }, { "All files", "*" } };
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetAtomicInt(&disc_pick_done, 0);
    SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, (void*)filters);
    SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, SDL_arraysize(filters));
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, disc_cb, NULL, props);
    SDL_DestroyProperties(props);
}
/* a disc image from the launcher's own file browser: 1 open (and remembered), -2 would not open (the old one stays) */
int host_disc_set(const char* path)
{
    char old[1024];
    SDL_strlcpy(old, g_config.iso_path, sizeof(old));
    if (!cdvd_open_image(path)) {
        host_log("launcher: cannot open the chosen disc image '%s'\n", path);
        if (old[0]) cdvd_open_image(old);
        return -2;
    }
    SDL_strlcpy(g_config.iso_path, path, sizeof(g_config.iso_path));
    config_save();
    disc_ok = 1;
    disc_prepare();
    return 1;
}
/* 0 still choosing, 1 a new disc is open, -1 nothing chosen, -2 the file would not open (the old disc stays) */
int host_disc_pick_poll(void)
{
    char old[1024];
    if (!SDL_GetAtomicInt(&disc_pick_done)) return 0;
    SDL_SetAtomicInt(&disc_pick_done, 0);
    if (!disc_pick_path[0]) return -1;
    SDL_strlcpy(old, g_config.iso_path, sizeof(old));
    if (!cdvd_open_image(disc_pick_path)) {
        host_log("launcher: cannot open the chosen disc image '%s'\n", disc_pick_path);
        if (old[0]) cdvd_open_image(old);
        return -2;
    }
    SDL_strlcpy(g_config.iso_path, disc_pick_path, sizeof(g_config.iso_path));
    config_save();
    disc_ok = 1;
    disc_prepare();
    return 1;
}
#endif

/* The game (the decomp's main loop, VU, GIF recording) runs on its own thread; the main
   thread renders (host/render.c). */
static int SDLCALL game_thread(void* ud)
{
    char* game_argv[] = { "cdrom0:\\SLUS_201.84;1", NULL };
    (void)ud;
    prof_thread(PROF_LOGIC);
    { extern void profile_thread(char); profile_thread('g'); }   /* RECVX_PROFILE */
#if defined(__ANDROID__) && defined(RECVX_HAVE_OPENXR)
    { extern void xr_android_game_thread(void); xr_android_game_thread(); }   /* Android: its role for the runtime (xr.c) */
#endif
#ifdef __ANDROID__
    { void and_sampler_thread(int); and_sampler_thread(1); }   /* android_sampler.c */
#endif
    exit(recvx_game_main(1, game_argv));
}

#ifdef _WIN32
__declspec(dllimport) void* __stdcall GetCurrentProcess(void);
__declspec(dllimport) int __stdcall TerminateProcess(void* process, unsigned code);
__declspec(dllimport) void* __stdcall GetModuleHandleW(const unsigned short* name);
__declspec(dllimport) void* __stdcall LoadImageW(void* inst, const unsigned short* name, unsigned type, int cx, int cy, unsigned flags);
__declspec(dllimport) intptr_t __stdcall SendMessageW(void* wnd, unsigned msg, uintptr_t wp, intptr_t lp);
__declspec(dllimport) int __stdcall GetSystemMetrics(int index);
#endif
/* vr144: registered first, so it runs last of the atexit handlers - after ours (the watch's summary, co-op goodbye,
   port mappings, settings) the process ends at once: the C runtime's own teardown (unloading the OpenXR runtime
   and the GL driver with the game thread still running) could hang, the window frozen until it was killed */
static void end_process(void)
{
    host_log("exit: our cleanup done, ending the process\n");
    fflush(stdout); fflush(stderr);
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), 0);
#endif
#ifdef __ANDROID__
    _exit(0);   /* Android (Quest): as on Windows - the log showed abort (signal 6) right after this line, in the teardown */
#endif
}

int main(int argc, char** argv)
{
    atexit(end_process);   /* vr144 */
    setvbuf(stdout, NULL, _IONBF, 0); /* the game's printf diagnostics */
    ps2_memory_init();   /* first: a DLL loaded later (WinHTTP's, the GPU driver's) moves aside, one loaded before can't */
    argv = pc_utf8_argv(argc, argv);   /* paths are UTF-8 everywhere (fileio.h) */
    { char* cwd = SDL_GetCurrentDirectory(); if (cwd) { SDL_strlcpy(start_dir, cwd, sizeof(start_dir)); SDL_free(cwd); } }
    parse_args(argc, argv);
    home_dir();
    if (log_path[0]) { extern void log_start(const char*); log_start(log_path); }
#ifdef __ANDROID__
    {   /* Android (Quest): test switches without a new APK - env.txt in the app's folder (adb push), one NAME=value a
           line (# comments), set as environment variables before anything reads them (RECVX_VU_CHECK, RECVX_NATIVE3D...) */
        FILE* ef = fopen("env.txt", "r");
        char line[256];
        if (ef) {
            while (fgets(line, sizeof(line), ef)) {
                char* eq = strchr(line, '=');
                size_t n = strcspn(line, "\r\n");
                line[n] = 0;
                if (line[0] == '#' || !eq) continue;
                *eq = 0;
                SDL_setenv_unsafe(line, eq + 1, 1);
                setenv(line, eq + 1, 1);   /* (getenv users too) */
                host_log("android: env.txt: %s=%s\n", line, eq + 1);
            }
            fclose(ef);
        }
    }
#endif
    host_log("recvx-enhanced-recomp " RECVX_VERSION "\n");
#ifdef __ANDROID__
    host_log("android: built " __DATE__ " " __TIME__ " (the version above is from the last cmake configure)\n");
#endif
    if (profile_path[0]) { extern void prof_open_csv(const char*); prof_open_csv(profile_path); }
#ifndef _WIN32
    if (getenv("RECVX_WATCHDOG")) { signal(SIGALRM, watchdog); alarm((unsigned)atoi(getenv("RECVX_WATCHDOG"))); }
#endif
    { extern void profile_start(void); profile_start(); }   /* --profile-samples (host/profile.c) */
    config_load();
    parse_args(argc, argv);   /* command line overrides the saved settings */

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
        host_fatal("SDL_Init: %s", SDL_GetError());
    bind_apply_source();   /* Gamepad only: gamepads also in the background */
    { extern void update_check(void); update_check(); }   /* VR: a newer release on GitHub (host/update.c) */

#ifdef __ANDROID__
    disc_ok = g_config.iso_path[0] && cdvd_open_image(g_config.iso_path);   /* (the launcher asks for it when it is missing) */
    if (disc_ok) disc_prepare();
#else
    if (!iso_given && (!g_config.iso_path[0] || !cdvd_open_image(g_config.iso_path))) {
        pick_iso();
        if (!g_config.iso_path[0]) { usage(); host_fatal("no game disc image selected (or start with: recvx <image.iso>)"); }
        if (!cdvd_open_image(g_config.iso_path)) host_fatal("cannot open game disc image '%s'", g_config.iso_path);
        config_save();
    } else {
        if (!g_config.iso_path[0]) { usage(); host_fatal("no game disc image given"); }
        if (!cdvd_open_image(g_config.iso_path)) host_fatal("cannot open game disc image '%s'", g_config.iso_path);
    }
    { extern void fontsz_from_iso(void); fontsz_from_iso(); }
    { extern void plates_from_iso(void); plates_from_iso(); }   /* vr151: the title's plate texture (glue/font_glue.c) */   /* vr138: translations' letter widths (glue/font_glue.c) */
#endif

#ifdef __ANDROID__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);   /* Android (Quest): OpenGL ES 3.2 - the shaders are converted (gs_gl.c compile) */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    if (SDL_getenv("RECVX_GL_DEBUG")) SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);   /* (gs_gl.c gl_debug_cb) */
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    g_window = SDL_CreateWindow("Resident Evil Code: Veronica X", g_config.window_w, g_config.window_h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (g_config.fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    if (!g_window) host_fatal("SDL_CreateWindow: %s", SDL_GetError());
#ifdef _WIN32
    {   /* vr145: the exe's icon (res/recvx.rc, id 1) on the window too - big for Alt+Tab, small for the title bar */
        void* hwnd = SDL_GetPointerProperty(SDL_GetWindowProperties(g_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        void* inst = GetModuleHandleW(NULL);
        void* big = LoadImageW(inst, (const unsigned short*)(uintptr_t)1, 1 /* IMAGE_ICON */, GetSystemMetrics(11), GetSystemMetrics(12), 0);
        void* small = LoadImageW(inst, (const unsigned short*)(uintptr_t)1, 1, GetSystemMetrics(49), GetSystemMetrics(50), 0);
        if (hwnd && big) SendMessageW(hwnd, 0x80 /* WM_SETICON */, 1, (intptr_t)big);
        if (hwnd && small) SendMessageW(hwnd, 0x80, 0, (intptr_t)small);
    }
#endif
    g_gl = SDL_GL_CreateContext(g_window);
    if (!g_gl) host_fatal("SDL_GL_CreateContext: %s", SDL_GetError());
    SDL_GL_SetSwapInterval(g_config.vsync ? 1 : 0);
    gs_renderer_init();
    overlay_init(g_window, g_gl);   /* (before the launcher; it was after xr_init) */
#ifndef __ANDROID__
    if (g_launcher && !no_launcher) {   /* vr120: the launcher - VR or desktop, the VR and graphics settings (overlay.cpp) */
        extern int overlay_launcher(void);
        if (!overlay_launcher()) exit(0);
    }
#endif
    if (xr_init() && g_vr_stereo) {   /* --vr: after the GL entry points are loaded */
        g_tps.on = 1;              /* stereo needs the free camera: the third-person hook hands it to the head */
        g_config.interpolate = 1;  /* replays of the executed frame (eyes, cinema in-betweens) */
    }
#ifdef __ANDROID__
    if (!no_launcher && !SDL_getenv("RECVX_NO_LAUNCHER")) { extern int overlay_launcher_xr(void); overlay_launcher_xr(); }   /* in the headset, before the game starts */
    if (!disc_ok) host_fatal("no game disc image selected");
#endif
    audio_init();

    stream_init();
    if (!SDL_CreateThread(game_thread, "game", NULL)) host_fatal("cannot start the game thread: %s", SDL_GetError());
    render_loop();   /* the main thread renders and presents; never returns */
    return 0;
}
