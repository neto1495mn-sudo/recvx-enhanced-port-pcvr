/* Any model as a player's appearance ("You appear as", docs/ACTORS.md "You appear as"; catalogue in
 * pc/src/host/actors.c).
 *
 * A model is read from the room file that carries it (or a player part file), into buffers of
 * our own, and drawn as a "display actor" in place of a source actor: our player, the co-op
 * partner, or in cutscenes the event double of the character played (the story cast's NPC
 * models that act for the player in in-engine cutscenes). The source stays what the game
 * animates, collides with and links weapons and effects to; only its drawing is replaced.
 *
 * Every tick the display actor is posed from the source:
 *  - humanoid rigs (all bind angles 0, a T-pose like the player's): a node map found at load
 *    from the rest skeleton (pelvis, spine, chest, neck, head, clavicle / upper arm / forearm /
 *    hand, hip / knee / ankle / toe) and, per mapped node, the source's final world rotation
 *    (aim pitch and all) times a rest correction that turns the model's rest bone direction onto
 *    the source's. Positions come from the model's own bone offsets; nodes without a map follow
 *    their parent rigidly (fingers, claws, face, tentacles). The weapon hands of the source are
 *    moved to the model's hands. Limbs are mapped by where the player's joints sit along each
 *    chain, the root's height and the feet follow the source's in proportion (docs/ACTORS.md
 *    "Retargeting"; RECVX_RETARGET=0: the previous mapping).
 *  - everything else: its own motion set's idle motion, looped (or its rest pose).
 * Textures go to the texture heap (shared by TIM2 index with a model the room already shows) and
 * are released when the model changes.
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/system.h"
#include "../../decomp/include/ps2/veronica/prog/dread.h"
#include "../../decomp/include/ps2/veronica/prog/binfunc.h"
#include "../../decomp/include/ps2/veronica/prog/njplus.h"
#include "../../decomp/include/ps2/veronica/prog/expand.h"
#include "../../decomp/include/ps2/veronica/prog/Motion.h"
#include "../../decomp/include/ps2/veronica/prog/MdlPut.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_texture.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaTextureFunction.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_dvd_image.h"
void njUnitMatrix(NJS_MATRIX* m);
void njRotateXYZ(NJS_MATRIX* m, Angle angx, Angle angy, Angle angz);
#include "../../decomp/include/ps2/veronica/prog/face.h"
#include "../../decomp/include/ps2/veronica/prog/pwksub.h"
#include "../../decomp/include/ps2/veronica/prog/objitm.h"
#include "../src/host/actors.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void host_log(const char* fmt, ...);
void bhArmIkMdk(BH_PWORK* ewP, int bas_no, NJS_POINT3* effP, int rot);   /* playpch.c */
Sint32 ADXF_ReadNw32(ADXF adxf, Sint32 nsct, void* buf);   /* PC: synchronous */
extern NJS_TEXMEMLIST* Ps2_tex_info;
extern unsigned int Ps2_texmemlist_num;
extern void *Ps2_now_free, *Ps2_free_last;
extern ETTY_WORK lkmtab[2];             /* player.c: Claire's hair (1211), Wesker's piece (1212) */

#define KEEP_SIZE (1536 * 1024)         /* model + motion chunks (largest in the catalogue: 0.2 + 0.96 MB) */
#define OW_MAX 256                      /* O_WORKs of all sub-models */
#define NODES 64                        /* nodes of a mapped body (largest: 45) */
#define TEX_MARGIN 32                   /* texture entries left free for the rooms to come */
#define HEAP_MARGIN (2 << 20)           /* texture heap bytes left free */
#define MAGIC_MASK 0x4B53414D           /* "MASK": face mask data of the cast (lip sync; the cutscene double's face, face_init) */

/* a humanoid skeleton: player node (0..21) -> node, parents, rest positions (model space) */
typedef struct Rig {
    signed char map[22];
    signed char par[NODES];
    float rest[NODES][3];
    int n, ok;
} Rig;

typedef struct Disguise {
    O_WORK ow[OW_MAX];                  /* first: 16-byte aligned */
    MN_WORK mth[512];
    unsigned char keep[KEEP_SIZE];      /* the model's chunk list, then its type's motion list */
    BH_PWORK w;                         /* the display actor */
    int model, fail, tex_gone, posed, mapped;   /* mapped: the last pose came from the source's skeleton */
    int face;                           /* faces in its face mask (face_init; 0 none): lip sync mirrored from the double */
    int lip, lip_max;                   /* the double's lip data (a line) and its highest phoneme */
    NJS_TEXLIST* tex[16];               /* texlists we uploaded */
    int tex_n;
    Rig rig;                            /* rig.ok: posed from the source's skeleton */
} Disguise;
static Disguise dg[ACTOR_SLOTS] __attribute__((aligned(64)));
static unsigned char pack[6 << 20] __attribute__((aligned(64)));
static unsigned char room[12 << 20] __attribute__((aligned(64)));   /* the game's arena size: any room it loads fits */
int actor_on;                           /* a display actor is shown (game.c hooks) */
static int dbl_tick;                    /* actor_double_tick is posing the double (its hands go to the model's) */
static BH_PWORK* dbl_posed;             /* the double the display actor's current pose was taken from */
static void face_init(Disguise* d, int idx);
/* the retargeting of docs/ACTORS.md "Retargeting" (limb roles along each chain, the root's height
   scaled, the aim solved on the drawn weapon, the camera scaled); RECVX_RETARGET=0: the previous one */
int actor_retarget = 1;

/* the player's chains: the node whose position gives a node's bone direction (-1: a chain end) */
static const signed char dir_child[22] = { -1, 2, 3, 4, 5, -1, 7, 8, 9, -1, 11, 12, 13, -1, 15, 16, 17, -1, 19, 20, 21, -1 };

/* the story cast's event models that act for the character played (sys->ply_id) in cutscenes */
static const unsigned char doubles[4][3] = { { 91 }, { 92, 42 }, { 93, 43 }, { 97, 49, 47 } };

static unsigned int rd32(const unsigned char* p) { unsigned int v; memcpy(&v, p, 4); return v; }

/* ---------------------------------------------------------------- humanoid rig */
static void rig_child_list(const Rig* r, int p, int* out, int* n)
{
    int i;
    *n = 0;
    for (i = 0; i < r->n; i++)
        if (r->par[i] == p) out[(*n)++] = i;
}

/* the next joint of a chain at `c`: the child going furthest in `axis` (1 = y down, 0 = x out); a
   child at c's own position (a zero-length bone: Rodrigo's and the hazmat man's upper arms, many
   toes) is passed through, so the chain goes on below it (the node follows its parent rigidly) */
static int rig_next(const Rig* r, int c, int axis, int sign)
{
    int ch[NODES], nc, i, best = -1, z = -1;
    rig_child_list(r, c, ch, &nc);
    for (i = 0; i < nc; i++) {
        float d = (r->rest[ch[i]][axis] - r->rest[c][axis]) * sign, e = 0;
        int k;
        if (d > 0.3f && (best < 0 || d > (r->rest[ch[best]][axis] - r->rest[c][axis]) * sign)) best = i;
        for (k = 0; k < 3; k++) e += fabsf(r->rest[ch[i]][k] - r->rest[c][k]);
        if (e < 0.05f) z = ch[i];
    }
    return best >= 0 ? ch[best] : z >= 0 ? rig_next(r, z, axis, sign) : -1;
}

/* along a chain from `c` */
static int rig_chain(const Rig* r, int c, int axis, int sign, int* out, int max)
{
    int n = 0;
    for (; c >= 0 && n < max; c = rig_next(r, c, axis, sign)) out[n++] = c;
    return n;
}

static float rig_dist(const Rig* r, int a, int b)
{
    float d = 0;
    int k;
    for (k = 0; k < 3; k++) d += (r->rest[a][k] - r->rest[b][k]) * (r->rest[a][k] - r->rest[b][k]);
    return sqrtf(d);
}

/* how far below node c its subtree reaches */
static float rig_reach(const Rig* r, int c)
{
    float m = 0;
    int i, p;
    for (i = c + 1; i < r->n; i++) {   /* (pre-order: the subtree follows c) */
        for (p = r->par[i]; p >= 0 && p != c; p = r->par[p]);
        if (p == c && rig_dist(r, i, c) > m) m = rig_dist(r, i, c);
    }
    return m;
}

/* An arm from its first node: along the child going furthest out (rig_next), ending at the hand: a
   node without children, or with two or more whose subtrees are short against the arm (fingers,
   claws; a spike on a forearm is not the hand: the arm goes on past it) */
static int rig_arm(const Rig* r, int c, int sign, int* out, int max)
{
    int n = 0;
    for (; c >= 0 && n < max; c = rig_next(r, c, 0, sign)) {
        int ch[NODES], nc, i, real = 0;
        out[n++] = c;
        rig_child_list(r, c, ch, &nc);
        for (i = 0; i < nc; i++) real += rig_dist(r, ch[i], c) >= 0.05f;
        if (real >= 2 && rig_reach(r, c) < 0.35f * rig_dist(r, c, out[0])) break;
    }
    return n;
}

/* Of the first `lim` nodes of chain c[0..n-1] (hanging off `from`, ending at `end`), the one nearest
   to fraction f of the chain's length from `from` to `end`; a tie goes to the later node (past a
   zero-length bone) */
static int rig_role(const Rig* r, int from, const int* c, int n, int lim, float f, int end)
{
    float len[16], tot = 0, d;
    int i, j, prev = from, best = -1;
    if (n > 15) n = 15;
    for (i = 0; i <= n; i++) {
        int node = i < n ? c[i] : end;
        for (d = 0, j = 0; j < 3; j++) d += (r->rest[node][j] - r->rest[prev][j]) * (r->rest[node][j] - r->rest[prev][j]);
        len[i] = tot += sqrtf(d);
        prev = node;
    }
    if (lim > n) lim = n;
    if (lim <= 0) return -1;
    if (tot < 1e-3f) return c[0];
    for (i = 0; i < lim; i++)
        if (best < 0 || fabsf(len[i] / tot - f) <= fabsf(len[best] / tot - f)) best = i;
    return c[best];
}

/* Node roles from the rest skeleton of a chunk object array (pre-order, child/sibling): a T-pose
   with all bind angles 0 is required of a model (`bind`); a source's node offsets are its rest
   pose whatever its current angles. Returns rig->ok. */
