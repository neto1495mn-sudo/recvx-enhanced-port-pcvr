/*
 * Native textures (rendering rework): texture images are uploaded to GS memory only when
 * something needs them there, and decoded once per content.
 *
 * The game keeps a single "current texture" slot in GS memory and re-uploads the whole
 * TIM2 (image and CLUT) whenever it switches textures (ps2_dummy.c Ps2TexLoad ->
 * ps2_loadtim2.c Send_Tim2_dataEx); the ordering table walker does the same for its own
 * slots. On the PC the game thread identifies each TIM2 (pc/glue/texture_glue.c): the first
 * upload of a content is recorded as a definition (its GS register writes and data, not
 * executed), every switch binds a definition to a slot (TBP of the image, CBP of the CLUT).
 *
 * A bind executes the definition's CLUT uploads on GS memory at once (they are small), so
 * GS memory and the CLUT buffer behave as on the GS and draws use whatever CLUT is loaded,
 * as the game expects (effects share and offset palettes). The image uploads are deferred:
 * they wait in an ordered queue and are executed only before something other than the
 * native lookup reads or overwrites their GS memory (other TEX0s, transfers, downloads,
 * render target reloads, real uploads) - earlier queued uploads overlapping them first, so
 * GS memory ends up as it would have been. A queued upload superseded by a later one to the
 * same slot is dropped.
 *
 * A TEX0 whose TBP is a bound slot is decoded from the definition: its image uploads are
 * replayed into a scratch copy of GS memory at the definition's own address (so
 * pre-swizzled images come out as on the GS) and decoded with the current CLUT buffer. The
 * result is cached per definition, TEX0 and palette.
 *
 * Memory: the game thread forgets the least recently used definitions beyond a budget
 * (ntex_forget) and reuses their id slots (NTEX_ID_SLOTS definitions at most); decoded
 * textures are capped in bytes. A definition that runs out of memory while recorded is dropped
 * (its slot is resolved through GS memory as without native textures).
 */
#include "gs.h"
#include "textures.h"
#include "texreplace.h"
#include "../render/gl_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __ANDROID__
#include <SDL3/SDL.h>
#endif

static int trace = -1;
#define TRACE(...) do { if (trace < 0) trace = getenv("RECVX_TRACE_NTEX") != NULL; if (trace) fprintf(stderr, __VA_ARGS__); } while (0)

typedef struct { int reg; uint64_t v; uint32_t off, len; } DefOp;   /* reg < 0: image data */
typedef struct {
    uint32_t id;                  /* the full id (slot + generation) this entry holds */
    int oom;                      /* recording ran out of memory: not used */
    DefOp* op; int nop, cop;
    int clut_end;                 /* ops [0, clut_end): the CLUT uploads (they come first) */
    uint8_t* data; uint32_t ndata, cdata;
    uint32_t tbp, cbp;            /* image / CLUT upload base addresses (blocks) */
    uint32_t image_pages;         /* pages covered from tbp */
    int valid;
    GLuint capture;               /* a render texture (StoreRenderTex): this texture, no GS data */
    int cw, ch;                   /* its size in GS texels */
} Def;

static Def defs[NTEX_ID_SLOTS];
static Def* cur;                  /* being recorded */
static int cur_in_clut;           /* the current upload goes to the CLUT */
static int cur_seen_xfer;

/* slots: TBP -> definition */
#define MAX_ALIAS 256
typedef struct { uint32_t id, tbp, cbp, page0, page1, bound; int used; } Alias;
static Alias alias[MAX_ALIAS];
static uint32_t bind_clock;

/* deferred image uploads, oldest first */
#define MAX_PENDING 256
typedef struct { uint32_t id, tbp, page0, page1; } Pending;
static Pending pending[MAX_PENDING];
static int npending;
static int executing;             /* our own GS writes: not real uploads */

#ifdef __ANDROID__
/* Android (Quest): the slots and queued uploads found without scanning the arrays (ntex_bind/ntex_lookup scanning 256
   slots and the queue at every texture switch were ~35% of a game frame's execution): slot by TBP, and per 8 KB page how
   many slots / queued uploads cover it (most accesses and uploads touch none) */
