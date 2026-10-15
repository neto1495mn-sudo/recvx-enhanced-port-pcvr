/*
 * Texture replacements (HD texture packs made for PCSX2).
 *
 * A pack is a folder of images named as PCSX2 names its texture dumps:
 *   <texture hash>-<palette hash>-<bits>.dds|png   (palette formats)
 *   <texture hash>-<bits>.dds|png                  (direct formats)
 * <bits> holds TEX0 PSM (6 bits), TW (4), TH (4), then TEXA TA0 (8, from bit 15), AEM (1),
 * TA1 (8) for the 16/24-bit formats. The texture hash is XXH3-64 over what the GS memory holds
 * for the texture: its whole blocks in raster block order when the texture covers whole blocks
 * of a format whose bits are all texture (CT32, T8, T4), else the texels expanded row by row
 * (palette indices, one byte each, or 32-bit colours as the GS reads them with TEXA). The
 * palette hash is XXH3-64 over the 16 or 256 32-bit palette entries the draw sees. These are the
 * rules of PCSX2's texture cache (GSTextureCache HashTextureLevel / HashCacheKey::Create),
 * reimplemented here on our GS memory.
 *
 * Files are looked for under "textures" next to the executable (any depth, so the pack can sit
 * in textures/SLUS-20184/replacements as for PCSX2). Images keep the GS alpha range (0x80 =
 * opaque), as PCSX2 loads them.
 *
 * The native texture decode (textures.c decode) asks here first; a match is uploaded once (all
 * its mipmap levels, or generated ones for PNG) and shared, drawn with trilinear filtering.
 */
#include "gs.h"
#include "texreplace.h"
#include "../render/gl_loader.h"
#include "../ps2_platform.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define XXH_INLINE_ALL
#include "../../third_party/xxhash.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "../../third_party/stb_image.h"

int g_tex_hd = 1;        /* ini tex_hd: use the packs */
int g_tex_hd_log = 0;    /* ini tex_hd_log: log the names of textures without a replacement */
int g_tex_hd_mb = 600;   /* ini tex_hd_mb: video memory for replacements (MB) */
int g_tex_hd_max = 1024; /* ini tex_hd_max: largest size kept (bigger images are reduced) */

#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

typedef struct {
    uint64_t h, clut;
    uint32_t bits;
    char* path;
    GLuint tex;          /* uploaded (0: not yet, or failed) */
    uint32_t bytes, last;
    int failed, w, ht, mips, queued;
} Entry;

static Entry* tab;
static uint32_t tab_cap, tab_n, skipped;
static int inited;
static uint64_t used_bytes;
static uint8_t hd_kind[65536];   /* GL name -> 1: replacement without mipmaps, 2: with */
static uint32_t n_lookups, n_hits;

static uint64_t mix(uint64_t h, uint64_t c, uint32_t b) { uint64_t k = h ^ (c * 0x9E3779B97F4A7C15ull) ^ ((uint64_t)b << 17); k ^= k >> 33; k *= 0xff51afd7ed558ccdull; k ^= k >> 33; return k; }

static Entry* find(uint64_t h, uint64_t clut, uint32_t bits)
{
    if (!tab_cap) return NULL;
    for (uint32_t i = (uint32_t)mix(h, clut, bits) & (tab_cap - 1);; i = (i + 1) & (tab_cap - 1)) {
        Entry* e = &tab[i];
        if (!e->path) return NULL;
        if (e->h == h && e->clut == clut && e->bits == bits) return e;
    }
}

static void insert(uint64_t h, uint64_t clut, uint32_t bits, const char* path)
{
    if (tab_n * 2 >= tab_cap) {   /* grow */
        uint32_t oc = tab_cap;
        Entry* o = tab;
        tab_cap = oc ? oc * 2 : 4096;
        tab = calloc(tab_cap, sizeof(Entry));
        if (!tab) { tab = o; tab_cap = oc; return; }
        tab_n = 0;
        for (uint32_t i = 0; i < oc; i++) if (o[i].path) {
            Entry* e;
            uint32_t j = (uint32_t)mix(o[i].h, o[i].clut, o[i].bits) & (tab_cap - 1);
            while (tab[j].path) j = (j + 1) & (tab_cap - 1);
            e = &tab[j]; *e = o[i]; tab_n++;
        }
        free(o);
    }
    if (find(h, clut, bits)) return;   /* the first one found wins */
    {
        uint32_t j = (uint32_t)mix(h, clut, bits) & (tab_cap - 1);
        while (tab[j].path) j = (j + 1) & (tab_cap - 1);
        tab[j].h = h; tab[j].clut = clut; tab[j].bits = bits;
        tab[j].path = strdup(path);
        tab_n++;
    }
}

