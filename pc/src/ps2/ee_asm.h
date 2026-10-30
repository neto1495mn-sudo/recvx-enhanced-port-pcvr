/*
 * Runtime helpers for C code generated from EE inline asm by pc/tools/asm2c.py.
 * EE GPRs are 128-bit; normal integer ops act on the low 64 bits and 32-bit ops
 * sign-extend their result, exactly as on the R5900.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "vu0.h"
#include "vu0_labels.h"

typedef union {
    uint64_t d[2];
    int64_t  sd[2];
    uint32_t w[4];
    int32_t  sw[4];
    uint16_t h[8];
    uint8_t  b[16];
    float    f[4];
} ee_reg;

#define EE_PTR(a) ((void*)(uintptr_t)(uint32_t)(a))

static inline uint32_t ee_f2u(float f)    { uint32_t u; memcpy(&u, &f, 4); return u; }
static inline float    ee_u2f(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }

#define EE_LB(a)  ((int64_t)*(int8_t*)EE_PTR(a))
#define EE_LBU(a) ((int64_t)*(uint8_t*)EE_PTR(a))
#define EE_LH(a)  ((int64_t)*(int16_t*)EE_PTR(a))
#define EE_LHU(a) ((int64_t)*(uint16_t*)EE_PTR(a))
#define EE_LW(a)  ((int64_t)*(int32_t*)EE_PTR(a))
#define EE_LWU(a) ((int64_t)*(uint32_t*)EE_PTR(a))
#define EE_LD(a)  (*(uint64_t*)EE_PTR(a))
#define EE_LWF(a) (*(float*)EE_PTR(a))
#define EE_SB(a, v) (*(uint8_t*)EE_PTR(a) = (uint8_t)(v))
#define EE_SH(a, v) (*(uint16_t*)EE_PTR(a) = (uint16_t)(v))
#define EE_SW(a, v) (*(uint32_t*)EE_PTR(a) = (uint32_t)(v))
#define EE_SD(a, v) (*(uint64_t*)EE_PTR(a) = (uint64_t)(v))
#define EE_SWF(a, v) (*(float*)EE_PTR(a) = (v))

/* lq/sq ignore the low 4 address bits. Android (Quest): clang does not align the decomp's arrays to 32 bytes as GCC does
   (docs/BUILDING.md), so a buffer can start 8 bytes past a quadword (Ps2_DRAW_TMP: the translucent strips' vertices came
   out shifted by 8 bytes - their colours stored a word-pair early, the depth lost); the address is used as it is, the
   same offsets from the buffer on both sides - identical to the rounding wherever the buffer is aligned */
#ifdef __ANDROID__
#define EE_QADDR(a) (a)
#else
#define EE_QADDR(a) ((a) & ~15u)
#endif
static inline void ee_lq(ee_reg* r, uint32_t a) { memcpy(r, EE_PTR(EE_QADDR(a)), 16); }
static inline void ee_sq(uint32_t a, const ee_reg* r) { memcpy(EE_PTR(EE_QADDR(a)), r, 16); }

/* little-endian unaligned doubleword load/store halves */
static inline uint64_t ee_ldl(uint64_t rt, uint32_t a)
{
    uint32_t sh = a & 7; uint64_t mem = EE_LD(a & ~7u);
    uint64_t keep = sh == 7 ? 0 : (~0ull >> ((sh + 1) * 8));
    return (rt & keep) | (mem << ((7 - sh) * 8));
}
static inline uint64_t ee_ldr(uint64_t rt, uint32_t a)
{
    uint32_t sh = a & 7; uint64_t mem = EE_LD(a & ~7u);
    uint64_t keep = sh == 0 ? 0 : (~0ull << ((8 - sh) * 8));
    return (rt & keep) | (mem >> (sh * 8));
}
static inline void ee_sdl(uint32_t a, uint64_t rt)
{
    uint32_t sh = a & 7; uint64_t* p = (uint64_t*)EE_PTR(a & ~7u);
    uint64_t keep = sh == 7 ? 0 : (~0ull << ((sh + 1) * 8));
    *p = (*p & keep) | (rt >> ((7 - sh) * 8));
}
static inline void ee_sdr(uint32_t a, uint64_t rt)
{
    uint32_t sh = a & 7; uint64_t* p = (uint64_t*)EE_PTR(a & ~7u);
    uint64_t keep = sh == 0 ? 0 : (~0ull >> ((8 - sh) * 8));
    *p = (*p & keep) | (rt << (sh * 8));
}

