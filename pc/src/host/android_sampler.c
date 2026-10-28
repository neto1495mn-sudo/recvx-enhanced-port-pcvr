/* Android (Quest) diagnosis: a sampling profiler that names the functions itself. A timer on each thread's own CPU
   clock (render, game) records the interrupted instruction at 1 kHz of its CPU time; every 20 s the samples are
   looked up in librecvx.so's symbol table (the APK's copy keeps it: build_apk.py strips debug info only) and the
   busiest functions logged per thread, the rest by library (the GL driver, libc). Nothing to install or read off
   the headset but the log. */
#ifdef __ANDROID__
#define _GNU_SOURCE
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <SDL3/SDL.h>
#ifndef sigev_notify_thread_id
#define sigev_notify_thread_id _sigev_un._tid
#endif

void host_log(const char* fmt, ...);

#define NT 2          /* threads: 0 render, 1 game */
#define CAP 65536
static uint32_t* buf[NT];      /* malloc'd: not in the library's data (the PS2 address window) */
static volatile unsigned cnt[NT];
static __thread int my_slot = -1;
static int installed;

static void on_sample(int sig, siginfo_t* si, void* ctx)
{
    ucontext_t* uc = ctx;
    int s = my_slot;
    unsigned i;
    (void)sig; (void)si;
    if (s < 0 || !buf[s] || (i = cnt[s]) >= CAP) return;
    buf[s][i] = (uint32_t)uc->uc_mcontext.arm_pc;
    cnt[s] = i + 1;
}

/* the calling thread: 0 render, 1 game */
void and_sampler_thread(int slot)
{
    struct sigevent ev;
    struct itimerspec it = { { 0, 1000000 }, { 0, 1000000 } };
    timer_t t;
    sigset_t set;
    if (!installed) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = on_sample;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigaction(SIGPROF, &sa, NULL);
        installed = 1;
    }
    if (!buf[slot] && !(buf[slot] = malloc(CAP * sizeof(uint32_t)))) return;
    my_slot = slot;
    sigemptyset(&set);
    sigaddset(&set, SIGPROF);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    memset(&ev, 0, sizeof(ev));
    ev.sigev_notify = SIGEV_THREAD_ID;
    ev.sigev_signo = SIGPROF;
    ev.sigev_notify_thread_id = (int)syscall(SYS_gettid);
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, &ev, &t) == 0 && timer_settime(t, 0, &it, NULL) == 0)
        host_log("android: profiler on for the %s thread\n", slot ? "game" : "render");
    else host_log("android: profiler: no timer for the %s thread\n", slot ? "game" : "render");
}

/* ---- librecvx.so's functions */
typedef struct { uint32_t lo, hi; const char* name; } Sym;
static Sym* syms;
static int nsyms, sym_state;   /* 0 not loaded, 1 loaded, -1 failed */
static char* strtab;
static uint32_t so_lo, so_hi;  /* the library's mapping (load base .. end) */
static char so_path[512];
typedef struct { uint32_t lo, hi; char name[48]; } Map;
static Map maps[512];
static int nmaps;

static int by_lo(const void* a, const void* b) { uint32_t x = ((const Sym*)a)->lo, y = ((const Sym*)b)->lo; return x < y ? -1 : x > y; }

static void read_maps(void)
{
    FILE* f = fopen("/proc/self/maps", "r");
    char line[768];
    nmaps = 0; so_lo = so_hi = 0;
    if (!f) return;
    while (fgets(line, sizeof(line), f) && nmaps < 512) {
        unsigned long lo, hi, off;
        char perm[8], path[600] = "";
        if (sscanf(line, "%lx-%lx %7s %lx %*s %*s %599[^\n]", &lo, &hi, perm, &off, path) < 4) continue;
        if (strstr(path, "librecvx.so")) {
            if (!so_lo || lo < so_lo) so_lo = (uint32_t)lo;
            if (hi > so_hi) so_hi = (uint32_t)hi;
            SDL_strlcpy(so_path, path, sizeof(so_path));
        }
        if (perm[2] != 'x') continue;
        {
            const char* base = strrchr(path, '/');
            maps[nmaps].lo = (uint32_t)lo; maps[nmaps].hi = (uint32_t)hi;
            SDL_strlcpy(maps[nmaps].name, base ? base + 1 : path[0] ? path : "[anon]", sizeof(maps[nmaps].name));
            nmaps++;
        }
    }
    fclose(f);
}