/* "<hex>-<hex>-<hex>.ext" or "<hex>-<hex>.ext"; region names (-r) and mipmap files are skipped */
static int parse_hex(const char** s, uint64_t* v)
{
    const char* p = *s;
    uint64_t x = 0;
    int n = 0;
    for (;; p++, n++) {
        int c = *p, d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        if (n >= 16) return 0;
        x = x << 4 | (uint64_t)d;
    }
    if (!n) return 0;
    *s = p; *v = x;
    return 1;
}

static void add_file(const char* dir, const char* name)
{
    uint64_t v[3];
    int n = 0;
    const char* s = name;
    char path[1024];
    while (n < 3 && parse_hex(&s, &v[n])) {
        n++;
        if (*s == '-') s++;
        else break;
    }
    if (*s != '.' || n < 2 || (SDL_strcasecmp(s, ".dds") && SDL_strcasecmp(s, ".png"))) { skipped++; return; }
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    {   /* as PCSX2 reads the bits: bit 14 (once TCC) is unused, TEXA only counts for 16/24-bit formats */
        uint32_t bits = (uint32_t)v[n - 1] & ~0x4000u, psm = bits & 0x3F;
        if (psm == PSMT8 || psm == PSMT4 || psm == PSMT8H || psm == PSMT4HL || psm == PSMT4HH || psm == PSMCT32 || psm == PSMZ32) bits &= 0x3FFF;
        if (n == 3) insert(v[0], v[1], bits, path);
        else insert(v[0], 0, bits, path);
    }
}