static int rig_build(Rig* r, const NJS_CNK_OBJECT* o, int n, int bind)
{
    int st[NODES], sp = 0, cur = -1, i, k, ch[NODES], nc, legs = 0, chest = -1, node, c[8], m;
    memset(r, 0, sizeof(*r));
    memset(r->map, -1, sizeof(r->map));
    if (n < 10 || n > NODES) return 0;
    r->n = n;
    for (i = 0; i < n; i++) {   /* parents and rest positions (bhCalcTree's walk) */
        if (bind && (o[i].ang[0] || o[i].ang[1] || o[i].ang[2])) return 0;
        r->par[i] = cur;
        for (k = 0; k < 3; k++) r->rest[i][k] = o[i].pos[k] + (cur >= 0 ? r->rest[cur][k] : 0);
        if (o[i].sibling) st[sp++] = cur;
        cur = o[i].child ? i : sp ? st[--sp] : -1;
        if (sp >= NODES - 1) return 0;
    }
    r->map[0] = 0;
    r->map[1] = 1;
    /* legs: chains going down from the pelvis */
    rig_child_list(r, 1, ch, &nc);
    for (i = 0; i < nc; i++) {
        int base;
        if (r->rest[ch[i]][1] > r->rest[1][1] - 0.5f) continue;
        base = r->rest[ch[i]][0] > r->rest[1][0] ? 14 : 18;   /* (the root may carry the world position) */
        if (r->map[base] >= 0) continue;
        m = rig_chain(r, ch[i], 1, -1, c, actor_retarget ? 8 : 4);
        if (m > 4) {   /* more joints than hip, knee, ankle, toe (Monster Steve): the ends, and the knee at mid-leg */
            int kk = rig_role(r, c[0], c + 1, m - 3, m - 3, 0.5f, c[m - 2]);
            c[1] = kk; c[2] = c[m - 2]; c[3] = c[m - 1]; m = 4;
        }
        for (k = 0; k < m; k++) r->map[base + k] = c[k];
        legs++;
    }
    /* spine: up from the pelvis to the first node with a sideways child (the chest) */
    for (i = 0; i < nc && chest < 0; i++) {
        int s[8], ns = 0;
        if (r->rest[ch[i]][1] < r->rest[1][1] - 0.5f) continue;
        for (node = ch[i]; node >= 0 && ns < 8;) {
            int cc[NODES], ncc, j, up = -1, side = 0;
            s[ns++] = node;
            rig_child_list(r, node, cc, &ncc);
            for (j = 0; j < ncc; j++) {
                float dx = fabsf(r->rest[cc[j]][0] - r->rest[node][0]);
                if (dx > 0.15f) side = 1;
                else if (up < 0 && r->rest[cc[j]][1] >= r->rest[node][1]) up = cc[j];
            }
            if (side) { chest = node; break; }
            node = up;
        }
        if (chest < 0) continue;
        r->map[3] = chest;
        r->map[2] = ns > 1 ? s[ns - 2] : -1;
    }
    if (chest < 0 || legs < 2) return 0;
    /* neck and head: up from the chest; arms: sideways from the chest or the spine below it */
    rig_child_list(r, chest, ch, &nc);
    for (i = 0; i < nc; i++)
        if (fabsf(r->rest[ch[i]][0] - r->rest[chest][0]) <= 0.15f && r->rest[ch[i]][1] > r->rest[chest][1] && r->map[4] < 0) {
            int cc[NODES], ncc;
            m = rig_chain(r, ch[i], 1, 1, c, 2);
            r->map[4] = c[0];
            rig_child_list(r, c[0], cc, &ncc);
            if (m > 1) r->map[5] = c[1];
            else if (ncc == 1) r->map[5] = cc[0];   /* a head hanging in front of the neck (hunter) */
        }
    for (node = chest; node > 1; node = r->par[node]) {
        rig_child_list(r, node, ch, &nc);
        for (i = 0; i < nc; i++) {
            float dx = r->rest[ch[i]][0] - r->rest[node][0];
            int base = dx > 0 ? 6 : 10, j = 0, e;
            if (fabsf(dx) <= 0.15f || r->map[base + 1] >= 0 || r->map[base] >= 0 || r->map[base + 3] >= 0) continue;
            m = actor_retarget ? rig_arm(r, ch[i], dx > 0 ? 1 : -1, c, 8) : rig_chain(r, ch[i], 0, dx > 0 ? 1 : -1, c, 5);
            e = c[m - 1];
            if (actor_retarget && m < 2) continue;   /* a stub (the Bandersnatch's other shoulder) */
            /* an "arm" that ends far above or below its root is a tentacle, a tail or a wing */
            if (fabsf(r->rest[e][1] - r->rest[ch[i]][1]) > fabsf(r->rest[e][0] - r->rest[ch[i]][0])) continue;
            if (actor_retarget) {   /* the hand at the chain's end, the others where the player has them along it */
                static const float frac[3] = { 0.03f, 0.35f, 0.68f };   /* clavicle, shoulder, elbow (Claire's and Chris's arms) */
                int lim = m - 1;
                r->map[base + 3] = e;
                for (k = 2; k >= 0 && lim > 0; k--) {
                    int t = rig_role(r, node, c, m - 1, lim, frac[k], e);
                    r->map[base + k] = t;
                    for (lim = 0; c[lim] != t; lim++);
                }
                continue;
            }
            if (sqrtf(dx * dx + powf(r->rest[ch[i]][1] - r->rest[node][1], 2)) < 1.0f) r->map[base] = c[j++];   /* clavicle */
            for (k = 1; k < 4 && j < m; k++) r->map[base + k] = c[j++];
        }
    }
    return r->ok = 1;
}

/* rotation (row-vector convention, NJS: p' = p * M) taking direction u onto direction v */
static void rot_from_to(const float* u, const float* v, float* m /* [9] */)
{
    float a[3], b[3], c[3], d, la = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]), lb = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]), k;
    int i, j;
    static const float id[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    memcpy(m, id, sizeof(id));
    if (la < 1e-4f || lb < 1e-4f) return;
    for (i = 0; i < 3; i++) { a[i] = u[i] / la; b[i] = v[i] / lb; }
    c[0] = a[1] * b[2] - a[2] * b[1]; c[1] = a[2] * b[0] - a[0] * b[2]; c[2] = a[0] * b[1] - a[1] * b[0];
    d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    if (d < -0.999f) return;   /* opposite: leave it (no rest pose of the catalogue needs it) */
    k = 1.0f / (1.0f + d);
    {   /* column form M = I + [c]x + [c]x^2 / (1 + d); the row form is its transpose */
        float x[9] = { 0, -c[2], c[1], c[2], 0, -c[0], -c[1], c[0], 0 }, x2[9] = { 0 };
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++) x2[i * 3 + j] = x[i * 3] * x[j] + x[i * 3 + 1] * x[3 + j] + x[i * 3 + 2] * x[6 + j];
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++) m[j * 3 + i] = id[i * 3 + j] + x[i * 3 + j] + x2[i * 3 + j] * k;
    }
}

/* Ninja angles (ax, ay, az) of a row-vector rotation r[9] as njRotateXYZ builds it (Rx Ry Rz) */
static void euler_xyz(const float* r, Sint32* ang)
{
    float sy = -r[2], cy = sqrtf(r[0] * r[0] + r[1] * r[1]);
    const float k = 32768.0f / 3.14159265f;
    ang[1] = (Sint32)lroundf(atan2f(sy, cy) * k);
    if (cy > 1e-4f) {
        ang[0] = (Sint32)lroundf(atan2f(r[5], r[8]) * k);
        ang[2] = (Sint32)lroundf(atan2f(r[1], r[0]) * k);
    } else {   /* gimbal lock: all of it in x */
        ang[0] = (Sint32)lroundf(atan2f(-r[7], r[4]) * k);
        ang[2] = 0;
    }
}

static void mul3(const float* a, const float* b, float* out)   /* out = a * b (3x3, row vectors) */
{
    float t[9];
    int j, c;
    for (j = 0; j < 3; j++)
        for (c = 0; c < 3; c++) t[j * 3 + c] = a[j * 3] * b[c] + a[j * 3 + 1] * b[3 + c] + a[j * 3 + 2] * b[6 + c];
    memcpy(out, t, sizeof(t));
}

/* world matrices of all nodes from their world rotations: positions from the model's own bone
   offsets (the root: at `root`), so every bone keeps its length and stays on its parent */
static void place(ML_WORK* m, const Rig* tr, float (*rot)[9], const float* dm, const float* root)
{
    int i, j;
    for (i = 0; i < tr->n; i++) {
        float* w = m->owP[i].mtx;
        int p = tr->par[i];
        for (j = 0; j < 3; j++) { w[j] = rot[i][j]; w[4 + j] = rot[i][3 + j]; w[8 + j] = rot[i][6 + j]; }
        w[3] = w[7] = w[11] = 0;
        w[15] = 1;
        if (i == 0 && root) memcpy(&w[12], root, 12);
        else {
            const float* pw = p >= 0 ? m->owP[p].mtx : dm;
            const float* off = m->objP[i].pos;
            for (j = 0; j < 3; j++) w[12 + j] = off[0] * pw[j] + off[1] * pw[4 + j] + off[2] * pw[8 + j] + pw[12 + j];
        }
    }
}

/* Two-bone IK on the model's own arm (upper arm u, forearm f, hand h; its own bone lengths): the
   hand to t, the elbow kept in the plane it bends in. The rotations of u and f are turned. */
static void arm_ik(ML_WORK* m, float (*rot)[9], int u, int f, int h, const float* t)
{
    const float *S = &m->owP[u].mtx[12], *E = &m->owP[f].mtx[12], *H = &m->owP[h].mtx[12];
    float se[3], eh[3], st[3], l1 = 0, l2 = 0, d = 0, k = 0, pole[3], pl = 0, e2[3], h1[3], t2[3], d1[9], d2[9], ca, sa;
    int j;
    for (j = 0; j < 3; j++) { se[j] = E[j] - S[j]; eh[j] = H[j] - E[j]; st[j] = t[j] - S[j]; }
    for (j = 0; j < 3; j++) { l1 += se[j] * se[j]; l2 += eh[j] * eh[j]; d += st[j] * st[j]; }
    l1 = sqrtf(l1); l2 = sqrtf(l2); d = sqrtf(d);
    if (l1 < 0.1f || l2 < 0.1f || d < 1e-3f) return;
    for (j = 0; j < 3; j++) { st[j] /= d; k += se[j] * st[j]; }
    for (j = 0; j < 3; j++) { pole[j] = se[j] - st[j] * k; pl += pole[j] * pole[j]; }
    if ((pl = sqrtf(pl)) < 1e-3f) return;   /* a straight arm: no bend plane */
    if (d > (l1 + l2) * 0.999f) d = (l1 + l2) * 0.999f;
    if (d < fabsf(l1 - l2) * 1.01f + 0.01f) d = fabsf(l1 - l2) * 1.01f + 0.01f;
    ca = (l1 * l1 + d * d - l2 * l2) / (2 * l1 * d);
    sa = sqrtf(fmaxf(0, 1 - ca * ca));
    for (j = 0; j < 3; j++) e2[j] = l1 * (ca * st[j] + sa * pole[j] / pl);   /* the new elbow, from S */
    rot_from_to(se, e2, d1);
    for (j = 0; j < 3; j++) h1[j] = eh[0] * d1[j] + eh[1] * d1[3 + j] + eh[2] * d1[6 + j];   /* forearm after d1 */
    for (j = 0; j < 3; j++) t2[j] = st[j] * d - e2[j];
    rot_from_to(h1, t2, d2);
    mul3(rot[u], d1, rot[u]);
    mul3(rot[f], d1, rot[f]);
    mul3(rot[f], d2, rot[f]);
}