static void load_syms(void)
{
    int fd;
    Elf32_Ehdr eh;
    Elf32_Shdr* sh = NULL;
    sym_state = -1;
    read_maps();
    if (!so_path[0] || (fd = open(so_path, O_RDONLY)) < 0) { host_log("android: profiler: librecvx.so not found in the maps\n"); return; }
    if (pread(fd, &eh, sizeof(eh), 0) != sizeof(eh) || memcmp(eh.e_ident, ELFMAG, SELFMAG) || eh.e_shentsize != sizeof(Elf32_Shdr)) goto out;
    sh = malloc((size_t)eh.e_shnum * sizeof(*sh));
    if (!sh || pread(fd, sh, (size_t)eh.e_shnum * sizeof(*sh), eh.e_shoff) != (ssize_t)(eh.e_shnum * sizeof(*sh))) goto out;
    for (int i = 0; i < eh.e_shnum; i++) {
        Elf32_Sym* st;
        int n, k = 0;
        if (sh[i].sh_type != SHT_SYMTAB || sh[i].sh_link >= eh.e_shnum) continue;
        n = (int)(sh[i].sh_size / sizeof(Elf32_Sym));
        st = malloc(sh[i].sh_size);
        strtab = malloc(sh[sh[i].sh_link].sh_size + 1);
        syms = malloc((size_t)n * sizeof(Sym));
        if (!st || !strtab || !syms) goto out;
        if (pread(fd, st, sh[i].sh_size, sh[i].sh_offset) != (ssize_t)sh[i].sh_size ||
            pread(fd, strtab, sh[sh[i].sh_link].sh_size, sh[sh[i].sh_link].sh_offset) != (ssize_t)sh[sh[i].sh_link].sh_size) { free(st); goto out; }
        strtab[sh[sh[i].sh_link].sh_size] = 0;
        for (int j = 0; j < n; j++)
            if (ELF32_ST_TYPE(st[j].st_info) == STT_FUNC && st[j].st_value && st[j].st_name < sh[sh[i].sh_link].sh_size) {
                syms[k].lo = (st[j].st_value & ~1u) + so_lo;   /* (bit 0: Thumb) */
                syms[k].hi = syms[k].lo + (st[j].st_size ? st[j].st_size : 4);
                syms[k].name = strtab + st[j].st_name;
                k++;
            }
        free(st);
        nsyms = k;
        qsort(syms, (size_t)nsyms, sizeof(Sym), by_lo);
        sym_state = 1;
        host_log("android: profiler: %d functions from %s (loaded at %08x)\n", nsyms, so_path, so_lo);
        break;
    }
    if (sym_state < 0) host_log("android: profiler: librecvx.so has no symbol table - samples by library only\n");
out:
    free(sh);
    close(fd);
}

static const char* name_of(uint32_t pc, int* idx)
{
    *idx = -1;
    if (sym_state > 0 && pc >= so_lo && pc < so_hi) {
        int lo = 0, hi = nsyms - 1, best = -1;
        while (lo <= hi) { int m = (lo + hi) / 2; if (syms[m].lo <= pc) { best = m; lo = m + 1; } else hi = m - 1; }
        if (best >= 0 && pc < syms[best].hi) { *idx = best; return syms[best].name; }
        return "librecvx.so (no symbol)";
    }
    for (int i = 0; i < nmaps; i++)
        if (pc >= maps[i].lo && pc < maps[i].hi) {   /* another library: its exported function if it has one (libc's memcpy...) */
            static char names[256][96];
            static int nn;
            Dl_info di;
            char nm[96];
            if (!dladdr((void*)(uintptr_t)pc, &di) || !di.dli_sname) return maps[i].name;
            snprintf(nm, sizeof(nm), "%s:%s", maps[i].name, di.dli_sname);
            for (int k = 0; k < nn; k++) if (!strcmp(names[k], nm)) return names[k];
            if (nn == 256) return maps[i].name;
            memcpy(names[nn], nm, sizeof(nm));
            return names[nn++];
        }
    return "?";
}

