/*
 * MPEG-2 video decoder (ISO/IEC 13818-2) for the game's PSS movies.
 *
 * Supported: MP@ML 4:2:0 frame pictures with frame_pred_frame_dct = 1 (every PSS on the
 * disc), i.e. frame prediction and frame DCT only; both intra VLC tables, zigzag and
 * alternate scan, linear and non-linear quantiser scale, MPEG-2 inverse quantisation with
 * mismatch control. Field pictures / field prediction are rejected (logged once).
 */
#include "mpeg2.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- bit reader */

typedef struct { const uint8_t* p; size_t len, pos; } Bits;   /* pos in bits */

static uint32_t peek(const Bits* b, int n)
{
    size_t byte = b->pos >> 3;
    uint64_t v = 0;
    for (int i = 0; i < 5; i++) v = (v << 8) | (byte + i < b->len ? b->p[byte + i] : 0);
    return (uint32_t)((v << (24 + (b->pos & 7))) >> (64 - n)) & (n == 32 ? 0xFFFFFFFFu : ((1u << n) - 1));
}
static void skip(Bits* b, int n) { b->pos += n; }
static uint32_t get(Bits* b, int n) { uint32_t v = n ? peek(b, n) : 0; b->pos += n; return v; }
static int overrun(const Bits* b) { return b->pos > b->len * 8; }

/* ---------------------------------------------------------------- VLC tables */

typedef struct { int16_t next[2]; int32_t val; } VNode;
typedef struct { VNode n[640]; int count; } Vlc;
#define VLC_ERR  (-1000000)

static void vlc_add(Vlc* t, const char* code, int32_t val)
{
    int cur = 0;
    if (!t->count) { t->count = 1; t->n[0].next[0] = t->n[0].next[1] = -1; t->n[0].val = VLC_ERR; }
    for (const char* c = code; *c; c++) {
        int bit;
        if (*c == ' ') continue;
        bit = *c - '0';
        if (t->n[cur].next[bit] < 0) {
            int k = t->count++;
            t->n[k].next[0] = t->n[k].next[1] = -1; t->n[k].val = VLC_ERR;
            t->n[cur].next[bit] = (int16_t)k;
        }
        cur = t->n[cur].next[bit];
        if (c[1] && t->n[cur].val != VLC_ERR) fprintf(stderr, "mpeg2: VLC prefix conflict at %s\n", code);
    }
    if (t->n[cur].next[0] >= 0 || t->n[cur].next[1] >= 0 || t->n[cur].val != VLC_ERR) fprintf(stderr, "mpeg2: VLC conflict %s\n", code);
    t->n[cur].val = val;
}

static int32_t vlc_get(const Vlc* t, Bits* b)
{
    int cur = 0;
    for (int i = 0; i < 24; i++) {
        int nx = t->n[cur].next[get(b, 1)];
        if (nx < 0) return VLC_ERR;
        cur = nx;
        if (t->n[cur].next[0] < 0 && t->n[cur].next[1] < 0) return t->n[cur].val;
    }
    return VLC_ERR;
}

static Vlc vlc_mba, vlc_mbtype[4], vlc_cbp, vlc_mv, vlc_dcl, vlc_dcc, vlc_dct[2];

#define MB_QUANT   1
#define MB_FWD     2
#define MB_BWD     4
#define MB_PATTERN 8
#define MB_INTRA   16
#define DCT_EOB    (-2)
#define DCT_ESC    (-3)
#define RL(r, l)   (((r) << 8) | (l))