/* an unmapped node's world rotation: rigidly with its parent; with a face mask (face_init) turned
   by its own angles, which the mask sets (jaw, tongue, eyes) */
static void follow(const Disguise* d, float (*rot)[9], int i, int p)
{
    NJS_MATRIX l;
    float r[9];
    int j;
    const Sint32* a = d->w.mlwP->objP[i].ang;
    if (!d->face || !(a[0] | a[1] | a[2])) { memcpy(rot[i], rot[p], sizeof(rot[i])); return; }
    njUnitMatrix(&l);
    njRotateXYZ(&l, a[0], a[1], a[2]);
    for (j = 0; j < 9; j++) r[j] = ((float*)l)[(j / 3) * 4 + j % 3];
    mul3(r, rot[p], rot[i]);
}

/* the display actor's world matrices from the source's posed skeleton */
static void retarget(Disguise* d, BH_PWORK* src, const Rig* sr)
{
    ML_WORK* m = d->w.mlwP;
    O_WORK* sow = src->mlwP->owP;
    const Rig* tr = &d->rig;
    const float* dm = (const float*)d->w.mtxbuf;   /* the actor's own matrix (d->w.mtx points here) */
    const float* root = sr->map[0] >= 0 ? &sow[(int)sr->map[0]].mtx[12] : NULL;   /* the root: feet on the source's floor */
    float rot[NODES][9], scaled[3];
    if (root && actor_retarget && tr->map[1] >= 0 && sr->map[1] >= 0) {
        /* The root's height above the floor (crouching, kneeling, the walk's bob) scaled by the hips'
           heights: a taller body's legs bend as the source's and reach as far down in proportion, so
           its feet stay on the floor instead of floating (a shorter one's: sinking). Only the height:
           room motions move cutscene doubles across the room with the root. */
        float hs = sr->rest[(int)sr->map[1]][1] - sr->rest[(int)sr->map[0]][1], ht = tr->rest[(int)tr->map[1]][1] - tr->rest[(int)tr->map[0]][1];
        memcpy(scaled, root, sizeof(scaled));
        if (hs > 1.0f && ht > 1.0f) scaled[1] = dm[13] + (root[1] - dm[13]) * ht / hs;
        root = scaled;
    }
    int i, k, j, from[NODES];
    for (i = 0; i < tr->n; i++) from[i] = -1;
    for (k = 0; k < 22; k++) if (tr->map[k] >= 0 && sr->map[k] >= 0) from[(int)tr->map[k]] = k;
    for (i = 0; i < tr->n; i++) {
        int p = tr->par[i];
        k = from[i];
        if (k >= 0) {   /* source world rotation, after the rest correction */
            const float* s = sow[(int)sr->map[k]].mtx;
            float a[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 }, sw[9];
            if (k > 0) {   /* the bone towards the chain's next node; a chain end takes its parent bone's */
                int kk = dir_child[k] >= 0 ? k : k - 1, dk = dir_child[kk];
                if (dk >= 0 && tr->map[dk] >= 0 && sr->map[dk] >= 0 && tr->map[kk] >= 0 && sr->map[kk] >= 0) {
                    float u[3], v[3];
                    for (j = 0; j < 3; j++) {
                        u[j] = tr->rest[(int)tr->map[dk]][j] - tr->rest[(int)tr->map[kk]][j];
                        v[j] = sr->rest[(int)sr->map[dk]][j] - sr->rest[(int)sr->map[kk]][j];
                    }
                    rot_from_to(u, v, a);
                }
            }
            for (j = 0; j < 9; j++) sw[j] = s[(j / 3) * 4 + j % 3];
            mul3(a, sw, rot[i]);
        } else if (p >= 0) follow(d, rot, i, p);   /* rigidly with its parent */
        else for (j = 0; j < 9; j++) rot[i][j] = dm[(j / 3) * 4 + j % 3];
    }
    place(m, tr, rot, dm, root);
    if (root == scaled) {   /* The feet as the source's: its lowest ankle or toe as far over the floor (in
        proportion) beyond its rest height as the model's beyond the model's. Feet of another shape (a
        Hunter's long foot, Monster Steve's claw) lift the leg differently when the source's toe bends. */
        float fs = 1e9f, ft = 1e9f, rs = 1e9f, rt = 1e9f, hs = sr->rest[(int)sr->map[1]][1] - sr->rest[(int)sr->map[0]][1],
              ht = tr->rest[(int)tr->map[1]][1] - tr->rest[(int)tr->map[0]][1];
        for (k = 16; k <= 21; k++) {
            if (k == 18 || k == 19 || tr->map[k] < 0 || sr->map[k] < 0) continue;
            fs = fminf(fs, sow[(int)sr->map[k]].mtx[13]);
            rs = fminf(rs, sr->rest[(int)sr->map[k]][1] - sr->rest[(int)sr->map[0]][1]);
            ft = fminf(ft, m->owP[(int)tr->map[k]].mtx[13]);
            rt = fminf(rt, tr->rest[(int)tr->map[k]][1] - tr->rest[(int)tr->map[0]][1]);
        }
        if (ft < 1e8f && hs > 1.0f && ht > 1.0f) {
            float shift = rt + (fs - dm[13] - rs) * ht / hs - (ft - dm[13]);
            if (shift > 3.0f) shift = 3.0f;
            if (shift < -3.0f) shift = -3.0f;
            scaled[1] += shift;
            place(m, tr, rot, dm, root);
        }
    }
    {   /* Both hands on the weapon (two-handed aim and grips): the source's left hand is near its
           right. A model of other proportions has its hands elsewhere (longer arms, wider shoulders),
           so its left arm is solved to keep that hand where the source has it relative to the right
           hand, on the model's own bone lengths. Faded out between 3 and 6 units apart. */
        int u = tr->map[11], f = tr->map[12], h = tr->map[13], r = tr->map[9];
        if (u >= 0 && f >= 0 && h >= 0 && r >= 0 && sr->map[9] >= 0 && sr->map[13] >= 0) {
            const float *sR = &sow[(int)sr->map[9]].mtx[12], *sL = &sow[(int)sr->map[13]].mtx[12];
            float t[3], ds = 0, w;
            for (j = 0; j < 3; j++) ds += (sL[j] - sR[j]) * (sL[j] - sR[j]);
            w = (6.0f - sqrtf(ds)) / 3.0f;
            if (w > 0) {
                if (w > 1) w = 1;
                for (j = 0; j < 3; j++) t[j] = m->owP[h].mtx[12 + j] + w * (m->owP[r].mtx[12 + j] + sL[j] - sR[j] - m->owP[h].mtx[12 + j]);
                arm_ik(m, rot, u, f, h, t);
                for (i = 0; i < tr->n; i++) if (from[i] < 0 && tr->par[i] >= 0) follow(d, rot, i, tr->par[i]);
                place(m, tr, rot, dm, root);
            }
        }
    }
    /* The node angles as bhCalcTree would take them (local rotation = world x parent world^T): the
       skin (npCalcSkin) blends the vertices near a joint between the node's world matrix and that
       matrix with the node's own angles undone (the parent's rotation at the joint). With the bind
       angles 0 left there, every joint was a hard crease: hands, forearms and shoulders stood off
       the limb they belong to. */
    for (i = 0; i < tr->n; i++) {
        int p = tr->par[i], c;
        float rp[9], l[9];
        if (d->face && from[i] < 0) continue;   /* (the mask's angles stay what they are) */
        for (j = 0; j < 9; j++) rp[j] = p >= 0 ? rot[p][j] : dm[(j / 3) * 4 + j % 3];
        for (j = 0; j < 3; j++)
            for (c = 0; c < 3; c++) l[j * 3 + c] = rot[i][j * 3] * rp[c * 3] + rot[i][j * 3 + 1] * rp[c * 3 + 1] + rot[i][j * 3 + 2] * rp[c * 3 + 2];
        euler_xyz(l, m->objP[i].ang);
    }
}

/* ---------------------------------------------------------------- loading */
/* length of a "u32 size, bytes ... u32 -1" chunk list, 0 if it runs past `end` */
static int list_len(const unsigned char* p, const unsigned char* end)
{
    const unsigned char* q = p;
    for (;;) {
        int n;
        if (q + 4 > end) return 0;
        n = (int)rd32(q);
        q += 4;
        if (n == -1) return q - p;
        if (n < 0 || n > end - q) return 0;
        q += n;
    }
}

static int is_mlb(const unsigned char* c) { return rd32(c) != MAGIC_MASK && rd32(c) != SKIN_MAGIC; }
static int mlb_textured(const unsigned char* c) { return (int)rd32(c + 8) != -1; }   /* MLB header: texlist offset */

/* bhSetRoom's texture table order: per enemy record with a model, per textured sub-model, the
   first (type, model version, ordinal) owns the next entry after the room model's. Returns the
   owner index of (id, ver, k), or -1. */
static int tex_owner(const ETTY_WORK* recs, int n, const unsigned int* mdltab, const unsigned char* end, int id, int ver, int k)
{
    static int seen[1024];
    int i, m = 1, owners = 0, want = (id << 16) | (ver << 8) | k;
    for (i = 0; i < n; i++) {
        const unsigned char* p;
        int t = -1, dt;
        if (!(recs[i].flg & 0x1)) continue;
        p = room + mdltab[m++];
        while (p + 4 <= end && (dt = (int)rd32(p)) != -1) {
            p += 4;
            if (dt > 0 && is_mlb(p) && mlb_textured(p)) {
                int key = (recs[i].id << 16) | (recs[i].mdlver << 8) | ++t, j;
                for (j = 0; j < owners && seen[j] != key; j++);
                if (j == owners) {
                    if (owners == 1024) return -1;
                    seen[owners++] = key;
                }
                if (key == want) return j;
            }
            p += dt > 0 ? dt : 0;
        }
    }
    return -1;
}

