/*
 * Four VU lanes (lane c = component c, x = 0) with the VU float rules of vu_clampf: every
 * result clamped (Inf/NaN -> +-FLT_MAX, denormal -> +-0). SSE2 where the compiler targets
 * it, otherwise (or with -DRECVX_VU_PORTABLE, e.g. a future arm64 build) plain C with the
 * same results: both do the IEEE single operations one at a time. Included by vu0.h.
 */
#pragma once

#if defined(__SSE2__) && !defined(RECVX_VU_PORTABLE)
#include <emmintrin.h>
typedef __m128 V4;
/* |x| below FLT_MIN (denormal, zero): +-0; Inf/NaN: +-FLT_MAX (MINPS returns its second operand
   for a NaN); the sign is kept */
static inline V4 v4clamp(V4 x)
{
    const V4 sg = _mm_set1_ps(-0.0f);
    V4 a = _mm_andnot_ps(sg, x);
    V4 keep = _mm_cmpnlt_ps(a, _mm_set1_ps(FLT_MIN));
    a = _mm_and_ps(_mm_min_ps(a, _mm_set1_ps(FLT_MAX)), keep);
    return _mm_or_ps(a, _mm_and_ps(sg, x));
}
static inline V4 v4sel(V4 dst, V4 r, int m)   /* VU field mask (x = 8): those lanes from r */
{
    V4 k = _mm_castsi128_ps(_mm_set_epi32((m & 1) ? -1 : 0, (m & 2) ? -1 : 0, (m & 4) ? -1 : 0, (m & 8) ? -1 : 0));
    return _mm_or_ps(_mm_andnot_ps(k, dst), _mm_and_ps(k, r));
}
static inline V4 v4ld(const VuVec* v) { return _mm_loadu_ps(v->f); }
static inline void v4st(VuVec* v, V4 x) { _mm_storeu_ps(v->f, x); }
static inline V4 v4mul(V4 a, V4 b) { return v4clamp(_mm_mul_ps(a, b)); }
static inline V4 v4add(V4 a, V4 b) { return v4clamp(_mm_add_ps(a, b)); }
static inline V4 v4sub(V4 a, V4 b) { return v4clamp(_mm_sub_ps(a, b)); }
static inline V4 v4max(V4 a, V4 b) { return v4clamp(_mm_max_ps(a, b)); }   /* A > B ? A : B */
static inline V4 v4min(V4 a, V4 b) { return v4clamp(_mm_min_ps(a, b)); }   /* A < B ? A : B */
static inline V4 v4abs(V4 a) { return v4clamp(_mm_andnot_ps(_mm_set1_ps(-0.0f), a)); }
static inline V4 v4set1(float f) { return _mm_set1_ps(f); }
static inline V4 v4ftoi(V4 a, float k) { return _mm_castsi128_ps(_mm_cvttps_epi32(_mm_mul_ps(a, _mm_set1_ps(k)))); }   /* (int32_t)(a * k) */
static inline V4 v4itof(V4 a, float k) { return v4clamp(_mm_div_ps(_mm_cvtepi32_ps(_mm_castps_si128(a)), _mm_set1_ps(k))); }   /* (float)a / k */
#define V4BC(v, c) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(c, c, c, c))
#define V4YZX(v) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(3, 0, 2, 1))
#define V4ZXY(v) _mm_shuffle_ps((v), (v), _MM_SHUFFLE(3, 1, 0, 2))
static inline V4 v4opmsub(V4 acc, V4 a, V4 b) { return v4clamp(_mm_sub_ps(acc, _mm_mul_ps(V4YZX(a), V4ZXY(b)))); }   /* product not clamped */
static inline float v4lane(V4 v, int c)
{
    switch (c) {
    case 0: return _mm_cvtss_f32(v);
    case 1: return _mm_cvtss_f32(V4BC(v, 1));
    case 2: return _mm_cvtss_f32(_mm_movehl_ps(v, v));
    default: return _mm_cvtss_f32(V4BC(v, 3));
    }
}
static inline uint32_t v4mac(V4 v, int m)   /* MAC flags of the masked lanes: zero x..w = bits 3..0, sign = bits 7..4 */
{
    static const uint8_t rev4[16] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
    uint32_t z = rev4[_mm_movemask_ps(_mm_cmpeq_ps(v, _mm_setzero_ps()))], n = rev4[_mm_movemask_ps(v)];
    return (z | n << 4) & (uint32_t)(m | m << 4);
}
/* Lane-wise helpers for running several vertices side by side (one per lane); bit c of a
   lane mask = lane c. */
