/*
 * GS local memory: 4 MB, organised in 8 KB pages / 256-byte blocks with the
 * format-specific block and column swizzles of the GS.
 */
#include "gs.h"
#include "textures.h"
#include "stream.h"
#include "../ps2_platform.h"
#include <string.h>

uint8_t gs_mem[GS_MEM_SIZE] __attribute__((aligned(64)));
#ifdef __ANDROID__
static uint32_t wgen = 1, page_wgen[512];   /* write generation per 8 KB page (gs_exec_upload's skipping) */
/* Android (Quest): a hash per 256-byte block, made where an upload assembles the block (it is in the L1 cache then) or
   later for blocks other writes left stale; the page hash (gs_gl.c texture lookups) combines its 32. Rehashing whole
   changed pages at their next texture lookup was 20-25% of a game frame. blk_clean: 1 byte a block (both upload
   threads store their own blocks' bytes, no shared words) */
static uint64_t blk_hash[GS_MEM_SIZE / 256];
static uint8_t blk_clean[GS_MEM_SIZE / 256];
#if defined(__ARM_NEON) && !defined(RECVX_HASH_SCALAR)
#include <arm_neon.h>
#endif
#ifdef __ANDROID__
__attribute__((noinline))   /* (its own name in the sampling profiler: how much of the uploads is the hashing) */
#endif
static uint64_t hash256(const uint8_t* p8, uint32_t b)
{
    uint32_t a = 0x9E3779B1u ^ b, c = 0x85EBCA77u, d = 0xC2B2AE3Du, e = 0x27D4EB2Fu ^ (b * 0x01000193u);
#define HROT(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
    /* 16 lanes in 4 independent chains (64 bytes a step): the multiply's latency no longer serialises the loop (one
       chain of 4 lanes was ~22% of a game frame's execution); the chains are folded into the 4 lanes at the end */
#if defined(__ARM_NEON) && !defined(RECVX_HASH_SCALAR)
    {   /* the same values as the scalar loop below */
        static const uint32_t mk[4] = { 0x9E3779B1u, 0x85EBCA77u, 0xC2B2AE3Du, 0x27D4EB2Fu };
        uint32_t st[4] = { a, c, d, e };
        uint32x4_t m = vld1q_u32(mk), v0 = vld1q_u32(st), v1 = veorq_u32(v0, vdupq_n_u32(0x165667B1u)),
                   v2 = veorq_u32(v0, vdupq_n_u32(0xD3A2646Cu)), v3 = veorq_u32(v0, vdupq_n_u32(0xFD7046C5u));
#define HSTEP(v, q) v = vmulq_u32(veorq_u32(v, vreinterpretq_u32_u8(vld1q_u8(q))), m); v = vorrq_u32(vshlq_n_u32(v, 13), vshrq_n_u32(v, 19))
        for (int i = 0; i < 256; i += 64) { HSTEP(v0, p8 + i); HSTEP(v1, p8 + i + 16); HSTEP(v2, p8 + i + 32); HSTEP(v3, p8 + i + 48); }
#define HFOLD(x) v0 = vmulq_u32(veorq_u32(v0, x), m); v0 = vorrq_u32(vshlq_n_u32(v0, 13), vshrq_n_u32(v0, 19))
        HFOLD(v1); HFOLD(v2); HFOLD(v3);
#undef HFOLD
#undef HSTEP
        vst1q_u32(st, v0);
        a = st[0]; c = st[1]; d = st[2]; e = st[3];
    }
#else
    {
        static const uint32_t mk[4] = { 0x9E3779B1u, 0x85EBCA77u, 0xC2B2AE3Du, 0x27D4EB2Fu }, sx[4] = { 0, 0x165667B1u, 0xD3A2646Cu, 0xFD7046C5u };
        uint32_t v[4][4];
        for (int c4 = 0; c4 < 4; c4++) { v[c4][0] = a ^ sx[c4]; v[c4][1] = c ^ sx[c4]; v[c4][2] = d ^ sx[c4]; v[c4][3] = e ^ sx[c4]; }
        for (int i = 0; i < 256; i += 64)
            for (int c4 = 0; c4 < 4; c4++)
                for (int l = 0; l < 4; l++) {
                    uint32_t w;
                    memcpy(&w, p8 + i + 16 * c4 + 4 * l, 4);
                    v[c4][l] = HROT((v[c4][l] ^ w) * mk[l], 13);
                }
        for (int c4 = 1; c4 < 4; c4++)
            for (int l = 0; l < 4; l++) v[0][l] = HROT((v[0][l] ^ v[c4][l]) * mk[l], 13);
        a = v[0][0]; c = v[0][1]; d = v[0][2]; e = v[0][3];
    }
#endif
    a ^= HROT(d, 7) * 0x9E3779B1u; c ^= HROT(e, 11) * 0x85EBCA77u;
#undef HROT
    a ^= a >> 15; a *= 0x2C1B3C6Du; a ^= a >> 12;
    c ^= c >> 15; c *= 0x297A2D39u; c ^= c >> 12;
    return ((uint64_t)a << 32 | c) ^ ((uint64_t)d << 17) ^ e;
}
#define BLK_DIRTY(byte_a) do { if (gs_memp == gs_mem) blk_clean[((byte_a) >> 8) & (GS_MEM_SIZE / 256 - 1)] = 0; } while (0)
#define BLK_SET(byte_a, data) do { if (gs_memp == gs_mem) { uint32_t b_ = ((byte_a) >> 8) & (GS_MEM_SIZE / 256 - 1); \
                                    blk_hash[b_] = hash256((const uint8_t*)(data), b_); blk_clean[b_] = 1; } } while (0)
/* gs_gl.c page_hash_get: the hash of 8 KB page p from its blocks' */
uint64_t gs_page_hash(uint32_t p)
{
    uint64_t h = 0x9E3779B97F4A7C15ull * (p + 1);
    for (uint32_t k = 0; k < 32; k++) {
        uint32_t b = (p * 32 + k) & (GS_MEM_SIZE / 256 - 1);
        if (!blk_clean[b]) { blk_hash[b] = hash256(gs_mem + (size_t)b * 256, b); blk_clean[b] = 1; }
        h = (h ^ blk_hash[b]) * 0x100000001B3ull;
        h ^= h >> 29;
    }
    return h;
}
#else
#define BLK_DIRTY(byte_a) ((void)0)
#define BLK_SET(byte_a, data) ((void)0)
#endif
/* the memory the functions below work on: gs_mem, or a scratch buffer while a native texture
   is decoded (gs_mem_scratch_begin/end) */
uint8_t* gs_memp = gs_mem;
static uint8_t gs_mem_scratch[GS_MEM_SIZE] __attribute__((aligned(64)));
static uint8_t clut_saved[GS_CLUT_STATE_SIZE];

void gs_mem_scratch_begin(void) { gs_memp = gs_mem_scratch; gs_clut_state_save(clut_saved); }
void gs_mem_scratch_end(void) { gs_memp = gs_mem; gs_clut_state_restore(clut_saved); }

/* ---- block arrangement inside a page ---- */
static const uint8_t blockTable32[4][8] = {
    { 0, 1, 4, 5, 16, 17, 20, 21 }, { 2, 3, 6, 7, 18, 19, 22, 23 },
    { 8, 9, 12, 13, 24, 25, 28, 29 }, { 10, 11, 14, 15, 26, 27, 30, 31 } };
static const uint8_t blockTableZ32[4][8] = {
    { 24, 25, 28, 29, 8, 9, 12, 13 }, { 26, 27, 30, 31, 10, 11, 14, 15 },
    { 16, 17, 20, 21, 0, 1, 4, 5 }, { 18, 19, 22, 23, 2, 3, 6, 7 } };
static const uint8_t blockTable16[8][4] = {
    { 0, 2, 8, 10 }, { 1, 3, 9, 11 }, { 4, 6, 12, 14 }, { 5, 7, 13, 15 },
    { 16, 18, 24, 26 }, { 17, 19, 25, 27 }, { 20, 22, 28, 30 }, { 21, 23, 29, 31 } };