static uint16_t alias_at[0x4000];   /* TBP -> slot index + 1 (0: none) */
static uint16_t al_cnt[512], pd_cnt[512];
static unsigned st_super, st_bind, st_pend_sum, st_pend_max, st_before, st_before_scan, st_exec, st_lookup, st_note, st_note_scan;
static void al_pages(const Alias* a, int d) { for (uint32_t p = a->page0; p <= a->page1 && p < 512; p++) al_cnt[p] += d; }
static void pd_pages(const Pending* q, int d) { for (uint32_t p = q->page0; p <= q->page1 && p < 512; p++) pd_cnt[p] += d; }
static void idx_rebuild(void)
{
    memset(alias_at, 0, sizeof(alias_at)); memset(al_cnt, 0, sizeof(al_cnt)); memset(pd_cnt, 0, sizeof(pd_cnt));
    for (int i = 0; i < MAX_ALIAS; i++) if (alias[i].used) { alias_at[alias[i].tbp & 0x3FFF] = (uint16_t)(i + 1); al_pages(&alias[i], 1); }
    for (int i = 0; i < npending; i++) pd_pages(&pending[i], 1);
}
/* android_prof.c: once a second */
void ntex_stats_log(void)
{
    void host_log(const char*, ...);
    host_log("android: native textures last second: %u binds (queued uploads avg %.1f max %u), %u executed, %u lookups; "
             "access checks %u (%u scanned the queue), upload checks %u (%u scanned the slots); %u superseded\n",
             st_bind, st_bind ? (double)st_pend_sum / st_bind : 0.0, st_pend_max, st_exec, st_lookup, st_before, st_before_scan, st_note, st_note_scan, st_super);
    st_super = st_bind = st_pend_sum = st_pend_max = st_before = st_before_scan = st_exec = st_lookup = st_note = st_note_scan = 0;
}
#endif
static Alias* alias_find(uint32_t tbp)
{
#ifdef __ANDROID__
    uint16_t k = alias_at[tbp & 0x3FFF];
    return k ? &alias[k - 1] : NULL;
#else
    for (int i = 0; i < MAX_ALIAS; i++) if (alias[i].used && alias[i].tbp == tbp) return &alias[i];
    return NULL;
#endif
}
static void alias_store(Alias* a, Alias v)
{
#ifdef __ANDROID__
    if (a->used) { al_pages(a, -1); if (alias_at[a->tbp & 0x3FFF] == (uint16_t)(a - alias + 1)) alias_at[a->tbp & 0x3FFF] = 0; }
#endif
    *a = v;
#ifdef __ANDROID__
    if (a->used) { al_pages(a, 1); alias_at[a->tbp & 0x3FFF] = (uint16_t)(a - alias + 1); }
#endif
}
static void alias_unuse(Alias* a) { Alias v = *a; v.used = 0; alias_store(a, v); }

_Static_assert(sizeof(alias) + sizeof(pending) + sizeof(int) <= NTEX_STATE_SIZE, "NTEX_STATE_SIZE");

#define NTEX_BYTES_MAX (256u << 20)
#define MAX_NTEX 4096
typedef struct { uint32_t id; uint64_t key, texa; uint32_t pal; GLuint tex; int w, h; uint64_t last; int used, hd; } NTex;   /* hd: tex is a replacement (texreplace.c owns it) */
static NTex ntex[MAX_NTEX];
static uint64_t use_clock, ntex_bytes;

/* the definition `id`, if it is the one its slot holds and complete */
static Def* def_find(uint32_t id)
{
    Def* d = &defs[id & (NTEX_ID_SLOTS - 1)];
    return d->id == id && d->valid ? d : NULL;
}

static void def_drop(Def* d);

/* the slot of `id` for (re)defining it; a previous occupant is dropped */
static Def* def_get(uint32_t id)
{
    Def* d = &defs[id & (NTEX_ID_SLOTS - 1)];
    if (d->id != id) {
        if (d->id) def_drop(d);
        d->id = id;
    }
    return d;
}

static int grow(void** p, size_t n)
{
    void* q = realloc(*p, n);
    if (!q) return 0;
    *p = q;
    return 1;
}

static void push_op(Def* d, DefOp op)
{
    if (d->oom) return;
    if (d->nop == d->cop) {
        int c = d->cop * 2 + 16;
        if (!grow((void**)&d->op, (size_t)c * sizeof(DefOp))) { d->oom = 1; return; }
        d->cop = c;
    }
    d->op[d->nop++] = op;
}

