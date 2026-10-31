/*
 * Android (Quest): loads the game, librecvx.so, inside the PS2 address window (PS2_WINDOW, 0x40000000; see
 * docs/ARCHITECTURE.md "Data model and memory"). In an app process 0x30000000 (the PC's window) is the Java heap;
 * 0x40000000..0x50000000 is free when the app starts, so the window is reserved first thing and the game library
 * is loaded into it with android_dlopen_ext (ANDROID_DLEXT_RESERVED_ADDRESS). The activity (RecvxActivity) loads
 * this library first; its JNI_OnLoad does the above and hands the VM to the game's own JNI_OnLoad (SDL's), so the
 * rest runs as any SDL app: SDLActivity runs SDL_main from librecvx.so, found already loaded at the window.
 */
#include <android/dlext.h>
#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>

#define WINDOW 0x40000000u
#define WINDOW_SIZE 0x10000000u
#define SPR_SIZE_ANDROID 0x4000u
/* where the scratchpad may go (PS2_SPR_BASE, ps2_platform.h): all of the 0x7xxxxxxx kind (the EE's aliases stay identities), above the boot image */
static const unsigned int spr_list[] = { 0x7F000000u, 0x7E000000u, 0x7D000000u, 0x7C000000u, 0x7B000000u, 0x7A000000u, 0x79000000u, 0x78000000u };

/* why the game cannot run: logcat, and a file RecvxErrorActivity shows to the player */
static void fail(const char* fmt, ...)
{
    char msg[512];
    FILE* f;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    __android_log_print(ANDROID_LOG_ERROR, "recvx", "%s", msg);
    f = fopen("/data/data/org.recvx.vr/files/start_error.txt", "w");
    if (f) { fputs(msg, f); fclose(f); }
}
#define L(...) __android_log_print(ANDROID_LOG_INFO, "recvx", __VA_ARGS__)
#define E(...) __android_log_print(ANDROID_LOG_ERROR, "recvx", __VA_ARGS__)

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    char path[512];
    Dl_info di;
    void* win;
    void* game;
    jint (*game_onload)(JavaVM*, void*);
    android_dlextinfo ext;
    char* slash;
    unsigned int spr_addr = 0, *spr_var;

    win = mmap((void*)WINDOW, WINDOW_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (win != (void*)WINDOW) {
        fail("loader: the PS2 address window 0x%08X is taken (mmap gave %p) - the game cannot run", WINDOW, win);
        if (win != MAP_FAILED) munmap(win, WINDOW_SIZE);
        return JNI_ERR;
    }
    for (size_t i = 0; i < sizeof(spr_list) / sizeof(spr_list[0]) && !spr_addr; i++) {   /* the scratchpad too, before anything else can land there; ps2_memory_init maps over this reservation */
        void* spr = mmap((void*)spr_list[i], SPR_SIZE_ANDROID, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (spr == (void*)spr_list[i]) spr_addr = spr_list[i];
        else {
            L("loader: the scratchpad address 0x%08X is taken, trying the next", spr_list[i]);
            if (spr != MAP_FAILED) munmap(spr, SPR_SIZE_ANDROID);
        }
    }
    if (!spr_addr) { fail("loader: no free address for the scratchpad (0x%08X to 0x%08X taken)", spr_list[sizeof(spr_list) / sizeof(spr_list[0]) - 1], spr_list[0]); return JNI_ERR; }
    if (!dladdr((void*)JNI_OnLoad, &di) || !di.dli_fname) { fail("loader: cannot find its own path"); return JNI_ERR; }
    snprintf(path, sizeof(path), "%s", di.dli_fname);
    slash = strrchr(path, '/');
    snprintf(slash ? slash + 1 : path, sizeof(path) - (slash ? (size_t)(slash + 1 - path) : 0), "librecvx.so");

    memset(&ext, 0, sizeof(ext));
    ext.flags = ANDROID_DLEXT_RESERVED_ADDRESS;
    ext.reserved_addr = win;
    ext.reserved_size = WINDOW_SIZE;
    game = android_dlopen_ext(path, RTLD_NOW | RTLD_GLOBAL, &ext);
    if (!game) { fail("loader: %s: %s", path, dlerror()); return JNI_ERR; }
    if (dladdr(dlsym(game, "SDL_main"), &di)) L("loader: %s loaded at %p (window 0x%08X)", path, di.dli_fbase, WINDOW);

    spr_var = (unsigned int*)dlsym(game, "g_ps2_spr_base");
    if (!spr_var) { fail("loader: the game has no g_ps2_spr_base"); return JNI_ERR; }
    *spr_var = spr_addr;
    L("loader: scratchpad at 0x%08X", spr_addr);

    game_onload = (jint (*)(JavaVM*, void*))dlsym(game, "JNI_OnLoad");
    if (!game_onload) { fail("loader: the game has no JNI_OnLoad (SDL)"); return JNI_ERR; }
    return game_onload(vm, reserved);
}