static const uint8_t blockTable16S[8][4] = {
    { 0, 2, 16, 18 }, { 1, 3, 17, 19 }, { 8, 10, 24, 26 }, { 9, 11, 25, 27 },
    { 4, 6, 20, 22 }, { 5, 7, 21, 23 }, { 12, 14, 28, 30 }, { 13, 15, 29, 31 } };
static const uint8_t blockTableZ16[8][4] = {
    { 24, 26, 16, 18 }, { 25, 27, 17, 19 }, { 28, 30, 20, 22 }, { 29, 31, 21, 23 },
    { 8, 10, 0, 2 }, { 9, 11, 1, 3 }, { 12, 14, 4, 6 }, { 13, 15, 5, 7 } };
static const uint8_t blockTableZ16S[8][4] = {
    { 24, 26, 8, 10 }, { 25, 27, 9, 11 }, { 16, 18, 0, 2 }, { 17, 19, 1, 3 },
    { 28, 30, 12, 14 }, { 29, 31, 13, 15 }, { 20, 22, 4, 6 }, { 21, 23, 5, 7 } };
static const uint8_t blockTable8[4][8] = {
    { 0, 1, 4, 5, 16, 17, 20, 21 }, { 2, 3, 6, 7, 18, 19, 22, 23 },
    { 8, 9, 12, 13, 24, 25, 28, 29 }, { 10, 11, 14, 15, 26, 27, 30, 31 } };
static const uint8_t blockTable4[8][4] = {
    { 0, 2, 8, 10 }, { 1, 3, 9, 11 }, { 4, 6, 12, 14 }, { 5, 7, 13, 15 },
    { 16, 18, 24, 26 }, { 17, 19, 25, 27 }, { 20, 22, 28, 30 }, { 21, 23, 29, 31 } };

/* ---- pixel arrangement inside a block ---- */
static const uint8_t columnTable32[8][8] = {
    { 0, 1, 4, 5, 8, 9, 12, 13 }, { 2, 3, 6, 7, 10, 11, 14, 15 },
    { 16, 17, 20, 21, 24, 25, 28, 29 }, { 18, 19, 22, 23, 26, 27, 30, 31 },
    { 32, 33, 36, 37, 40, 41, 44, 45 }, { 34, 35, 38, 39, 42, 43, 46, 47 },
    { 48, 49, 52, 53, 56, 57, 60, 61 }, { 50, 51, 54, 55, 58, 59, 62, 63 } };
static const uint8_t columnTable16[8][16] = {
    { 0, 2, 8, 10, 16, 18, 24, 26, 1, 3, 9, 11, 17, 19, 25, 27 },
    { 4, 6, 12, 14, 20, 22, 28, 30, 5, 7, 13, 15, 21, 23, 29, 31 },
    { 32, 34, 40, 42, 48, 50, 56, 58, 33, 35, 41, 43, 49, 51, 57, 59 },
    { 36, 38, 44, 46, 52, 54, 60, 62, 37, 39, 45, 47, 53, 55, 61, 63 },
    { 64, 66, 72, 74, 80, 82, 88, 90, 65, 67, 73, 75, 81, 83, 89, 91 },
    { 68, 70, 76, 78, 84, 86, 92, 94, 69, 71, 77, 79, 85, 87, 93, 95 },
    { 96, 98, 104, 106, 112, 114, 120, 122, 97, 99, 105, 107, 113, 115, 121, 123 },
    { 100, 102, 108, 110, 116, 118, 124, 126, 101, 103, 109, 111, 117, 119, 125, 127 } };
static const uint8_t columnTable8[16][16] = {
    { 0, 4, 16, 20, 32, 36, 48, 52, 2, 6, 18, 22, 34, 38, 50, 54 },
    { 8, 12, 24, 28, 40, 44, 56, 60, 10, 14, 26, 30, 42, 46, 58, 62 },
    { 33, 37, 49, 53, 1, 5, 17, 21, 35, 39, 51, 55, 3, 7, 19, 23 },
    { 41, 45, 57, 61, 9, 13, 25, 29, 43, 47, 59, 63, 11, 15, 27, 31 },
    { 96, 100, 112, 116, 64, 68, 80, 84, 98, 102, 114, 118, 66, 70, 82, 86 },
    { 104, 108, 120, 124, 72, 76, 88, 92, 106, 110, 122, 126, 74, 78, 90, 94 },
    { 65, 69, 81, 85, 97, 101, 113, 117, 67, 71, 83, 87, 99, 103, 115, 119 },
    { 73, 77, 89, 93, 105, 109, 121, 125, 75, 79, 91, 95, 107, 111, 123, 127 },
    { 128, 132, 144, 148, 160, 164, 176, 180, 130, 134, 146, 150, 162, 166, 178, 182 },
    { 136, 140, 152, 156, 168, 172, 184, 188, 138, 142, 154, 158, 170, 174, 186, 190 },
    { 161, 165, 177, 181, 129, 133, 145, 149, 163, 167, 179, 183, 131, 135, 147, 151 },
    { 169, 173, 185, 189, 137, 141, 153, 157, 171, 175, 187, 191, 139, 143, 155, 159 },
    { 224, 228, 240, 244, 192, 196, 208, 212, 226, 230, 242, 246, 194, 198, 210, 214 },
    { 232, 236, 248, 252, 200, 204, 216, 220, 234, 238, 250, 254, 202, 206, 218, 222 },
    { 193, 197, 209, 213, 225, 229, 241, 245, 195, 199, 211, 215, 227, 231, 243, 247 },
    { 201, 205, 217, 221, 233, 237, 249, 253, 203, 207, 219, 223, 235, 239, 251, 255 } };
static const uint16_t columnTable4[16][32] = {
    { 0, 8, 32, 40, 64, 72, 96, 104, 2, 10, 34, 42, 66, 74, 98, 106, 4, 12, 36, 44, 68, 76, 100, 108, 6, 14, 38, 46, 70, 78, 102, 110 },
    { 16, 24, 48, 56, 80, 88, 112, 120, 18, 26, 50, 58, 82, 90, 114, 122, 20, 28, 52, 60, 84, 92, 116, 124, 22, 30, 54, 62, 86, 94, 118, 126 },
    { 65, 73, 97, 105, 1, 9, 33, 41, 67, 75, 99, 107, 3, 11, 35, 43, 69, 77, 101, 109, 5, 13, 37, 45, 71, 79, 103, 111, 7, 15, 39, 47 },
    { 81, 89, 113, 121, 17, 25, 49, 57, 83, 91, 115, 123, 19, 27, 51, 59, 85, 93, 117, 125, 21, 29, 53, 61, 87, 95, 119, 127, 23, 31, 55, 63 },
    { 192, 200, 224, 232, 128, 136, 160, 168, 194, 202, 226, 234, 130, 138, 162, 170, 196, 204, 228, 236, 132, 140, 164, 172, 198, 206, 230, 238, 134, 142, 166, 174 },
    { 208, 216, 240, 248, 144, 152, 176, 184, 210, 218, 242, 250, 146, 154, 178, 186, 212, 220, 244, 252, 148, 156, 180, 188, 214, 222, 246, 254, 150, 158, 182, 190 },
    { 129, 137, 161, 169, 193, 201, 225, 233, 131, 139, 163, 171, 195, 203, 227, 235, 133, 141, 165, 173, 197, 205, 229, 237, 135, 143, 167, 175, 199, 207, 231, 239 },
    { 145, 153, 177, 185, 209, 217, 241, 249, 147, 155, 179, 187, 211, 219, 243, 251, 149, 157, 181, 189, 213, 221, 245, 253, 151, 159, 183, 191, 215, 223, 247, 255 },
    { 256, 264, 288, 296, 320, 328, 352, 360, 258, 266, 290, 298, 322, 330, 354, 362, 260, 268, 292, 300, 324, 332, 356, 364, 262, 270, 294, 302, 326, 334, 358, 366 },
    { 272, 280, 304, 312, 336, 344, 368, 376, 274, 282, 306, 314, 338, 346, 370, 378, 276, 284, 308, 316, 340, 348, 372, 380, 278, 286, 310, 318, 342, 350, 374, 382 },
    { 321, 329, 353, 361, 257, 265, 289, 297, 323, 331, 355, 363, 259, 267, 291, 299, 325, 333, 357, 365, 261, 269, 293, 301, 327, 335, 359, 367, 263, 271, 295, 303 },
    { 337, 345, 369, 377, 273, 281, 305, 313, 339, 347, 371, 379, 275, 283, 307, 315, 341, 349, 373, 381, 277, 285, 309, 317, 343, 351, 375, 383, 279, 287, 311, 319 },
    { 448, 456, 480, 488, 384, 392, 416, 424, 450, 458, 482, 490, 386, 394, 418, 426, 452, 460, 484, 492, 388, 396, 420, 428, 454, 462, 486, 494, 390, 398, 422, 430 },
    { 464, 472, 496, 504, 400, 408, 432, 440, 466, 474, 498, 506, 402, 410, 434, 442, 468, 476, 500, 508, 404, 412, 436, 444, 470, 478, 502, 510, 406, 414, 438, 446 },
    { 385, 393, 417, 425, 449, 457, 481, 489, 387, 395, 419, 427, 451, 459, 483, 491, 389, 397, 421, 429, 453, 461, 485, 493, 391, 399, 423, 431, 455, 463, 487, 495 },
    { 401, 409, 433, 441, 465, 473, 497, 505, 403, 411, 435, 443, 467, 475, 499, 507, 405, 413, 437, 445, 469, 477, 501, 509, 407, 415, 439, 447, 471, 479, 503, 511 } };

