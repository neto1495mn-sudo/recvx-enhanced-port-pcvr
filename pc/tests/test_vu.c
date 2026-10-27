/* VU0 core self-check: macro ops and microprograms against hand-computed values, the native
   paths (CALCPOINT, inline macro ops, strip clip) against the interpreter.
   Build/run: ctest (target test_vu). */
#include "../src/ps2/vu0.h"
#include "../src/ps2/vu0_labels.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* vu.c reports to the profiler; the test runs without it */
int prof_enabled;
void prof_push(int cat) { (void)cat; }
void prof_pop(void) {}

static int fails;
#define CHECK(cond, ...) do { if (!(cond)) { fails++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)
#define NEAR(a, b) (fabsf((a) - (b)) <= 1e-4f * (1.0f + fabsf(b)))

void vu1_xgkick(const VuVec* mem, uint32_t qw, uint32_t mask) { (void)mem; (void)qw; (void)mask; }

/* NaN inputs are negative: with two NaN operands of a commutative op (ADD, MUL) the NaN that
   propagates, and so the sign of the clamped result, depends on the operand order the compiler
   picked, in the interpreter too (the PS2 has no NaN; game data has none) */
static uint32_t neg_nan(uint32_t u) { return (u & 0x7F800000u) == 0x7F800000u && (u & 0x7FFFFFu) ? u | 0x80000000u : u; }

/* the interpreter's macro-mode op (vu.c upper_on, as vu_upper was before it was inlined): the
   reference for the inline vu_upper */
static void ref_upper(int op, int mask, int fd, int fs, int ft, int bc)
{
    VuVec vf_in[32], acc_in = g_vu0.acc;
    int wrote_clip;
    uint32_t bits = 0;
    memcpy(vf_in, g_vu0.vf, sizeof(vf_in));
    g_vu0.mac = vu_upper_on(&g_vu0, vf_in, &acc_in, g_vu0.q, op, mask, fd, fs, ft, bc, &wrote_clip, &bits);
    if (wrote_clip) g_vu0.clip = ((g_vu0.clip << 6) | bits) & 0xFFFFFF;
    g_vu0.vf[0].f[0] = g_vu0.vf[0].f[1] = g_vu0.vf[0].f[2] = 0.0f; g_vu0.vf[0].f[3] = 1.0f; g_vu0.vi[0] = 0;
}