static void unload(Disguise* d)
{
    int i;
    if (!d->tex_gone && d->tex_n) {
        for (i = 0; i < d->tex_n; i++) njReleaseTexture(d->tex[i]);
        bhGarbageTexture(NULL, 0);
    }
    d->tex_n = d->tex_gone = d->posed = d->face = d->lip = d->lip_max = 0;
    if (d == &dg[ACTOR_DOUBLE]) dbl_posed = NULL;
    d->model = -1;
    memset(&d->w, 0, sizeof(d->w));
    d->rig.ok = 0;
}

static int free_entries(void)
{
    unsigned int i, n = 0;
    for (i = 0; i < Ps2_texmemlist_num; i++) n += Ps2_tex_info[i].count == 0;
    return n;
}

static int heap_free(void) { return (char*)Ps2_free_last - (char*)Ps2_now_free; }

/* ---------------------------------------------------------------- player part files (actors.h) */
int afs_read(int part, int file, void* buf, int cap)
{
    void* h = ADXF_OpenAfs(part, file);
    int ok;
    if (!h) return 0;
    ok = ADXF_GetFsizeByte(h) > 4096 && ADXF_GetFsizeSct(h) * 2048 <= cap;   /* unused costumes are 11-byte placeholders */
    if (ok) {
        ADXF_ReadNw32(h, ADXF_GetFsizeSct(h), buf);
        ok = ADXF_GetStat(h) == 3;   /* ADXF_STAT_READEND */
    }
    ADXF_Close(h);
    return ok;
}
static int read_file(int part, int file) { return afs_read(part, file, pack, sizeof(pack)); }

/* The stock loader reads from sys->memp and places the O_WORK arrays with bhGetFreeMemory, which
   refuses addresses more than 12.25 MB above freemem, so freemem is moved too. */
void player_model_read(BH_PWORK* who, unsigned char* file, unsigned char* mdl, unsigned char* bmt, unsigned char* mth, unsigned char* zmt)
{
    BH_PWORK* plp_b = plp;
    unsigned char *memp_b = sys->memp, *mdl_b = sys->plmdlp, *bmt_b = sys->plbmtp, *mth_b = sys->plmthp,
                  *zmt_b = sys->plzmtp, *free_b = freemem;
    NJS_POINT3 hd_b = sys->hd_pos;
    unsigned int bmt_size_b = sys->bmt_size;
    plp = who;
    sys->memp = file;
    sys->plmdlp = mdl; sys->plbmtp = bmt; sys->plmthp = mth; sys->plzmtp = zmt;
    freemem = mdl;
    bhReadPlayerData();
    plp = plp_b;
    sys->memp = memp_b;
    sys->plmdlp = mdl_b; sys->plbmtp = bmt_b; sys->plmthp = mth_b; sys->plzmtp = zmt_b;
    freemem = free_b;
    sys->hd_pos = hd_b;
    sys->bmt_size = bmt_size_b;
}

/* bhReadWeaponData for `who` (weapon wpn): the hand objects objs[0..1], the weapon motions into
   who->mnwP[100..] (plp, sys->obwp, sys->memp and the weapon buffers swapped for it and put back) */
void player_weapon_read(BH_PWORK* who, O_WRK* objs, int wpn, unsigned char* file, unsigned char* wr, unsigned char* wl, unsigned char* wmt)
{
    BH_PWORK* plp_b = plp;
    O_WRK* obwp_b = sys->obwp;
    unsigned char *memp_b = sys->memp, *wr_b = sys->wrmdlp, *wl_b = sys->wlmdlp, *wmt_b = sys->plwmtp;
    unsigned int wmt_size_b = sys->wmt_size;
    plp = who;
    who->wpnr_no = wpn;
    sys->obwp = objs;
    sys->memp = file;
    sys->wrmdlp = wr; sys->wlmdlp = wl; sys->plwmtp = wmt;
    bhReadWeaponData();
    plp = plp_b;
    sys->obwp = obwp_b;
    sys->memp = memp_b;
    sys->wrmdlp = wr_b; sys->wlmdlp = wl_b; sys->plwmtp = wmt_b;
    sys->wmt_size = wmt_size_b;
}

/* object objs[2] linked to head node 5 of `who`, set up as bhSetPlayer does for player 1's sys->obwp[2] */
void player_hair(O_WRK* objs, BH_PWORK* who, int chr, unsigned char* exp0, unsigned char* exp3)
{
    O_WRK* obwp_b = sys->obwp;
    O_WRK* op = &objs[2];
    int k = chr == 0 ? 0 : 1, v;
    op->flg = 0;
    if ((chr != 0 && chr != 3) || who->mdl_n <= lkmtab[k].mdlver) return;
    sys->obwp = objs;
    bhSetObject(&lkmtab[k], 2, (unsigned char*)who);
    sys->obwp = obwp_b;
    v = op->mdlver;
    op->lkono = 5;
    op->lox = 0;
    op->loy = chr == 0 ? 1.5869f : 0.85f;
    op->loz = chr == 0 ? 0.7747f : -1.03f;
    if (chr == 0) op->skp[0] = who->skp[v];
    op->mdl[0] = who->mdl[v];
    op->mlwP = &op->mdl[0];
    if (chr == 0 && exp0) {   /* bhObjClpn would take its work from the room heap (plp's: sys->pletcp) */
        op->flg |= 0x100000;
        op->exp0 = exp0;
        op->exp3 = exp3;
    }
}

/* the sub-models of the chunk list in d->keep at `p`, as bhSetEneMdl / bhReadPlayerData realize
   them; own[i]: sub-model i has a texlist of its own; returns the texture count */
static int realize(Disguise* d, unsigned char* p, unsigned char* own)
{
    NJS_TEXLIST* last = NULL;
    int dt, ow = 0, texn = 0;
    for (; (dt = (int)rd32(p)) != -1; p += dt) {
        ML_WORK* m = &d->w.mdl[d->w.mdl_n];
        p += 4;
        if (!dt || d->w.mdl_n == 16) continue;
        if (rd32(p) == MAGIC_MASK) { d->w.mskp = p; continue; }   /* (as room.c bhSetEneMdl; set up only for the double, face_init) */
        if (rd32(p) == SKIN_MAGIC) { d->w.skp[d->w.mdl_n] = (int*)(p + 4); continue; }
        bhMlbBinRealize(p, m);
        if (d->w.skp[d->w.mdl_n]) npSkinConvert(m->objP, d->w.skp[d->w.mdl_n]);
        if (m->texP) { last = m->texP; own[d->w.mdl_n] = 1; texn += m->texP->nbTexture; }
        else m->texP = last;   /* as sys->et_lp: the last textured sub-model's */
        d->w.mbp[d->w.mdl_n] = m->objP;
        d->w.txp[d->w.mdl_n] = m->texP;
        if (ow + (int)m->obj_num > OW_MAX) break;
        m->owP = &d->ow[ow];
        memset(m->owP, 0, m->obj_num * sizeof(O_WORK));
        ow += m->obj_num;
        d->w.mdl_n++;
    }
    return texn;
}

static int tex_room(const char* name, int texn)
{
    int heap;
    bhGarbageTexture(NULL, 0);
    heap = heap_free();
    if (free_entries() >= texn + TEX_MARGIN && heap >= HEAP_MARGIN) return 1;
    host_log("actor: %s: not enough texture memory (%d entries free, %d needed; %d KB heap)\n", name, free_entries(), texn, heap >> 10);
    return 0;
}
void actor_tex_free(int* entries, int* heap) { bhGarbageTexture(NULL, 0); *entries = free_entries(); *heap = heap_free(); }

/* Where the catalogue-format model `am` is in its room file: the file is read and expanded into
   `room` (kept until another room file is requested; nothing is realized in it), and `out` gets the
   record, its model chunk list, its type's motion list (the first record of the type that has one,
   as bhFinishRoom) and the texture data of each textured sub-model in chunk order. Also used by the
   enemy spawner (spawn_glue.c). */
static const unsigned char* end;        /* of the room file in `room` */

static int load_rdx(int rm)
{
    static int loaded = -1;
    RDT_WORK* hdr = (RDT_WORK*)room;
    char name[16];
    int f;
    if (loaded == rm) return 1;
    loaded = -1;
    snprintf(name, sizeof(name), "RM_%04d.RDX", rm);
    for (f = 0; f < rdx_image_data_max && strcmp(rdx_files[f], name); f++);
    if (f == rdx_image_data_max || !read_file(17, f)) return 0;
    end = room + Expand((char*)pack, room);
    if (hdr->ver < 1.72f || hdr->hed00 + sizeof(ROM_WORK) > (unsigned)(end - room)) return 0;
    loaded = rm;
    return 1;
}

int actor_find(const ActorModel* am, ActorData* out)
{
    RDT_WORK* hdr = (RDT_WORK*)room;
    const ROM_WORK* rw;
    const ETTY_WORK* recs;
    const unsigned int* mdltab;
    const unsigned char* p;
    int n, i, k = 1, dt;
    char name[16];

    memset(out, 0, sizeof(*out));
    snprintf(name, sizeof(name), "RM_%04d.RDX", am->room);
    if (!load_rdx(am->room)) return 0;
    rw = (const ROM_WORK*)(room + hdr->hed00);
    recs = (const ETTY_WORK*)(room + (unsigned int)rw->enep);
    n = rw->ene_n;
    if (am->rec >= n || recs[am->rec].id != am->id || recs[am->rec].mdlver != am->mdlver || !(recs[am->rec].flg & 0x1)) {
        host_log("actor: %s: record %d of %s is not type %d version %d\n", am->name, am->rec, name, am->id, am->mdlver);
        return 0;
    }
    out->rec = &recs[am->rec];
    mdltab = (const unsigned int*)(room + hdr->hed01);
    for (i = 0; i < am->rec; i++) k += recs[i].flg & 0x1;
    out->mdl = room + mdltab[k];
    out->mlen = list_len(out->mdl, end);
    {   /* the type's motion set: the first record of the type that has one (bhFinishRoom) */
        const unsigned int* mtntab = (const unsigned int*)(room + hdr->hed02);
        for (i = 0; i < n && !(recs[i].id == am->id && mtntab[i]); i++);
        if (i < n) { out->mtn = room + mtntab[i]; out->nlen = list_len(out->mtn, end); }
    }
    if (!out->mlen || (out->mtn && !out->nlen)) { host_log("actor: %s: bad chunk list in %s\n", am->name, name); return 0; }
    /* the entries of the room's texture table the game would upload them from */
    for (p = out->mdl; (dt = (int)rd32(p)) != -1 && out->ntex < 16; p += 4 + (dt > 0 ? dt : 0))
        if (dt > 0 && is_mlb(p + 4) && mlb_textured(p + 4)) {
            const unsigned char* texdat = room + hdr->hed04;
            int o = tex_owner(recs, n, mdltab, end, am->id, am->mdlver, out->ntex);
            out->tex[out->ntex++] = o < 0 || o + 1 >= (int)rd32(texdat) ? NULL : room + rd32(texdat + 8 + 4 * o);
        }
    return 1;
}