static inline V4 v4zero(void) { return _mm_setzero_ps(); }
static inline V4 v4blend(V4 a, V4 b, int bits)   /* the lanes in bits from b */
{
    V4 k = _mm_castsi128_ps(_mm_set_epi32(-(bits >> 3 & 1), -(bits >> 2 & 1), -(bits >> 1 & 1), -(bits & 1)));
    return _mm_or_ps(_mm_andnot_ps(k, a), _mm_and_ps(k, b));
}
static inline int v4neg_bits(V4 v) { return _mm_movemask_ps(v); }   /* sign bit set (-0 too) */
static inline int v4zero_bits(V4 v) { return _mm_movemask_ps(_mm_cmpeq_ps(v, _mm_setzero_ps())); }
static inline int v4gt_bits(V4 a, V4 b) { return _mm_movemask_ps(_mm_cmpgt_ps(a, b)); }
static inline int v4lt_bits(V4 a, V4 b) { return _mm_movemask_ps(_mm_cmplt_ps(a, b)); }
static inline V4 v4fabs(V4 a) { return _mm_andnot_ps(_mm_set1_ps(-0.0f), a); }   /* not clamped */
static inline V4 v4fneg(V4 a) { return _mm_xor_ps(_mm_set1_ps(-0.0f), a); }
static inline V4 v4sqrt(V4 a) { return _mm_sqrt_ps(a); }
static inline V4 v4div(V4 a, V4 b)   /* the VU divide: b = +-0 -> +-FLT_MAX by the signs, else a / b clamped */
{
    V4 z = _mm_cmpeq_ps(b, _mm_setzero_ps());
    V4 big = _mm_or_ps(_mm_set1_ps(FLT_MAX), _mm_and_ps(_mm_xor_ps(a, b), _mm_set1_ps(-0.0f)));
    return _mm_or_ps(_mm_and_ps(z, big), _mm_andnot_ps(z, v4clamp(_mm_div_ps(a, b))));
}
static inline void v4transpose(V4* a, V4* b, V4* c, V4* d) { _MM_TRANSPOSE4_PS(*a, *b, *c, *d); }
#else
typedef VuVec V4;
#define V4OP(name, expr) static inline V4 name(V4 a, V4 b) { V4 r; for (int c = 0; c < 4; c++) r.f[c] = vu_clampf(expr); (void)b; return r; }
V4OP(v4mul, a.f[c] * b.f[c])
V4OP(v4add, a.f[c] + b.f[c])
V4OP(v4sub, a.f[c] - b.f[c])
V4OP(v4max, a.f[c] > b.f[c] ? a.f[c] : b.f[c])
V4OP(v4min, a.f[c] < b.f[c] ? a.f[c] : b.f[c])
#undef V4OP
static inline V4 v4abs(V4 a) { for (int c = 0; c < 4; c++) a.f[c] = vu_clampf(fabsf(a.f[c])); return a; }
static inline V4 v4clamp(V4 a) { for (int c = 0; c < 4; c++) a.f[c] = vu_clampf(a.f[c]); return a; }
static inline V4 v4sel(V4 dst, V4 r, int m) { for (int c = 0; c < 4; c++) if (vu_field_on(m, c)) dst.u[c] = r.u[c]; return dst; }
static inline V4 v4ld(const VuVec* v) { return *v; }
static inline void v4st(VuVec* v, V4 x) { *v = x; }
static inline V4 v4set1(float f) { V4 r = { { f, f, f, f } }; return r; }
static inline V4 v4ftoi(V4 a, float k) { for (int c = 0; c < 4; c++) a.i[c] = (int32_t)(a.f[c] * k); return a; }
static inline V4 v4itof(V4 a, float k) { for (int c = 0; c < 4; c++) a.f[c] = vu_clampf((float)a.i[c] / k); return a; }
static inline V4 v4bc(V4 v, int c) { return v4set1(v.f[c]); }
#define V4BC(v, c) v4bc((v), (c))
static inline V4 v4opmsub(V4 acc, V4 a, V4 b)
{
    V4 r = acc;
    r.f[0] = vu_clampf(acc.f[0] - a.f[1] * b.f[2]);
    r.f[1] = vu_clampf(acc.f[1] - a.f[2] * b.f[0]);
    r.f[2] = vu_clampf(acc.f[2] - a.f[0] * b.f[1]);
    return r;
}
static inline V4 v4yzx(V4 v) { V4 r = { { v.f[1], v.f[2], v.f[0], v.f[3] } }; return r; }
static inline V4 v4zxy(V4 v) { V4 r = { { v.f[2], v.f[0], v.f[1], v.f[3] } }; return r; }
#define V4YZX(v) v4yzx(v)
#define V4ZXY(v) v4zxy(v)
static inline float v4lane(V4 v, int c) { return v.f[c]; }
static inline uint32_t v4mac(V4 v, int m)
{
    uint32_t r = 0;
    for (int c = 0; c < 4; c++)
        if (vu_field_on(m, c)) r |= (v.f[c] == 0.0f ? 1u << (3 - c) : 0) | (signbit(v.f[c]) ? 1u << (7 - c) : 0);
    return r;
}
static inline V4 v4zero(void) { return v4set1(0.0f); }
static inline V4 v4blend(V4 a, V4 b, int bits) { for (int c = 0; c < 4; c++) if (bits >> c & 1) a.u[c] = b.u[c]; return a; }
#define V4BITS(name, cond) static inline int name(V4 a, V4 b) { int r = 0; for (int c = 0; c < 4; c++) r |= (cond) << c; (void)b; return r; }
V4BITS(v4gt_bits_, a.f[c] > b.f[c])
V4BITS(v4lt_bits_, a.f[c] < b.f[c])
V4BITS(v4neg_bits_, signbit(a.f[c]) != 0)
V4BITS(v4zero_bits_, a.f[c] == 0.0f)
#undef V4BITS
#define v4gt_bits(a, b) v4gt_bits_(a, b)
#define v4lt_bits(a, b) v4lt_bits_(a, b)
#define v4neg_bits(a) v4neg_bits_(a, a)
#define v4zero_bits(a) v4zero_bits_(a, a)
static inline V4 v4fabs(V4 a) { for (int c = 0; c < 4; c++) a.f[c] = fabsf(a.f[c]); return a; }
static inline V4 v4fneg(V4 a) { for (int c = 0; c < 4; c++) a.u[c] ^= 0x80000000u; return a; }
static inline V4 v4sqrt(V4 a) { for (int c = 0; c < 4; c++) a.f[c] = sqrtf(a.f[c]); return a; }
static inline V4 v4div(V4 a, V4 b)
{
    for (int c = 0; c < 4; c++)
        a.f[c] = b.f[c] == 0.0f ? (signbit(a.f[c]) != signbit(b.f[c]) ? -FLT_MAX : FLT_MAX) : vu_clampf(a.f[c] / b.f[c]);
    return a;
}
static inline void v4transpose(V4* a, V4* b, V4* c, V4* d)
{
    V4* r[4] = { a, b, c, d };
    for (int i = 0; i < 4; i++) for (int j = i + 1; j < 4; j++) { uint32_t t = r[i]->u[j]; r[i]->u[j] = r[j]->u[i]; r[j]->u[i] = t; }
}
#endif
static inline V4 v4madd(V4 acc, V4 a, V4 b) { return v4add(acc, v4mul(a, b)); }
static inline V4 v4msub(V4 acc, V4 a, V4 b) { return v4sub(acc, v4mul(a, b)); }
