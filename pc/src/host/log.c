/*
 * --log[=file]: everything written to stdout/stderr (the game's own printf diagnostics and
 * the port's messages) is copied to a log file, each line prefixed with the time since start.
 * A fatal signal (segfault, abort) appends a backtrace to the log before the process dies.
 */
#ifdef _WIN32
#define GetThreadId win_GetThreadId   /* the PS2 kernel shim declares its own */
#define CreateThread win_CreateThread
#include <windows.h>
#include <io.h>
#undef GetThreadId
#undef CreateThread
#endif
#include "../ps2_platform.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <execinfo.h>
#if defined(__ANDROID__) && __ANDROID_API__ < 33   /* bionic's backtrace() is API 33+: no backtrace before */
static inline int backtrace(void** f, int n) { (void)f; (void)n; return 0; }
static inline void backtrace_symbols_fd(void* const* f, int n, int fd) { (void)f; (void)n; (void)fd; }
#endif
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

static int log_fd = -1, console_fd = -1, pipe_rd = -1;
static uint64_t t0;

static void write_all(int fd, const char* p, size_t n)
{
    while (n) { ssize_t k = write(fd, p, n); if (k <= 0) return; p += k; n -= (size_t)k; }
}

static int SDLCALL tee_thread(void* ud)
{
    char buf[4096], stamp[32];
    int at_line_start = 1;
    (void)ud;
    for (;;) {
        ssize_t n = read(pipe_rd, buf, sizeof(buf));
        if (n <= 0) break;
        write_all(console_fd, buf, (size_t)n);
        for (ssize_t i = 0; i < n;) {
            ssize_t j = i;
            if (at_line_start) {
                double t = (SDL_GetTicksNS() - t0) / 1e9;
                int len = SDL_snprintf(stamp, sizeof(stamp), "[%9.3f] ", t);
                write_all(log_fd, stamp, (size_t)len);
                at_line_start = 0;
            }
            while (j < n && buf[j] != '\n') j++;
            if (j < n) { j++; at_line_start = 1; }
            write_all(log_fd, buf + i, (size_t)(j - i));
            i = j;
        }
    }
    return 0;
}

static void on_fatal(int sig)
{
    void* frames[64];
    int n = backtrace(frames, 64);
    char msg[96];
    int len = SDL_snprintf(msg, sizeof(msg), "\nfatal signal %d (%s), backtrace:\n", sig, sig == SIGSEGV ? "segmentation fault" : sig == SIGABRT ? "abort" : "signal");
    write_all(console_fd, msg, (size_t)len);
    backtrace_symbols_fd(frames, n, console_fd);
    if (log_fd >= 0) { write_all(log_fd, msg, (size_t)len); backtrace_symbols_fd(frames, n, log_fd); fsync(log_fd); }
    signal(sig, SIG_DFL);
    raise(sig);
}

#if defined(__ANDROID__) && defined(__arm__)
#include <ucontext.h>
/* Android (Quest): no backtrace() before API 33 - the registers and, as a stand-in backtrace, the words on the stack
   that point into the game's code (librecvx.so is loaded at PS2_WINDOW: an address minus 0x40000000 is its offset
   for addr2line on the unstripped library) */
