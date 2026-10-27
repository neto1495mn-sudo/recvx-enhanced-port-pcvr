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
int g_tex_loaders = 4;   /* ini tex_loaders: threads reading the replacement files (vr160: 4..6) */
int g_tex_upload = 0;    /* ini tex_upload_thread: the upload to the video card on a thread of its own (vr160): 0 by the card, 1 on, 2 off */
int g_tex_hd_per_frame = 2;   /* ini tex_hd_per_frame: replacements converted per frame (vr119: 4 made frames of 25-38 ms on entering a room) */

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
    /* vr157: read in the background (aikatore's hdpack.c 1352f3b1, here for our pack); job under jm */
    int job;             /* J_NONE, J_QUEUED (in jq), J_RUNNING, J_DONE (in dq) */
    uint8_t* data; size_t len;   /* the file read, until uploaded */
    GLuint native[8];    /* the original decodes drawn meanwhile (dropped when the HD one is in; vr158: all of them -
                            the game can hold the same texture in several copies, and one left kept switching back) */
    int nnative;
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

/* ignorar.txt: one pack file name per line (or its first part: the texture hash, with or
   without the palette hash) - those textures stay as the game's own */
#define MAX_IGNORE 1024
static struct { uint64_t h, clut; int any_clut; } ign[MAX_IGNORE];
static int nign;
static void read_ignore(const char* path)
{
    FILE* f = fopen(path, "rb");
    char line[512];
    if (!f) return;
    while (fgets(line, sizeof(line), f) && nign < MAX_IGNORE) {
        const char* s = line;
        uint64_t h, c;
        while (*s == ' ' || *s == '\t' || *s == '\xEF' || *s == '\xBB' || *s == '\xBF') s++;   /* (a UTF-8 mark from Notepad) */
        if (!parse_hex(&s, &h)) continue;
        ign[nign].h = h; ign[nign].any_clut = 1;
        if (*s == '-') {
            const char* t = s + 1;
            const char* u = t;
            if (parse_hex(&t, &c) && *t == '-') { ign[nign].clut = c; ign[nign].any_clut = 0; }   /* h-clut-bits */
            (void)u;
        }
        nign++;
    }
    fclose(f);
    host_log("tex: %s: %d textures left as the game's own\n", path, nign);
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
        else if (!SDL_strcasecmp(de->d_name, "ignorar.txt") || !SDL_strcasecmp(de->d_name, "ignore.txt")) read_ignore(p);
        else add_file(dir, de->d_name);
    }
    closedir(d);
}

static void loader_start(void);
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
    for (int k = 0; k < nign; k++)   /* the ignore list wins over the pack */
        for (uint32_t i = 0; i < tab_cap; i++)
            if (tab[i].path && tab[i].h == ign[k].h && (ign[k].any_clut || tab[i].clut == ign[k].clut)) tab[i].failed = 2;
    if (tab_n) loader_start();   /* vr157 */
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