static const struct { const char* c; int v; } tab_mba[] = {
    { "1", 1 }, { "011", 2 }, { "010", 3 }, { "0011", 4 }, { "0010", 5 }, { "00011", 6 }, { "00010", 7 },
    { "0000111", 8 }, { "0000110", 9 }, { "00001011", 10 }, { "00001010", 11 }, { "00001001", 12 }, { "00001000", 13 },
    { "00000111", 14 }, { "00000110", 15 }, { "0000010111", 16 }, { "0000010110", 17 }, { "0000010101", 18 },
    { "0000010100", 19 }, { "0000010011", 20 }, { "0000010010", 21 }, { "00000100011", 22 }, { "00000100010", 23 },
    { "00000100001", 24 }, { "00000100000", 25 }, { "00000011111", 26 }, { "00000011110", 27 }, { "00000011101", 28 },
    { "00000011100", 29 }, { "00000011011", 30 }, { "00000011010", 31 }, { "00000011001", 32 }, { "00000011000", 33 },
    { "00000001000", 0 /* escape: +33 */ }, { "00000001111", -1 /* stuffing */ },
};
static const struct { const char* c; int v; } tab_mbtype_i[] = { { "1", MB_INTRA }, { "01", MB_INTRA | MB_QUANT } };
static const struct { const char* c; int v; } tab_mbtype_p[] = {
    { "1", MB_FWD | MB_PATTERN }, { "01", MB_PATTERN }, { "001", MB_FWD }, { "00011", MB_INTRA },
    { "00010", MB_FWD | MB_PATTERN | MB_QUANT }, { "00001", MB_PATTERN | MB_QUANT }, { "000001", MB_INTRA | MB_QUANT },
};
static const struct { const char* c; int v; } tab_mbtype_b[] = {
    { "10", MB_FWD | MB_BWD }, { "11", MB_FWD | MB_BWD | MB_PATTERN }, { "010", MB_BWD }, { "011", MB_BWD | MB_PATTERN },
    { "0010", MB_FWD }, { "0011", MB_FWD | MB_PATTERN }, { "00011", MB_INTRA },
    { "00010", MB_FWD | MB_BWD | MB_PATTERN | MB_QUANT }, { "000011", MB_FWD | MB_PATTERN | MB_QUANT },
    { "000010", MB_BWD | MB_PATTERN | MB_QUANT }, { "000001", MB_INTRA | MB_QUANT },
};
static const struct { const char* c; int v; } tab_cbp[] = {
    { "111", 60 }, { "1101", 4 }, { "1100", 8 }, { "1011", 16 }, { "1010", 32 },
    { "10011", 12 }, { "10010", 48 }, { "10001", 20 }, { "10000", 40 }, { "01111", 28 }, { "01110", 44 }, { "01101", 52 },
    { "01100", 56 }, { "01011", 1 }, { "01010", 61 }, { "01001", 2 }, { "01000", 62 },
    { "001111", 24 }, { "001110", 36 }, { "001101", 3 }, { "001100", 63 },
    { "0010111", 5 }, { "0010110", 9 }, { "0010101", 17 }, { "0010100", 33 }, { "0010011", 6 }, { "0010010", 10 },
    { "0010001", 18 }, { "0010000", 34 },
    { "00011111", 7 }, { "00011110", 11 }, { "00011101", 19 }, { "00011100", 35 }, { "00011011", 13 }, { "00011010", 49 },
    { "00011001", 21 }, { "00011000", 41 }, { "00010111", 14 }, { "00010110", 50 }, { "00010101", 22 }, { "00010100", 42 },
    { "00010011", 15 }, { "00010010", 51 }, { "00010001", 23 }, { "00010000", 43 },
    { "00001111", 25 }, { "00001110", 37 }, { "00001101", 26 }, { "00001100", 38 }, { "00001011", 29 }, { "00001010", 45 },
    { "00001001", 53 }, { "00001000", 57 }, { "00000111", 30 }, { "00000110", 46 }, { "00000101", 54 }, { "00000100", 58 },
    { "000000111", 31 }, { "000000110", 47 }, { "000000101", 55 }, { "000000100", 59 }, { "000000011", 27 },
    { "000000010", 39 }, { "000000001", 0 },
};
static const struct { const char* c; int v; } tab_mv[] = {   /* magnitude; sign bit follows (except 0) */
    { "1", 0 }, { "01", 1 }, { "001", 2 }, { "0001", 3 }, { "000011", 4 }, { "0000101", 5 }, { "0000100", 6 },
    { "0000011", 7 }, { "000001011", 8 }, { "000001010", 9 }, { "000001001", 10 }, { "0000010001", 11 },
    { "0000010000", 12 }, { "0000001111", 13 }, { "0000001110", 14 }, { "0000001101", 15 }, { "0000001100", 16 },
};
static const struct { const char* c; int v; } tab_dcl[] = {
    { "100", 0 }, { "00", 1 }, { "01", 2 }, { "101", 3 }, { "110", 4 }, { "1110", 5 }, { "11110", 6 }, { "111110", 7 },
    { "1111110", 8 }, { "11111110", 9 }, { "111111110", 10 }, { "111111111", 11 },
};
static const struct { const char* c; int v; } tab_dcc[] = {
    { "00", 0 }, { "01", 1 }, { "10", 2 }, { "110", 3 }, { "1110", 4 }, { "11110", 5 }, { "111110", 6 }, { "1111110", 7 },
    { "11111110", 8 }, { "111111110", 9 }, { "1111111110", 10 }, { "1111111111", 11 },
};
/* Table B.14 (sign bit follows each run/level code). "11" is (0,1) except as the first
   coefficient of a non-intra block, where "1" is (0,1) (handled in decode_block). */