/* The texture data room file `rm` has for room effect `fx` (ids 30..99 and 400+; NULL: none).
   room.c bhSetRoom uploads one texture table entry per such id, in effect record order, after the
   room model's, enemies', objects' and items': the table's last entries. Valid as actor_find's. */
const unsigned char* actor_fx_tex(int rm, int fx)
{
    const RDT_WORK* hdr = (const RDT_WORK*)room;
    const ROM_WORK* rw;
    const EF_WRK* e;
    const unsigned char* texdat;
    int ids[64], m = 0, i, j, k = -1, cnt;
    if (!load_rdx(rm)) return NULL;
    rw = (const ROM_WORK*)(room + hdr->hed00);
    e = (const EF_WRK*)(room + (unsigned int)rw->effp);
    for (i = 0; i < rw->eff_n && m < 64; i++)
        if ((e[i].id >= 30 && e[i].id < 100) || e[i].id >= 400) {
            for (j = 0; j < m && ids[j] != e[i].id; j++);
            if (j < m) continue;
            if (e[i].id == fx) k = m;
            ids[m++] = e[i].id;
        }
    texdat = room + hdr->hed04;
    cnt = (int)rd32(texdat);
    return k < 0 || cnt <= m ? NULL : room + rd32(texdat + 4 + 4 * (cnt - m + k));
}

/* catalogue model idx from its room file */
static int load_room_model(Disguise* d, int idx)
{
    const ActorModel* am = &actor_models[idx];
    ActorData a;
    unsigned char* p;
    int i, mofs, nofs = 0, t = -1, texn, dt;
    unsigned char own[16] = { 0 };

    if (!actor_find(am, &a)) return 0;
    mofs = (int)((unsigned int)a.mdl & 63);   /* keep the data's alignment */
    if (a.mtn) nofs = ((mofs + a.mlen + 63) & ~63) + (int)((unsigned int)a.mtn & 63);
    if (mofs + a.mlen > KEEP_SIZE || nofs + a.nlen > KEEP_SIZE) { host_log("actor: %s does not fit\n", am->name); return 0; }

    unload(d);
    memcpy(d->keep + mofs, a.mdl, a.mlen);
    if (a.mtn) memcpy(d->keep + nofs, a.mtn, a.nlen);
    texn = realize(d, d->keep + mofs, own);
    if (!d->w.mdl_n || !d->w.mdl[0].objP || !d->w.mdl[0].owP || !tex_room(am->name, texn)) return 0;
    for (i = 0; i < d->w.mdl_n; i++) {
        if (!own[i]) continue;
        if (++t >= a.ntex || !a.tex[t]) { host_log("actor: %s: no texture entry for sub-model %d\n", am->name, i); continue; }
        bhSetMemPvpTexture(d->w.mdl[i].texP, (unsigned char*)a.tex[t], 0);
        d->w.mdl[i].flg |= 0x200;
        d->tex[d->tex_n++] = d->w.mdl[i].texP;
    }
    if (a.mtn)   /* the type's motions (bhSetEneMtn) */
        for (i = 0, p = d->keep + nofs; (dt = (int)rd32(p)) != -1 && i < 512; i++, p += dt) {
            p += 4;
            if (dt) bhMnbBinRealize(p, &d->mth[i]);
        }
    return 1;
}

/* the model of player part file `file` (dread.c bhReadPlayerData's layout) */
static int load_part_model(Disguise* d, int file)
{
    unsigned char *p, own[16] = { 0 };
    int i, size, texn;
    char name[24];
    snprintf(name, sizeof(name), "part file %d", file);
    if (!read_file(sys->sys_partid, file) || (size = (int)rd32(pack)) <= 0 || size > KEEP_SIZE) return 0;
    unload(d);
    memcpy(d->keep, pack + 4, size);
    texn = realize(d, d->keep, own);
    if (!d->w.mdl_n || !d->w.mdl[0].owP || !tex_room(name, texn)) return 0;
    p = pack + 4 + size;
    for (i = 0; i < 2; i++) {   /* motions, z translation */
        if (p + 4 > pack + sizeof(pack) || rd32(p) > (unsigned)(pack + sizeof(pack) - p - 4)) return 1;   /* (no textures) */
        p += 4 + rd32(p);
    }
    for (i = 0; i < d->w.mdl_n; i++) {
        unsigned int n;
        if (!own[i]) continue;
        if (p + 4 > pack + sizeof(pack)) break;
        n = rd32(p);
        p += 4;
        if (n & 0x80000000) { p = pack + ((p - pack + 31) & ~31); n &= ~0x80000000; }
        if (n > (unsigned)(pack + sizeof(pack) - p)) break;
        bhSetMemPvpTexture(d->w.mdl[i].texP, p, 0);
        d->w.mdl[i].flg |= 0x200;
        d->tex[d->tex_n++] = d->w.mdl[i].texP;
        p += n;
    }
    return 1;
}

static int load(Disguise* d, int idx)
{
    int heap = heap_free(), anim;
    const char* name = idx >= ACTOR_PART ? "part file" : actor_models[idx].name;
    memset(d->mth, 0, sizeof(d->mth));
    if (!(idx >= ACTOR_PART ? load_part_model(d, idx - ACTOR_PART + 10) : load_room_model(d, idx))) {
        unload(d);
        return 0;
    }
    d->w.flg = 0x1;
    d->w.mdflg = 0x20;
    d->w.mlwP = d->w.mdl;
    d->w.mtx = (float(*)[16])d->w.mtxbuf;
    d->w.sx = d->w.sy = d->w.sz = d->w.sxb = d->w.syb = d->w.szb = 1.0f;
    d->w.mnwP = d->w.mnwPb = d->mth;
    d->w.mtn_add = 65536;
    anim = idx >= ACTOR_PART || actor_models[idx].anim;
    if (anim) rig_build(&d->rig, d->w.mdl[0].objP, d->w.mdl[0].obj_num, 1);
    host_log("actor: %s loaded (%d sub-models, %d nodes, %d KB texture heap), %s; map", name, d->w.mdl_n,
             d->w.mdl[0].obj_num, (heap - heap_free()) >> 10, d->rig.ok ? "player animations" : "own idle motion");
    if (d->rig.ok) { int k; for (k = 0; k < 22; k++) host_log(" %d", d->rig.map[k]); }
    host_log("\n");
    if (getenv("RECVX_RIG_DUMP")) {   /* test aid: the rest skeleton (node, parent, model-space position, bind angles) */
        Rig r;
        const NJS_CNK_OBJECT* o = d->w.mdl[0].objP;
        int i;
        rig_build(&r, o, d->w.mdl[0].obj_num, 0);
        for (i = 0; i < r.n; i++)
            host_log("actor:  node %2d par %2d rest %7.2f %7.2f %7.2f ang %6d %6d %6d%s\n", i, r.par[i], r.rest[i][0], r.rest[i][1], r.rest[i][2],
                     (int)o[i].ang[0], (int)o[i].ang[1], (int)o[i].ang[2], o[i].model ? " mesh" : "");
    }
    return 1;
}

/* ---------------------------------------------------------------- gender-matched motions
   A model of the other sex than the character played moves with the motions of a player
   character of its own sex (appear_motion_chr: Chris's for a man while Claire is played,
   Claire's for a woman while a man is played). A hidden copy of the source's skeleton (its node
   offsets) plays, every tick, the motion of that character equivalent to the source's: the same
   number in the base and weapon sets (the numbers are the same for all characters: PlMtnAct,
   PlMtnWpn), the other character's motion of the enemy's set while grabbed
   (appear_grab_motion), and the source's own motion where the other set has none (room acting,
   motions only Claire has); at the source's phase (frame scaled by the motions' lengths),
   blending when the source blends, mirrored when it is, the head looking where the source's
   looks and, for our player, the third-person aim bend. The display actor is posed from the copy
   instead of from the source; a part-file mesh on the source itself is drawn with the copy's node
   matrices (actor_put_body). The source stays what the game moves, collides and hits with. */
typedef struct Shadow {
    unsigned char base[400 << 10] __attribute__((aligned(64)));   /* the motion chunk list of the part file (bhReadPlayerData: 384 KB) */
    unsigned char wpn[96 << 10] __attribute__((aligned(64)));     /* of the weapon file (bhReadWeaponData: 64 KB) */
    O_WORK ow[22] __attribute__((aligned(16)));
    NJS_CNK_OBJECT obj[22];
    MN_WORK mth[512], base_mth[512];    /* the set; the base motions alone (a weapon change starts from them) */
    BH_PWORK w;
    int chr, fail, wpn_no;              /* the motion set loaded (ply_id, -1 none), one that failed, the weapon's */
    int mesh, motion;                   /* actor_source */
    BH_PWORK *src, *drv;                /* actor_source's source; this tick's pose (NULL: the source's own) */
    NJS_CNK_OBJECT* of;                 /* the source's objects the copy was made from */
    int body;                           /* the source's own mesh is drawn with our matrices */
} Shadow;
static Shadow sh[2] __attribute__((aligned(64)));
static int redraw;                      /* actor_put_player: pose again from this tick's copy */

/* motions of a chunk list into mth[first..] (bhReadPlayerData / bhReadWeaponData) */
static void realize_motions(MN_WORK* mth, int first, unsigned char* p)
{
    int i, dt;
    for (i = first; (dt = (int)rd32(p)) != -1 && i < 512; i++, p += dt) {
        p += 4;
        if (dt) bhMnbBinRealize(p, &mth[i]);
    }
}

/* the motion set of player character chr (part file 10 + chr), with the weapon motions of weapon
   wpn (file wpn + chr * 30 + 20) over slots 100.. as the game loads them; for another weapon of the
   same character only the weapon file is read (the base motions are kept realized in base_mth) */
static int shadow_load(Shadow* s, int chr, int wpn)
{
    unsigned char* p;
    unsigned int n;
    int k;
    if (chr != s->chr) {
        s->chr = -1;
        memset(s->base_mth, 0, sizeof(s->base_mth));
        if (!read_file(sys->sys_partid, 10 + chr)) return 0;
        p = pack + 4 + rd32(pack);   /* past the model block */
        n = rd32(p);
        if (n > sizeof(s->base) || p + 4 + n > pack + sizeof(pack) || !list_len(p + 4, p + 4 + n)) return 0;
        memcpy(s->base, p + 4, n);
        realize_motions(s->base_mth, 0, s->base);   /* (in place: realized once) */
        s->chr = chr;
    }
    memcpy(s->mth, s->base_mth, sizeof(s->mth));
    s->wpn_no = wpn;
    if (wpn < 0 || wpn >= 30 || !read_file(sys->sys_partid, wpn + chr * 30 + 20)) return 1;
    for (p = pack, k = 0; k < 4; k++) {   /* right model, its texture, left model, its texture */
        n = rd32(p);
        p += 4;
        if ((k & 1) && (n & 0x80000000)) { p = pack + ((p - pack + 63) & ~63); n &= ~0x80000000; }
        if (n > (unsigned)(pack + sizeof(pack) - p)) return 1;
        p += n;
    }
    n = rd32(p);
    if (n && n <= sizeof(s->wpn) && n <= (unsigned)(pack + sizeof(pack) - p - 4) && list_len(p + 4, p + 4 + n)) {
        memcpy(s->wpn, p + 4, n);
        realize_motions(s->mth, 100, s->wpn);
    }
    return 1;
}