/* per-format addressing (inlined into the transfer loops) */
static inline uint32_t addr32(const uint8_t (*bt)[8], uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t page = (x >> 6) + (y >> 5) * bw, block = bt[(y >> 3) & 3][(x >> 3) & 7];
    return (((bp + page * 32 + block) * 64 + columnTable32[y & 7][x & 7]) * 4) & (GS_MEM_SIZE - 1);
}
static inline uint32_t addr16(const uint8_t (*bt)[4], uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t page = (x >> 6) + (y >> 6) * bw, block = bt[(y >> 3) & 7][(x >> 4) & 3];
    return ((bp + page * 32 + block) * 256 + columnTable16[y & 7][x & 15] * 2) & (GS_MEM_SIZE - 1);
}
static inline uint32_t addr8(uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t page = (x >> 7) + (y >> 6) * (bw >> 1 ? bw >> 1 : 1), block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
    return ((bp + page * 32 + block) * 256 + columnTable8[y & 15][x & 15]) & (GS_MEM_SIZE - 1);
}
static inline uint32_t addr4(uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t page = (x >> 7) + (y >> 7) * (bw >> 1 ? bw >> 1 : 1), block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
    return ((bp + page * 32 + block) * 512 + columnTable4[y & 15][x & 31]) & (GS_MEM_SIZE * 2 - 1);
}
static const uint8_t (*bt16(int psm))[4]
{
    return psm == PSMCT16 ? blockTable16 : psm == PSMCT16S ? blockTable16S : psm == PSMZ16 ? blockTableZ16 : blockTableZ16S;
}

/* byte address for 32/24/16-bit formats, nibble address for 4-bit formats */
uint32_t gs_pixel_addr(int psm, uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    if (bw == 0) bw = 1;
    switch (psm) {
    case PSMCT32: case PSMCT24: case PSMT8H: case PSMT4HL: case PSMT4HH: return addr32(blockTable32, bp, bw, x, y);
    case PSMZ32: case PSMZ24: return addr32(blockTableZ32, bp, bw, x, y);
    case PSMCT16: case PSMCT16S: case PSMZ16: case PSMZ16S: return addr16(bt16(psm), bp, bw, x, y);
    case PSMT8: return addr8(bp, bw, x, y);
    case PSMT4: return addr4(bp, bw, x, y);
    }
    return 0;
}

uint32_t gs_read_pixel(int psm, uint32_t bp, uint32_t bw, uint32_t x, uint32_t y)
{
    uint32_t a = gs_pixel_addr(psm, bp, bw, x, y), v;
    switch (psm) {
    case PSMCT32: case PSMZ32: memcpy(&v, gs_memp + a, 4); return v;
    case PSMCT24: case PSMZ24: memcpy(&v, gs_memp + a, 4); return v & 0xFFFFFF;
    case PSMCT16: case PSMCT16S: case PSMZ16: case PSMZ16S: { uint16_t h; memcpy(&h, gs_memp + a, 2); return h; }
    case PSMT8: return gs_memp[a];
    case PSMT4: return (gs_memp[a >> 1] >> ((a & 1) * 4)) & 15;
    case PSMT8H: return gs_memp[a + 3];
    case PSMT4HL: return gs_memp[a + 3] & 15;
    case PSMT4HH: return gs_memp[a + 3] >> 4;
    }
    return 0;
}

void gs_write_pixel(int psm, uint32_t bp, uint32_t bw, uint32_t x, uint32_t y, uint32_t v)
{
    uint32_t a = gs_pixel_addr(psm, bp, bw, x, y);
    switch (psm) {
    case PSMCT32: case PSMZ32: memcpy(gs_memp + a, &v, 4); break;
    case PSMCT24: case PSMZ24: gs_memp[a] = v; gs_memp[a + 1] = v >> 8; gs_memp[a + 2] = v >> 16; break;
    case PSMCT16: case PSMCT16S: case PSMZ16: case PSMZ16S: { uint16_t h = (uint16_t)v; memcpy(gs_memp + a, &h, 2); break; }
    case PSMT8: gs_memp[a] = (uint8_t)v; break;
    case PSMT4: { uint8_t* p = &gs_memp[a >> 1]; int sh = (a & 1) * 4; *p = (uint8_t)((*p & ~(15 << sh)) | ((v & 15) << sh)); break; }
    case PSMT8H: gs_memp[a + 3] = (uint8_t)v; break;
    case PSMT4HL: gs_memp[a + 3] = (uint8_t)((gs_memp[a + 3] & 0xF0) | (v & 15)); break;
    case PSMT4HH: gs_memp[a + 3] = (uint8_t)((gs_memp[a + 3] & 0x0F) | ((v & 15) << 4)); break;
    }
#ifdef __ANDROID__
    if (gs_memp == gs_mem) page_wgen[((psm == PSMT4 ? a >> 1 : a) >> 13) & 511] = ++wgen;   /* (the upload skipping, below) */
    BLK_DIRTY(psm == PSMT4 ? a >> 1 : a);
#endif
}

static int psm_bits(int psm)
{
    switch (psm) {
    case PSMCT32: case PSMZ32: return 32;
    case PSMCT24: case PSMZ24: return 24;
    case PSMCT16: case PSMCT16S: case PSMZ16: case PSMZ16S: return 16;
    case PSMT8: case PSMT8H: return 8;
    default: return 4;
    }
}

/* ---------------- transfers ---------------- */

/* pages written by the current host->local transfer, and whether any byte changed: games
   re-upload the same textures every frame; identical data must not invalidate the cache */
static uint64_t xfer_pages[512 / 64];
#ifdef __ANDROID__
/* Android (Quest): repeated uploads skipped (gs_exec_upload below). A write generation per 8 KB page of gs_mem, and the
   pages and change of the upload chunk being run */
#define XXH_INLINE_ALL
#include "../../third_party/xxhash.h"
static uint64_t chunk_pages[512 / 64];
static int chunk_changed;
#define CHUNK_CHANGED (chunk_changed = 1)
#define CHUNK_PAGE(pg_) (chunk_pages[(pg_) >> 6] |= 1ull << ((pg_) & 63))
#else
#define CHUNK_CHANGED 1
#define CHUNK_PAGE(pg_) ((void)0)
#endif
static int xfer_changed;