void ntex_define_begin(uint32_t id, uint32_t tbp, uint32_t cbp, uint32_t image_pages)
{
    cur = def_get(id);
    cur->nop = 0; cur->ndata = 0; cur->valid = 0; cur->clut_end = 0; cur->oom = 0;
    cur->tbp = tbp; cur->cbp = cbp; cur->image_pages = image_pages ? image_pages : 1;
    cur_in_clut = 0; cur_seen_xfer = 0;
}

int ntex_defining(void) { return cur != NULL; }

void ntex_define_reg(int reg, uint64_t v)
{
    if (!cur) return;
    if (reg == 0x50) {   /* a new transfer: to the CLUT or to the image (Clut_Load_Func runs first) */
        uint32_t dbp = (uint32_t)(v >> 32) & 0x3FFF;
        int to_clut = dbp == cur->cbp && dbp != cur->tbp;
        if (!to_clut && cur_in_clut) { cur->clut_end = cur->nop; cur_in_clut = 0; }
        if (to_clut && !cur_seen_xfer) cur_in_clut = 1;
        cur_seen_xfer = 1;
    }
    push_op(cur, (DefOp){ reg, v, 0, 0 });
}

void ntex_define_image(const uint8_t* data, uint32_t len)
{
    if (!cur || cur->oom) return;
    if (cur->ndata + len > cur->cdata) {
        uint32_t c = (cur->ndata + len) * 2;
        if (!grow((void**)&cur->data, c)) { cur->oom = 1; return; }
        cur->cdata = c;
    }
    memcpy(cur->data + cur->ndata, data, len);
    push_op(cur, (DefOp){ -1, 0, cur->ndata, len });
    cur->ndata += len;
}

void ntex_define_end(void)
{
    if (!cur) return;
    if (cur_in_clut) cur->clut_end = cur->nop;   /* only a CLUT */
    cur->valid = cur->ndata > 0 && !cur->oom;
    if (cur->oom) fprintf(stderr, "ntex: out of memory recording texture %u, drawn from GS memory\n", cur->id);
    cur = NULL;
}

void ntex_define_capture(uint32_t id, unsigned int gl_tex, int w, int h)
{
    Def* d = def_get(id);
    if (d->capture) gs_tex_delete(d->capture);   /* this frame's replays may still bind the old one */
    d->capture = gl_tex; d->cw = w; d->ch = h;
    d->nop = 0; d->ndata = 0; d->clut_end = 0; d->oom = 0;
    d->image_pages = ((uint32_t)w * (uint32_t)h * 4u + 8191u) / 8192u;   /* CT32 */
    d->valid = 1;
}

/* run ops [a, b) of a definition on the current GS memory, transfers rebased from the
   definition's addresses to tbp/cbp */
#ifdef __ANDROID__
__attribute__((noinline))   /* (own names in the sampling profiler) */
#endif
static void run_ops(const Def* d, int a, int b, uint32_t tbp, uint32_t cbp)
{
    GsState save = gs;
    gs_flush_blocked++;   /* we may be inside gs_flush (tex_get): no flush in between */
    executing++;
    for (int k = a; k < b; k++) {
        uint64_t v = d->op[k].v;
        if (d->op[k].reg == 0x50) {
            uint32_t dbp = (uint32_t)(v >> 32) & 0x3FFF;
            if (dbp == d->tbp) dbp = tbp;
            else if (dbp == d->cbp) dbp = cbp;
            v = (v & ~(0x3FFFull << 32)) | ((uint64_t)dbp << 32);
        }
        if (d->op[k].reg >= 0) gs_exec_reg(d->op[k].reg, v);
        else gs_exec_upload(d->data + d->op[k].off, d->op[k].len);
    }
    executing--;
    gs_flush_blocked--;
    gs = save;
    gs_reg_serial++;
}

static int overlaps(uint32_t a0, uint32_t a1, uint32_t b0, uint32_t b1) { return a0 <= b1 && b0 <= a1; }

/* execute pending upload i and, before it, every earlier one overlapping it or one of
   those (so GS memory gets the writes in their original order); they leave the queue */