/* ---- MMI ---- */
static inline void ee_pcpyld(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; r.d[0] = t->d[0]; r.d[1] = s->d[0]; *d = r; }
static inline void ee_pcpyud(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; r.d[0] = s->d[1]; r.d[1] = t->d[1]; *d = r; }
static inline void ee_pextlw(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; r.w[0] = t->w[0]; r.w[1] = s->w[0]; r.w[2] = t->w[1]; r.w[3] = s->w[1]; *d = r; }
static inline void ee_pextuw(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; r.w[0] = t->w[2]; r.w[1] = s->w[2]; r.w[2] = t->w[3]; r.w[3] = s->w[3]; *d = r; }
static inline void ee_pextlh(ee_reg* d, const ee_reg* s, const ee_reg* t)
{
    ee_reg r;
    for (int k = 0; k < 4; k++) { r.h[2 * k] = t->h[k]; r.h[2 * k + 1] = s->h[k]; }
    *d = r;
}
static inline void ee_pextlb(ee_reg* d, const ee_reg* s, const ee_reg* t)
{
    ee_reg r;
    for (int k = 0; k < 8; k++) { r.b[2 * k] = t->b[k]; r.b[2 * k + 1] = s->b[k]; }
    *d = r;
}
static inline void ee_paddub(ee_reg* d, const ee_reg* s, const ee_reg* t)
{
    ee_reg r;
    for (int k = 0; k < 16; k++) { unsigned v = s->b[k] + t->b[k]; r.b[k] = v > 255 ? 255 : (uint8_t)v; }
    *d = r;
}
static inline void ee_paddw(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; for (int k = 0; k < 4; k++) r.w[k] = s->w[k] + t->w[k]; *d = r; }
static inline void ee_psubw(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; for (int k = 0; k < 4; k++) r.w[k] = s->w[k] - t->w[k]; *d = r; }
static inline void ee_pand(ee_reg* d, const ee_reg* s, const ee_reg* t) { d->d[0] = s->d[0] & t->d[0]; d->d[1] = s->d[1] & t->d[1]; }
static inline void ee_por(ee_reg* d, const ee_reg* s, const ee_reg* t)  { d->d[0] = s->d[0] | t->d[0]; d->d[1] = s->d[1] | t->d[1]; }
static inline void ee_pxor(ee_reg* d, const ee_reg* s, const ee_reg* t) { d->d[0] = s->d[0] ^ t->d[0]; d->d[1] = s->d[1] ^ t->d[1]; }
static inline void ee_pmaxw(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; for (int k = 0; k < 4; k++) r.sw[k] = s->sw[k] > t->sw[k] ? s->sw[k] : t->sw[k]; *d = r; }
static inline void ee_pminw(ee_reg* d, const ee_reg* s, const ee_reg* t) { ee_reg r; for (int k = 0; k < 4; k++) r.sw[k] = s->sw[k] < t->sw[k] ? s->sw[k] : t->sw[k]; *d = r; }
static inline void ee_prot3w(ee_reg* d, const ee_reg* t) { ee_reg r; r.w[0] = t->w[1]; r.w[1] = t->w[2]; r.w[2] = t->w[0]; r.w[3] = t->w[3]; *d = r; }
static inline void ee_plzcw(ee_reg* d, const ee_reg* s)
{
    ee_reg r = *d;
    for (int k = 0; k < 2; k++) {
        uint32_t v = s->w[k], sign = v >> 31; int n = 0;
        for (int b = 30; b >= 0 && ((v >> b) & 1) == sign; b--) n++;
        r.w[k] = (uint32_t)n;
    }
    d->d[0] = r.d[0];
}