static inline void write_tracked(int psm, uint32_t bp, uint32_t bw, uint32_t x, uint32_t y, uint32_t v)
{
    uint32_t a = gs_pixel_addr(psm, bp, bw, x, y), page = (psm == PSMT4 ? a >> 1 : a) >> 13;
    uint8_t* m;
    xfer_pages[page >> 6] |= 1ull << (page & 63);
    CHUNK_PAGE(page);
    switch (psm) {
    case PSMCT32: case PSMZ32: {
        uint32_t old; m = gs_memp + a; memcpy(&old, m, 4);
        if (old != v) { memcpy(m, &v, 4); xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a); }
        return;
    }
    case PSMCT16: case PSMCT16S: case PSMZ16: case PSMZ16S: {
        uint16_t old, h = (uint16_t)v; m = gs_memp + a; memcpy(&old, m, 2);
        if (old != h) { memcpy(m, &h, 2); xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a); }
        return;
    }
    case PSMT8:
        if (gs_memp[a] != (uint8_t)v) { gs_memp[a] = (uint8_t)v; xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a); }
        return;
    default:
        if (gs_read_pixel(psm, bp, bw, x, y) != v) { gs_write_pixel(psm, bp, bw, x, y, v); xfer_changed = CHUNK_CHANGED; }
        return;
    }
}

#ifdef __ANDROID__
/* Android (Quest): whole block rows of an upload chunk, in a function so that two threads can share them (the uploads
   were ~30% of a game frame's execution on the render thread; the Quest has idle cores). Each row of blocks writes its
   own blocks; pages and the changed flag are kept per job and merged after. */
#include <pthread.h>
#include <semaphore.h>
typedef struct { int fmt; const uint8_t* src; uint32_t rrw, dbp, bw, dsax, y0, nr; const uint8_t (*bt32)[8]; const uint8_t (*bt16)[4];
                 uint64_t pages[512 / 64]; int changed; unsigned nchg; } UpRows;
static unsigned up_bytes[4], up_chg;   /* uploaded in whole block rows per format (CT32, CT16, T8, T4), and of the CT32
                                          blocks the ones whose content changed (bytes), for the log */
void gs_upload_fmt_bytes(unsigned* out) { for (int k = 0; k < 4; k++) { out[k] = up_bytes[k]; up_bytes[k] = 0; } out[4] = up_chg; up_chg = 0; }
#define JMARK(j_, a_) do { uint32_t pg_ = ((a_) >> 13) & 511; (j_)->pages[pg_ >> 6] |= 1ull << (pg_ & 63); } while (0)
static void up_rows(UpRows* j)
{
    const uint32_t rrw = j->rrw, bw = j->bw;
    for (uint32_t r = 0; r < j->nr; r++) {
        if (j->fmt == 0) {   /* CT32/Z32: 8x8 blocks */
            const uint8_t* s = j->src + (size_t)r * rrw * 32;
            uint32_t y0 = j->y0 + r * 8;
            for (uint32_t bx = 0; bx < rrw; bx += 8) {
                uint32_t a = addr32(j->bt32, j->dbp, bw, j->dsax + bx, y0);
                /* columnTable32: rows 2k and 2k+1 interleaved two pixels at a time from word 16k - eight-byte copies, no
                   table (checked: (y/2)*16 + (x/2)*4 + (y%2)*2 + x%2 for all 64 entries) */
#if defined(__ARM_NEON) && !defined(RECVX_HASH_SCALAR)
                {   /* one pass: the new block straight into memory, compared with the old on the way (no staging copy,
                       no second compare pass) */
                    uint8_t* d = gs_memp + a;
                    uint8x16_t df = vdupq_n_u8(0);
                    for (uint32_t k = 0; k < 4; k++) {
                        const uint8_t* ra = s + ((size_t)(2 * k) * rrw + bx) * 4;
                        const uint8_t* rb = ra + (size_t)rrw * 4;
                        uint8_t* dk = d + 64 * k;
                        uint8x16_t A0 = vld1q_u8(ra), A1 = vld1q_u8(ra + 16), B0 = vld1q_u8(rb), B1 = vld1q_u8(rb + 16);
                        uint8x16_t o0 = vcombine_u8(vget_low_u8(A0), vget_low_u8(B0)), o1 = vcombine_u8(vget_high_u8(A0), vget_high_u8(B0));
                        uint8x16_t o2 = vcombine_u8(vget_low_u8(A1), vget_low_u8(B1)), o3 = vcombine_u8(vget_high_u8(A1), vget_high_u8(B1));
                        df = vorrq_u8(df, vorrq_u8(vorrq_u8(veorq_u8(vld1q_u8(dk), o0), veorq_u8(vld1q_u8(dk + 16), o1)),
                                                   vorrq_u8(veorq_u8(vld1q_u8(dk + 32), o2), veorq_u8(vld1q_u8(dk + 48), o3))));
                        vst1q_u8(dk, o0); vst1q_u8(dk + 16, o1); vst1q_u8(dk + 32, o2); vst1q_u8(dk + 48, o3);
                    }
                    {
                        uint32x4_t w = vreinterpretq_u32_u8(df);
                        uint32x2_t m = vorr_u32(vget_low_u32(w), vget_high_u32(w));
                        if (vget_lane_u32(m, 0) | vget_lane_u32(m, 1)) { j->changed = 1; j->nchg++; BLK_SET(a, d); }
                    }
                    JMARK(j, a);
                    continue;
                }
#else
                uint32_t blk[64];
                for (uint32_t k = 0; k < 4; k++) {
                    const uint8_t* ra = s + ((size_t)(2 * k) * rrw + bx) * 4;
                    const uint8_t* rb = ra + (size_t)rrw * 4;
                    uint8_t* o = (uint8_t*)&blk[16 * k];
                    for (uint32_t q = 0; q < 4; q++) { memcpy(o + q * 16, ra + q * 8, 8); memcpy(o + q * 16 + 8, rb + q * 8, 8); }
                }
                if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); j->changed = 1; j->nchg++; BLK_SET(a, blk); }
                JMARK(j, a);
#endif
            }
        } else if (j->fmt == 1) {   /* CT16 family: 16x8 blocks */
            const uint8_t* s = j->src + (size_t)r * rrw * 16;
            uint32_t y0 = j->y0 + r * 8;
            for (uint32_t bx = 0; bx < rrw; bx += 16) {
                uint16_t blk[128];
                uint32_t a = addr16(j->bt16, j->dbp, bw, j->dsax + bx, y0) - columnTable16[0][0] * 2;
                for (uint32_t yy = 0; yy < 8; yy++)
                    for (uint32_t xx = 0; xx < 16; xx++) memcpy(&blk[columnTable16[yy][xx]], s + ((size_t)yy * rrw + bx + xx) * 2, 2);
                if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); j->changed = 1; BLK_SET(a, blk); }
                JMARK(j, a);
            }
        } else if (j->fmt == 2) {   /* T8: 16x16 blocks */
            const uint8_t* s = j->src + (size_t)r * rrw * 16;
            uint32_t y0 = j->y0 + r * 16;
            for (uint32_t bx = 0; bx < rrw; bx += 16) {
                uint8_t blk[256];
                uint32_t a = addr8(j->dbp, bw, j->dsax + bx, y0) - columnTable8[0][0];
                for (uint32_t yy = 0; yy < 16; yy++) {
                    const uint8_t* row = s + (size_t)yy * rrw + bx;
                    for (uint32_t xx = 0; xx < 16; xx++) blk[columnTable8[yy][xx]] = row[xx];
                }
                if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); j->changed = 1; BLK_SET(a, blk); }
                JMARK(j, a);
            }
        } else {   /* T4: 32x16 blocks */
            const uint8_t* s = j->src + (size_t)r * rrw * 8;
            uint32_t y0 = j->y0 + r * 16;
            for (uint32_t bx = 0; bx < rrw; bx += 32) {
                uint8_t blk[256];
                uint32_t a = (addr4(j->dbp, bw, j->dsax + bx, y0) - columnTable4[0][0]) >> 1;
                memset(blk, 0, sizeof(blk));
                for (uint32_t yy = 0; yy < 16; yy++) {
                    const uint8_t* row = s + ((size_t)yy * rrw + bx) / 2;
                    for (uint32_t xx = 0; xx < 32; xx++) {
                        uint32_t o = columnTable4[yy][xx], v = (row[xx >> 1] >> ((xx & 1) * 4)) & 15;
                        blk[o >> 1] |= (uint8_t)(v << ((o & 1) * 4));
                    }
                }
                if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); j->changed = 1; BLK_SET(a, blk); }
                JMARK(j, a);
            }
        }
    }
}
/* the helper thread: runs the second half of a big chunk's rows while the render thread does the first */
static pthread_t up_thr;
static sem_t up_go, up_done;
static UpRows* volatile up_job;
static int up_state;   /* 0 not started, 1 running, -1 none (could not start) */
static void* up_main(void* ud)
{
    (void)ud;
    for (;;) { sem_wait(&up_go); up_rows(up_job); sem_post(&up_done); }
    return NULL;
}
static int up_helper(void)
{
    if (!up_state) {
        up_state = sem_init(&up_go, 0, 0) == 0 && sem_init(&up_done, 0, 0) == 0 && pthread_create(&up_thr, NULL, up_main, NULL) == 0 ? 1 : -1;
    }
    return up_state > 0;
}
/* the chunk's whole block rows from the current position: 1 if it handled some */
static int up_block_rows(const uint8_t* src, uint32_t bytes, uint32_t* pi, int dpsm, uint32_t dbp, uint32_t bw, uint32_t dsax, uint32_t dsay, uint32_t rrw, uint32_t rrh)
{
    int fmt = dpsm == PSMCT32 || dpsm == PSMZ32 ? 0 : dpsm == PSMCT16 || dpsm == PSMCT16S || dpsm == PSMZ16 || dpsm == PSMZ16S ? 1 :
              dpsm == PSMT8 ? 2 : dpsm == PSMT4 ? 3 : -1;
    uint32_t bwid = fmt == 0 ? 8 : fmt == 3 ? 32 : 16, bh = fmt <= 1 ? 8 : 16, rowbytes, n, i = *pi;
    if (fmt < 0 || (dsax & (bwid - 1)) || (rrw & (bwid - 1)) || gs.xfer_x || (fmt == 3 && gs.xfer_pending_bits) || ((dsay + gs.xfer_y) & (bh - 1))) return 0;
    rowbytes = fmt == 0 ? rrw * 32 : fmt == 3 ? rrw * 8 : rrw * 16;
    n = (rrh - gs.xfer_y) / bh;
    if ((bytes - i) / rowbytes < n) n = (bytes - i) / rowbytes;
    if (!n) return 0;
    up_bytes[fmt] += n * rowbytes;
    {
        UpRows a, b;
        memset(&a, 0, sizeof(a));
        a.fmt = fmt; a.src = src + i; a.rrw = rrw; a.dbp = dbp; a.bw = bw; a.dsax = dsax; a.y0 = dsay + gs.xfer_y; a.nr = n;
        a.bt32 = dpsm == PSMZ32 ? blockTableZ32 : blockTable32; a.bt16 = fmt == 1 ? bt16(dpsm) : blockTable16;
        if (n >= 2 && (size_t)n * rowbytes >= 131072 && up_helper()) {   /* (128 KB: waking the helper is a kernel call each way - 6% at 32 KB) */
            uint32_t half = n / 2;
            b = a;
            a.nr = half;
            b.src += (size_t)half * rowbytes; b.y0 += half * bh; b.nr = n - half;
            up_job = &b;
            sem_post(&up_go);
            up_rows(&a);
            sem_wait(&up_done);
            for (int w = 0; w < 8; w++) a.pages[w] |= b.pages[w];
            a.changed |= b.changed; a.nchg += b.nchg;
        } else up_rows(&a);
        if (fmt == 0) up_chg += a.nchg * 256;
        for (int w = 0; w < 8; w++) { xfer_pages[w] |= a.pages[w]; chunk_pages[w] |= a.pages[w]; }
        if (a.changed) xfer_changed = CHUNK_CHANGED;
    }
    *pi = i + n * rowbytes;
    gs.xfer_y += n * bh;
    return 1;
}
#endif