#ifdef __ANDROID__
__attribute__((noinline))
#endif
static void execute_pending(int i)
{
    uint8_t mark[MAX_PENDING] = { 0 };
    Pending run[MAX_PENDING];
    int nrun = 0, m = 0;
    mark[i] = 1;
    for (int k = i; k >= 0; k--)
        if (mark[k])
            for (int j = 0; j < k; j++)
                if (!mark[j] && overlaps(pending[j].page0, pending[j].page1, pending[k].page0, pending[k].page1)) mark[j] = 1;
    for (int k = 0; k < npending; k++) {
#ifdef __ANDROID__
        if (mark[k]) pd_pages(&pending[k], -1);
#endif
        if (mark[k]) run[nrun++] = pending[k];
        else pending[m++] = pending[k];
    }
    npending = m;
#ifdef __ANDROID__
    st_exec += nrun;
#endif
    for (int k = 0; k < nrun; k++) {
        const Pending* p = &run[k];
        const Def* d = def_find(p->id);
        if (d) {
            TRACE("ntex: write image of def %u to tbp %u\n", p->id, p->tbp);
            run_ops(d, d->clut_end, d->nop, p->tbp, 0);
        }
    }
}

#ifdef __ANDROID__
/* gs_mem.c: would ntex_before_access(page0, page1) execute queued uploads (they write GS memory: a pending draw must be
   flushed before) */
int ntex_pending_on(uint32_t page0, uint32_t page1)
{
    if (executing || gs_memp != gs_mem || gs_replaying || !npending) return 0;
    if (page0 > page1) return 1;
    for (uint32_t p = page0; p <= page1 && p < 512; p++) if (pd_cnt[p]) return 1;
    return 0;
}
#endif
void ntex_before_access(uint32_t page0, uint32_t page1)
{
    if (executing || gs_memp != gs_mem || gs_replaying) return;   /* a replay writes no GS memory */
#ifdef __ANDROID__
    st_before++;
    if (!npending) return;
    if (page0 <= page1) {   /* (a reversed range: the full scan, as overlaps() reads it) */
        uint32_t e = page1 < 512 ? page1 : 511, p;
        for (p = page0; p <= e && !pd_cnt[p]; p++) {}
        if (p > e) return;   /* no queued upload on these pages */
    }
    st_before_scan++;
#endif
    for (int i = 0; i < npending; i++)
        if (overlaps(pending[i].page0, pending[i].page1, page0, page1)) { execute_pending(i); i = -1; }
}

void ntex_bind(uint32_t id, uint32_t tbp, uint32_t cbp)
{
    Def* d = def_find(id);
    Alias* a = NULL;
    uint32_t page0 = tbp / 32, page1;
    if (!d) return;
    gs_flush();   /* batched draws resolve their texture when flushed: before the slot changes */
    if (gs_replaying) {   /* textures come from the executed frame: the slot only, no GS memory writes */
        a = alias_find(tbp);
        for (int i = 0; !a && i < MAX_ALIAS; i++) if (!alias[i].used) a = &alias[i];
        if (a) alias_store(a, (Alias){ id, tbp, cbp, page0, page0 + d->image_pages - 1, ++bind_clock, 1 });
        return;
    }
#ifdef __ANDROID__
    st_bind++; st_pend_sum += (unsigned)npending;
    if ((unsigned)npending > st_pend_max) st_pend_max = (unsigned)npending;
#endif
    page1 = page0 + d->image_pages - 1;
    if (page1 > 511) page1 = 511;
    if (d->clut_end) {   /* the CLUT: written now, after anything queued under it */
        ntex_before_access(cbp / 32, cbp / 32 + 1);
        run_ops(d, 0, d->clut_end, tbp, cbp);
    }
    /* the image: queued; a queued upload to the same slot, not larger, is superseded */
#ifdef __ANDROID__
    /* Android (Quest): also when later queued uploads overlap it - this one is the last writer of all its pages, so
       its writes are never seen (an access to its pages executes this one too). The queue was always full (256) and
       each bind ran the overlap check of every later entry for each same-slot entry */
    {
        int m = 0;
        for (int i = 0; i < npending; i++) {
            if (pending[i].tbp == tbp && pending[i].page1 <= page1) { pd_pages(&pending[i], -1); st_super++; continue; }
            pending[m++] = pending[i];
        }
        npending = m;
    }
    if (0)
#endif
    for (int i = 0; i < npending; i++)
        if (pending[i].tbp == tbp && pending[i].page1 <= page1) {
            int later = 0;   /* unless something queued after it overlaps it */
            for (int j = i + 1; j < npending; j++) if (overlaps(pending[j].page0, pending[j].page1, pending[i].page0, pending[i].page1)) later = 1;
            if (!later) {
#ifdef __ANDROID__
                pd_pages(&pending[i], -1);
#endif
                memmove(&pending[i], &pending[i + 1], (size_t)(npending - i - 1) * sizeof(Pending)); npending--; i--;
            }
        }
    if (npending == MAX_PENDING) execute_pending(0);
    pending[npending++] = (Pending){ id, tbp, page0, page1 };
#ifdef __ANDROID__
    pd_pages(&pending[npending - 1], 1);
#endif
    a = alias_find(tbp);
    for (int i = 0; !a && i < MAX_ALIAS; i++) if (!alias[i].used) a = &alias[i];
    if (!a) {   /* full: the least recently bound slot is resolved through GS memory */
        a = &alias[0];
        for (int i = 1; i < MAX_ALIAS; i++) if (alias[i].bound < a->bound) a = &alias[i];
        ntex_before_access(a->page0, a->page1);
    }
    alias_store(a, (Alias){ id, tbp, cbp, page0, page1, ++bind_clock, 1 });
    TRACE("ntex: bind %u tbp %u cbp %u (def tbp %u cbp %u pages %u, %d queued)\n", id, tbp, cbp, d->tbp, d->cbp, d->image_pages, npending);
}