static void scan(const char* dir, int depth)
{
    DIR* d = opendir(dir);
    struct dirent* de;
    if (!d) return;
    while ((de = readdir(d))) {
        char p[1024];
        struct stat st;
        if (de->d_name[0] == '.') continue;
        snprintf(p, sizeof(p), "%s/%s", dir, de->d_name);
        if (stat(p, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) { if (depth < 6) scan(p, depth + 1); }
        else add_file(dir, de->d_name);
    }
    closedir(d);
}

static void init(void)
{
    const char* base = SDL_GetBasePath();
    char dir[1024];
    Uint64 t0 = SDL_GetTicks();
    inited = 1;
    snprintf(dir, sizeof(dir), "%stextures", base ? base : "");
    scan(dir, 0);
    if (!tab_n && base) scan("textures", 0);   /* (started from another folder) */
    host_log("tex: %u replacement textures found in %s (%u other files) in %u ms\n", tab_n, dir, skipped, (unsigned)(SDL_GetTicks() - t0));
}

int trep_active(void)
{
    if (!g_tex_hd) return 0;
    if (!inited) init();
    return tab_n != 0 || g_tex_hd_log;
}

/* ---------------- the PCSX2 names of a texture ---------------- */

static int is_pal(uint32_t psm) { return psm == PSMT8 || psm == PSMT4 || psm == PSMT8H || psm == PSMT4HL || psm == PSMT4HH; }

static uint64_t texture_hash(uint64_t tex0, uint64_t texa)
{
    uint32_t tbp = tex0 & 0x3FFF, tbw = (tex0 >> 14) & 0x3F, psm = (tex0 >> 20) & 0x3F;
    uint32_t tw = 1u << (((tex0 >> 26) & 0xF) > 10 ? 10 : ((tex0 >> 26) & 0xF));
    uint32_t th = 1u << (((tex0 >> 30) & 0xF) > 10 ? 10 : ((tex0 >> 30) & 0xF));
    uint32_t bx = 8, by = 8;
    int whole_bits = psm == PSMCT32 || psm == PSMT8 || psm == PSMT4 || psm == PSMZ32;
    XXH3_state_t st;
    if (psm == PSMCT16 || psm == PSMCT16S || psm == PSMZ16 || psm == PSMZ16S) { bx = 16; by = 8; }
    else if (psm == PSMT8) { bx = 16; by = 16; }
    else if (psm == PSMT4) { bx = 32; by = 16; }
    XXH3_64bits_reset(&st);
    if (tw < bx || th < by || !whole_bits) {   /* expanded, row by row */
        static uint32_t row[1024];
        int pal = is_pal(psm);
        for (uint32_t y = 0; y < th; y++) {
            if (pal) {
                uint8_t* r8 = (uint8_t*)row;
                for (uint32_t x = 0; x < tw; x++) r8[x] = (uint8_t)gs_read_pixel((int)psm, tbp, tbw, x, y);
                XXH3_64bits_update(&st, row, tw);
            } else {
                for (uint32_t x = 0; x < tw; x++) row[x] = gs_texel_rgba((int)psm, gs_read_pixel((int)psm, tbp, tbw, x, y), texa);
                XXH3_64bits_update(&st, row, (size_t)tw * 4);
            }
        }
    } else {   /* the blocks as they are in GS memory */
        for (uint32_t y = 0; y < th; y += by)
            for (uint32_t x = 0; x < tw; x += bx) {
                uint32_t a = gs_pixel_addr((int)psm, tbp, tbw ? tbw : 1, x, y);
                if (psm == PSMT4) a >>= 1;
                XXH3_64bits_update(&st, gs_memp + (a & ~255u & (GS_MEM_SIZE - 1)), 256);
            }
    }
    return XXH3_64bits_digest(&st);
}

static void names(uint64_t tex0, uint64_t texa, uint64_t* h, uint64_t* clut, uint32_t* bits)
{
    uint32_t psm = (tex0 >> 20) & 0x3F, b = psm | (uint32_t)((tex0 >> 26) & 0xF) << 6 | (uint32_t)((tex0 >> 30) & 0xF) << 10;
    *h = texture_hash(tex0, texa);
    *clut = 0;
    if (is_pal(psm)) {
        uint32_t pal[256];
        int n = (psm == PSMT8 || psm == PSMT8H) ? 256 : 16;
        gs_get_palette(tex0, texa, pal, n);
        *clut = XXH3_64bits(pal, (size_t)n * 4);
    } else if (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S || psm == PSMZ24 || psm == PSMZ16 || psm == PSMZ16S) {
        b |= (uint32_t)(texa & 0xFF) << 15 | (uint32_t)((texa >> 15) & 1) << 23 | (uint32_t)((texa >> 32) & 0xFF) << 24;
    }
    *bits = b;
}

/* ---------------- loading ---------------- */

static uint8_t* read_file(const char* path, size_t* len)
{
    FILE* f = fopen(path, "rb");
    uint8_t* d = NULL;
    long n;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0 && (d = malloc((size_t)n))) {
        if (fread(d, 1, (size_t)n, f) != (size_t)n) { free(d); d = NULL; }
        else *len = (size_t)n;
    }
    fclose(f);
    return d;
}

static uint32_t rd32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