/* the copy posed in character s->chr's motion equivalent to src's (character `from`), or NULL */
static BH_PWORK* shadow_step(Shadow* s, BH_PWORK* src, int from)
{
    ML_WORK* sm = src->mlwP;
    MN_WORK *set = src->mnwP, *own;
    int mtn = src->mtn_no, i, k;
    long frm = src->frm_no;
    if (!set || mtn < 0 || mtn >= 512 || !set[mtn].datP || set[mtn].obj_num > 22 || sm->obj_num < 22) return NULL;
    own = &set[mtn];
    if (set == src->mnwPb) {   /* base and weapon motions: the same number */
        if (s->mth[mtn].datP && s->mth[mtn].obj_num <= 22) set = s->mth;
    } else {   /* grabbed: the player plays the enemy's set */
        for (i = 0; i < (int)sys->ewk_n && i < 128; i++)
            if ((ene[i].flg & 0x1) && (ene[i].mnwP == set || ene[i].mnwPb == set)) {
                k = appear_grab_motion(ene[i].id, mtn, from, s->chr);
                if (k >= 0 && set[k].datP && set[k].obj_num <= 22) mtn = k;
                break;
            }
    }
    if (&set[mtn] != own && own->frm_num) frm = frm * set[mtn].frm_num / own->frm_num;   /* the same phase */
    if (frm >= (long)set[mtn].frm_num << 16) frm = ((long)set[mtn].frm_num << 16) - 1;
    if (s->of != sm->objP) {   /* a new source: its hierarchy, a pose without blending */
        memcpy(s->obj, sm->objP, sizeof(s->obj));
        memset(s->ow, 0, sizeof(s->ow));
        memset(&s->w, 0, sizeof(s->w));
        s->w.mdl[0].objP = s->obj;
        s->w.mdl[0].owP = s->ow;
        s->w.mdl[0].obj_num = 22;
        s->w.mlwP = &s->w.mdl[0];
        s->w.mdl_n = 1;
        s->w.mtx = (float(*)[16])s->w.mtxbuf;
        s->w.flg = 0x1000;   /* bhCalcModel: the base matrix as it is (the source's) */
        s->of = sm->objP;
    }
    for (i = 1; i < 22; i++) memcpy(s->obj[i].pos, sm->objP[i].pos, sizeof(s->obj[i].pos));   /* the source's bones */
    if (set != s->w.mnwP || mtn != (int)s->w.mtn_no) {   /* a new motion blends as the source's (which took one step already) */
        s->w.hokan_count = s->w.mnwP && src->hokan_count ? src->hokan_count + 1 : 0;
        s->w.hokan_rate = src->hokan_rate;
    }
    s->w.mnwP = set;
    s->w.mtn_no = mtn;
    s->w.frm_no = (int)frm;
    bhSetMotion(&s->w, 0, src->mtn_md, src->mtn_tp);
    for (i = 4; i <= 5; i++) memcpy(s->obj[i].ang, sm->objP[i].ang, sizeof(s->obj[i].ang));   /* neck and head look as the source's (bhControlPlayerHead) */
    memcpy(s->w.mtxbuf, src->mtx, sizeof(float) * 16);
    bhCalcModel(&s->w);
    if ((sm->owP[7].flg & 0x2) && (sm->owP[9].flg & 0x2)) {   /* the source's arm is held by the lighter's IK (player.c
        bhControlPlayer, coop_glue.c lighter_arm), which the other set's motions do not have: the same IK on the
        copy, the arm towards a point in front of its chest, as the game does for the character played */
        NJS_POINT3 eff1 = { from == 0 ? 1.8f : 2.4f, -3.0f, from == 0 ? -3.0f : -3.5f }, eff0;
        BH_PWORK* keep = plp;
        njCalcPoint(&s->ow[4].mtx, &eff1, &eff0);
        plp = &s->w;   /* bhArmIkMdk reads the model's nodes through plp */
        bhArmIkMdk(&s->w, 6, &eff0, 35498);
        plp = keep;
        s->obj[9].ang[1] = -910;
        bhCalcModel(&s->w);
    }
    if (src == plp) tps_bend(&s->w);
    return &s->w;
}

void actor_source(int who, BH_PWORK* src, int mesh, int motion)
{
    if (who < 0 || who > 1) return;
    sh[who].src = src;
    sh[who].mesh = mesh;
    sh[who].motion = motion;
}

/* this tick's gender-matched pose of slot who's source, or NULL */
static BH_PWORK* shadow(int who, BH_PWORK* src)
{
    Shadow* s = &sh[who];
    Disguise* d = &dg[who];
    int anim, chr, k;
    s->body = 0;
    if (s->src != src || s->motion < 0 || s->motion > 3) return NULL;   /* no actor_source this tick (a lobby preview) */
    s->src = NULL;
    if (d->model >= 0) anim = !d->rig.ok ? ANIM_IDLE : d->model >= ACTOR_PART ? appear_part_anim(d->model - ACTOR_PART) : actor_models[d->model].anim;
    else anim = s->mesh >= 0 && s->mesh <= 3 ? appear_part_anim(s->mesh) : ANIM_IDLE;
    chr = appear_motion_chr(anim, s->motion);
    if (chr < 0 || chr == s->fail) return NULL;
    if ((chr != s->chr || src->wpnr_no != s->wpn_no) && !shadow_load(s, chr, src->wpnr_no)) {
        s->fail = chr;
        host_log("actor: the motions of part file %d cannot be read\n", 10 + chr);
        return NULL;
    }
    if (!shadow_step(s, src, s->motion)) return NULL;
    if (d->model < 0) {   /* a part-file mesh on the source: what is linked to its head and hands goes where they are drawn */
        for (k = 5; k <= 13; k += 4) memcpy(src->mlwP->owP[k].mtx, s->ow[k].mtx, sizeof(s->ow[k].mtx));
        s->body = 1;
        actor_on = 1;
    }
    return &s->w;
}

/* src drawn (bhPutModel), with the node matrices of its gender-matched pose if it has one */
int actor_put_body(int who, BH_PWORK* src)
{
    static float keep[22][16];
    static Sint32 ang[22][3];
    Shadow* s = &sh[who & 1];
    O_WORK* ow = src->mlwP ? src->mlwP->owP : NULL;
    NJS_CNK_OBJECT* o;
    int i;
    if (!s->body || !ow || s->of != src->mlwP->objP || src->mlwP->obj_num < 22) { bhPutModel(src); return 1; }
    o = src->mlwP->objP;
    for (i = 0; i < 22; i++) {   /* the matrices and the angles (the skin blends joints with them, npCalcSkin) */
        memcpy(keep[i], ow[i].mtx, sizeof(keep[i])); memcpy(ow[i].mtx, s->ow[i].mtx, sizeof(keep[i]));
        memcpy(ang[i], o[i].ang, sizeof(ang[i])); memcpy(o[i].ang, s->obj[i].ang, sizeof(ang[i]));
    }
    bhPutModel(src);
    for (i = 0; i < 22; i++) { memcpy(ow[i].mtx, keep[i], sizeof(keep[i])); memcpy(o[i].ang, ang[i], sizeof(ang[i])); }
    return 1;
}

/* ---------------------------------------------------------------- the source's head and hands
   What the game links to the source's head and hands (weapons, the head part, the lighter flame,
   the muzzle flash) is drawn where the model has them: for this tick's effects, objects and drawing
   the source's nodes 5, 9 and 13 carry the model's (actor_pose, shadow). Before the next tick's
   player control the game's own come back (actor_hands_back, game.c): bhControlPlayer reads the hand
   for the shot (the wall test of bhCheckGunAtari starts at the muzzle, owP[9]), the knife and sounds,
   so a taller model shot higher than the crosshair. */
static struct { BH_PWORK* src; O_WORK* ow; float mtx[3][16]; } real[2];
static float aim_ofs[3];   /* our weapon hand as drawn minus the real one, in the frame of the real chest (node 3) */
static float aim_rot[9];   /* the drawn weapon hand's rotation in that frame (the gender-matched pose's hand turns otherwise) */
static int aim_set;        /* aim_rot is set */

static void hands_save(int who, BH_PWORK* src)
{
    int i;
    if (real[who].src == src && real[who].ow == src->mlwP->owP) return;   /* saved this tick */
    if (src->mlwP->obj_num < 14) return;
    real[who].src = src;
    real[who].ow = src->mlwP->owP;
    for (i = 0; i < 3; i++) memcpy(real[who].mtx[i], real[who].ow[5 + 4 * i].mtx, sizeof(real[who].mtx[i]));
}

static void hands_back(int who)
{
    O_WORK* ow = real[who].ow;
    int i, j;
    if (!real[who].src) return;
    if (real[who].src->mlwP && ow == real[who].src->mlwP->owP) {
        if (who == 0 && real[0].src == plp) {   /* local = world x R^T, R: the chest's rows */
            float dl[3];
            for (j = 0; j < 3; j++) dl[j] = ow[9].mtx[12 + j] - real[0].mtx[1][12 + j];
            for (j = 0; j < 3; j++) aim_ofs[j] = dl[0] * ow[3].mtx[j * 4] + dl[1] * ow[3].mtx[j * 4 + 1] + dl[2] * ow[3].mtx[j * 4 + 2];
            for (i = 0; i < 3; i++)   /* hand rows x chest^T */
                for (j = 0; j < 3; j++)
                    aim_rot[i * 3 + j] = ow[9].mtx[i * 4] * ow[3].mtx[j * 4] + ow[9].mtx[i * 4 + 1] * ow[3].mtx[j * 4 + 1] + ow[9].mtx[i * 4 + 2] * ow[3].mtx[j * 4 + 2];
            aim_set = 1;
        }
        for (i = 0; i < 3; i++) memcpy(ow[5 + 4 * i].mtx, real[who].mtx[i], sizeof(real[who].mtx[i]));
    }
    real[who].src = NULL;
}

/* game.c, before bhControlPlayer */
void actor_hands_back(void)
{
    if (!real[0].src) aim_ofs[0] = aim_ofs[1] = aim_ofs[2] = 0, aim_set = 0;
    hands_back(0);
    hands_back(1);
}