void ntex_note_upload(const uint64_t* pages)
{
    if (executing) return;
#ifdef __ANDROID__
    st_note++;
    {   /* an upload over no slot's pages: nothing to unbind */
        int hit = 0;
        for (int w = 0; w < 8 && !hit; w++)
            for (uint64_t m = pages[w]; m && !hit; m &= m - 1) hit = al_cnt[w * 64 + __builtin_ctzll(m)] != 0;
        if (!hit) return;
    }
    st_note_scan++;
#endif
    for (int i = 0; i < MAX_ALIAS; i++) {
        Alias* a = &alias[i];
        if (!a->used) continue;
        for (uint32_t p = a->page0; p <= a->page1 && p < 512; p++)
            if (pages[p >> 6] & (1ull << (p & 63))) { alias_unuse(a); TRACE("ntex: unbind tbp %u (upload over page %u)\n", a->tbp, p); break; }
    }
}

void ntex_forget(uint32_t id)
{
    Def* d = &defs[id & (NTEX_ID_SLOTS - 1)];
    if (d->id == id) def_drop(d);
}

/* everything of a definition: queued uploads run, slots unbound, decodes deleted, memory freed */
static void def_drop(Def* d)
{
    uint32_t id = d->id;
    for (int i = 0; i < npending; i++) if (pending[i].id == id) { execute_pending(i); i = -1; }
    for (int i = 0; i < MAX_ALIAS; i++) if (alias[i].used && alias[i].id == id) alias_unuse(&alias[i]);
    if (d->capture) gs_tex_delete(d->capture);
    for (int i = 0; i < MAX_NTEX; i++)
        if (ntex[i].used && ntex[i].id == id) { if (!ntex[i].hd) { gs_tex_delete(ntex[i].tex); ntex_bytes -= (uint64_t)ntex[i].w * ntex[i].h * 4; } ntex[i].used = 0; }
    free(d->op); free(d->data);
    memset(d, 0, sizeof(*d));
}

void ntex_state_save(void* out)
{
    uint8_t* o = out;
    memcpy(o, alias, sizeof(alias)); o += sizeof(alias);
    memcpy(o, pending, sizeof(pending)); o += sizeof(pending);
    memcpy(o, &npending, sizeof(npending));
}

void ntex_state_restore(const void* in)
{
    const uint8_t* s = in;
    memcpy(alias, s, sizeof(alias)); s += sizeof(alias);
    memcpy(pending, s, sizeof(pending)); s += sizeof(pending);
    memcpy(&npending, s, sizeof(npending));
#ifdef __ANDROID__
    idx_rebuild();
#endif
}

