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

#define WINDOW 0x40000000u
#define WINDOW_SIZE 0x10000000u
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

    win = mmap((void*)WINDOW, WINDOW_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (win != (void*)WINDOW) {
        E("loader: the PS2 address window 0x%08X is taken (mmap gave %p) - the game cannot run", WINDOW, win);
        if (win != MAP_FAILED) munmap(win, WINDOW_SIZE);
        return JNI_ERR;
    }
    if (!dladdr((void*)JNI_OnLoad, &di) || !di.dli_fname) { E("loader: cannot find its own path"); return JNI_ERR; }
    snprintf(path, sizeof(path), "%s", di.dli_fname);
    slash = strrchr(path, '/');
    snprintf(slash ? slash + 1 : path, sizeof(path) - (slash ? (size_t)(slash + 1 - path) : 0), "librecvx.so");

    memset(&ext, 0, sizeof(ext));
    ext.flags = ANDROID_DLEXT_RESERVED_ADDRESS;
    ext.reserved_addr = win;
    ext.reserved_size = WINDOW_SIZE;
    game = android_dlopen_ext(path, RTLD_NOW | RTLD_GLOBAL, &ext);
    if (!game) { E("loader: %s: %s", path, dlerror()); return JNI_ERR; }
    if (dladdr(dlsym(game, "SDL_main"), &di)) L("loader: %s loaded at %p (window 0x%08X)", path, di.dli_fbase, WINDOW);

    game_onload = (jint (*)(JavaVM*, void*))dlsym(game, "JNI_OnLoad");
    if (!game_onload) { E("loader: the game has no JNI_OnLoad (SDL)"); return JNI_ERR; }
    return game_onload(vm, reserved);
}