static void xfer_finish(void)
{
    gs.xfer_active = 0;
    if (gs_memp == gs_mem) {   /* scratch uploads (texture decoding) touch nothing visible */
        gs_notify_pages(xfer_pages, xfer_changed);
        stream_note_upload(xfer_pages);
    }
    memset(xfer_pages, 0, sizeof(xfer_pages));
    xfer_changed = 0;
}

#ifdef __ANDROID__
static void exec_upload_(const uint8_t* src, uint32_t bytes);
/* Android (Quest): the game uploads the same textures every frame; after the 16/8/4-bit block paths the uploads were
   still ~17% of the render thread (assembling and comparing what had not changed). A chunk with the same transfer,
   position in it, size and contents (XXH3) as one before, whose pages nothing has written since, would write what is
   there: skipped - its pages still count as uploaded, the transfer goes on where that chunk left it */
typedef struct { uint64_t bitbltbuf, trxpos, trxreg, h; uint32_t x, y, pend, bytes, gen, ax, ay, apend; uint64_t pages[512 / 64]; int done, used; } UpChunk;
static UpChunk upc[4096];   /* (a frame uploads hundreds of chunks: 256 entries thrashed) */
static unsigned up_hits, up_miss;
void gs_upload_stats(unsigned* hits, unsigned* miss) { *hits = up_hits; *miss = up_miss; up_hits = up_miss = 0; }
void gs_exec_upload(const uint8_t* src, uint32_t bytes)
{
    uint64_t h;
    UpChunk* e;
    /* off: on the Quest only 5-10% of the chunks repeated (the game moves textures around in GS memory) and XXH3 took
       9% of the render thread - more than the skips saved */
    if (1 || gs_memp != gs_mem || !gs.xfer_active || !(gs.trxreg & 0xFFF) || !bytes) { exec_upload_(src, bytes); return; }
    h = XXH3_64bits(src, bytes);
    e = &upc[(h ^ gs.bitbltbuf ^ (gs.trxpos * 31) ^ ((uint64_t)gs.xfer_y << 20) ^ gs.xfer_x) & 4095];
    if (e->used && e->h == h && e->bitbltbuf == gs.bitbltbuf && e->trxpos == gs.trxpos && e->trxreg == gs.trxreg &&
        e->x == gs.xfer_x && e->y == gs.xfer_y && e->pend == gs.xfer_pending_bits && e->bytes == bytes) {
        int same = 1;
        for (int w = 0; w < 8 && same; w++)
            for (uint64_t m = e->pages[w]; m && same; m &= m - 1)
                if (page_wgen[w * 64 + __builtin_ctzll(m)] > e->gen) same = 0;
        if (same) {
            for (int w = 0; w < 8; w++) xfer_pages[w] |= e->pages[w];
            gs.xfer_x = e->ax; gs.xfer_y = e->ay; gs.xfer_pending_bits = e->apend;
            if (e->done) xfer_finish();
            up_hits++;
            return;
        }
    }
    up_miss++;
    memset(chunk_pages, 0, sizeof(chunk_pages));
    chunk_changed = 0;
    e->used = 1; e->h = h; e->bitbltbuf = gs.bitbltbuf; e->trxpos = gs.trxpos; e->trxreg = gs.trxreg;
    e->x = gs.xfer_x; e->y = gs.xfer_y; e->pend = gs.xfer_pending_bits; e->bytes = bytes;
    exec_upload_(src, bytes);
    if (chunk_changed)
        for (int w = 0; w < 8; w++)
            for (uint64_t m = chunk_pages[w]; m; m &= m - 1) page_wgen[w * 64 + __builtin_ctzll(m)] = ++wgen;
    memcpy(e->pages, chunk_pages, sizeof(chunk_pages));
    e->gen = wgen;
    e->ax = gs.xfer_x; e->ay = gs.xfer_y; e->apend = gs.xfer_pending_bits;
    e->done = !gs.xfer_active;
}
static void exec_upload_(const uint8_t* src, uint32_t bytes)
#else
void gs_exec_upload(const uint8_t* src, uint32_t bytes)
#endif
{
    uint32_t dbp = (gs.bitbltbuf >> 32) & 0x3FFF, dbw = (gs.bitbltbuf >> 48) & 0x3F, dpsm = (gs.bitbltbuf >> 56) & 0x3F;
    uint32_t dsax = (gs.trxpos >> 32) & 0x7FF, dsay = (gs.trxpos >> 48) & 0x7FF;
    uint32_t rrw = gs.trxreg & 0xFFF, rrh = (gs.trxreg >> 32) & 0xFFF;
    int bits = psm_bits(dpsm);
    uint32_t i = 0;

    if (!gs.xfer_active || rrw == 0) return;
    {   /* fast paths: per-format addressing hoisted out of the pixel loop */
        uint32_t bw = dbw ? dbw : 1;
#define MARK(a_) do { uint32_t pg_ = (a_) >> 13; xfer_pages[pg_ >> 6] |= 1ull << (pg_ & 63); CHUNK_PAGE(pg_); } while (0)
#define ADVANCE() do { if (++gs.xfer_x >= rrw) { gs.xfer_x = 0; gs.xfer_y++; } } while (0)
#ifdef __ANDROID__
        up_block_rows(src, bytes, &i, (int)dpsm, dbp, bw, dsax, dsay, rrw, rrh);   /* (whole block rows, maybe on two threads) */
#endif
        if ((dpsm == PSMCT32 || dpsm == PSMZ32) && !(dsax & 7) && !(rrw & 7)) {
            /* whole 8x8 blocks: assemble each 256-byte block in swizzled order, compare and
               store it at once (texture streaming uploads megabytes per frame) */
            const uint8_t (*bt)[8] = dpsm == PSMCT32 ? blockTable32 : blockTableZ32;
            while (gs.xfer_x == 0 && !((dsay + gs.xfer_y) & 7) && rrh - gs.xfer_y >= 8 && bytes - i >= rrw * 32) {
                uint32_t y0 = dsay + gs.xfer_y;
                for (uint32_t bx = 0; bx < rrw; bx += 8) {
                    uint32_t blk[64], a = addr32(bt, dbp, bw, dsax + bx, y0);   /* block base: column 0 */
                    for (uint32_t yy = 0; yy < 8; yy++)
                        for (uint32_t xx = 0; xx < 8; xx++)
                            memcpy(&blk[columnTable32[yy][xx]], src + i + ((size_t)yy * rrw + bx + xx) * 4, 4);
                    if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); xfer_changed = CHUNK_CHANGED; BLK_SET(a, blk); }
                    MARK(a);
                }
                i += rrw * 32;
                gs.xfer_y += 8;
            }
        }