static const struct { const char* c; int v; } tab_b14[] = {
    { "10", DCT_EOB }, { "11", RL(0, 1) }, { "011", RL(1, 1) }, { "0100", RL(0, 2) }, { "0101", RL(2, 1) },
    { "00101", RL(0, 3) }, { "00111", RL(3, 1) }, { "00110", RL(4, 1) }, { "000110", RL(1, 2) }, { "000111", RL(5, 1) },
    { "000101", RL(6, 1) }, { "000100", RL(7, 1) }, { "0000110", RL(0, 4) }, { "0000100", RL(2, 2) },
    { "0000111", RL(8, 1) }, { "0000101", RL(9, 1) }, { "000001", DCT_ESC },
    { "00100110", RL(0, 5) }, { "00100001", RL(0, 6) }, { "00100101", RL(1, 3) }, { "00100100", RL(3, 2) },
    { "00100111", RL(10, 1) }, { "00100011", RL(11, 1) }, { "00100010", RL(12, 1) }, { "00100000", RL(13, 1) },
    { "0000001010", RL(0, 7) }, { "0000001100", RL(1, 4) }, { "0000001011", RL(2, 3) }, { "0000001111", RL(4, 2) },
    { "0000001001", RL(5, 2) }, { "0000001110", RL(14, 1) }, { "0000001101", RL(15, 1) }, { "0000001000", RL(16, 1) },
    { "000000011101", RL(0, 8) }, { "000000011000", RL(0, 9) }, { "000000010011", RL(0, 10) },
    { "000000010000", RL(0, 11) }, { "000000011011", RL(1, 5) }, { "000000010100", RL(2, 4) },
    { "000000011100", RL(3, 3) }, { "000000010010", RL(4, 3) }, { "000000011110", RL(6, 2) },
    { "000000010101", RL(7, 2) }, { "000000010001", RL(8, 2) }, { "000000011111", RL(17, 1) },
    { "000000011010", RL(18, 1) }, { "000000011001", RL(19, 1) }, { "000000010111", RL(20, 1) },
    { "000000010110", RL(21, 1) },
    { "0000000011010", RL(0, 12) }, { "0000000011001", RL(0, 13) }, { "0000000011000", RL(0, 14) },
    { "0000000010111", RL(0, 15) },
    /* 13-bit and longer codes shared with table B.15 */
    { NULL, 0 },
};
/* Table B.15 (intra blocks when intra_vlc_format = 1) */
static const struct { const char* c; int v; } tab_b15[] = {
    { "0110", DCT_EOB }, { "10", RL(0, 1) }, { "010", RL(1, 1) }, { "110", RL(0, 2) }, { "00101", RL(2, 1) },
    { "0111", RL(0, 3) }, { "00111", RL(3, 1) }, { "000110", RL(4, 1) }, { "00110", RL(1, 2) }, { "000111", RL(5, 1) },
    { "0000110", RL(6, 1) }, { "0000100", RL(7, 1) }, { "11100", RL(0, 4) }, { "0000111", RL(2, 2) },
    { "0000101", RL(8, 1) }, { "1111000", RL(9, 1) }, { "000001", DCT_ESC }, { "11101", RL(0, 5) },
    { "000101", RL(0, 6) }, { "1111001", RL(1, 3) }, { "00100110", RL(3, 2) }, { "1111010", RL(10, 1) },
    { "00100001", RL(11, 1) }, { "00100101", RL(12, 1) }, { "00100100", RL(13, 1) }, { "000100", RL(0, 7) },
    { "00100111", RL(1, 4) }, { "11111100", RL(2, 3) }, { "11111101", RL(4, 2) }, { "000000100", RL(5, 2) },
    { "000000101", RL(14, 1) }, { "000000111", RL(15, 1) }, { "0000001101", RL(16, 1) }, { "1111011", RL(0, 8) },
    { "1111100", RL(0, 9) }, { "00100011", RL(0, 10) }, { "00100010", RL(0, 11) }, { "00100000", RL(1, 5) },
    { "0000001100", RL(2, 4) }, { "000000011100", RL(3, 3) }, { "000000010010", RL(4, 3) },
    { "000000011110", RL(6, 2) }, { "000000010101", RL(7, 2) }, { "000000010001", RL(8, 2) },
    { "000000011111", RL(17, 1) }, { "000000011010", RL(18, 1) }, { "000000011001", RL(19, 1) },
    { "000000010111", RL(20, 1) }, { "000000010110", RL(21, 1) }, { "11111010", RL(0, 12) },
    { "11111011", RL(0, 13) }, { "11111110", RL(0, 14) }, { "11111111", RL(0, 15) },
    { NULL, 0 },
};
static const struct { const char* c; int v; } tab_dct_long[] = {   /* identical in B.14 and B.15 */
    { "0000000010110", RL(1, 6) }, { "0000000010101", RL(1, 7) }, { "0000000010100", RL(2, 5) },
    { "0000000010011", RL(3, 4) }, { "0000000010010", RL(5, 3) }, { "0000000010001", RL(9, 2) },
    { "0000000010000", RL(10, 2) }, { "0000000011111", RL(22, 1) }, { "0000000011110", RL(23, 1) },
    { "0000000011101", RL(24, 1) }, { "0000000011100", RL(25, 1) }, { "0000000011011", RL(26, 1) },
    { "00000000011111", RL(0, 16) }, { "00000000011110", RL(0, 17) }, { "00000000011101", RL(0, 18) },
    { "00000000011100", RL(0, 19) }, { "00000000011011", RL(0, 20) }, { "00000000011010", RL(0, 21) },
    { "00000000011001", RL(0, 22) }, { "00000000011000", RL(0, 23) }, { "00000000010111", RL(0, 24) },
    { "00000000010110", RL(0, 25) }, { "00000000010101", RL(0, 26) }, { "00000000010100", RL(0, 27) },
    { "00000000010011", RL(0, 28) }, { "00000000010010", RL(0, 29) }, { "00000000010001", RL(0, 30) },
    { "00000000010000", RL(0, 31) },
    { "000000000011000", RL(0, 32) }, { "000000000010111", RL(0, 33) }, { "000000000010110", RL(0, 34) },
    { "000000000010101", RL(0, 35) }, { "000000000010100", RL(0, 36) }, { "000000000010011", RL(0, 37) },
    { "000000000010010", RL(0, 38) }, { "000000000010001", RL(0, 39) }, { "000000000010000", RL(0, 40) },
    { "000000000011111", RL(1, 8) }, { "000000000011110", RL(1, 9) }, { "000000000011101", RL(1, 10) },
    { "000000000011100", RL(1, 11) }, { "000000000011011", RL(1, 12) }, { "000000000011010", RL(1, 13) },
    { "000000000011001", RL(1, 14) },
    { "0000000000010011", RL(1, 15) }, { "0000000000010010", RL(1, 16) }, { "0000000000010001", RL(1, 17) },
    { "0000000000010000", RL(1, 18) }, { "0000000000010100", RL(6, 3) }, { "0000000000011010", RL(11, 2) },
    { "0000000000011001", RL(12, 2) }, { "0000000000011000", RL(13, 2) }, { "0000000000010111", RL(14, 2) },
    { "0000000000010110", RL(15, 2) }, { "0000000000010101", RL(16, 2) }, { "0000000000011111", RL(27, 1) },
    { "0000000000011110", RL(28, 1) }, { "0000000000011101", RL(29, 1) }, { "0000000000011100", RL(30, 1) },
    { "0000000000011011", RL(31, 1) },
};