void tps_trace_hands(const float* drawn, const float* real);
static void trace_hands(int who, BH_PWORK* src)
{
    if (who == 0 && !redraw && real[0].src == src && src == plp) tps_trace_hands(src->mlwP->owP[9].mtx, real[0].mtx[1]);
}

/* tps_glue.c (free aim): where p's weapon hand is drawn relative to its own, in world space for p's
   current chest; 0 without a model */
void actor_aim_offset(const BH_PWORK* p, float* out)
{
    const float* r = p->mlwP->owP[3].mtx;
    int j;
    for (j = 0; j < 3; j++) out[j] = p == plp && actor_on ? aim_ofs[0] * r[j] + aim_ofs[1] * r[4 + j] + aim_ofs[2] * r[8 + j] : 0;
}

/* tps_glue.c (free aim, with the retargeting): the weapon hand as it will be drawn, for p's current
   (bent) skeleton: hand[16] a node matrix (rows: axes, origin). 0 when no model is shown. */
int actor_aim_hand(const BH_PWORK* p, float* hand)
{
    const float* r = p->mlwP->owP[3].mtx;
    int i, j;
    if (!actor_retarget || p != plp || !actor_on || !aim_set) return 0;
    memset(hand, 0, 16 * sizeof(float));
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) hand[i * 4 + j] = aim_rot[i * 3] * r[j] + aim_rot[i * 3 + 1] * r[4 + j] + aim_rot[i * 3 + 2] * r[8 + j];
    actor_aim_offset(p, &hand[12]);
    for (j = 0; j < 3; j++) hand[12 + j] += p->mlwP->owP[9].mtx[12 + j];
    hand[15] = 1;
    return 1;
}

/* tps_glue.c: the over-the-shoulder camera's height, shoulder offset and distance for our model,
   relative to the character's (its head's height over the source's: a Tyrant's 1.4; 1 without a model
   or with the previous retargeting) */
float actor_cam_scale(void)
{
    const Disguise* d = &dg[0];
    const NJS_CNK_OBJECT* o;
    int k, i;
    float hs = 0, ht, s;
    if (!actor_retarget || d->model < 0 || !d->rig.ok || !plp || !plp->mlwP || plp->mlwP->obj_num < 6) return 1;
    k = d->rig.map[5] >= 0 ? d->rig.map[5] : d->rig.map[4];
    if (k < 0) return 1;
    ht = d->rig.rest[k][1] - d->rig.rest[0][1];
    for (o = plp->mlwP->objP, i = 1; i <= 5; i++) hs += o[i].pos[1];   /* the player's chain 1..5 (pelvis .. head) */
    if (hs < 5 || ht < 3) return 1;
    s = ht / hs;
    return s < 0.8f ? 0.8f : s > 1.8f ? 1.8f : s;
}

/* ---------------------------------------------------------------- interface */
int actor_set(int who, int idx)
{
    Disguise* d = &dg[who];
    if (idx == d->model) return idx >= 0;
    if (idx >= 0 && idx == d->fail) return 0;
    unload(d);
    actor_on = 0;
    for (who = 0; who < ACTOR_SLOTS; who++) actor_on |= dg[who].model >= 0;
    actor_on |= sh[0].body | sh[1].body;
    if (idx < 0 || (idx >= ACTOR_MODELS && idx < ACTOR_PART) || idx > ACTOR_PART + 9) return 0;
    if (!load(d, idx)) {
        d->fail = idx;
        host_log("actor: model %d cannot be shown\n", idx);
        return 0;
    }
    d->model = idx;
    d->fail = -1;
    actor_on = 1;
    if (d == &dg[ACTOR_DOUBLE] && g_appear_face) face_init(d, idx);
    return 1;
}

int actor_shown(int who) { return dg[who].model >= 0; }

/* pose slot `who` from `src` (after its bhSetMotion and bhCalcModel), in its gender-matched pose
   if it has one (slots 0, 1: actor_source) */
void actor_pose(int who, BH_PWORK* src)
{
    Disguise* d = &dg[who];
    ML_WORK* m = d->w.mlwP;
    BH_PWORK* drv;
    if (!src->mlwP || !src->mlwP->objP || !src->mlwP->owP || !src->mtx) return;
    if (who < 2) hands_save(who, src);
    if (who < 2 && !redraw) sh[who].drv = shadow(who, src);
    drv = who < 2 && sh[who].drv ? sh[who].drv : src;
    if (d->model < 0) { trace_hands(who, src); return; }
    d->w.px = src->px; d->w.py = src->py; d->w.pz = src->pz;
    d->w.ax = src->ax; d->w.ay = src->ay; d->w.az = src->az;
    d->w.draw_tp = src->draw_tp;
    memcpy(d->w.mtxbuf, src->mtx, sizeof(float) * 16);   /* the source's base matrix (also when linked) */
    d->posed = 1;
    d->mapped = 0;
    if (d->rig.ok) {
        Rig sr;   /* the source's skeleton: from its current node offsets (bind angles are 0); built each pose (a few
                     dozen nodes): a cache keyed by the objects' address kept a stale rig when a room load put another model there */
        rig_build(&sr, drv->mlwP->objP, drv->mlwP->obj_num, 0);   /* (a cast double's face mask context has 31 nodes, 9 of them face) */
        if (sr.ok) {
            static const void* dumped;
            if (getenv("RECVX_RIG_DUMP") && dumped != drv->mlwP->objP) {
                int i;
                dumped = drv->mlwP->objP;
                host_log("actor: source skeleton; map");
                for (i = 0; i < 22; i++) host_log(" %d", sr.map[i]);
                host_log("\n");
                for (i = 0; i < sr.n; i++) host_log("actor:  node %2d par %2d rest %7.2f %7.2f %7.2f\n", i, sr.par[i], sr.rest[i][0], sr.rest[i][1], sr.rest[i][2]);
            }
            retarget(d, drv, &sr);
            if (getenv("RECVX_RIG_DUMP") && who == 0 && !redraw) {   /* the feet over the floor: the source's lowest ankle/toe and the model's */
                float fs = 1e9f, ft = 1e9f;
                int k;
                for (k = 16; k <= 21; k++) {
                    if (k == 18 || k == 19) continue;
                    if (sr.map[k] >= 0) fs = fminf(fs, drv->mlwP->owP[(int)sr.map[k]].mtx[13] - (*src->mtx)[13]);
                    if (d->rig.map[k] >= 0) ft = fminf(ft, m->owP[(int)d->rig.map[k]].mtx[13] - (*src->mtx)[13]);
                }
                host_log("actor: feet motion %d frame %d: source %.2f model %.2f over the floor\n", (int)drv->mtn_no, (int)(drv->frm_no >> 16), fs, ft);
            }
            d->mapped = 1;
            if (who < 2) {   /* the source's weapon hands where the model's hands are, turned as the pose's (the copy has the source's nodes) */
                int k;
                for (k = 9; k <= 13; k += 4)
                    if (d->rig.map[k] >= 0 && sr.map[k] >= 0) {
                        float* h = src->mlwP->owP[(int)sr.map[k]].mtx;
                        if (drv != src) memcpy(h, drv->mlwP->owP[(int)sr.map[k]].mtx, 12 * sizeof(float));
                        memcpy(&h[12], &m->owP[(int)d->rig.map[k]].mtx[12], 12);
                    } else if (actor_retarget && sr.map[k] >= 0) {   /* a model without that hand (Nosferatu's right, the
                        Bandersnatch's left): what the game links to it (a weapon, a gloved hand) shrunk to a point
                        instead of floating where the source's hand is */
                        float* h = src->mlwP->owP[(int)sr.map[k]].mtx;
                        int j;
                        for (j = 0; j < 12; j++) h[j] *= 1e-3f;
                    }
                trace_hands(who, src);
            } else if (dbl_tick) {   /* the cutscene double: what the script links to its hands (the cast's hand
                pieces, props) goes to the model's hands; its own come back with its next bhCalcModel */
                int k;
                for (k = 9; k <= 13; k += 4)
                    if (d->rig.map[k] >= 0 && sr.map[k] >= 0) memcpy(&src->mlwP->owP[(int)sr.map[k]].mtx[12], &m->owP[(int)d->rig.map[k]].mtx[12], 12);
            }
            return;
        }
    }
    {   /* own idle motion, looped; a motion with more tracks than the model has nodes would write past it */
        int idle = d->model < ACTOR_MODELS ? actor_models[d->model].idle : 0;
        d->w.mtn_no = idle;
        if (d->mth[idle].datP && d->mth[idle].obj_num <= m->obj_num) bhSetMotion(&d->w, 65536, 0, NULL);
        bhCalcTree((NJS_MATRIX*)d->w.mtx, m);
    }
}

void actor_draw(int who)
{
    if (dg[who].model >= 0 && dg[who].posed) bhPutModel(&dg[who].w);
}

/* game.c bhAllDrawModel, in place of bhPutModel(plp): 1 = drawn */
int actor_put_player(void)
{
    if (dg[0].model < 0 || !dg[0].posed) return sh[0].body && actor_put_body(0, plp);   /* a part-file mesh in its gender-matched pose */
    if (dg[0].rig.ok) {   /* again from the final matrices (code after bhControlPlayer may bend bones); a gender-matched pose: this tick's */
        hands_back(0);   /* (the game's own hands, which the arm IK reads; moved again) */
        redraw = 1;
        actor_pose(0, plp);
        redraw = 0;
    }
    bhPutModel(&dg[0].w);
    return 1;
}

/* eneset.c bhDrawEnemy, in place of bhPutModel(ep): the event double of the character played
   (cutscenes) drawn as the chosen model. 1 = drawn. */
int actor_put_enemy(BH_PWORK* ep)
{
    int i, id = ep->id, pid = sys->ply_id & 3;
    if (dg[ACTOR_DOUBLE].model < 0) return 0;
    for (i = 0; i < 3 && doubles[pid][i] != id; i++);
    if (i == 3 || !id) return 0;
    if (ep != dbl_posed) {   /* (posed from this one last: the same pose, its hands already moved) */
        actor_pose(ACTOR_DOUBLE, ep);
        dbl_posed = dg[ACTOR_DOUBLE].mapped ? ep : NULL;   /* two doubles: each drawn in its own pose */
    }
    if (!dg[ACTOR_DOUBLE].mapped) return 0;   /* only a skeleton-driven model stands in for a cutscene double */
    bhPutModel(&dg[ACTOR_DOUBLE].w);
    return 1;
}