static uint32_t palette_hash(uint64_t tex0, uint64_t texa)
{
    uint32_t psm = (tex0 >> 20) & 0x3F, n, h = 2166136261u, tmp[256];
    if (psm == PSMT8 || psm == PSMT8H) n = 256;
    else if (psm == PSMT4 || psm == PSMT4HL || psm == PSMT4HH) n = 16;
    else return 0;
    gs_get_palette(tex0, texa, tmp, (int)n);
    for (uint32_t i = 0; i < n; i++) { h ^= tmp[i]; h *= 16777619u; }
    return h;
}

/* the image from the definition (replayed into scratch GS memory at its own address), the
   palette from the current CLUT buffer */
static GLuint decode(const Def* d, uint64_t tex0, uint64_t texa, int* w, int* h, int* hd)
{
    uint32_t tw = (tex0 >> 26) & 0xF, th = (tex0 >> 30) & 0xF;
    uint32_t W = 1u << (tw > 10 ? 10 : tw), H = 1u << (th > 10 ? 10 : th);
    uint32_t* px = malloc((size_t)W * H * 4);
    GLuint t;
    if (!px) return 0;
    gs_mem_scratch_begin();
    run_ops(d, d->clut_end, d->nop, d->tbp, d->cbp);
    if (trep_active() && (uint32_t)(tex0 & 0x3FFF) != d->tbp) {   /* vr194 log */
        extern void host_log(const char* fmt, ...);
        static int nl;
        if (nl++ < 50) host_log("tex: drawn from inside a bigger upload (tbp %u, upload at %u) - its own decode, not the upload's HD image\n", (unsigned)(tex0 & 0x3FFF), (unsigned)d->tbp);
    }
    /* vr194: only when the draw starts at the upload's own address - one starting inside it (the equipped weapon's
       box: tbp 14336 in the inventory icons' upload at 13312) got the whole upload's HD image at its own
       coordinates, a different picture of it (a gold Luger for any weapon) */
    if (trep_active() && (uint32_t)(tex0 & 0x3FFF) == d->tbp) {   /* an HD pack's image for it (texreplace.c); sampled with the original size's coordinates */
        GLuint r = trep_lookup((tex0 & ~0x3FFFull) | d->tbp, texa);
        if (r) { gs_mem_scratch_end(); free(px); *w = (int)W; *h = (int)H; *hd = 1; return r; }
    }
    gs_decode_texture((tex0 & ~0x3FFFull) | d->tbp, texa, W, H, px);
    gs_mem_scratch_end();
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)W, (GLsizei)H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    if (trep_active()) { extern void trep_pending_native(unsigned int gl_tex); trep_pending_native(t); }   /* vr157: replaced once its HD file is read */
    {   /* RECVX_DUMP_TEX=dir: also the native decodes */
        static const char* dd; static int chk, ndump;
        if (!chk++) dd = getenv("RECVX_DUMP_TEX");
        if (dd && ndump < 400) {
            char path[1024];
            FILE* fp;
            snprintf(path, sizeof(path), "%s/ntex%03d_%llx.rgba", dd, ndump++, (unsigned long long)tex0);
            if ((fp = fopen(path, "wb"))) { fwrite(&W, 4, 1, fp); fwrite(&H, 4, 1, fp); fwrite(px, 4, (size_t)W * H, fp); fclose(fp); }
        }
    }
    free(px);
    *w = (int)W; *h = (int)H;
    return t;
}