static void on_fatal_info(int sig, siginfo_t* si, void* ctx)
{
    ucontext_t* uc = ctx;
    const struct sigcontext* m = &uc->uc_mcontext;
    char msg[1024];
    int len = SDL_snprintf(msg, sizeof(msg), "\nfatal signal %d (%s) at %p: pc %08lx lr %08lx sp %08lx r0 %08lx r1 %08lx r2 %08lx r3 %08lx\nstack words in the game's code (offset in librecvx.so):",
                           sig, sig == SIGSEGV ? "segmentation fault" : sig == SIGABRT ? "abort" : "signal", si->si_addr,
                           m->arm_pc, m->arm_lr, m->arm_sp, m->arm_r0, m->arm_r1, m->arm_r2, m->arm_r3);
    const unsigned long* sp = (const unsigned long*)m->arm_sp;
    int found = 0;
    for (int i = 0; i < 512 && found < 24 && len < (int)sizeof(msg) - 16; i++) {
        unsigned long w = sp[i];
        if (w >= 0x40000000ul && w < 0x40600000ul) { len += SDL_snprintf(msg + len, sizeof(msg) - len, " %lx", w - 0x40000000ul); found++; }
    }
    len += SDL_snprintf(msg + len, sizeof(msg) - len, "\n");
    write_all(console_fd, msg, (size_t)len);
    if (log_fd >= 0) { write_all(log_fd, msg, (size_t)len); fsync(log_fd); }
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

/* exit(): what the game printed just before is still in the pipe (the tee thread dies
   with the process) - copy it now and say that the process exited */
static void on_exit_drain(void)
{
    char buf[4096];
    ssize_t n;
    static const char msg[] = "\nprocess exiting (exit() called)\n";
    fflush(stdout); fflush(stderr);
    if (pipe_rd < 0 || log_fd < 0) return;
    fcntl(pipe_rd, F_SETFL, fcntl(pipe_rd, F_GETFL) | O_NONBLOCK);
    while ((n = read(pipe_rd, buf, sizeof(buf))) > 0) { write_all(log_fd, buf, (size_t)n); write_all(console_fd, buf, (size_t)n); }
    write_all(log_fd, msg, sizeof(msg) - 1);
    fsync(log_fd);
}

void log_start(const char* path)
{
    int p[2];
    t0 = SDL_GetTicksNS();
    log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log_fd < 0) { fprintf(stderr, "log: cannot write %s\n", path); return; }
    if (pipe(p) != 0) { close(log_fd); log_fd = -1; return; }
    fflush(stdout); fflush(stderr);
    console_fd = dup(2);
    pipe_rd = p[0];
    dup2(p[1], 1);
    dup2(p[1], 2);
    close(p[1]);
    SDL_DetachThread(SDL_CreateThread(tee_thread, "log", NULL));
    {   /* an own stack for the handler, so stack overflows are reported too */
        static char altstack[64 * 1024];
        stack_t ss = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
        struct sigaction sa;
        static const int sigs[] = { SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL, SIGTRAP };
        sigaltstack(&ss, NULL);
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = on_fatal;
        sa.sa_flags = SA_ONSTACK;
#if defined(__ANDROID__) && defined(__arm__)
        sa.sa_sigaction = on_fatal_info;
        sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
#endif
        for (unsigned i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) sigaction(sigs[i], &sa, NULL);
    }
    atexit(on_exit_drain);
    host_log("log: writing %s\n", path);
}
#else
/* fixed image base (no ASLR): addresses map to functions with i686-w64-mingw32-addr2line -f -e recvx.exe;
   the Windows build keeps frame pointers (pc/CMakeLists.txt), so EBP is the frame chain */
static LONG WINAPI on_fatal(EXCEPTION_POINTERS* ep)
{
    const CONTEXT* c = ep->ContextRecord;
    uint32_t* fp = (uint32_t*)c->Ebp;
    fprintf(stderr, "\nfatal exception 0x%08lX at 0x%08lX (esp 0x%08lX), frame chain:",
            ep->ExceptionRecord->ExceptionCode, (unsigned long)c->Eip, (unsigned long)c->Esp);
    for (int i = 0; i < 32 && fp && !IsBadReadPtr(fp, 8); i++, fp = (uint32_t*)fp[0]) fprintf(stderr, " 0x%08X", fp[1]);
    fprintf(stderr, "\n");
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

void log_start(const char* path)
{
    wchar_t w[1024];   /* the path is UTF-8 (fileio.h) */
    /* vr128: a window program (no console) has no stderr to duplicate onto: both streams opened on the file, appending */
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, w, 1024) || !_wfreopen(w, L"w", stderr) || !_wfreopen(w, L"a", stderr)) return;
    _wfreopen(w, L"a", stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);   /* (buffered, the last lines were lost when the game was closed) */
    SetUnhandledExceptionFilter(on_fatal);
    host_log("log: writing %s\n", path);
}
#endif