#define ADD_TABLE(vlc, tab) for (size_t i_ = 0; i_ < sizeof(tab) / sizeof(tab[0]) && tab[i_].c; i_++) vlc_add(&(vlc), tab[i_].c, tab[i_].v)

static void init_tables(void)
{
    static int done;
    if (done) return;
    done = 1;
    ADD_TABLE(vlc_mba, tab_mba);
    ADD_TABLE(vlc_mbtype[1], tab_mbtype_i);
    ADD_TABLE(vlc_mbtype[2], tab_mbtype_p);
    ADD_TABLE(vlc_mbtype[3], tab_mbtype_b);
    ADD_TABLE(vlc_cbp, tab_cbp);
    ADD_TABLE(vlc_mv, tab_mv);
    ADD_TABLE(vlc_dcl, tab_dcl);
    ADD_TABLE(vlc_dcc, tab_dcc);
    ADD_TABLE(vlc_dct[0], tab_b14);
    ADD_TABLE(vlc_dct[0], tab_dct_long);
    ADD_TABLE(vlc_dct[1], tab_b15);
    ADD_TABLE(vlc_dct[1], tab_dct_long);
}

static const uint8_t zigzag[2][64] = {
    { 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
      35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 },
    { 0, 8, 16, 24, 1, 9, 2, 10, 17, 25, 32, 40, 48, 56, 57, 49, 41, 33, 26, 18, 3, 11, 4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
      51, 59, 20, 28, 5, 13, 6, 14, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61, 22, 30, 7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63 },
};
static const uint8_t default_intra_matrix[64] = {   /* zigzag order */
    8, 16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27, 27, 27, 26, 26, 26, 26, 27, 27, 27, 29, 29, 29, 34, 34, 34, 29,
    29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38, 37, 35, 35, 34, 35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83,
};
static const uint8_t nonlinear_q[32] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 40, 44, 48, 52,
                                         56, 64, 72, 80, 88, 96, 104, 112 };

/* ---------------------------------------------------------------- decoder state */

typedef struct { uint8_t* y; uint8_t* cb; uint8_t* cr; } Planes;

struct Mpeg2 {
    int width, height, mbw, mbh;
    uint8_t intra_q[64], nonintra_q[64];       /* natural order */
    int f_code[2][2], dc_prec, pic_struct, fpfd, conceal_mv, q_type, intra_vlc, alt_scan;
    int pic_type, mpeg2;
    Planes buf[3];                              /* fwd anchor, bwd anchor, B picture */
    int fwd, bwd, cur;                          /* indices into buf */
    int have_fwd, have_bwd, in_picture, bwd_shown;
    /* slice state */
    int qscale, dc_pred[3], pmv[2][2];
    int last_mb_type, last_mv[2][2];
    Mpeg2FrameFn on_frame;
    void* user;
    unsigned slice_errors, pictures, emitted;
    float idct_c[8][8];
};