#ifdef __ANDROID__
        /* whole blocks of the 16-, 8- and 4-bit formats the same way (vr: the Quest's CPU spent 15-23% of the render
           thread in these uploads, pixel by pixel): one 256-byte block assembled in swizzled order, compared, stored */
        if ((dpsm == PSMCT16 || dpsm == PSMCT16S || dpsm == PSMZ16 || dpsm == PSMZ16S) && !(dsax & 15) && !(rrw & 15)) {
            const uint8_t (*bt)[4] = bt16(dpsm);
            while (gs.xfer_x == 0 && !((dsay + gs.xfer_y) & 7) && rrh - gs.xfer_y >= 8 && bytes - i >= rrw * 16) {
                uint32_t y0 = dsay + gs.xfer_y;
                for (uint32_t bx = 0; bx < rrw; bx += 16) {
                    uint16_t blk[128];
                    uint32_t a = addr16(bt, dbp, bw, dsax + bx, y0) - columnTable16[0][0] * 2;   /* block base */
                    for (uint32_t yy = 0; yy < 8; yy++)
                        for (uint32_t xx = 0; xx < 16; xx++)
                            memcpy(&blk[columnTable16[yy][xx]], src + i + ((size_t)yy * rrw + bx + xx) * 2, 2);
                    if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); xfer_changed = CHUNK_CHANGED; BLK_SET(a, blk); }
                    MARK(a);
                }
                i += rrw * 16;
                gs.xfer_y += 8;
            }
        }
        if (dpsm == PSMT8 && !(dsax & 15) && !(rrw & 15)) {
            while (gs.xfer_x == 0 && !((dsay + gs.xfer_y) & 15) && rrh - gs.xfer_y >= 16 && bytes - i >= rrw * 16) {
                uint32_t y0 = dsay + gs.xfer_y;
                for (uint32_t bx = 0; bx < rrw; bx += 16) {
                    uint8_t blk[256];
                    uint32_t a = addr8(dbp, bw, dsax + bx, y0) - columnTable8[0][0];
                    for (uint32_t yy = 0; yy < 16; yy++) {
                        const uint8_t* row = src + i + (size_t)yy * rrw + bx;
                        for (uint32_t xx = 0; xx < 16; xx++) blk[columnTable8[yy][xx]] = row[xx];
                    }
                    if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); xfer_changed = CHUNK_CHANGED; BLK_SET(a, blk); }
                    MARK(a);
                }
                i += rrw * 16;
                gs.xfer_y += 16;
            }
        }
        if (dpsm == PSMT4 && !(dsax & 31) && !(rrw & 31)) {
            while (gs.xfer_x == 0 && !gs.xfer_pending_bits && !((dsay + gs.xfer_y) & 15) && rrh - gs.xfer_y >= 16 && bytes - i >= rrw * 8) {
                uint32_t y0 = dsay + gs.xfer_y;
                for (uint32_t bx = 0; bx < rrw; bx += 32) {
                    uint8_t blk[256];
                    uint32_t a = (addr4(dbp, bw, dsax + bx, y0) - columnTable4[0][0]) >> 1;   /* (nibble address: the block's byte) */
                    memset(blk, 0, sizeof(blk));
                    for (uint32_t yy = 0; yy < 16; yy++) {
                        const uint8_t* row = src + i + ((size_t)yy * rrw + bx) / 2;
                        for (uint32_t xx = 0; xx < 32; xx++) {
                            uint32_t o = columnTable4[yy][xx], v = (row[xx >> 1] >> ((xx & 1) * 4)) & 15;   /* low nibble first */
                            blk[o >> 1] |= (uint8_t)(v << ((o & 1) * 4));
                        }
                    }
                    if (memcmp(gs_memp + a, blk, 256)) { memcpy(gs_memp + a, blk, 256); xfer_changed = CHUNK_CHANGED; BLK_SET(a, blk); }
                    MARK(a);
                }
                i += rrw * 8;
                gs.xfer_y += 16;
            }
        }
#endif
        if (dpsm == PSMCT32 || dpsm == PSMZ32) {
            const uint8_t (*bt)[8] = dpsm == PSMCT32 ? blockTable32 : blockTableZ32;
            while (i + 4 <= bytes && gs.xfer_y < rrh) {
                uint32_t a = addr32(bt, dbp, bw, dsax + gs.xfer_x, dsay + gs.xfer_y);
                if (memcmp(gs_memp + a, src + i, 4)) { memcpy(gs_memp + a, src + i, 4); xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a); }
                MARK(a); i += 4; ADVANCE();
            }
        } else if (dpsm == PSMCT16 || dpsm == PSMCT16S || dpsm == PSMZ16 || dpsm == PSMZ16S) {
            const uint8_t (*bt)[4] = bt16(dpsm);
            while (i + 2 <= bytes && gs.xfer_y < rrh) {
                uint32_t a = addr16(bt, dbp, bw, dsax + gs.xfer_x, dsay + gs.xfer_y);
                if (memcmp(gs_memp + a, src + i, 2)) { memcpy(gs_memp + a, src + i, 2); xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a); }
                MARK(a); i += 2; ADVANCE();
            }
        } else if (dpsm == PSMT8) {
            while (i < bytes && gs.xfer_y < rrh) {
                uint32_t a = addr8(dbp, bw, dsax + gs.xfer_x, dsay + gs.xfer_y);
                if (gs_memp[a] != src[i]) { gs_memp[a] = src[i]; xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a); }
                MARK(a); i++; ADVANCE();
            }
        } else if (dpsm == PSMT4) {
            while (i < bytes && gs.xfer_y < rrh) {
                uint32_t a = addr4(dbp, bw, dsax + gs.xfer_x, dsay + gs.xfer_y), sh = (a & 1) * 4;
                uint8_t v = (uint8_t)((src[i] >> (gs.xfer_pending_bits ? 4 : 0)) & 15), *p = &gs_memp[a >> 1];
                if (gs.xfer_pending_bits) { i++; gs.xfer_pending_bits = 0; } else gs.xfer_pending_bits = 1;
                if (((*p >> sh) & 15) != v) { *p = (uint8_t)((*p & ~(15 << sh)) | (v << sh)); xfer_changed = CHUNK_CHANGED; BLK_DIRTY(a >> 1); }
                MARK(a >> 1); ADVANCE();
            }
        }
