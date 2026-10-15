/*
 * Lightweight frame profiler: categories form a stack, the innermost one is charged
 * (VU1 kicks GS work, GS work inside VU1 counts as GS). Whatever is not inside a pushed
 * category is game logic (the EE code of the decomp plus its platform layer).
 */
#include "prof.h"
#include "fileio.h"
#include <SDL3/SDL.h>
#include <stdio.h>

const char* const prof_names[PROF_COUNT] = { "logic", "vu0", "vu1", "gif", "queue", "wait",
                                             "r_exec", "r_interp", "r_present", "r_idle" };

#define DEPTH 16
#define HISTORY 512
static __thread int stack[DEPTH], sp, base;
static __thread uint64_t last_ns;
static uint64_t acc[PROF_COUNT], frame_start;   /* categories belong to one thread each; the game thread
                                                   takes the render thread's too (atomic) */
static ProfFrame hist[HISTORY];
static unsigned hist_n;
static FILE* csv;
static uint64_t frame_no;

static void charge(void)
{
    uint64_t now = SDL_GetTicksNS();
    if (last_ns) __atomic_fetch_add(&acc[sp ? stack[sp - 1] : base], now - last_ns, __ATOMIC_RELAXED);
    last_ns = now;
}

int prof_enabled;

void prof_thread(int base_cat) { base = base_cat; last_ns = 0; }

/* the stack is kept while measuring is off too: switching it on between a push and its pop
   left the stack skewed */
void prof_push(int cat)
{
    if (prof_enabled) charge(); else last_ns = 0;
    if (sp < DEPTH) stack[sp++] = cat;
}

void prof_pop(void)
{
    if (prof_enabled) charge(); else last_ns = 0;
    if (sp) sp--;
}

void prof_open_csv(const char* path)
{
    csv = pc_fopen(path, "w");
    prof_enabled = 1;
    if (!csv) { fprintf(stderr, "profile: cannot write %s\n", path); return; }
    fprintf(csv, "frame,total_ms");
    for (int i = 0; i < PROF_COUNT; i++) fprintf(csv, ",%s_ms", prof_names[i]);
    fprintf(csv, "\n");
}

void prof_frame_end(void)
{
    ProfFrame f;
    uint64_t now;
    if (!prof_enabled) { frame_start = 0; return; }
    charge();
    now = last_ns;
    if (!frame_start) frame_start = now;
    f.total = (float)((now - frame_start) / 1e6);
    for (int i = 0; i < PROF_COUNT; i++) f.ms[i] = (float)(__atomic_exchange_n(&acc[i], 0, __ATOMIC_RELAXED) / 1e6);
    frame_start = now;
    hist[hist_n++ % HISTORY] = f;
    if (csv) {
        fprintf(csv, "%llu,%.3f", (unsigned long long)frame_no, f.total);
        for (int i = 0; i < PROF_COUNT; i++) fprintf(csv, ",%.3f", f.ms[i]);
        fprintf(csv, "\n");
        if (frame_no % 60 == 0) fflush(csv);
    }
    frame_no++;
    { extern volatile unsigned g_prof_frame; g_prof_frame = (unsigned)frame_no; }   /* (the Windows sampler tags samples with it, profile.c) */
}

int prof_history(ProfFrame* out, int n)
{
    int have = hist_n < HISTORY ? (int)hist_n : HISTORY;
    if (n > have) n = have;
    for (int i = 0; i < n; i++) out[i] = hist[(hist_n - n + i) % HISTORY];
    return n;
}