Mpeg2* mpeg2_create(Mpeg2FrameFn on_frame, void* user)
{
    Mpeg2* d = calloc(1, sizeof(*d));
    init_tables();
    d->on_frame = on_frame; d->user = user;
    for (int i = 0; i < 64; i++) { d->intra_q[zigzag[0][i]] = default_intra_matrix[i]; d->nonintra_q[i] = 16; }
    for (int x = 0; x < 8; x++)
        for (int u = 0; u < 8; u++) d->idct_c[x][u] = (float)((u ? 1.0 : sqrt(0.5)) * cos((2 * x + 1) * u * M_PI / 16.0) / 2.0);
    d->fwd = 0; d->bwd = 1; d->cur = 2;
    return d;
}

void mpeg2_destroy(Mpeg2* d)
{
    if (!d) return;
    for (int i = 0; i < 3; i++) free(d->buf[i].y);
    free(d);
}

unsigned mpeg2_slice_errors(const Mpeg2* d) { return d->slice_errors; }
unsigned mpeg2_pictures(const Mpeg2* d) { return d->pictures; }

static void alloc_frames(Mpeg2* d)
{
    size_t ys = (size_t)d->mbw * 16 * d->mbh * 16;
    for (int i = 0; i < 3; i++) {
        free(d->buf[i].y);
        d->buf[i].y = calloc(ys * 3 / 2, 1);
        d->buf[i].cb = d->buf[i].y + ys;
        d->buf[i].cr = d->buf[i].cb + ys / 4;
        memset(d->buf[i].cb, 128, ys / 2);
    }
}

static void emit(Mpeg2* d, int idx)
{
    Mpeg2Frame f = { d->buf[idx].y, d->buf[idx].cb, d->buf[idx].cr, d->width, d->height, d->mbw * 16, d->mbw * 8 };
    d->emitted++;
    if (d->on_frame) d->on_frame(&f, d->user);
}

/* ---------------------------------------------------------------- headers */

static void seq_header(Mpeg2* d, Bits* b)
{
    int w = get(b, 12), h = get(b, 12);
    skip(b, 4 + 4 + 18 + 1 + 10 + 1);
    if (get(b, 1)) for (int i = 0; i < 64; i++) d->intra_q[zigzag[0][i]] = (uint8_t)get(b, 8);
    else for (int i = 0; i < 64; i++) d->intra_q[zigzag[0][i]] = default_intra_matrix[i];
    if (get(b, 1)) for (int i = 0; i < 64; i++) d->nonintra_q[zigzag[0][i]] = (uint8_t)get(b, 8);
    else for (int i = 0; i < 64; i++) d->nonintra_q[i] = 16;
    if (w != d->width || h != d->height) {
        d->width = w; d->height = h;
        d->mbw = (w + 15) / 16; d->mbh = (h + 15) / 16;
        alloc_frames(d);
        d->have_fwd = d->have_bwd = 0;
    }
}

static void extension(Mpeg2* d, Bits* b)
{
    int id = get(b, 4);
    if (id == 1) d->mpeg2 = 1;                       /* sequence extension: 4:2:0 / progressive assumed */
    else if (id == 3) {                              /* quant matrix extension */
        if (get(b, 1)) for (int i = 0; i < 64; i++) d->intra_q[zigzag[0][i]] = (uint8_t)get(b, 8);
        if (get(b, 1)) for (int i = 0; i < 64; i++) d->nonintra_q[zigzag[0][i]] = (uint8_t)get(b, 8);
    } else if (id == 8) {                            /* picture coding extension */
        d->f_code[0][0] = get(b, 4); d->f_code[0][1] = get(b, 4);
        d->f_code[1][0] = get(b, 4); d->f_code[1][1] = get(b, 4);
        d->dc_prec = get(b, 2);
        d->pic_struct = get(b, 2);
        skip(b, 1);                                  /* top_field_first */
        d->fpfd = get(b, 1);
        d->conceal_mv = get(b, 1);
        d->q_type = get(b, 1);
        d->intra_vlc = get(b, 1);
        d->alt_scan = get(b, 1);
    }
}

/* ---------------------------------------------------------------- reconstruction */

static void idct_put(Mpeg2* d, const int16_t* blk, uint8_t* dst, int stride, int add)
{
    float tmp[64];
    for (int y = 0; y < 8; y++)                       /* rows */
        for (int x = 0; x < 8; x++) {
            float s = 0;
            for (int u = 0; u < 8; u++) s += d->idct_c[x][u] * blk[y * 8 + u];
            tmp[y * 8 + x] = s;
        }
    for (int x = 0; x < 8; x++)                       /* columns */
        for (int y = 0; y < 8; y++) {
            float s = 0;
            int v;
            for (int u = 0; u < 8; u++) s += d->idct_c[y][u] * tmp[u * 8 + x];
            v = (int)lrintf(s);
            if (v < -256) v = -256; else if (v > 255) v = 255;
            if (add) v += dst[y * stride + x];
            dst[y * stride + x] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
        }
}