int ntex_lookup(uint64_t tex0, uint64_t texa, unsigned int* out_tex, int* out_w, int* out_h)
{
    uint32_t tbp = tex0 & 0x3FFF, psm = (tex0 >> 20) & 0x3F, pal;
    const Alias* a = NULL;
    const Def* d;
    uint64_t key = tex0 & 0x1FFFFFFFFFFFFFull & ~(0x3FFFull << 37);   /* TBP0..CSA without CBP (the palette is hashed); CLD excluded */
    NTex* e = NULL;
#ifdef __ANDROID__
    st_lookup++;
#endif
    a = alias_find(tbp);
    if (!a || !(d = def_find(a->id))) return 0;   /* (a replay can see a slot whose definition is gone; its image is in GS memory) */
    if (d->capture) {   /* rendered pixels: sampled as CT32 (what StoreImage + upload give) */
        if (psm != PSMCT32 && psm != PSMCT24) { TRACE("ntex: capture %u sampled as psm %u\n", a->id, psm); return 0; }
        *out_tex = d->capture; *out_w = d->cw; *out_h = d->ch;
        return 2;
    }
    pal = palette_hash(tex0, texa);
    use_clock++;
#ifdef __ANDROID__
    /* Android (Quest): the decode last found for this id/key/palette remembered by a hash (checked before use: a freed
       or reused entry just misses) - the scan of all 4096 entries was ~9% of a game frame's execution */
    static int16_t memo[4096];
    uint64_t hk = (uint64_t)a->id * 0x9E3779B97F4A7C15ull ^ key * 0xC2B2AE3D27D4EB4Full ^ (uint64_t)pal * 0x165667B19E3779F9ull ^
                  (psm != PSMCT16 && psm != PSMCT24 ? 0 : texa * 0x27D4EB2F165667C5ull);
    unsigned hs = (unsigned)((hk ^ (hk >> 29) ^ (hk >> 47)) & 4095);
    {
        int i = memo[hs] - 1;
        if (i >= 0) {
            NTex* t = &ntex[i];
            if (t->used && t->id == a->id && t->key == key && t->pal == pal && (psm != PSMCT16 && psm != PSMCT24 ? 1 : t->texa == texa)) {
                t->last = use_clock; *out_tex = t->tex; *out_w = t->w; *out_h = t->h; return t->hd ? 3 : 1;
            }
        }
    }
#endif
    for (int i = 0; i < MAX_NTEX; i++) {
        NTex* t = &ntex[i];
        if (t->used && t->id == a->id && t->key == key && t->pal == pal && (psm != PSMCT16 && psm != PSMCT24 ? 1 : t->texa == texa)) {
#ifdef __ANDROID__
            memo[hs] = (int16_t)(i + 1);
#endif
            t->last = use_clock; *out_tex = t->tex; *out_w = t->w; *out_h = t->h; return t->hd ? 3 : 1;
        }
    }
    for (int i = 0; i < MAX_NTEX; i++) if (!ntex[i].used) { e = &ntex[i]; break; }
    while (!e || ntex_bytes > NTEX_BYTES_MAX) {   /* least recently used */
        NTex* o = NULL;
        for (int i = 0; i < MAX_NTEX; i++) if (ntex[i].used && (!o || ntex[i].last < o->last)) o = &ntex[i];
        if (!o) break;
        if (!o->hd) { gs_tex_delete(o->tex); ntex_bytes -= (uint64_t)o->w * o->h * 4; }
        o->used = 0;
        if (!e) e = o;
    }
    e->hd = 0;
#ifdef __ANDROID__
    {
        uint64_t and_t0 = SDL_GetTicksNS();
        e->tex = decode(d, tex0, texa, &e->w, &e->h, &e->hd);
        g_and_stall.ntex_ns += SDL_GetTicksNS() - and_t0; g_and_stall.ntex_n++;
    }
#else
    e->tex = decode(d, tex0, texa, &e->w, &e->h, &e->hd);
#endif
    if (!e->tex) return 0;
    e->used = 1; e->id = a->id; e->key = key; e->texa = texa; e->pal = pal; e->last = use_clock;
#ifdef __ANDROID__
    memo[hs] = (int16_t)(e - ntex + 1);
#endif
    if (!e->hd) ntex_bytes += (uint64_t)e->w * e->h * 4;
    *out_tex = e->tex; *out_w = e->w; *out_h = e->h;
    return e->hd ? 3 : 1;
}

/* vr157: an original decode whose HD replacement has come in (texreplace.c): dropped, decoded again at the next draw */
void ntex_drop_native(unsigned int gl_tex)
{
    for (int i = 0; i < MAX_NTEX; i++)
        if (ntex[i].used && !ntex[i].hd && ntex[i].tex == gl_tex) {
            gs_texcache_drop_gl(gl_tex);
            gs_tex_delete(gl_tex);
            ntex_bytes -= (uint64_t)ntex[i].w * ntex[i].h * 4;
            ntex[i].used = 0;
            return;
        }
}

void ntex_drop_gl(unsigned int gl_tex)
{
    for (int i = 0; i < MAX_NTEX; i++) if (ntex[i].used && ntex[i].hd && ntex[i].tex == gl_tex) ntex[i].used = 0;
}
