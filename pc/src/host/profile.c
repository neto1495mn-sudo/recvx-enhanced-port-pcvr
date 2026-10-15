/*
 * RECVX_PROFILE=<file>: sampling profiler for builds without perf. A timer per thread (the
 * main/render thread from profile_start, the game thread from profile_thread) on that
 * thread's own CPU clock records the interrupted instruction pointer at 1 kHz of its CPU
 * time; at exit the raw addresses are written to <file>, one per line with the thread
 * ("r" render, "g" game), for symbolisation with pc/tools/profile.py. (A process-wide
 * ITIMER_PROF signal lands on any thread, mostly ones asleep in a syscall.)
 */
#ifndef _WIN32
#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <ucontext.h>
#ifndef sigev_notify_thread_id
#define sigev_notify_thread_id _sigev_un._tid
#endif

#define MAX_SAMPLES (1 << 20)
static uint32_t* samples;      /* malloc'd only with RECVX_PROFILE: not in the PS2 address window */
static char* sample_thread;
static volatile unsigned nsamples;
static const char* out_path;
static __thread char thread_tag;

static void on_prof(int sig, siginfo_t* si, void* ctx)
{
    ucontext_t* uc = ctx;
    unsigned i;
    (void)sig; (void)si;
    if (!thread_tag || (i = __atomic_fetch_add(&nsamples, 1, __ATOMIC_RELAXED)) >= MAX_SAMPLES) return;
#if defined(__i386__)
    samples[i] = (uint32_t)uc->uc_mcontext.gregs[REG_EIP];
#else
    samples[i] = (uint32_t)uc->uc_mcontext.gregs[REG_RIP];
#endif
    sample_thread[i] = thread_tag;
}

static void dump(void)
{
    FILE* f = fopen(out_path, "w");
    unsigned n_s = nsamples < MAX_SAMPLES ? nsamples : MAX_SAMPLES;
    if (!f) return;
    for (unsigned i = 0; i < n_s; i++) fprintf(f, "%x %c\n", samples[i], sample_thread[i] ? sample_thread[i] : '?');
    fclose(f);
    {   /* <file>.maps: which library a sample outside the executable fell in (profile.py) */
        char path[1024], buf[4096];
        size_t n;
        FILE *in = fopen("/proc/self/maps", "r"), *out;
        snprintf(path, sizeof(path), "%s.maps", out_path);
        out = in ? fopen(path, "w") : NULL;
        while (out && (n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
        if (out) fclose(out);
        if (in) fclose(in);
    }
}

/* sample the calling thread (tag 'r', 'g') */
void profile_thread(char tag)
{
    struct sigevent ev = { 0 };
    struct itimerspec it = { { 0, 1000000 }, { 0, 1000000 } };   /* 1 kHz of this thread's CPU time */
    timer_t t;
    sigset_t set;
    if (!out_path) return;
    thread_tag = tag;
    sigemptyset(&set);
    sigaddset(&set, SIGPROF);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);   /* SDL's threads start with SIGPROF blocked */
    ev.sigev_notify = SIGEV_THREAD_ID;
    ev.sigev_signo = SIGPROF;
    ev.sigev_notify_thread_id = (int)syscall(SYS_gettid);
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &ev, &t) == 0) timer_settime(t, 0, &it, NULL);
}

void profile_start(void)
{
    struct sigaction sa = { 0 };
    out_path = getenv("RECVX_PROFILE");
    if (!out_path) return;
    samples = malloc(MAX_SAMPLES * sizeof(*samples));
    sample_thread = malloc(MAX_SAMPLES);
    if (!samples || !sample_thread) { out_path = NULL; return; }
    sa.sa_sigaction = on_prof;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, NULL);
    profile_thread('r');
    atexit(dump);
}
#else
/* Windows: a sampler thread suspends the game thread every millisecond and records where it
   was (EIP) plus the code addresses found on top of its stack (a rough call chain), tagged with
   the --profile frame number, so the slow frames of the CSV can be looked up. Written as it
   goes (the game may be closed abruptly). Lines: "<frame> <eip> <g|r thread> <stack code addresses...>";
   the first line "base <image base> <text end>". */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
volatile unsigned g_prof_frame;
static HANDLE game_thread, render_thread;
static FILE* out;
static uintptr_t img_lo, img_hi;

static DWORD WINAPI sampler(LPVOID arg)
{
    unsigned n = 0;
    (void)arg;
    timeBeginPeriod(1);
    for (;;) {
        Sleep(1);
        for (int k = 0; k < 2; k++) {
            HANDLE th = k ? render_thread : game_thread;
            CONTEXT c;
            if (!th || SuspendThread(th) == (DWORD)-1) continue;
            c.ContextFlags = CONTEXT_CONTROL;
            if (GetThreadContext(th, &c)) {
                uint32_t sp = (uint32_t)c.Esp, found = 0;
                int deep = !(c.Eip >= img_lo && c.Eip < img_hi);   /* in a system call: look further up for the caller */
                static uint32_t stk[16384];
                MEMORY_BASIC_INFORMATION mi;
                SIZE_T want = deep ? sizeof(stk) : 8192, got = 0;
                fprintf(out, "%u %x %c", g_prof_frame, (unsigned)c.Eip, k ? 'r' : 'g');
                if (VirtualQuery((void*)(uintptr_t)sp, &mi, sizeof(mi))) {   /* (one read, within the stack's region) */
                    uintptr_t end = (uintptr_t)mi.BaseAddress + mi.RegionSize;
                    if (sp + want > end) want = end - sp;
                    if (!ReadProcessMemory(GetCurrentProcess(), (void*)(uintptr_t)sp, stk, want, &got)) got = 0;
                }
                for (SIZE_T i = 0; i < got / 4 && found < (deep ? 48u : 12u); i++)
                    if (stk[i] >= img_lo && stk[i] < img_hi) { fprintf(out, " %x", stk[i]); found++; }
                fputc('\n', out);
            }
            ResumeThread(th);
        }
        if (++n % 500 == 0) fflush(out);
    }
    return 0;
}

void profile_thread(char tag)
{
    if (tag != 'g' || !out) return;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &game_thread,
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, 0);
}

void profile_start(void)
{
    extern const char* SDL_getenv(const char*);
    const char* path = SDL_getenv("RECVX_PROFILE");   /* (set with SDL_setenv_unsafe: not in the CRT's getenv on Windows) */
    IMAGE_DOS_HEADER* dos;
    IMAGE_NT_HEADERS* nt;
    if (!path || !(out = fopen(path, "w"))) return;
    dos = (IMAGE_DOS_HEADER*)GetModuleHandle(NULL);
    nt = (IMAGE_NT_HEADERS*)((char*)dos + dos->e_lfanew);
    img_lo = (uintptr_t)dos + nt->OptionalHeader.BaseOfCode;   /* code only (stack words pointing at data are noise) */
    img_hi = img_lo + nt->OptionalHeader.SizeOfCode;
    fprintf(out, "base %x %x\n", (unsigned)img_lo, (unsigned)img_hi);
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &render_thread,
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, 0);   /* (main thread = render thread) */
    CreateThread(NULL, 0, sampler, NULL, 0, NULL);
}
#endif