/* half-pel prediction of a size x size block from ref at (x*2+hx, y*2+hy)/2; avg blends with dst */
static void predict(uint8_t* dst, int dstride, const uint8_t* ref, int rw, int rh, int x, int y, int mvx, int mvy, int size, int avg)
{
    int ix = x + (mvx >> 1), iy = y + (mvy >> 1), hx = mvx & 1, hy = mvy & 1;
    for (int j = 0; j < size; j++)
        for (int i = 0; i < size; i++) {
#define PX(a, b) ref[(size_t)((b) < 0 ? 0 : (b) >= rh ? rh - 1 : (b)) * rw + ((a) < 0 ? 0 : (a) >= rw ? rw - 1 : (a))]
            int sx = ix + i, sy = iy + j, v;
            if (!hx && !hy) v = PX(sx, sy);
            else if (hx && !hy) v = (PX(sx, sy) + PX(sx + 1, sy) + 1) >> 1;
            else if (!hx && hy) v = (PX(sx, sy) + PX(sx, sy + 1) + 1) >> 1;
            else v = (PX(sx, sy) + PX(sx + 1, sy) + PX(sx, sy + 1) + PX(sx + 1, sy + 1) + 2) >> 2;
#undef PX
            if (avg) v = (dst[j * dstride + i] + v + 1) >> 1;
            dst[j * dstride + i] = (uint8_t)v;
        }
}

static void motion_comp(Mpeg2* d, int mbx, int mby, int type, const int mv[2][2])
{
    Planes* cur = &d->buf[d->cur];
    int yw = d->mbw * 16, yh = d->mbh * 16, cw = yw / 2, ch = yh / 2, avg = 0;
    for (int dir = 0; dir < 2; dir++) {
        Planes* ref;
        int cmx, cmy;
        if (!(type & (dir ? MB_BWD : MB_FWD))) continue;
        ref = &d->buf[dir ? d->bwd : d->fwd];
        predict(cur->y + (size_t)mby * 16 * yw + mbx * 16, yw, ref->y, yw, yh, mbx * 16, mby * 16, mv[dir][0], mv[dir][1], 16, avg);
        cmx = mv[dir][0] / 2; cmy = mv[dir][1] / 2;   /* 4:2:0: truncating division */
        predict(cur->cb + (size_t)mby * 8 * cw + mbx * 8, cw, ref->cb, cw, ch, mbx * 8, mby * 8, cmx, cmy, 8, avg);
        predict(cur->cr + (size_t)mby * 8 * cw + mbx * 8, cw, ref->cr, cw, ch, mbx * 8, mby * 8, cmx, cmy, 8, avg);
        avg = 1;
    }
}

static int decode_block(Mpeg2* d, Bits* b, int comp, int intra, int16_t blk[64])
{
    const uint8_t* scan = zigzag[d->alt_scan];
    const uint8_t* W = intra ? d->intra_q : d->nonintra_q;
    const Vlc* tab = &vlc_dct[intra && d->intra_vlc];
    int i = 0, sum = 0;
    memset(blk, 0, 64 * sizeof(int16_t));
    if (intra) {
        int cc = comp < 4 ? 0 : comp - 3;
        int size = vlc_get(cc ? &vlc_dcc : &vlc_dcl, b), diff = 0;
        if (size == VLC_ERR) return 0;
        if (size) {
            diff = get(b, size);
            if (!(diff & (1 << (size - 1)))) diff -= (1 << size) - 1;
        }
        d->dc_pred[cc] += diff;
        blk[0] = (int16_t)(d->dc_pred[cc] << (3 - d->dc_prec));
        sum = blk[0];
        i = 1;
    }
    for (;; i++) {
        int run, level;
        int32_t v;
        if (!intra && i == 0 && peek(b, 1) == 1) { skip(b, 1); v = RL(0, 1); }
        else v = vlc_get(tab, b);
        if (v == VLC_ERR) return 0;
        if (v == DCT_EOB) break;
        if (v == DCT_ESC) {
            run = get(b, 6);
            level = (int)get(b, 12);
            if (level & 0x800) level -= 0x1000;
            if (level == 0 || level == -2048) return 0;
        } else {
            run = v >> 8; level = v & 0xFF;
            if (get(b, 1)) level = -level;
        }
        i += run;
        if (i > 63) return 0;
        {
            int idx = scan[i], val;
            if (intra) val = (level * W[idx] * d->qscale * 2) / 32;
            else val = ((2 * level + (level > 0 ? 1 : -1)) * W[idx] * d->qscale) / 32;
            if (val > 2047) val = 2047; else if (val < -2048) val = -2048;
            blk[idx] = (int16_t)val;
            sum += val;
        }
    }
    if (!(sum & 1)) blk[63] ^= 1;                    /* mismatch control */
    return 1;
}