/* a DDS: BC1/2/3/7 or 32-bit RGBA/BGRA, its mipmap levels; the GL texture or 0 */
static GLuint load_dds(const uint8_t* d, size_t len, uint32_t* bytes, int* mips, int* ow, int* oh)
{
    uint32_t h, w, nmip, pf_flags, fourcc, bpp, rmask, off = 128, fmt = 0, block = 0;
    int bgra = 0;
    GLuint t;
    if (len < 128 || memcmp(d, "DDS ", 4)) return 0;
    h = rd32(d + 12); w = rd32(d + 16); nmip = rd32(d + 28); if (!(rd32(d + 8) & 0x20000) || !nmip) nmip = 1;
    pf_flags = rd32(d + 80); fourcc = rd32(d + 84); bpp = rd32(d + 88); rmask = rd32(d + 92);
    if (pf_flags & 4) {
        if (fourcc == 0x31545844) { fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; block = 8; }        /* DXT1 */
        else if (fourcc == 0x33545844 || fourcc == 0x32545844) { fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; block = 16; }   /* DXT3/2 */
        else if (fourcc == 0x35545844 || fourcc == 0x34545844) { fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; block = 16; }   /* DXT5/4 */
        else if (fourcc == 0x30315844 && len >= 148) {   /* DX10 */
            uint32_t dx = rd32(d + 128);
            off = 148;
            if (dx == 70 || dx == 71 || dx == 72) { fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; block = 8; }
            else if (dx >= 73 && dx <= 75) { fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; block = 16; }
            else if (dx >= 76 && dx <= 78) { fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; block = 16; }
            else if (dx >= 97 && dx <= 99) { fmt = GL_COMPRESSED_RGBA_BPTC_UNORM; block = 16; }
            else if (dx == 28 || dx == 29) bpp = 32, rmask = 0xFF;
            else if (dx == 87 || dx == 91) bpp = 32, rmask = 0xFF0000;
            else return 0;
        } else return 0;
    }
    if (!fmt) {
        if (bpp != 32) return 0;
        bgra = rmask == 0xFF0000;
    }
    if (fmt && !glCompressedTexImage2D) return 0;
    if (!w || !h || w > 16384 || h > 16384) return 0;
    *ow = (int)w; *oh = (int)h;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    *bytes = 0;
    {
        uint32_t lw = w, lh = h, i;
        for (i = 0; i < nmip && i < 16; i++) {
            size_t sz = fmt ? (size_t)((lw + 3) / 4) * ((lh + 3) / 4) * block : (size_t)lw * lh * 4;
            if (off + sz > len) break;
            if (fmt) glCompressedTexImage2D(GL_TEXTURE_2D, (GLint)i, fmt, (GLsizei)lw, (GLsizei)lh, 0, (GLsizei)sz, d + off);
            else glTexImage2D(GL_TEXTURE_2D, (GLint)i, GL_RGBA8, (GLsizei)lw, (GLsizei)lh, 0, bgra ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, d + off);
            off += (uint32_t)sz; *bytes += (uint32_t)sz;
            if (lw == 1 && lh == 1) { i++; break; }
            lw = lw > 1 ? lw / 2 : 1; lh = lh > 1 ? lh / 2 : 1;
        }
        if (!i) { glDeleteTextures(1, &t); return 0; }
        if (i == 1 && !fmt && glGenerateMipmap) { glGenerateMipmap(GL_TEXTURE_2D); *bytes += *bytes / 3; i = 2; }
        else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)i - 1);
        *mips = i > 1;
    }
    return t;
}

static GLuint load_png(const uint8_t* d, size_t len, uint32_t* bytes, int* mips, int* ow, int* oh)
{
    int w, h, n;
    uint8_t* px = stbi_load_from_memory(d, (int)len, &w, &h, &n, 4);
    GLuint t;
    if (!px) return 0;
    *ow = w; *oh = h;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    *bytes = (uint32_t)w * (uint32_t)h * 4;
    *mips = 0;
    if (glGenerateMipmap) { glGenerateMipmap(GL_TEXTURE_2D); *bytes += *bytes / 3; *mips = 1; }
    else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    return t;
}

/* video memory: the replacements not drawn for a while go first */
static void make_room(uint32_t need)
{
    uint64_t budget = (uint64_t)(g_tex_hd_mb > 16 ? g_tex_hd_mb : 16) << 20;
    uint32_t now = (uint32_t)SDL_GetTicks();
    while (used_bytes + need > budget) {
        Entry* o = NULL;
        for (uint32_t i = 0; i < tab_cap; i++)
            if (tab[i].tex && now - tab[i].last > 2000 && (!o || tab[i].last < o->last)) o = &tab[i];
        if (!o) break;   /* everything is in use: over budget rather than flicker */
        ntex_drop_gl(o->tex);
        gs_texcache_drop_gl(o->tex);
        if (o->tex < 65536) hd_kind[o->tex] = 0;
        glDeleteTextures(1, &o->tex);
        used_bytes -= o->bytes;
        o->tex = 0; o->bytes = 0;
    }
}

