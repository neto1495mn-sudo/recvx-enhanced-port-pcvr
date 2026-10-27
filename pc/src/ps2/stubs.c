/* Not-yet-implemented services. Each stub states which phase replaces it. */
#include "ps2_platform.h"
#include <string.h>

#define STUB_ONCE(name) do { static int once; if (!once++) host_log("stub: %s\n", name); } while (0)

/* libc symbol the decomp calls under its EE name (see pc_prelude.h) */
char* recvx_index(const char* s, int c) { return strchr(s, c); }

#ifdef _WIN32
/* glibc's rand()/srand() (TYPE_3 additive feedback generator, random_r.c) for the Windows
   build, see pc_prelude.h. Single game thread: no lock. */
int recvx_rand(void);
static int32_t rand_st[31], *rand_f, *rand_b;
void recvx_srand(unsigned seed)
{
    int32_t w = seed ? (int32_t)seed : 1;
    rand_st[0] = w;
    for (int i = 1; i < 31; i++) {   /* 16807 * w % 0x7fffffff without overflow (Schrage) */
        int32_t hi = w / 127773, lo = w % 127773;
        w = 16807 * lo - 2836 * hi;
        if (w < 0) w += 2147483647;
        rand_st[i] = w;
    }
    rand_f = &rand_st[3];
    rand_b = &rand_st[0];
    for (int i = 0; i < 310; i++) recvx_rand();
}
int recvx_rand(void)
{
    if (!rand_f) recvx_srand(1);
    uint32_t v = (uint32_t)*rand_f + (uint32_t)*rand_b;
    *rand_f = (int32_t)v;
    if (++rand_f >= &rand_st[31]) { rand_f = rand_st; ++rand_b; }
    else if (++rand_b >= &rand_st[31]) rand_b = rand_st;
    return (int)(v >> 1);
}
#endif

/* VU microcode DMA packets (ps2_vu0sub0, ...): the interpreter carries its own tables,
   so the uploads only need a valid, empty chain. */
const uint32_t ps2_vu0sub0[4] __attribute__((aligned(16))) = { 0x70000000, 0, 0, 0 };
const uint32_t ps2_vu1sub0[4] __attribute__((aligned(16))) = { 0x70000000, 0, 0, 0 };
const uint32_t ps2_vu1sub1[4] __attribute__((aligned(16))) = { 0x70000000, 0, 0, 0 };

/* ---- libsd / synth (audio phase) ---- */
int sceSdRemoteInit(void) { return 0; }
int sceSSyn_SetOutputMode(int mode) { (void)mode; return 0; }