static int read_mv(Mpeg2* d, Bits* b, int dir, int comp)
{
    int r_size = d->f_code[dir][comp] - 1, f = 1 << r_size;
    int code = vlc_get(&vlc_mv, b), delta, pred, low = -16 * f, high = 16 * f - 1, range = 32 * f;
    if (code == VLC_ERR) return VLC_ERR;
    if (code && get(b, 1)) code = -code;
    if (f == 1 || code == 0) delta = code;
    else {
        int res = get(b, r_size);
        delta = ((abs(code) - 1) * f) + res + 1;
        if (code < 0) delta = -delta;
    }
    pred = d->pmv[dir][comp] + delta;
    if (pred < low) pred += range; else if (pred > high) pred -= range;
    d->pmv[dir][comp] = pred;
    return pred;
}

static void reset_dc(Mpeg2* d) { d->dc_pred[0] = d->dc_pred[1] = d->dc_pred[2] = 1 << (7 + d->dc_prec); }

static int set_qscale(Mpeg2* d, int code) { return d->qscale = d->q_type ? nonlinear_q[code] : code * 2; }

static void slice(Mpeg2* d, Bits* b, int vpos)
{
    int mb_addr, mby = vpos - 1, ok = 1, slice_start = (vpos - 1) * d->mbw;
    int16_t blk[64];
    set_qscale(d, get(b, 5));
    while (get(b, 1)) skip(b, 8);                    /* extra_information_slice */
    reset_dc(d);
    memset(d->pmv, 0, sizeof(d->pmv));
    mb_addr = mby * d->mbw - 1;
    d->last_mb_type = 0;
    while (ok) {
        int inc = 0, type, mv[2][2] = { { 0, 0 }, { 0, 0 } }, cbp, mbx;
        if (peek(b, 23) == 0) break;                 /* next start code */
        for (;;) {
            int v = vlc_get(&vlc_mba, b);
            if (v == VLC_ERR) { ok = 0; break; }
            if (v == 0) inc += 33;
            else if (v > 0) { inc += v; break; }
        }
        if (!ok) break;
        if (inc > 1 && mb_addr >= slice_start) {    /* skipped macroblocks (not at slice start) */
            for (int s = 1; s < inc; s++) {
                int a = mb_addr + s, sx = a % d->mbw, sy = a / d->mbw;
                reset_dc(d);
                if (d->pic_type == 2) {
                    int zero[2][2] = { { 0, 0 }, { 0, 0 } };
                    memset(d->pmv, 0, sizeof(d->pmv));
                    motion_comp(d, sx, sy, MB_FWD, zero);
                } else if (d->pic_type == 3) {
                    motion_comp(d, sx, sy, d->last_mb_type & (MB_FWD | MB_BWD), d->last_mv);
                }
            }
        }
        mb_addr += inc;
        if (mb_addr >= d->mbw * d->mbh) { ok = 0; break; }
        mbx = mb_addr % d->mbw;
        mby = mb_addr / d->mbw;
        type = vlc_get(&vlc_mbtype[d->pic_type], b);
        if (type == VLC_ERR) { ok = 0; break; }
        if (type & MB_QUANT) set_qscale(d, get(b, 5));
        if (type & MB_INTRA) {
            if (d->conceal_mv) { read_mv(d, b, 0, 0); read_mv(d, b, 0, 1); skip(b, 1); }
            else memset(d->pmv, 0, sizeof(d->pmv));
            for (int c = 0; c < 6; c++) {
                Planes* p = &d->buf[d->cur];
                int yw = d->mbw * 16;
                if (!decode_block(d, b, c, 1, blk)) { ok = 0; break; }
                if (c < 4) idct_put(d, blk, p->y + (size_t)(mby * 16 + (c >> 1) * 8) * yw + mbx * 16 + (c & 1) * 8, yw, 0);
                else idct_put(d, blk, (c == 4 ? p->cb : p->cr) + (size_t)mby * 8 * (yw / 2) + mbx * 8, yw / 2, 0);
            }
            d->last_mb_type = type;
            continue;
        }
        reset_dc(d);
        for (int dir = 0; dir < 2; dir++)
            if (type & (dir ? MB_BWD : MB_FWD)) {
                int x = read_mv(d, b, dir, 0), y = read_mv(d, b, dir, 1);
                if (x == VLC_ERR || y == VLC_ERR) { ok = 0; break; }
                mv[dir][0] = x; mv[dir][1] = y;
            }
        if (!ok) break;
        if (d->pic_type == 2 && !(type & MB_FWD)) {   /* P "no MC": zero vector, predictors reset */
            memset(d->pmv, 0, sizeof(d->pmv));
            type |= MB_FWD;
        }
        motion_comp(d, mbx, mby, type, mv);
        d->last_mb_type = type;
        memcpy(d->last_mv, mv, sizeof(mv));
        cbp = (type & MB_PATTERN) ? vlc_get(&vlc_cbp, b) : 0;
        if (cbp == VLC_ERR) { ok = 0; break; }
        for (int c = 0; c < 6; c++) {
            Planes* p = &d->buf[d->cur];
            int yw = d->mbw * 16;
            if (!(cbp & (32 >> c))) continue;
            if (!decode_block(d, b, c, 0, blk)) { ok = 0; break; }
            if (c < 4) idct_put(d, blk, p->y + (size_t)(mby * 16 + (c >> 1) * 8) * yw + mbx * 16 + (c & 1) * 8, yw, 1);
            else idct_put(d, blk, (c == 4 ? p->cb : p->cr) + (size_t)mby * 8 * (yw / 2) + mbx * 8, yw / 2, 1);
        }
    }
    /* a correctly decoded slice ends with zero stuffing up to the next start code */
    if (!ok || overrun(b) || peek(b, (int)((8 - (b->pos & 7)) & 7)) != 0) d->slice_errors++;
}