/* replacements without mipmaps or too big: redrawn at the next frame start (gs_gl.c) */
#define MAX_QUEUE 256
static Entry* queue[MAX_QUEUE];
static int nqueue;
static void queue_convert(Entry* e)
{
    if (e->queued || nqueue >= MAX_QUEUE) return;
    e->queued = 1;
    queue[nqueue++] = e;
}

void trep_frame_begin(void)
{
    int done = 0;
    while (nqueue && done < 4) {   /* a few per frame: no long hitch on entering a room */
        Entry* e = queue[0];
        int dw = e->w, dh = e->ht;
        memmove(queue, queue + 1, (size_t)(--nqueue) * sizeof(queue[0]));
        e->queued = 0;
        if (!e->tex) continue;   /* (dropped meanwhile) */
        while (dw > g_tex_hd_max || dh > g_tex_hd_max) { dw = dw > 1 ? dw / 2 : 1; dh = dh > 1 ? dh / 2 : 1; }
        {
            GLuint t = gs_tex_to_rgba_mips(e->tex, dw, dh);
            uint32_t nb = (uint32_t)dw * (uint32_t)dh * 4 / 3 * 4;
            if (!t) continue;
            ntex_drop_gl(e->tex);
            gs_texcache_drop_gl(e->tex);
            if (e->tex < 65536) hd_kind[e->tex] = 0;
            glDeleteTextures(1, &e->tex);
            used_bytes -= e->bytes;
            e->tex = t; e->bytes = nb; e->w = dw; e->ht = dh; e->mips = 1;
            used_bytes += nb;
            if (t < 65536) hd_kind[t] = 2;
        }
        done++;
    }
}

unsigned int trep_lookup(uint64_t tex0, uint64_t texa)
{
    uint64_t h, clut;
    uint32_t bits;
    Entry* e;
    names(tex0, texa, &h, &clut, &bits);
    n_lookups++;
    e = find(h, clut, bits);
    if (!e) {
        static int nlog;
        if (g_tex_hd_log && nlog < 2000) {
            nlog++;
            if (clut) host_log("tex: no replacement for %llx-%llx-%08x\n", (unsigned long long)h, (unsigned long long)clut, bits);
            else host_log("tex: no replacement for %llx-%08x\n", (unsigned long long)h, bits);
        }
        if ((n_lookups % 200) == 0) host_log("tex: %u textures seen, %u replaced\n", n_lookups, n_hits);
        return 0;
    }
    e->last = (uint32_t)SDL_GetTicks();
    if (!e->tex && !e->failed) {
        size_t len = 0;
        uint8_t* d = read_file(e->path, &len);
        int mips = 0;
        uint32_t bytes = 0;
        GLuint t = 0;
        if (d) {
            t = (len > 4 && !memcmp(d, "DDS ", 4)) ? load_dds(d, len, &bytes, &mips, &e->w, &e->ht) : load_png(d, len, &bytes, &mips, &e->w, &e->ht);
            free(d);
        }
        if (!t) { e->failed = 1; host_log("tex: could not load %s\n", e->path); return 0; }
        make_room(bytes);
        e->tex = t; e->bytes = bytes; used_bytes += bytes; e->mips = mips;
        if (t < 65536) hd_kind[t] = (uint8_t)(mips ? 2 : 1);
        if (!mips || e->w > g_tex_hd_max || e->ht > g_tex_hd_max) queue_convert(e);
        {
            static int nlog;
            if (nlog++ < 100) host_log("tex: HD %s (%u KB, %u MB in use)\n", e->path, bytes >> 10, (unsigned)(used_bytes >> 20));
        }
    }
    if (!e->tex) return 0;
    n_hits++;
    if ((n_lookups % 200) == 0) host_log("tex: %u textures seen, %u replaced\n", n_lookups, n_hits);
    return e->tex;
}

int trep_kind(unsigned int gl_tex) { return gl_tex < 65536 ? hd_kind[gl_tex] : 0; }