/* ---------------------------------------------------------------- the cutscene double's face
   The cast's cutscene models (ids 91..99) talk with a face mask (face.c, face_bh.c): a muscle model
   that bends the face mesh, the jaw, tongue and eyes, driven per phoneme by the room's lip data
   (bhSetLip; LIP_WORK: phoneme, time). A cast model shown for the double gets its own mask set up
   (bhInitMask, its allocations in face_mem) and every tick the double's lip state; its own face
   table gives the mouth shapes. The double's expressions (bhSetMask: face sets authored for that
   character's mask) are not taken: the model keeps its neutral face. Option "Talking faces"
   (g_appear_face). */
static unsigned char face_mem[1 << 20] __attribute__((aligned(64)));

/* A rest mesh of the mask (its face, its tongue) on a 64-byte boundary, as bhInitMask places the
   copies it morphs: _fmCnkCalcMuscle / _fmCnkCalcTang read the rest vertices with lq, which drops the
   low 4 address bits. In the room file they are aligned for the room the model was made for; our copy
   of a model from another room's list kept its 8-byte offset, and every moved vertex was read two
   floats early (the face's rim jumped out of the head as a spike). */
static NJS_CNK_MODEL* face_src(NJS_CNK_MODEL* m, int vofs)
{
    NJS_CNK_MODEL* c;
    int n;
    if (!m) return NULL;
    n = (((unsigned int)m->vlist[1] >> 16) * vofs + 128) * 4;   /* (bhInitMask's copy size) */
    if (!((uintptr_t)m->vlist & 15)) return m;
    if (!(c = (NJS_CNK_MODEL*)bhGetFreeMemory(sizeof(NJS_CNK_MODEL), 64))) return NULL;   /* face_mem full */
    *c = *m;
    if (!(c->vlist = (int*)bhGetFreeMemory(n, 64))) return NULL;
    memcpy(c->vlist, m->vlist, n);
    return c;
}

static void face_init(Disguise* d, int idx)
{
    ML_WORK* m = d->w.mlwP;
    MASK_WORK* fm;
    unsigned char *memp = sys->memp, *arena = freemem;
    int used, ok;
    if (!d->w.mskp || !d->rig.ok) return;
    sys->memp = face_mem;   /* bhGetFreeMemory's limit (freemem + 12.25 MB + PC_ARENA_EXTRA) at its end */
    freemem = (unsigned char*)((uintptr_t)face_mem + sizeof(face_mem) - 12845056 - PC_ARENA_EXTRA);
    bhInitMask(&d->w);   /* (copies the face mesh, inserts the jaw/tongue/eye pivots: a new object array) */
    fm = (MASK_WORK*)d->w.exp1;   /* (bhGetFreeMemory: NULL when face_mem is full) */
    ok = fm && d->w.exp0 && d->w.exp2 && fm->head;
    if (ok) {
        NJS_CNK_MODEL *src = face_src(fm->src, fm->vofs), *tang = fm->tangsrc ? face_src(fm->tangsrc, fm->vofs) : NULL;   /* the morph reads its rest vertices 16 bytes at a time (lq) */
        ok = src && (tang || !fm->tangsrc);
        if (ok) { fm->src = src; fm->tangsrc = tang; }
    }
    used = sys->memp - face_mem;
    sys->memp = memp;
    freemem = arena;
    /* bhControlMask writes the jaw, tongue and eye angles to nodes 7, 11, 9, 13 of the new array (the
       cast's layout); the pivots it inserted have the same model as these nodes and no child */
    ok = ok && used <= (int)sizeof(face_mem) && m->obj_num <= NODES && fm->toothsrc && fm->tangorg && fm->eyesrc[0] && fm->eyesrc[1] &&
         m->objP[7].model == fm->toothsrc->model && m->objP[7].child && m->objP[11].model == fm->tangorg->model && m->objP[11].child &&
         m->objP[9].model == fm->eyesrc[0]->model && m->objP[9].child && m->objP[13].model == fm->eyesrc[1]->model && m->objP[13].child &&
         rig_build(&d->rig, m->objP, m->obj_num, 0);
    host_log("actor: face mask %s (%d nodes, %d faces, %d KB)\n", ok ? "set up" : "does not fit, left out", m->obj_num, fm && fm->head ? (int)fm->head->nFace : 0, used >> 10);
    if (ok) {
        d->face = (int)fm->head->nFace;
        d->w.mdflg |= 0x100;   /* npCalcSkinFM: the skin over the array with the pivots, as the cast's */
        /* The mask's own face table holds the mouth shapes (banks of 6 phonemes, bhLipSet), not key
           frames: played as the face (fmCnkSetCurrentFrame) its last shape was taken, a spike out of
           the head. The face at rest is its first: phoneme 0 of bank 0, the closed mouth. */
        fm->fnum = 1;
        ((int*)d->w.exp0)[8] = 1;   /* (what bhControlMask resets the face to) */
    }
    else if (load(d, idx)) d->model = idx;   /* (the objects were changed: again without it; load leaves model -1) */
    else d->fail = idx;
}

/* the double's lip sync on our face (coop_control_remote, after the enemies' bhControlMask) */
static void face_step(Disguise* d, const BH_PWORK* ep)
{
    int* e = (int*)d->w.exp0;
    const int* s = (const int*)ep->exp0;
    if (!ep->mskp || !s || !ep->exp2) return;
    e[0] = (e[0] & ~0x4) | (s[0] & 0x4);    /* lip sync running */
    e[4] = s[4];                            /* the phoneme bank (bhLipSet): 6 faces each */
    e[268] = s[268];                        /* the lip data (bhSetLip) */
    *(LIP_WORK*)d->w.exp2 = *(const LIP_WORK*)ep->exp2;
    if (e[268] && e[268] != d->lip) {   /* a new line: its highest phoneme (pairs phoneme, time; 0xFF ends) */
        const unsigned char* p = (const unsigned char*)e[268];
        int k;
        d->lip = e[268];
        for (d->lip_max = k = 0; k < 4096 && p[k] != 0xFF; k += 2) if (p[k] > d->lip_max) d->lip_max = p[k];
    }
    if (e[4] < 0 || e[4] * 6 + d->lip_max >= d->face) { e[0] &= ~0x4; ((LIP_WORK*)d->w.exp2)->flag = 0; }   /* mouth shapes past our face table: none */
    bhControlMask(&d->w);
}

/* coop_control_remote, every tick after the enemies moved and before the objects follow their links:
   the double of the character played posed as our model, its face and hands with it */
void actor_double_tick(void)
{
    Disguise* d = &dg[ACTOR_DOUBLE];
    int i, j, pid = sys->ply_id & 3;
    dbl_posed = NULL;
    if (d->model < 0 || !d->rig.ok) return;
    for (i = 0; i < (int)sys->ewk_n && i < 128; i++) {
        BH_PWORK* ep = &ene[i];
        if (!(ep->flg & 0x1) || (ep->stflg & 0x41000000) || !ep->id || !ep->mlwP) continue;
        for (j = 0; j < 3 && doubles[pid][j] != ep->id; j++);
        if (j == 3) continue;
        if (d->face) face_step(d, ep);
        dbl_tick = 1;
        actor_pose(ACTOR_DOUBLE, ep);
        dbl_tick = 0;
        if (d->mapped) dbl_posed = ep;
    }
}

/* at the hand-off: the model for the cutscene doubles of the character played (appearance: a
   catalogue index, ACTOR_PART + part file - 10, or -1); loaded only while the room has one */
void actor_doubles(int idx)
{
    int i, j, pid = sys->ply_id & 3, any = 0;
    static const signed char twin[5] = { 0, 1, 3, 5, -1 };   /* the player files' cutscene models (a face mask): Claire, Chris, Steve, Wesker */
    if (g_appear_face && idx >= ACTOR_PART && idx < ACTOR_PART + 5 && twin[idx - ACTOR_PART] >= 0) idx = twin[idx - ACTOR_PART];
    if (idx >= 0)
        for (i = 0; i < (int)sys->ewk_n && i < 128 && !any; i++)
            for (j = 0; j < 3; j++) any |= (ene[i].flg & 0x1) && doubles[pid][j] && ene[i].id == doubles[pid][j];
    actor_set(ACTOR_DOUBLE, any ? idx : -1);
}

/* njReleaseTextureAll took ours too */
void actor_textures_released(void)
{
    int i;
    for (i = 0; i < ACTOR_SLOTS; i++) {
        dg[i].tex_gone = 1;
        unload(&dg[i]);
        dg[i].fail = -1;
    }
    actor_on = 0;
}

__attribute__((constructor)) static void actor_init(void)
{
    int i;
    for (i = 0; i < ACTOR_SLOTS; i++) dg[i].model = dg[i].fail = -1;
    for (i = 0; i < 2; i++) sh[i].chr = sh[i].fail = sh[i].wpn_no = sh[i].motion = -1;
    if (getenv("RECVX_RETARGET")) actor_retarget = atoi(getenv("RECVX_RETARGET")) != 0;
}

/* the display actor's work (its model, node matrices after actor_pose), NULL when none is shown */
BH_PWORK* actor_work(int who) { return who >= 0 && who < ACTOR_SLOTS && dg[who].model >= 0 ? &dg[who].w : NULL; }

/* node n of w's model has a mesh of its own: geometry, or skinned vertices (npSkinConvert's records
   in node order: flg, mesh mno, with flg 2 the mesh's vertex table; flg 1: vertices bound to the node) */
static int node_has_mesh(const BH_PWORK* w, int n)
{
    const ML_WORK* m = w->mlwP;
    const int* s = w->skp[m - w->mdl];
    int i, flg = 0;
    if (m->objP[n].model) return 1;
    for (i = 0; s && i <= n; i++) {
        if (m->objP[i].evalflags & 0x20000000) { flg = 0; continue; }
        if ((flg = *s++)) { s++; if (flg & 2) s += 1 + *s; }
    }
    return flg & 1;
}

/* the source's hand node (9, 13) is at the model's hand (actor_pose) and the model has no hand
   there of its own (the story cast's models; zombies and creatures have theirs) */
int actor_hand_free(int who, int node)
{
    Disguise* d = &dg[who];
    int n = d->model >= 0 && d->mapped ? d->rig.map[node] : -1;
    return n >= 0 && !node_has_mesh(&d->w, n);
}

int actor_head(int who, float* out)
{
    Disguise* d = &dg[who];
    ML_WORK* m = d->w.mlwP;
    int i, k = d->rig.ok ? d->rig.map[5] : -1;
    if (d->model < 0 || !d->posed || !m || !m->owP || m->obj_num <= 0) return 0;
    if (k < 0 || k >= (int)m->obj_num)   /* no head found: the node farthest above (y up) its position */
        for (i = k = 0; i < (int)m->obj_num; i++)
            if (m->owP[i].mtx[13] > m->owP[k].mtx[13]) k = i;
    memcpy(out, &m->owP[k].mtx[12], 3 * sizeof(float));
    return 1;
}