static void finish_picture(Mpeg2* d)
{
    if (!d->in_picture) return;
    d->in_picture = 0;
    d->pictures++;
    if (d->pic_type == 3) emit(d, d->cur);
    else if (d->have_fwd && !d->bwd_shown) { emit(d, d->fwd); d->bwd_shown = 1; }
}

static void picture_header(Mpeg2* d, Bits* b)
{
    finish_picture(d);
    skip(b, 10);                                     /* temporal_reference */
    d->pic_type = get(b, 3);
    if (d->pic_type < 1 || d->pic_type > 3 || !d->buf[0].y) { d->pic_type = 0; return; }
    if (d->pic_type == 3 && !(d->have_fwd && d->have_bwd)) { d->pic_type = 0; return; }   /* B without two anchors */
    if (d->pic_type != 3) {
        /* new anchor: the older one becomes the forward reference */
        int t = d->fwd;
        d->fwd = d->bwd; d->bwd = t;
        d->have_fwd = d->have_bwd;
        d->have_bwd = 1;
        d->bwd_shown = 0;
    }
    d->in_picture = 1;
}

/* index of the buffer the current picture decodes into */
static void select_target(Mpeg2* d)
{
    if (d->pic_type == 3) d->cur = 3 - d->fwd - d->bwd;   /* the remaining buffer */
    else d->cur = d->bwd;
}

static size_t decode(Mpeg2* d, const uint8_t* data, size_t len, int eof, int one)
{
    size_t pos = 0;
    unsigned emitted = d->emitted;
    for (;;) {
        if (one && d->emitted != emitted) return pos;
        size_t sc = pos, next;
        while (sc + 3 < len && !(data[sc] == 0 && data[sc + 1] == 0 && data[sc + 2] == 1)) sc++;
        if (sc + 3 >= len) return eof ? len : pos;
        next = sc + 4;
        while (next + 3 < len && !(data[next] == 0 && data[next + 1] == 0 && data[next + 2] == 1)) next++;
        if (next + 3 >= len) {
            if (!eof) return sc;                     /* unit not complete yet */
            next = len;
        }
        {
            int code = data[sc + 3];
            Bits b = { data + sc + 4, next - (sc + 4), 0 };
            if (code == 0xB3) seq_header(d, &b);
            else if (code == 0xB5) extension(d, &b);
            else if (code == 0x00) { picture_header(d, &b); if (d->pic_type) select_target(d); }
            else if (code >= 0x01 && code <= 0xAF && d->in_picture && d->pic_type) {
                if (!d->fpfd || d->pic_struct != 3) {
                    static int warned;
                    if (!warned++) fprintf(stderr, "mpeg2: field pictures / field prediction not supported\n");
                } else slice(d, &b, code);
            } else if (code == 0xB7) {               /* sequence end: show the last anchor */
                finish_picture(d);
                if (d->have_bwd) emit(d, d->bwd);
                d->have_fwd = d->have_bwd = 0;
            }
        }
        pos = next;
        if (next >= len) {
            if (eof) {
                finish_picture(d);
                if (d->have_bwd) { emit(d, d->bwd); d->have_bwd = 0; }
            }
            return len;
        }
    }
}

size_t mpeg2_decode(Mpeg2* d, const uint8_t* data, size_t len, int eof) { return decode(d, data, len, eof, 0); }
size_t mpeg2_decode_frame(Mpeg2* d, const uint8_t* data, size_t len, int eof) { return decode(d, data, len, eof, 1); }