#undef MARK
#undef ADVANCE
    }
    while (i < bytes && gs.xfer_y < rrh) {
        uint32_t v;
        switch (bits) {
        case 32: if (i + 4 > bytes) return; memcpy(&v, src + i, 4); i += 4; break;
        case 24: if (i + 3 > bytes) return; v = src[i] | (src[i + 1] << 8) | (src[i + 2] << 16); i += 3; break;
        case 16: if (i + 2 > bytes) return; v = src[i] | (src[i + 1] << 8); i += 2; break;
        case 8: v = src[i++]; break;
        default: /* 4-bit: low nibble first */
            v = src[i] >> (gs.xfer_pending_bits ? 4 : 0);
            if (gs.xfer_pending_bits) { i++; gs.xfer_pending_bits = 0; } else gs.xfer_pending_bits = 1;
            v &= 15;
            break;
        }
        write_tracked(dpsm, dbp, dbw, dsax + gs.xfer_x, dsay + gs.xfer_y, v);
        if (++gs.xfer_x >= rrw) { gs.xfer_x = 0; gs.xfer_y++; }
    }
    if (gs.xfer_y >= rrh) xfer_finish();
}

void gs_local_to_local(void)
{
    uint32_t sbp = gs.bitbltbuf & 0x3FFF, sbw = (gs.bitbltbuf >> 16) & 0x3F, spsm = (gs.bitbltbuf >> 24) & 0x3F;
    uint32_t dbp = (gs.bitbltbuf >> 32) & 0x3FFF, dbw = (gs.bitbltbuf >> 48) & 0x3F, dpsm = (gs.bitbltbuf >> 56) & 0x3F;
    uint32_t ssax = gs.trxpos & 0x7FF, ssay = (gs.trxpos >> 16) & 0x7FF;
    uint32_t dsax = (gs.trxpos >> 32) & 0x7FF, dsay = (gs.trxpos >> 48) & 0x7FF;
    uint32_t rrw = gs.trxreg & 0xFFF, rrh = (gs.trxreg >> 32) & 0xFFF;
    gs_flush();
    for (uint32_t y = 0; y < rrh; y++)
        for (uint32_t x = 0; x < rrw; x++)
            gs_write_pixel(dpsm, dbp, dbw, dsax + x, dsay + y, gs_read_pixel(spsm, sbp, sbw, ssax + x, ssay + y));
    gs_notify_mem_write(dbp, dbw, dpsm, dsax, dsay, rrw, rrh);
}

/* local -> host (StoreImage, render thread): rendered pixels live in OpenGL render targets,
   so the region is first read back into GS memory */
void gs_exec_download(uint8_t* dst, uint32_t bytes)
{
    ntex_before_access(0, 511);
    uint32_t sbp = gs.bitbltbuf & 0x3FFF, sbw = (gs.bitbltbuf >> 16) & 0x3F, spsm = (gs.bitbltbuf >> 24) & 0x3F;
    uint32_t ssax = gs.trxpos & 0x7FF, ssay = (gs.trxpos >> 16) & 0x7FF;
    uint32_t rrw = gs.trxreg & 0xFFF, rrh = (gs.trxreg >> 32) & 0xFFF;
    int bits = psm_bits(spsm);
    uint32_t i = 0, x = 0, y = 0;
    gs_flush();
    gs_rt_to_mem(sbp, sbw, spsm, ssax, ssay, rrw, rrh);
    while (rrw && y < rrh && i + (bits >= 8 ? (uint32_t)bits / 8 : 1) <= bytes) {   /* the rectangle, at most the destination */
        uint32_t v = gs_read_pixel(spsm, sbp, sbw, ssax + x, ssay + y);
        if (bits == 32) { memcpy(dst + i, &v, 4); i += 4; }
        else if (bits == 24) { dst[i] = v; dst[i + 1] = v >> 8; dst[i + 2] = v >> 16; i += 3; }
        else if (bits == 16) { dst[i] = v; dst[i + 1] = v >> 8; i += 2; }
        else dst[i++] = (uint8_t)v;
        if (++x >= rrw) { x = 0; y++; }
    }
}

/* ---------------- texture decode ---------------- */

static uint32_t expand16(uint32_t c, uint64_t texa)
{
    uint32_t r = (c & 0x1F) << 3, g = ((c >> 5) & 0x1F) << 3, b = ((c >> 10) & 0x1F) << 3;
    uint32_t ta0 = texa & 0xFF, ta1 = (texa >> 32) & 0xFF, aem = (texa >> 15) & 1;
    uint32_t a = (c & 0x8000) ? ta1 : ((aem && !(c & 0x7FFF)) ? 0 : ta0);
    return r | (g << 8) | (b << 16) | (a << 24);
}

static uint32_t to_rgba(int psm, uint32_t v, uint64_t texa, int tcc)
{
    switch (psm) {
    case PSMCT32: case PSMZ32: return v;
    case PSMCT24: case PSMZ24: {
        uint32_t aem = (texa >> 15) & 1, ta0 = texa & 0xFF;
        uint32_t a = (aem && !(v & 0xFFFFFF)) ? 0 : ta0;
        return (v & 0xFFFFFF) | (a << 24);
    }
    default: return expand16(v, texa);
    }
    (void)tcc;
}

uint32_t gs_texel_rgba(int psm, uint32_t v, uint64_t texa) { return to_rgba(psm, v, texa, 0); }

/* ---------------- CLUT buffer (GS internal, 512 x 16 bit) ---------------- */
static uint16_t clut_buf[512];
static uint32_t cbp0, cbp1;
static uint32_t clut_gen = 1;   /* changes whenever clut_buf may have (gs_gl.c clut_hash's memo) */
uint32_t gs_clut_gen(void) { return clut_gen; }

/* memory position of CLUT entry i in CSM1 storage */
void gs_clut_state_save(uint8_t* out)
{
    memcpy(out, clut_buf, sizeof(clut_buf));
    memcpy(out + sizeof(clut_buf), &cbp0, 4);
    memcpy(out + sizeof(clut_buf) + 4, &cbp1, 4);
}
void gs_clut_state_restore(const uint8_t* in)
{
    clut_gen++;
    memcpy(clut_buf, in, sizeof(clut_buf));
    memcpy(&cbp0, in + sizeof(clut_buf), 4);
    memcpy(&cbp1, in + sizeof(clut_buf) + 4, 4);
}

static void clut_pos(int entries, uint32_t i, uint32_t* x, uint32_t* y)
{
    if (entries == 16) { *x = i & 7; *y = i >> 3; return; }
    *x = (i & 7) | ((i & 0x10) >> 1);
    *y = ((i & 0x08) >> 3) | ((i & 0xE0) >> 4);
}