int main(void)
{
    putenv((char*)"RECVX_VU_CHECK=1");   /* every microprogram call is also interpreted and compared (read at the first call) */
    /* no reset: the power-on state must already have vf0 = (0,0,0,1) (njInitMatrix is the first VU op) */
    /* njInitMatrix: vaddw.xyz vf1, vf0, vf0w ; vmul.w vf1, vf0, vf0 -> (1,1,1,1) */
    vu_upper(U_ADD, VU_XYZ, 1, 0, 0, BC_W);
    vu_upper(U_MUL, VU_W, 1, 0, 0, BC_NONE);
    for (int c = 0; c < 4; c++) CHECK(g_vu0.vf[1].f[c] == 1.0f, "vf1[%d] = %f, want 1", c, g_vu0.vf[1].f[c]);

    /* broadcast + MADD chain: vf18 = M * (1,2,3,1) with M rows vf28..vf31 */
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) g_vu0.vf[28 + r].f[c] = (float)(r * 4 + c);
    g_vu0.vf[4].f[0] = 1; g_vu0.vf[4].f[1] = 2; g_vu0.vf[4].f[2] = 3; g_vu0.vf[4].f[3] = 1;
    vu_upper(U_MULA, VU_XYZW, 0, 28, 4, BC_X);
    vu_upper(U_MADDA, VU_XYZW, 0, 29, 4, BC_Y);
    vu_upper(U_MADDA, VU_XYZW, 0, 30, 4, BC_Z);
    vu_upper(U_MADD, VU_XYZW, 18, 31, 0, BC_W);
    for (int c = 0; c < 4; c++) {
        float want = 1 * (0 + c) + 2 * (4 + c) + 3 * (8 + c) + (12 + c);
        CHECK(NEAR(g_vu0.vf[18].f[c], want), "vf18[%d] = %f, want %f", c, g_vu0.vf[18].f[c], want);
    }

    /* microprogram: VU0_WAVE_INIT sets vf4 = vf5 = (0,-1,0,0) */
    vu0_callms(VU0_ADDR_VU0_WAVE_INIT);
    CHECK(g_vu0.vf[4].f[1] == -1.0f && g_vu0.vf[5].f[1] == -1.0f && g_vu0.vf[4].f[0] == 0.0f, "WAVE_INIT");

    /* CALCPOINT's native path (vu.c calcpoint_fast) against the interpreter: random matrices,
       vertices and leftover state, every light off, directional or point (in and out of range,
       facing or not), chained as a node's vertices are, with overflow, denormal, zero, -0, Inf
       and NaN inputs; RECVX_VU_CHECK compares the whole VU state after every call */
    {
        extern long vu_check_mismatches(void);
        static const uint32_t odd[] = { 0x00000000u, 0x80000000u, 0x00000001u, 0x807FFFFFu, 0x00800000u, 0x7F7FFFFFu,
                                        0xFF7FFFFFu, 0x7F800000u, 0xFF800000u, 0xFFC00000u, 0xFF800001u };
        uint32_t seed = 12345;
#define RND() ((seed = seed * 1103515245u + 12345u) >> 8 & 0xFFFF)
#define RF(lo, hi) ((float)(lo) + ((float)(hi) - (float)(lo)) * (float)RND() / 65535.0f)
#define ODD() (odd[RND() % (sizeof(odd) / sizeof(odd[0]))])
        extern int vu_avx;   /* vu.c: 0 SSE2, -1 detect (AVX where the CPU has it) */
        for (int pass = 0; pass < 2; pass++) for (int node = 0; node < 3000; node++) {
            if (node == 0) vu_avx = pass ? -1 : 0;
            for (int r = 1; r < 32; r++) for (int c = 0; c < 4; c++) g_vu0.vf[r].f[c] = RF(-50, 50);
            for (int r = 1; r < 16; r++) g_vu0.vi[r] = (uint16_t)RND();
            g_vu0.vf[17].f[0] = RF(100, 400); g_vu0.vf[17].f[1] = RF(100, 400); g_vu0.vf[23].f[2] = RF(0.5, 2);
            for (int r = 28; r < 32; r++) for (int c = 0; c < 4; c++) g_vu0.vf[r].f[c] = RF(-1, 1);   /* view matrix */
            for (int c = 0; c < 2; c++) g_vu0.vf[31].f[c] = RF(-100, 100);
            g_vu0.vf[31].f[2] = RF(50, 300);   /* view z in front: 1/z */
            g_vu0.q = RF(-1, 1); g_vu0.clip = RND() * 256u;
            /* INIT: vi10 = vf07.x, the lights (on: bits 7-4, point: 3-0); vf04-06 -> vf24-26 the
               light positions (x, y, z rows), vf08/vf09 -> vf12/vf13 the point light ranges (squared) */
            g_vu0.vf[7].u[0] = RND() & 0xFF;
            for (int r = 4; r < 7; r++) for (int c = 0; c < 4; c++) g_vu0.vf[r].f[c] = RF(-300, 300);
            for (int c = 0; c < 4; c++) { g_vu0.vf[8].f[c] = RF(0, 1e6); g_vu0.vf[9].f[c] = RF(0, 2e6); }
            if (node % 7 == 0) g_vu0.vf[8].f[0] = 0.0f;
            if (node % 11 == 3) g_vu0.vf[1 + RND() % 31].u[RND() & 3] = ODD();   /* an odd value in the leftover or matrix state */
            vu0_callms(VU0_ADDR_VU0_CALCPOINT_INIT);
            for (int v = 0; v < 6; v++) {
                for (int c = 0; c < 3; c++) { g_vu0.vf[4].f[c] = RF(-200, 200); g_vu0.vf[5].f[c] = RF(-1, 1); }
                g_vu0.vf[4].f[3] = 1.0f;
                if (v == 5) g_vu0.vf[4].f[0] = -0.0f;
                if (node % 5 == 1) g_vu0.vf[4].f[v % 3] *= 1e37f;    /* overflow: clamped to +-FLT_MAX */
                if (node % 5 == 2) g_vu0.vf[5].f[v % 3] *= 1e-37f;   /* denormal products: flushed */
                if (node % 5 == 3) g_vu0.vf[4 + (v & 1)].u[RND() & 3] = ODD();
                vu0_callms(VU0_ADDR_VU0_CALCPOINT);
            }
            {   /* the rest of the node as one run (vu0_calcpoint_n: four vertices per lane set, the
                   last ones per vertex); RECVX_VU_CHECK compares it with per-vertex calls */
                static float vin[24 * 8];
                static uint8_t vout[24 * 64];
                const int nv = 1 + (int)(RND() % 24);
                for (int v = 0; v < nv; v++) {
                    float* p = vin + 8 * v;
                    for (int c = 0; c < 3; c++) { p[c] = RF(-200, 200); p[4 + c] = RF(-1, 1); }
                    p[3] = 1.0f; p[7] = RF(-1, 1);
                    if (node % 5 == 1) p[v % 3] *= 1e37f;
                    if (node % 5 == 2) p[4 + v % 3] *= 1e-37f;
                    if (node % 5 == 3) ((uint32_t*)p)[RND() & 7] = ODD();
                }
                if (node % 13 == 5) g_vu0.vf[29].f[1] += 1.0f;   /* vf28-31 not the matrix CALCPOINT restores */
                vu0_calcpoint_n(vin, vout, nv);
            }
            vu0_callms(VU0_ADDR_VU0_CALCPOINT_END);
        }
        CHECK(vu_check_mismatches() == 0, "CALCPOINT native path: %ld calls differ from the interpreter", vu_check_mismatches());
#undef RF
#undef ODD
#undef RND
    }

    /* the inline macro-mode op (vu0.h vu_upper) against the interpreter's: every op, field mask
       and broadcast, overlapping registers, random and odd (Inf, NaN, denormal, -0) values */
    {
        static const uint32_t odd[] = { 0x00000000u, 0x80000000u, 0x00000001u, 0x807FFFFFu, 0x00800000u, 0x7F7FFFFFu,
                                        0xFF7FFFFFu, 0x7F800000u, 0xFF800000u, 0xFFC00000u, 0xFF800001u, 0x4F000000u,
                                        0xCF000000u, 0x3F800000u, 0x3F800001u, 0x00000000u, 0x80000000u };
        uint32_t seed = 4242;
        long bad = 0;
#define RND() ((seed = seed * 1103515245u + 12345u) >> 8 & 0xFFFF)
        for (int t = 0; t < 200000; t++) {
            VuState a, b;
            const int op = 1 + (int)(RND() % (U_OPMSUB));   /* U_ADD .. U_OPMSUB */
            const int mask = 1 + (int)(RND() % 15), fd = (int)(RND() % 32), fs = (int)(RND() % 32), ft = (int)(RND() % 32);
            const int bc = (int)(RND() % 7) - 1;
            for (int r = 1; r < 32; r++) for (int c = 0; c < 4; c++)
                g_vu0.vf[r].u[c] = neg_nan((RND() & 7) ? (uint32_t)RND() << 16 ^ RND() ^ 0x40000000u * (RND() & 1) : odd[RND() % (sizeof(odd) / sizeof(odd[0]))]);
            for (int c = 0; c < 4; c++) g_vu0.acc.u[c] = neg_nan((RND() & 7) ? (uint32_t)RND() << 16 ^ RND() : odd[RND() % (sizeof(odd) / sizeof(odd[0]))]);
            g_vu0.q = (float)((int)RND() - 32768) / 256.0f; g_vu0.i = (float)((int)RND() - 32768) / 64.0f;
            g_vu0.mac = RND(); g_vu0.clip = (uint32_t)RND() << 8;
            if (fs && !(RND() & 3)) g_vu0.vf[fs].u[RND() % 3] = neg_nan(g_vu0.vf[ft].u[3] ^ (RND() & 1) << 31);   /* CLIP: x/y/z = +-w */
            a = g_vu0;
            vu_upper(op, mask, fd, fs, ft, bc);
            b = g_vu0; g_vu0 = a;
            ref_upper(op, mask, fd, fs, ft, bc);
            if (memcmp(&b, &g_vu0, sizeof(b)) && bad++ < 5) fprintf(stderr, "vu_upper op %d mask %d fd %d fs %d ft %d bc %d differs\n", op, mask, fd, fs, ft, bc);
        }
        CHECK(bad == 0, "inline vu_upper: %ld ops differ from the interpreter", bad);
#undef RND
    }

    /* the strip clip test (vu_strip_clip) against the asm2c sequence it replaces */
    {
        static uint8_t vbuf[16 * 64] __attribute__((aligned(16)));
        uint16_t idx[16 * 3];
        uint32_t seed = 777;
#define RND() ((seed = seed * 1103515245u + 12345u) >> 8 & 0xFFFF)
        for (int t = 0; t < 500; t++) {
            VuState a, b;
            uint32_t ra, rb = 63;
            int n = 3 + (int)(RND() % 12);
            for (int k = 0; k < 16 * 16; k++) ((float*)vbuf)[k] = (float)((int)RND() - 32768) / 64.0f;
            for (int k = 0; k < n; k++) idx[k * 3] = (uint16_t)(RND() % 16);
            for (int r = 24; r < 28; r++) for (int c = 0; c < 4; c++) g_vu0.vf[r].f[c] = (float)((int)RND() - 32768) / 16384.0f;
            g_vu0.clip = RND();
            a = g_vu0;
            ra = vu_strip_clip(idx, n, vbuf);
            b = g_vu0; g_vu0 = a;
            vu_load_vf(4, vbuf + idx[0] * 64 + 32);
            ref_upper(U_MULA, 15, 0, 24, 4, BC_X); ref_upper(U_MADDA, 15, 0, 25, 4, BC_Y);
            ref_upper(U_MADDA, 15, 0, 26, 4, BC_Z); ref_upper(U_MADD, 15, 5, 27, 0, BC_W);
            for (int k = 1; k < n && rb; k++) {
                vu_load_vf(4, vbuf + idx[k * 3] * 64 + 32);
                ref_upper(U_CLIP, VU_XYZ, 0, 5, 5, BC_W);
                ref_upper(U_MULA, 15, 0, 24, 4, BC_X); ref_upper(U_MADDA, 15, 0, 25, 4, BC_Y);
                ref_upper(U_MADDA, 15, 0, 26, 4, BC_Z); ref_upper(U_MADD, 15, 5, 27, 0, BC_W);
                rb &= vu_cfc2(18);
                if (k == n - 1) { ref_upper(U_CLIP, VU_XYZ, 0, 5, 5, BC_W); rb &= vu_cfc2(18); }
            }
            CHECK(ra == rb && !memcmp(&b, &g_vu0, sizeof(b)), "vu_strip_clip differs (test %d: %u vs %u)", t, ra, rb);
        }
#undef RND
    }

    if (fails) { fprintf(stderr, "%d check(s) failed\n", fails); return 1; }
    puts("test_vu: ok");
    return 0;
}