/* the function holding a code address of librecvx.so (NULL: not one of its functions); loads the symbols on first use */
const char* and_symbol(uint32_t pc)
{
    int idx;
    const char* nm;
    if (!sym_state) load_syms();
    if (sym_state <= 0 || pc < so_lo || pc >= so_hi) return NULL;
    nm = name_of(pc, &idx);
    return idx >= 0 ? nm : NULL;
}
/* the game functions on the calling thread's stack (code addresses found in its top words): "a < b < c", the
   innermost first - the callers of a draw (no frame pointers to walk) */
void and_callers(char* out, size_t n, int skip)
{
    uint32_t words[256];
    const char* last = NULL;
    size_t k = 0;
    int found = 0;
    memcpy(words, __builtin_frame_address(0), sizeof(words));
    out[0] = 0;
    for (int i = 0; i < 256 && found < 6 + skip; i++) {
        const char* nm = and_symbol(words[i] & ~1u);
        if (!nm || nm == last) continue;
        last = nm;
        if (found++ < skip) continue;
        k += (size_t)snprintf(out + k, k < n ? n - k : 0, "%s%s", k ? " < " : "", nm);
        if (k >= n) break;
    }
}

typedef struct { const char* name; unsigned n; } Bin;
static int by_n(const void* a, const void* b) { unsigned x = ((const Bin*)a)->n, y = ((const Bin*)b)->n; return x < y ? 1 : x > y ? -1 : 0; }

/* render thread, once a second: every 20 s the profile of the last 20 s */
void and_sampler_report(void)
{
    static uint64_t last;
    static uint32_t* copy;
    static Bin* bins;
    uint64_t now = SDL_GetTicksNS();
    if (!installed) return;
    if (!copy && (!(copy = malloc(CAP * sizeof(uint32_t))) || !(bins = malloc(1024 * sizeof(Bin))))) { installed = 0; return; }
    if (!last) { last = now; return; }
    if (now - last < 20000000000ull) return;
    last = now;
    if (!sym_state) load_syms();
    else read_maps();   /* (libraries loaded since) */
    for (int t = 0; t < NT; t++) {
        unsigned n = cnt[t] < CAP ? cnt[t] : CAP, nb = 0;
        if (!buf[t]) continue;
        char line[1024];
        int k = 0;
        memcpy(copy, buf[t], n * sizeof(uint32_t));
        cnt[t] = 0;
        if (!n) continue;
        for (unsigned i = 0; i < n; i++) {
            int idx;
            const char* nm = name_of(copy[i], &idx);
            unsigned b = 0;
            while (b < nb && bins[b].name != nm) b++;
            if (b == nb) { if (nb == 1024) continue; bins[nb].name = nm; bins[nb].n = 0; nb++; }
            bins[b].n++;
        }
        qsort(bins, nb, sizeof(Bin), by_n);
        host_log("android: profile %s thread, %u ms of CPU in 20 s (%.0f%% of a core):\n", t ? "game" : "render", n, n / 200.0);
        for (unsigned b = 0; b < nb && b < 30; b++) {
            k += snprintf(line + k, sizeof(line) - k, " %4.1f%% %s;", 100.0 * bins[b].n / n, bins[b].name);
            if (b % 5 == 4 || b + 1 == nb || b == 29) { host_log("android:  %s\n", line); k = 0; }
        }
    }
}
#else
typedef int android_sampler_unused;
#endif