static void bg_finish(void);   /* vr157 (below) */
void trep_frame_begin(void)
{
    int done = 0;
    bg_finish();   /* vr157 */
    while (nqueue && done < (g_tex_hd_per_frame > 0 ? g_tex_hd_per_frame : 1)) {   /* a few per frame: no long hitch on entering a room */
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

/* vr131: an entry's file loaded (from trep_lookup; trep_named) - 0 when it could not be */
/* the GL texture from the file's bytes (the render thread, or vr160's upload thread on its shared context) */
static GLuint make_gl(Entry* e, uint8_t* d, size_t len, uint32_t* bytes, int* mips)
{
    GLuint t = 0;
    *bytes = 0; *mips = 0;
    if (d) {
        t = (len > 4 && !memcmp(d, "DDS ", 4)) ? load_dds(d, len, bytes, mips, &e->w, &e->ht) : load_png(d, len, bytes, mips, &e->w, &e->ht);
        free(d);
    }
    return t;
}

/* (render thread) the texture made is the entry's now */
static int finish_entry(Entry* e, GLuint t, uint32_t bytes, int mips)
{
    if (!t) { e->failed = 1; host_log("tex: could not load %s\n", e->path); return 0; }
    if (e->nnative) {   /* vr157: the original decodes drawn meanwhile go; the next draws get this one */
        extern void ntex_drop_native(unsigned int gl_tex);
        static unsigned nl;
        for (int k = 0; k < e->nnative; k++) ntex_drop_native(e->native[k]);
        if (e->nnative > 1 && g_tex_hd_log && nl++ < 50) host_log("tex: background: %d originals replaced by one HD (%s)\n", e->nnative, e->path);
        e->nnative = 0;
    }
    make_room(bytes);
    e->tex = t; e->bytes = bytes; used_bytes += bytes; e->mips = mips;
    if (t < 65536) hd_kind[t] = (uint8_t)(mips ? 2 : 1);
    if (!mips || e->w > g_tex_hd_max || e->ht > g_tex_hd_max) queue_convert(e);
    {
        static int nlog;
        const char* name = strrchr(e->path, '/');
        if (nlog++ < 1000) host_log("tex: HD at %.1f s: %s (%u KB, %u MB in use)\n", SDL_GetTicks() / 1000.0, name ? name + 1 : e->path, bytes >> 10, (unsigned)(used_bytes >> 20));
    }
    return e->tex != 0;
}

static int upload_entry(Entry* e, uint8_t* d, size_t len)
{
    uint32_t bytes; int mips;
    GLuint t = make_gl(e, d, len, &bytes, &mips);
    return finish_entry(e, t, bytes, mips);
}

static int load_entry(Entry* e)   /* at once, on the render thread (trep_named) */
{
    size_t len = 0;
    uint8_t* d = read_file(e->path, &len);
    return upload_entry(e, d, len);
}

/* ---------------- vr157: loading in the background ----------------
   As aikatore's HD pack (hdpack.c, v0.6.4 "load replacements in the background"): loader threads
   read the files while the draws keep the original texture; at the start of each frame the render
   thread uploads what is read, up to 3 ms of it (at least one file). Read but not uploaded: at most
   128 MB. The original decode drawn meanwhile is dropped once the replacement is in. */
enum { J_NONE, J_QUEUED, J_RUNNING, J_DONE };
#define UPLOAD_BUDGET_NS 3000000
#define READY_MAX (128u << 20)
#define LOADERS_MAX 6   /* vr160: g_tex_loaders, 4..6 (vr159: 4) */
static SDL_Mutex* jm;
static SDL_Condition* jc;
static Entry** jq; static uint32_t jq_h, jq_n;   /* to read (ring of tab_cap) */
static Entry** dq; static uint32_t dq_n;         /* read, to upload */
static uint64_t ready;                           /* bytes read, not uploaded */
static int loaders;
/* vr160: the upload thread - its own GL context shared with the render thread's, so the upload to the
   video card runs on another CPU; the render thread only takes the finished textures (gq) */
typedef struct { Entry* e; GLuint t; uint32_t bytes; int mips; } Made;
static Made* gq; static uint32_t gq_n;
static SDL_Window* up_win; static SDL_GLContext up_ctx;
static SDL_AtomicInt up_on;                      /* 1 the upload thread runs, 0 the render thread uploads (as vr157) */
static uint64_t up_ns; static uint32_t up_n;     /* its time, for the log */
static Entry* pending;                           /* the lookup just answered "not yet" (trep_pending_native) */
static uint32_t n_bg, n_bg_frames;

static int SDLCALL loader(void* unused)
{
    (void)unused;
    SDL_LockMutex(jm);
    for (;;) {
        Entry* e;
        size_t len = 0;
        uint8_t* d;
        while (!jq_n || ready > READY_MAX) SDL_WaitCondition(jc, jm);
        e = jq[jq_h]; jq_h = (jq_h + 1) % tab_cap; jq_n--;
        if (e->job != J_QUEUED) continue;
        e->job = J_RUNNING;
        SDL_UnlockMutex(jm);
        d = read_file(e->path, &len);
        SDL_LockMutex(jm);
        e->data = d; e->len = len;
        e->job = J_DONE;
        ready += len;
        dq[dq_n++] = e;
    }
    return 0;
}

#ifndef GL_VENDOR
#define GL_VENDOR 0x1F00
#endif
static int SDLCALL uploader(void* unused)
{
    (void)unused;
    if (!SDL_GL_MakeCurrent(up_win, up_ctx)) {
        host_log("tex: upload thread: cannot use its GL context (%s) - uploads on the render thread\n", SDL_GetError());
        return 0;
    }
    SDL_SetAtomicInt(&up_on, 1);
    host_log("tex: upload thread on (its own GL context)\n");
    SDL_LockMutex(jm);
    for (;;) {
        Entry* e;
        uint8_t* d; size_t len;
        Made m;
        uint64_t t0;
        while (!dq_n) SDL_WaitCondition(jc, jm);
        e = dq[0]; memmove(dq, dq + 1, (size_t)(--dq_n) * sizeof(Entry*));
        d = e->data; len = e->len; e->data = NULL; e->len = 0;
        SDL_UnlockMutex(jm);
        t0 = SDL_GetTicksNS();
        m.e = e;
        m.t = make_gl(e, d, len, &m.bytes, &m.mips);
        glFinish();   /* complete before the render thread draws with it */
        SDL_LockMutex(jm);
        up_ns += SDL_GetTicksNS() - t0; up_n++;
        ready -= len;
        gq[gq_n++] = m;
        SDL_BroadcastCondition(jc);
    }
    return 0;
}

static void loader_start(void)
{
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* card = (const char*)glGetString(GL_RENDERER);
    int n = g_tex_loaders < 4 ? 4 : g_tex_loaders > LOADERS_MAX ? LOADERS_MAX : g_tex_loaders, up;
    if (!(jq = malloc((size_t)tab_cap * sizeof(Entry*))) || !(dq = malloc((size_t)tab_cap * sizeof(Entry*))) ||
        !(gq = malloc((size_t)tab_cap * sizeof(Made))) || !(jm = SDL_CreateMutex()) || !(jc = SDL_CreateCondition())) return;
    for (int i = 0; i < n; i++) {
        SDL_Thread* th = SDL_CreateThread(loader, "texreplace", NULL);
        if (th) { SDL_DetachThread(th); loaders++; }
    }
    host_log("tex: background loading with %d threads\n", loaders);
    /* vr160: the upload thread - by the card (NVIDIA and AMD share contexts well), or as the launcher says */
    up = g_tex_upload == 1 || (g_tex_upload == 0 && vendor && (SDL_strcasestr(vendor, "NVIDIA") || SDL_strcasestr(vendor, "AMD") || SDL_strcasestr(vendor, "ATI")));
    host_log("tex: video card %s / %s - upload thread %s (%s)\n", vendor ? vendor : "?", card ? card : "?", up ? "wanted" : "off",
             g_tex_upload == 1 ? "launcher: on" : g_tex_upload == 2 ? "launcher: off" : "automatic");
    if (up && loaders) {
        SDL_GLContext main_ctx = SDL_GL_GetCurrentContext();
        up_win = SDL_GL_GetCurrentWindow();
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        up_ctx = up_win ? SDL_GL_CreateContext(up_win) : NULL;
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
        if (up_win) SDL_GL_MakeCurrent(up_win, main_ctx);   /* (creating it made it current here) */
        if (!up_ctx) host_log("tex: upload thread: no shared GL context (%s) - uploads on the render thread\n", SDL_GetError());
        else {
            SDL_Thread* th;
            SDL_GL_MakeCurrent(up_win, main_ctx);
            if ((th = SDL_CreateThread(uploader, "texupload", NULL))) SDL_DetachThread(th);
            else host_log("tex: upload thread: cannot start it - uploads on the render thread\n");
        }
    }
}

static void bg_queue(Entry* e)   /* (render thread) */
{
    SDL_LockMutex(jm);
    if (e->job == J_NONE && jq_n < tab_cap) { e->job = J_QUEUED; jq[(jq_h + jq_n) % tab_cap] = e; jq_n++; SDL_SignalCondition(jc); }
    SDL_UnlockMutex(jm);
}

/* frame start: upload what the loader threads read, up to the budget */
static void bg_finish(void)
{
    uint64_t t0 = SDL_GetTicksNS();
    int done = 0;
    if (!loaders) return;
    if (SDL_GetAtomicInt(&up_on)) {   /* vr160: the upload thread made them - only taken here */
        Made* got = NULL;
        uint32_t n = 0;
        SDL_LockMutex(jm);
        if (gq_n && (got = malloc(gq_n * sizeof(Made)))) { memcpy(got, gq, gq_n * sizeof(Made)); n = gq_n; gq_n = 0; }
        SDL_UnlockMutex(jm);
        for (uint32_t k = 0; k < n; k++) {
            Entry* e = got[k].e;
            SDL_LockMutex(jm); e->job = J_NONE; SDL_UnlockMutex(jm);
            if (!e->tex && !e->failed) finish_entry(e, got[k].t, got[k].bytes, got[k].mips);
            else if (got[k].t) glDeleteTextures(1, &got[k].t);
            n_bg++;
        }
        free(got);
        if (n) n_bg_frames++;
        if (n && g_tex_hd_log && (n_bg_frames % 50) == 1)
            host_log("tex: upload thread: %u files (%.1f ms each on its thread), %.2f ms on the render thread this frame\n",
                     up_n, up_n ? up_ns / 1e6 / up_n : 0.0, (SDL_GetTicksNS() - t0) / 1e6);
        return;
    }
    for (;;) {
        Entry* e = NULL;
        uint8_t* d; size_t len;
        SDL_LockMutex(jm);
        if (dq_n && (!done || SDL_GetTicksNS() - t0 < UPLOAD_BUDGET_NS)) {
            e = dq[0]; memmove(dq, dq + 1, (size_t)(--dq_n) * sizeof(Entry*));
            d = e->data; len = e->len; e->data = NULL; e->len = 0;
            ready -= len; e->job = J_NONE;
            SDL_BroadcastCondition(jc);
        }
        SDL_UnlockMutex(jm);
        if (!e) break;
        if (!e->tex && !e->failed) upload_entry(e, d, len); else free(d);
        done++; n_bg++;
    }
    if (done) n_bg_frames++;
    if (done && g_tex_hd_log && (n_bg_frames % 50) == 1)
        host_log("tex: background: %u files uploaded in %u frames (%.1f ms this frame)\n", n_bg, n_bg_frames, (SDL_GetTicksNS() - t0) / 1e6);
}

/* textures.c decode: the original made for a lookup that answered "not yet" */
void trep_pending_native(unsigned int gl_tex)
{
    if (pending) {
        int k;
        for (k = 0; k < pending->nnative && pending->native[k] != gl_tex; k++) {}
        if (k == pending->nnative && k < 8) pending->native[pending->nnative++] = gl_tex;
        pending = NULL;
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
    pending = NULL;
    if (!e->tex && !e->failed) {
        if (!loaders) { if (!load_entry(e)) return 0; }
        else { bg_queue(e); pending = e; return 0; }   /* vr157: read in the background; the original meanwhile */
    }
    if (!e->tex) return 0;
    n_hits++;
    if ((n_lookups % 200) == 0) host_log("tex: %u textures seen, %u replaced\n", n_lookups, n_hits);
    return e->tex;
}

/* vr188 diagnosis: the HD pack's file for this GS texture, NULL when it has none */
const char* trep_file_for(uint64_t tex0, uint64_t texa)
{
    uint64_t h, clut;
    uint32_t bits;
    Entry* e;
    if (!trep_active()) return NULL;
    names(tex0, texa, &h, &clut, &bits);
    e = find(h, clut, bits);
    if (!e) return NULL;
    { const char* n = strrchr(e->path, '\\'); const char* m = strrchr(e->path, '/'); if (m > n) n = m; return n ? n + 1 : e->path; }
}

/* vr192 diagnosis: the HD file of an uploaded replacement, by its GL name */
const char* trep_file_of_gl(unsigned int gl_tex)
{
    for (uint32_t i = 0; gl_tex && i < tab_cap; i++)
        if (tab[i].path && tab[i].tex == gl_tex) { const char* n = strrchr(tab[i].path, '/'); const char* m = strrchr(tab[i].path, 92); if (m > n) n = m; return n ? n + 1 : tab[i].path; }
    return NULL;
}

int trep_kind(unsigned int gl_tex) { return gl_tex < 65536 ? hd_kind[gl_tex] : 0; }

/* vr131: a replacement texture by its file name (the VR ammo box's own picture) - 0 when not in the pack */
unsigned int trep_named(const char* file, int* mips)
{
    static char last[128];
    static long idx = -1;
    size_t fl = strlen(file);
    if (strcmp(last, file)) {
        snprintf(last, sizeof(last), "%s", file);
        idx = -1;
        for (uint32_t i = 0; i < tab_cap && idx < 0; i++) {
            size_t pl = tab[i].path ? strlen(tab[i].path) : 0;
            if (pl >= fl && !strcmp(tab[i].path + pl - fl, file)) idx = (long)i;
        }
    }
    if (idx < 0) return 0;
    tab[idx].last = (uint32_t)SDL_GetTicks();
    if (!tab[idx].tex && !tab[idx].failed && tab[idx].job == J_NONE && !load_entry(&tab[idx])) return 0;
    if (mips) *mips = tab[idx].mips;
    return tab[idx].tex;
}

/* vr130 diagnosis: an HD texture drawn while the inventory shows item `item` - its file, once per item and texture */
void trep_note(unsigned int gl_tex, int item)
{
    static struct { unsigned int t; int item; } seen[1024];
    static int n;
    for (int i = 0; i < n; i++) if (seen[i].t == gl_tex && seen[i].item == item) return;
    if (n >= 1024) return;
    seen[n].t = gl_tex; seen[n].item = item; n++;
    for (uint32_t i = 0; i < tab_cap; i++)
        if (tab[i].path && tab[i].tex == gl_tex) {
            const char* name = strrchr(tab[i].path, '/');
            host_log("vr-diag: inventory item %d: texture %s (%dx%d)\n", item, name ? name + 1 : tab[i].path, tab[i].w, tab[i].ht);
            return;
        }
}