/* Handle the CLD field of a TEX0 write. */
void gs_clut_load(uint64_t tex0)
{
    uint32_t psm = (tex0 >> 20) & 0x3F, cbp = (tex0 >> 37) & 0x3FFF, cpsm = (tex0 >> 51) & 0xF;
    uint32_t csa = (tex0 >> 56) & 0x1F, cld = (tex0 >> 61) & 7;
    int entries, load = 0;
    if (psm == PSMT8 || psm == PSMT8H) entries = 256;
    else if (psm == PSMT4 || psm == PSMT4HL || psm == PSMT4HH) entries = 16;
    else return;
    switch (cld) {
    case 1: load = 1; break;
    case 2: load = 1; cbp0 = cbp; break;
    case 3: load = 1; cbp1 = cbp; break;
    case 4: load = cbp != cbp0; cbp0 = cbp; break;
    case 5: load = cbp != cbp1; cbp1 = cbp; break;
    }
    if (!load) return;
    gs_flush();
    clut_gen++;
    ntex_before_access(cbp / 32, cbp / 32 + 1);
#ifdef __ANDROID__
    {   /* Android (Quest): the entries' addresses from a table - within the CLUT they are cbp * 256 plus a fixed offset (the
           profiler: gs_clut_load and gs_read_pixel ~8% of the render thread, 256 address computations a load) */
        static uint32_t off[3][256];   /* CT32, CT16, CT16S; for 256 and 16 entries the same positions (clut_pos) */
        static int init;
        int k = cpsm == 0 ? 0 : cpsm == 2 ? 1 : 2;
        if (!init) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t x, y;
                clut_pos(256, i, &x, &y);
                off[0][i] = gs_pixel_addr(PSMCT32, 0, 1, x, y);
                off[1][i] = gs_pixel_addr(PSMCT16, 0, 1, x, y);
                off[2][i] = gs_pixel_addr(PSMCT16S, 0, 1, x, y);
            }
            init = 1;
        }
        for (uint32_t i = 0; i < (uint32_t)entries; i++) {
            uint32_t x, y, j = i;
            if (entries == 16) { clut_pos(16, i, &x, &y); j = 0; }   /* (16 entries: the 8x2 layout, looked up directly) */
            {
                uint32_t a = entries == 16 ? gs_pixel_addr(k == 0 ? PSMCT32 : k == 1 ? PSMCT16 : PSMCT16S, cbp, 1, x, y)
                                           : (cbp * 256 + off[k][j]) & (GS_MEM_SIZE - 1);
                if (k == 0) {
                    uint32_t c, e = (csa * 16 + i) & 255;
                    memcpy(&c, gs_memp + a, 4);
                    clut_buf[e] = (uint16_t)c; clut_buf[256 + e] = (uint16_t)(c >> 16);
                } else {
                    uint16_t h;
                    memcpy(&h, gs_memp + a, 2);
                    clut_buf[(csa * 16 + i) & 511] = h;
                }
            }
        }
        return;
    }
#endif
    for (uint32_t i = 0; i < (uint32_t)entries; i++) {
        uint32_t x, y;
        clut_pos(entries, i, &x, &y);
        if (cpsm == 0) {
            uint32_t c = gs_read_pixel(PSMCT32, cbp, 1, x, y), e = (csa * 16 + i) & 255;
            clut_buf[e] = (uint16_t)c; clut_buf[256 + e] = (uint16_t)(c >> 16);
        } else {
            clut_buf[(csa * 16 + i) & 511] = (uint16_t)gs_read_pixel(cpsm == 2 ? PSMCT16 : PSMCT16S, cbp, 1, x, y);
        }
    }
}

static uint32_t clut_lookup(uint64_t tex0, uint64_t texa, uint32_t idx)
{
    uint32_t cpsm = (tex0 >> 51) & 0xF, csa = (tex0 >> 56) & 0x1F;
    if (cpsm == 0) {
        uint32_t e = (csa * 16 + idx) & 255;
        return clut_buf[e] | ((uint32_t)clut_buf[256 + e] << 16);
    }
    return expand16(clut_buf[(csa * 16 + idx) & 511], texa);
}

void gs_get_palette(uint64_t tex0, uint64_t texa, uint32_t* out, int n)
{
    for (int i = 0; i < n; i++) out[i] = clut_lookup(tex0, texa, (uint32_t)i);
}

void gs_decode_texture(uint64_t tex0, uint64_t texa, uint32_t w, uint32_t h, uint32_t* out)
{
    uint32_t tbp = tex0 & 0x3FFF, tbw = (tex0 >> 14) & 0x3F, psm = (tex0 >> 20) & 0x3F, tcc = (tex0 >> 34) & 1;
    uint32_t pal[256];
    int indexed = (psm == PSMT8 || psm == PSMT4 || psm == PSMT8H || psm == PSMT4HL || psm == PSMT4HH);
    if (indexed) {
        int n = (psm == PSMT8 || psm == PSMT8H) ? 256 : 16;
        for (int i = 0; i < n; i++) pal[i] = clut_lookup(tex0, texa, (uint32_t)i);
    }
#ifdef __ANDROID__
    /* Android (Quest): block by block - the block's address once, the pixels in it from the swizzle tables (each pixel
       through gs_read_pixel computed its whole address; with the CLUT loads ~7-9% of a game frame's execution) */
    if ((psm == PSMCT32 || psm == PSMCT16 || psm == PSMCT16S || psm == PSMT8 || psm == PSMT4) &&
        !columnTable32[0][0] && !columnTable16[0][0] && !columnTable8[0][0] && !columnTable4[0][0]) {
        uint32_t bw = tbw ? tbw : 1;
        uint32_t BW = psm == PSMCT32 ? 8 : psm == PSMT4 ? 32 : 16, BH = psm == PSMCT32 || psm == PSMCT16 || psm == PSMCT16S ? 8 : 16;
        const uint8_t (*b16)[4] = psm == PSMCT16S ? blockTable16S : blockTable16;
        for (uint32_t by = 0; by < h; by += BH)
            for (uint32_t bx = 0; bx < w; bx += BW) {
                uint32_t ym = h - by < BH ? h - by : BH, xm = w - bx < BW ? w - bx : BW;
                if (psm == PSMCT32) {
                    uint32_t base = addr32(blockTable32, tbp, bw, bx, by);
                    for (uint32_t yy = 0; yy < ym; yy++)
                        for (uint32_t xx = 0; xx < xm; xx++) {
                            uint32_t v;
                            memcpy(&v, gs_memp + ((base + columnTable32[yy][xx] * 4u) & (GS_MEM_SIZE - 1)), 4);
                            out[(by + yy) * w + bx + xx] = to_rgba(psm, v, texa, tcc);
                        }
                } else if (psm == PSMCT16 || psm == PSMCT16S) {
                    uint32_t base = addr16(b16, tbp, bw, bx, by);
                    for (uint32_t yy = 0; yy < ym; yy++)
                        for (uint32_t xx = 0; xx < xm; xx++) {
                            uint16_t v;
                            memcpy(&v, gs_memp + ((base + columnTable16[yy][xx] * 2u) & (GS_MEM_SIZE - 1)), 2);
                            out[(by + yy) * w + bx + xx] = to_rgba(psm, v, texa, tcc);
                        }
                } else if (psm == PSMT8) {
                    uint32_t base = addr8(tbp, bw, bx, by);
                    for (uint32_t yy = 0; yy < ym; yy++)
                        for (uint32_t xx = 0; xx < xm; xx++)
                            out[(by + yy) * w + bx + xx] = pal[gs_memp[(base + columnTable8[yy][xx]) & (GS_MEM_SIZE - 1)]];
                } else {
                    uint32_t base = addr4(tbp, bw, bx, by);   /* (nibble address) */
                    for (uint32_t yy = 0; yy < ym; yy++)
                        for (uint32_t xx = 0; xx < xm; xx++) {
                            uint32_t a = (base + columnTable4[yy][xx]) & (GS_MEM_SIZE * 2 - 1);
                            out[(by + yy) * w + bx + xx] = pal[(gs_memp[a >> 1] >> ((a & 1) * 4)) & 15];
                        }
                }
            }
        return;
    }
#endif
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) {
            uint32_t v = gs_read_pixel(psm, tbp, tbw, x, y);
            uint32_t c = indexed ? pal[v] : to_rgba(psm, v, texa, tcc);
            out[y * w + x] = c;
        }
}

/* ---------------- self test: swizzle tables must be permutations ---------------- */
int gs_mem_selftest(void)
{
    static uint8_t seen[512];
    memset(seen, 0, sizeof(seen));
    for (int y = 0; y < 16; y++) for (int x = 0; x < 32; x++) { if (seen[columnTable4[y][x]]++) return 0; }
    memset(seen, 0, sizeof(seen));
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { if (seen[columnTable8[y][x]]++) return 0; }
    memset(seen, 0, sizeof(seen));
    for (int y = 0; y < 8; y++) for (int x = 0; x < 16; x++) { if (seen[columnTable16[y][x]]++) return 0; }
    memset(seen, 0, sizeof(seen));
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { if (seen[columnTable32[y][x]]++) return 0; }
    return 1;
}
