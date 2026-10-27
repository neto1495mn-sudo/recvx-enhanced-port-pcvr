/* fileio.h: UTF-8 paths, saving by replace-on-rename. */
#include "fileio.h"
#include <stdlib.h>
#ifdef __ANDROID__
#include <string.h>
#include <SDL3/SDL.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
static wchar_t* wide(const char* s)
{
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    wchar_t* w = n > 0 ? malloc(n * sizeof(wchar_t)) : NULL;
    if (w && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, w, n)) { free(w); w = NULL; }
    return w;
}
#endif

int pc_chdir(const char* path)
{
#ifdef _WIN32
    wchar_t* w = wide(path);
    int ok = w && SetCurrentDirectoryW(w);
    free(w);
    return ok;
#else
    return chdir(path) == 0;
#endif
}

FILE* pc_fopen(const char* path, const char* mode)
{
#ifdef _WIN32
    wchar_t *wp = wide(path), *wm = wide(mode);
    FILE* f = wp && wm ? _wfopen(wp, wm) : NULL;
    free(wp);
    free(wm);
    return f;
#else
#ifdef __ANDROID__
    if (!strncmp(path, "content://", 10)) {   /* Android: a file picked in the system's file selector (the disc image anywhere) -
                                                 SDL opens it through the content resolver; its stdio stream is kept */
        SDL_IOStream* io = SDL_IOFromFile(path, mode);
        FILE* f = io ? (FILE*)SDL_GetPointerProperty(SDL_GetIOProperties(io), SDL_PROP_IOSTREAM_STDIO_FILE_POINTER, NULL) : NULL;
        if (io && !f) SDL_CloseIO(io);
        return f;   /* (the stream wrapper is left open: closing the FILE ends it) */
    }
#endif
    return fopen(path, mode);
#endif
}

static int replace(const char* from, const char* to)
{
#ifdef _WIN32
    wchar_t *wf = wide(from), *wt = wide(to);
    int ok = wf && wt && MoveFileExW(wf, wt, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    free(wf);
    free(wt);
    return ok ? 0 : -1;
#else
    return rename(from, to);
#endif
}

static void delete_file(const char* path)
{
#ifdef _WIN32
    wchar_t* w = wide(path);
    if (w) _wremove(w);
    free(w);
#else
    remove(path);
#endif
}

int pc_save_file(const char* path, const char* mode, int (*fill)(FILE*, void*), void* ud)
{
    char tmp[4096];
    FILE* f;
    int ok;
    /* per process: two instances sharing a save directory (co-op on one PC) never share one */
#ifdef _WIN32
    snprintf(tmp, sizeof(tmp), "%s.%d.tmp", path, _getpid());
#else
    snprintf(tmp, sizeof(tmp), "%s.%d.tmp", path, (int)getpid());
#endif
    if (!(f = pc_fopen(tmp, mode))) return -1;
    ok = fill(f, ud) == 0 && !ferror(f) && fflush(f) == 0;
#ifdef _WIN32
    ok = ok && _commit(_fileno(f)) == 0;
#else
    ok = ok && fsync(fileno(f)) == 0;
#endif
    ok = fclose(f) == 0 && ok;
    if (ok && replace(tmp, path) == 0) return 0;
    delete_file(tmp);
    return -1;
}

char** pc_utf8_argv(int argc, char** argv)
{
#ifdef _WIN32
    /* the CRT's argv is in the ANSI code page: rebuild it from the UTF-16 command line */
    int n;
    wchar_t** w = CommandLineToArgvW(GetCommandLineW(), &n);
    char** out = w && n == argc ? calloc((size_t)n + 1, sizeof(char*)) : NULL;
    for (int i = 0; out && i < n; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, w[i], -1, NULL, 0, NULL, NULL);
        if (len <= 0 || !(out[i] = malloc((size_t)len))) { out = NULL; break; }   /* startup only: a leak at most */
        WideCharToMultiByte(CP_UTF8, 0, w[i], -1, out[i], len, NULL, NULL);
    }
    if (w) LocalFree(w);
    return out ? out : argv;
#else
    (void)argc;
    return argv;
#endif
}
