/* VR: the magazine in the left hand. Reloading, the left hand takes the ammo from the pouch at the holster (vr_glue.c mag_held): the game's own magazine model - the one that drops
   to the floor when B is pressed (bhSetMagazine, effect 7, sys->efm[7].objP[mdlver]) - is drawn at the left hand until it is put in the gun. It is drawn as bhDrawMdfEffect draws
   an effect model, from the hand node, as a hand object (tag 3): the renderer then moves it with the left controller at every headset image (stream.c g_hand_M). Called at the end
   of bhDrawEffect (effect.c). */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/player.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NinjaCnk.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaGraphics3D.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaSystem.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

int vr_mag_held(void);   /* vr_glue.c: the left hand holds the ammo taken from the pouch */
int vr_held_arrow(void);   /* vr_glue.c: the bow gun's arrow held (0 the arrow, 1 the explosive one), -1 none */
int vr_on(void);
void vr_log_str(const char* s);
extern int g_native_tag;
void njCnkModDrawModel(NJS_CNK_MODEL* model);

int bhSetEffectTb(EF_WORK* efp, NJS_POINT3* off, unsigned char* lkp, int lkono);

/* The Magnum's round in the hand is the item's own model - the one the inventory shows when it is examined: the item archive holds it as file (item id - 1), a model and its textures (system.c,
   mn_mode0 6 reads it for the inventory). It is read here once, into a buffer of its own, with the game's file calls only while no read of the game's is going, realized (bhMlbBinRealize,
   bhSetMemPvpTexture) and kept: nothing of the game's state is touched, and it is not an object of the room. */
int vr_held_item(void);   /* vr_glue.c: the ammo item the gun in hand takes (the first in the inventory) */
int bhMlbBinRealize(void* bin_datP, ML_WORK* mlwP);
unsigned char* bhKeepObjWork(ML_WORK* mp, unsigned char* sp);
int bhSetMemPvpTexture(NJS_TEXLIST* tlp, unsigned char* datp, int offset);
int RequestReadInsideFile(unsigned int PartitionId, unsigned int FileId, void* DestPtr);
int GetInsideFileSize(unsigned int PartitionId, unsigned int FileId);
int GetReadFileStatus();
typedef struct { int item, state, wait, sz; unsigned char* buf; ML_WORK mdl; } ItemModel;
static ItemModel item_models[3];   /* (the Magnum's rounds: items 13 and 30) */
static void item_flag_erase(NJS_CNK_OBJECT* op)   /* as itemview.c FlagErase: the object's own position and turn are used */
{
    for (; op; op = op->sibling) {
        op->evalflags &= ~0x3;
        if (op->child) item_flag_erase(op->child);
    }
}
static ItemModel* item_model(int item)
{
    ItemModel* m = NULL;
    if (item <= 0) return NULL;
    for (int i = 0; i < 3; i++) if (item_models[i].item == item) m = &item_models[i];
    for (int i = 0; i < 3 && !m; i++) if (!item_models[i].item) { m = &item_models[i]; m->item = item; }
    if (!m || m->state < 0) return NULL;
    if (m->state == 0) {
        int sz;
        unsigned char* a;
        if (GetReadFileStatus() == 1) return NULL;   /* the game is reading */
        sz = GetInsideFileSize(sys->itm_partid, item - 1);
        if (sz <= 0) { m->state = -1; return NULL; }
        m->sz = sz;
        m->buf = (unsigned char*)malloc((size_t)sz + 65536 + 64);   /* the file, then room for the model's object work */
        if (!m->buf) { m->state = -1; return NULL; }
        a = (unsigned char*)(((size_t)m->buf + 63) & ~(size_t)63);
        RequestReadInsideFile(sys->itm_partid, item - 1, a);
        m->state = 1; m->wait = 16;
        return NULL;
    }
    if (m->state == 1) {
        unsigned char *a, *d, *mlb;
        if (m->wait > 0) m->wait--;
        if (GetReadFileStatus() != 0 || m->wait > 0) return NULL;
        a = (unsigned char*)(((size_t)m->buf + 63) & ~(size_t)63);
        d = a + 4; mlb = d; d += *(int*)a;   /* the model, then the textures (system.c case 6) */
        if (*(int*)d & 0x80000000) d = (unsigned char*)(((size_t)d + 4 + 31) & ~(size_t)31); else d += 4;
        bhMlbBinRealize(mlb, &m->mdl);
        bhSetMemPvpTexture(m->mdl.texP, d, 0);
        bhKeepObjWork(&m->mdl, (unsigned char*)(((size_t)a + (size_t)m->sz + 63) & ~(size_t)63));
        if (m->mdl.objP) item_flag_erase(m->mdl.objP);
        m->state = 2;
        { char ln[120]; snprintf(ln, sizeof ln, "vr: item %d model read (%d bytes)", item, m->sz); vr_log_str(ln); }
        return NULL;
    }
    return m->state == 2 && m->mdl.objP ? m : NULL;
}
static float pose[6][7];   /* (the hand poses, below) */
/* the item's model in the left hand, as the magazine is: from the hand node, as a hand object (tag 3) */
static void draw_item_ammo(int item)
{
    ItemModel* im = item_model(item);
    NJS_CNK_OBJECT* o;
    float mtx[16], sc;
    if (!im) return;
    for (o = im->mdl.objP; o && !o->model; o = o->child) ;
    sc = o && o->model->r > 0.01f ? 0.4f / o->model->r : 1.0f;   /* 8 cm across (a radius of 0.4 units) */
    njCnkSetEasyLightColor(1.0f, 1.0f, 1.0f);
    njCnkSetEasyLightIntensity(0.7f, 0.2f);
    njCnkSetEasyLight(0, 0.3f, 0.7f);
    njTextureFilterMode(1);
    njSetTexture(im->mdl.texP);
    njPushMatrixEx();
    njSetMatrix(mtx, &plp->mlwP->owP[13].mtx);
    njTranslate(mtx, pose[5][0], pose[5][1], pose[5][2]);
    njRotateXYZ(mtx, (Angle)pose[5][3], (Angle)pose[5][4], (Angle)pose[5][5]);
    njScale(mtx, sc * pose[5][6], sc * pose[5][6], sc * pose[5][6]);
    njMultiMatrix(NULL, mtx);
    g_native_tag = 3;
    njCnkEasyDrawObject(im->mdl.objP);
    g_native_tag = 0;
    njPopMatrixEx();
}

/* B with the shotgun: one of its shells falls from the hand, the game's casing effect (effect 0, model 1, as WpnEffTab[11][3] ejects it; the effect is a "polygon model" drawn with the texture 5). */
void pc_vr_drop_shell(void)
{
    EF_WORK* eft = &sys->ef;
    NJS_POINT3 ops;
    eft->id = 0; eft->flg = 0x40601; eft->type = 10; eft->flr_no = sys->yk_ct; eft->mdlver = 1;
    eft->ay = plp->ay; eft->ax = 0; eft->sx = eft->sy = eft->sz = 1.0f;
    ops.x = 1.0f; ops.y = 0; ops.z = 0;
    if (bhSetEffectTb(eft, &ops, (unsigned char*)plp, 9) >= 0) sys->yk_ct = (sys->yk_ct + 1) & 0x1F;
}

/* where the magazine sits in the hand, in the left hand node's frame: x, y, z (game units), rotation (angles, 65536 = 360). RECVX_MAG_POSE="x,y,z,ax,ay,az,scale" (env.txt) tunes it. */
/* one pose per magazine model: [0] the pistols' 9 mm, [1] weapon 12 (the shotgun), [2] weapon 13. Tuned by the owner on the headset (pistol); the others start as the pistol's. */
static float pose[6][7] = {
    { -0.85f, -0.4f, 0.0f, 0, 0, 32768, 1.0f },
    { -0.85f, -0.4f, 0.0f, 0, 0, 32768, 1.0f },
    { -0.85f, -0.4f, 0.0f, 0, 0, 32768, 1.0f },
    { -0.85f, -0.4f, 0.0f, 0, 0, 32768, 1.0f },   /* [3]: the shotgun's shell */
    { -0.85f, -0.4f, 0.0f, 0, 32768, 0, 1.0f },   /* [4]: the bow gun's arrow (effect 13: the bolt that flies), turned 180 degrees about the up axis */
    { -0.85f, -0.4f, 0.0f, 0, 0, 0, 1.0f } };   /* [5]: the Magnum's round (the item's model, made 8 cm across) */

void pc_vr_held_magazine(void)
{
    static int init;
    ML_WORK* mlp;
    float mtx[16];
    int ver, shell, rev;
    int show = vr_mag_held(), arrow = vr_held_arrow(), pi;
    static int enabled = 1;   /* (RECVX_MAG_POSE, _POSE1, _POSE2 in env.txt tune the poses: test aid) */
    if (!enabled) return;
    if (!init) {
        const char* nm[6] = { "RECVX_MAG_POSE", "RECVX_MAG_POSE1", "RECVX_MAG_POSE2", "RECVX_MAG_POSE3", "RECVX_MAG_POSE4", "RECVX_MAG_POSE5" };
        int k;
        init = 1;
        for (k = 0; k < 6; k++) { const char* e = getenv(nm[k]); if (e) sscanf(e, "%f,%f,%f,%f,%f,%f,%f", &pose[k][0], &pose[k][1], &pose[k][2], &pose[k][3], &pose[k][4], &pose[k][5], &pose[k][6]); }
        if (getenv("RECVX_SHOW_MAG")) init = 2;   /* test aid: shown always */
    }
    if ((!show && arrow < 0 && init != 2) || !vr_on() || !plp || !plp->mlwP || plp->mlwP->obj_num <= 13) return;
    shell = plp->wpnr_no >= 0 && plp->wpnr_no < 23 && (WpnTab[plp->wpnr_no].flg & 0x1000);   /* the shotgun: its shell (effect 0, model 1, texture 5), not a magazine */
    rev = plp->wpnr_no >= 0 && plp->wpnr_no < 23 && (WpnTab[plp->wpnr_no].flg & 0x10) && !(WpnTab[plp->wpnr_no].flg & 0x80) && !shell;   /* the Magnum (loaded round by round, flag 0x10): its round is the item's own model */
    if (rev) { draw_item_ammo(vr_held_item()); return; }   /* the Magnum's round: the item's own model */
    mlp = &sys->efm[arrow >= 0 ? 13 : shell ? 0 : 7];
    ver = arrow >= 0 ? arrow : shell ? 1 : plp->wpnr_no == 12 ? 1 : plp->wpnr_no == 13 ? 2 : 0;
    pi = arrow >= 0 ? 4 : shell ? 3 : ver;   /* which pose */
    if (!mlp->objP || !mlp->objP[ver].model) return;
    /* The falling magazine is a "polygon model" effect (bhDrawPolEffect): the effect texture list moved to texture 7 (op->tex_id), texture 0 of that, drawn by njCnkEasyDrawModel
       with the effect light. The same here. */
    njCnkSetEasyLightColor(1.0f, 1.0f, 1.0f);
    njCnkSetEasyLightIntensity(0.7f, 0.2f);
    njCnkSetEasyLight(0, 0.3f, 0.7f);
    njTextureFilterMode(1);
    {
        NJS_TEXNAME* tnp = sys->ef_tlist.textures;
        sys->ef_tlist.textures = &tnp[arrow >= 0 ? 8 : shell ? 5 : 7];
        njSetTexture(&sys->ef_tlist);
        njSetTextureNum(0);
        njPushMatrixEx();
        njSetMatrix(mtx, &plp->mlwP->owP[13].mtx);
        njTranslate(mtx, pose[pi][0], pose[pi][1], pose[pi][2]);
        njRotateXYZ(mtx, (Angle)pose[pi][3], (Angle)pose[pi][4], (Angle)pose[pi][5]);
        njScale(mtx, pose[pi][6], pose[pi][6], pose[pi][6]);
        njMultiMatrix(NULL, mtx);
        g_native_tag = getenv("RECVX_MAG_TAG") ? atoi(getenv("RECVX_MAG_TAG")) : 3;
        njCnkEasyDrawModel(mlp->objP[ver].model);
        g_native_tag = 0;
        njPopMatrixEx();
        sys->ef_tlist.textures = tnp;
    }
}

/* test aid (vr_debug_log): the objects linked to the right hand (the weapon) - their id, nodes and each node's position/angle/flags, to find the pistol's slide */
void pc_slide_trace(int tag, int rounds)
{
    int pass, i;
    for (pass = 0; pass < 2; pass++) {
        int n = pass ? sys->ob_hlgn : sys->ob_nlgn;
        for (i = 0; i < n; i++) {
            O_WRK* op = pass ? sys->ob_hlg[i] : sys->ob_nlg[i];
            char ln[600]; int k, m = 0;
            ML_WORK* ml;
            if (!op || op->lkwkp != (unsigned char*)plp || op->lkono != 9 || !op->mlwP) continue;
            ml = op->mlwP;
            { int mi = -1, q; for (q = 0; q < 16; q++) if (ml == &op->mdl[q]) mi = q; m += snprintf(ln + m, sizeof ln - m, "mdl_n %d mdlver %d mdl_no %d cur %d frm %d mode %d/%d/%d ", (int)op->mdl_n, (int)op->mdlver, (int)op->mdl_no, mi, (int)op->frm_no, op->mode0, op->mode1, op->mode2); }
            m += snprintf(ln + m, sizeof ln - m, "slide-obj %d: id %d nodes %d rounds %d mode %d/%d/%d:", tag, (int)op->id, (int)ml->obj_num, rounds, plp->mode1, plp->mode2, plp->mode3);
            for (k = 0; k < (int)ml->obj_num && k < 12 && m < 520; k++) {
                NJS_CNK_OBJECT* o = &ml->objP[k];
                m += snprintf(ln + m, sizeof ln - m, " [%d f%x] %.2f %.2f %.2f a %d %d %d", k, (unsigned)o->evalflags, o->pos[0], o->pos[1], o->pos[2], (int)o->ang[0], (int)o->ang[1], (int)o->ang[2]);
            }
            vr_log_str(ln);
        }
    }
}

/* the weapon object's matrix, as the game draws the weapon in the right hand (bhDrawObject): the hand node's, then the object's own offset and turn (the turn differs with and without the aim) -
   what the ammo guide (vr_glue.c) is fixed to */
int pc_vr_weapon_frame(float m[16])
{
    O_WRK* op = sys ? (O_WRK*)sys->obwp : NULL;
    if (!op || !plp || !plp->mlwP || plp->mlwP->obj_num <= 9) return 0;
    njSetMatrix(m, &plp->mlwP->owP[9].mtx);
    njTranslate(m, op->lox, op->loy, op->loz);
    njRotateXYZ(m, op->ax, op->ay, op->az);
    return 1;
}

/* The pistols' magazine guide (vr_glue.c guide_draw): the magazine's own model - the game's, as in the hand - drawn at the fitting place in the weapon's frame as a light: orange, translucent,
   a little larger, blinking as the ammo guide does, so the owner sees where to put it (and, adjusting, where it is). Called at the end of bhDrawEffect, with the held magazine. */
int vr_mag_guide(float c[3], float* a, float* yaw, float* pitch);   /* vr_glue.c: the guide's middle in the weapon's frame (game units), its alpha this frame and its turns (degrees); 0 none */
extern int g_native_notex;   /* stream.c */
int pc_vr_weapon_frame(float m[16]);
extern unsigned int _nj_control_3d_flag_;

/* the model's longest axis, from its vertices: the effect model's vertex list (a header, then vertices of some size in words) is read with the stride at which every vertex lies within the model's
   radius of its centre; -1 when none does (then the model's y is taken). Logged once. */
static int mag_thin = 2;   /* the model's thinnest axis (the width is added there) */
static int mag_axis(NJS_CNK_MODEL* md, float cen[3])
{
    static NJS_CNK_MODEL* done;
    static int axis = -1;
    static float c0[3];
    int o, st, n, i;
    if (done == md) { for (i = 0; i < 3; i++) cen[i] = c0[i]; return axis; }
    done = md; axis = -1;
    for (i = 0; i < 3; i++) c0[i] = (&md->center.x)[i];
    n = (int)((unsigned int)md->vlist[1] >> 16);
    if (n > 0 && n < 1000 && md->r > 0.01f) {
        float bestscore = 1e9f;
        for (o = 2; o <= 4; o++)
            for (st = 3; st <= 9; st++) {
                float mn[3] = { 1e9f, 1e9f, 1e9f }, mx[3] = { -1e9f, -1e9f, -1e9f };
                int ok = 1;
                for (i = 0; i < n && ok; i++) {
                    const float* v = (const float*)&md->vlist[o + i * st];
                    float d = 0;
                    for (int k = 0; k < 3; k++) {
                        float t = v[k];
                        if (!(t > -100.0f && t < 100.0f)) { ok = 0; break; }
                        if (t < mn[k]) mn[k] = t;
                        if (t > mx[k]) mx[k] = t;
                        d += (t - c0[k]) * (t - c0[k]);
                    }
                    if (ok && d > md->r * md->r * 1.7f) ok = 0;
                }
                if (ok) {   /* the reading whose box fits the model's centre and radius */
                    float err = 0, hd = 0, e[3] = { mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2] };
                    for (i = 0; i < 3; i++) { float cc = (mn[i] + mx[i]) * 0.5f; err += (cc - (&md->center.x)[i]) * (cc - (&md->center.x)[i]); hd += e[i] * e[i] * 0.25f; }
                    err = sqrtf(err) + fabsf(sqrtf(hd) - md->r);
                    if (err < bestscore && err <= md->r * 0.35f) {
                        bestscore = err;
                        axis = e[0] >= e[1] && e[0] >= e[2] ? 0 : e[1] >= e[2] ? 1 : 2;
                        mag_thin = e[0] <= e[1] && e[0] <= e[2] ? 0 : e[1] <= e[2] ? 1 : 2;
                        for (i = 0; i < 3; i++) c0[i] = (mn[i] + mx[i]) * 0.5f;
                        { char ln[200]; snprintf(ln, sizeof ln, "vr: magazine guide: model %d vertices (header %d, stride %d), size %.2f %.2f %.2f, long axis %d, thin %d", n, o, st, e[0], e[1], e[2], axis, mag_thin); vr_log_str(ln); }
                    }
                }
            }
    }
    if (axis < 0) { axis = 1; mag_thin = 2; vr_log_str("vr: magazine guide: the model's vertices were not found - its y taken as the long axis"); }
    for (i = 0; i < 3; i++) cen[i] = c0[i];
    return axis;
}

void pc_vr_mag_guide(void)
{
    float c[3], a, cen[3], mtx[16], yaw, pitch;
    NJS_ARGB col;
    NJS_CNK_MODEL* md;
    int axis;
    unsigned int c3df = _nj_control_3d_flag_;
    if (!vr_on() || !plp || !vr_mag_guide(c, &a, &yaw, &pitch)) return;
    if (!sys->efm[7].objP || !(md = sys->efm[7].objP[0].model) || !md->vlist) return;
    if (!pc_vr_weapon_frame(mtx)) return;
    axis = mag_axis(md, cen);
    /* the light breathes in its colour (added to what is behind it), not in its alpha: the alpha did not reach the picture */
    col.a = 0.75f; col.r = a; col.g = a * 140.0f / 255.0f; col.b = a * 26.0f / 255.0f;   /* the ammo guide's orange, 0xFF8C1A, at a (0.15 .. 0.6) */
    njTextureFilterMode(1);
    {
        NJS_TEXNAME* tnp = sys->ef_tlist.textures;
        sys->ef_tlist.textures = &tnp[7];
        njSetTexture(&sys->ef_tlist);
        njSetTextureNum(0);
        njPushMatrix(cam.mtx);   /* as the weapon's own nodes are drawn (MdlPut.c): the view, then the object's matrix */
        njTranslate(mtx, c[0], c[1], c[2]);   /* its middle at the guide's */
        njRotateXYZ(mtx, 0, 0, (Angle)(int)(yaw * (65536.0f / 360.0f)));     /* turned about its own long axis (the weapon's z) */
        njRotateXYZ(mtx, 0, (Angle)(int)(pitch * (65536.0f / 360.0f)), 0);   /* tipped forward or back (about the weapon's y) */
        if (axis == 0) njRotateXYZ(mtx, 0, 16384, 0);   /* the long axis along the weapon's z (the pistols' magazine is upright there) */
        else if (axis == 1) njRotateXYZ(mtx, 16384, 0, 0);
        njScale(mtx, 1.15f, 1.15f, 1.15f);   /* 15 % larger than the magazine (the owner) */
        njTranslate(mtx, -cen[0], -cen[1], -cen[2]);
        njMultiMatrix(NULL, mtx);
        njSetConstantMaterial(&col);
        njControl3D(c3df | 0x10);   /* constant material */
        njColorBlendingMode(0, 8);   /* source alpha, added to what is there */
        njColorBlendingMode(1, 10);
        g_native_tag = 2;     /* the gun's hand: the renderer moves it with the weapon, at every headset image */
        g_native_notex = 1;   /* a light: the shape only, no texture */
        njCnkEasyDrawModel(md);
        g_native_notex = 0;
        g_native_tag = 0;
        njControl3D(c3df);
        njColorBlendingMode(0, 8);
        njColorBlendingMode(1, 6);   /* the game's usual */
        njPopMatrixEx();
        sys->ef_tlist.textures = tnp;
    }
}

/* ---------------------------------------------------------------- the pistol slide's zone to take it
   The left grip takes the slide anywhere on its own piece (the one that glows orange while it has to be pulled), from its front end to its back end, and also from above: the zone reaches 4 cm
   over it, so the left hand comes down on the slide and does not hit the right controller. vr_glue.c asks pc_slide_zone once per frame with the left hand's place.

   How the zone is known, with nothing read from the model's vertices (their layout is not known to us):
   * where the slide is: the slide's node (the weapon object's node 2) has a matrix N. The game's own (owP[2]) is used when it lies where a slide can (near the weapon's origin - it is not filled for every
     weapon object); else N is worked out from the object's tree as the game draws it: the hand's matrix and the object's offsets (pc_vr_weapon_frame), then each node's position, turn and scale.
   * how big it is: every model carries its centre and radius. The slide is long and thin, so the radius is a little more than half its length: that is the zone's half length along the slide's own x
     axis (the one it moves on: its position x goes 0 .. -0.5 in a shot). A radius outside 5 .. 20 cm is not a slide's: the old point is then used.
   * its section: the thickness of a slide is about 2 cm either side of its middle; the zone takes 1.5 cm more all round.
   * above it: the hand is tried 0, 1 .. 4 cm lower along the world's up (the game's y), so the zone reaches 4 cm higher than the box.
   Units: a game unit is 10 cm (vr_scale 100), as the slide's travel (0.5 unit = 5 cm) and everything in the weapon's frame.
   The left hand has two points: the controller's, and where the hand holds things (the left hand node's frame at the place the magazine is held: what the owner sees touch the slide - the controller
   itself is about 12 cm to the side of it). Either in the zone takes the slide.
   Return: a mask, bit 0 the controller's point in the zone, bit 1 the holding point; 0 out; -1 the zone is not known (and why is in recvx.log, once). along[i]: point i along the slide's own x axis, in game
   units, the pull is measured on it (the slide goes to -x when pulled: a hand that goes back along the barrel makes it smaller). */
int vr_mat_invert(float out[16], const float in[16]);   /* vr_math.c */
uint64_t SDL_GetTicks(void);   /* SDL3 (this file does not include SDL) */
void njRotateX(NJS_MATRIX* m, Angle ang);
void njRotateY(NJS_MATRIX* m, Angle ang);
void njRotateZ(NJS_MATRIX* m, Angle ang);

#define SLIDE_R_MIN 0.5f           /* a slide's radius: 5 .. 20 cm */
#define SLIDE_R_MAX 2.0f
#define SLIDE_HALF_SECTION 0.20f   /* half its thickness about its middle: 2 cm */
#define SLIDE_MARGIN 0.15f         /* 1.5 cm all round */
#define SLIDE_ABOVE_CM 4           /* and 4 cm above */

/* the world matrix of `target` in the tree under `o`, parent matrix `parent` (as njCnkEasyDrawObject builds it) */
static int tree_world(NJS_CNK_OBJECT* o, NJS_CNK_OBJECT* target, const float* parent, float out[16])
{
    for (; o; o = o->sibling) {
        float t[16];
        unsigned int f = o->evalflags;
        memcpy(t, parent, sizeof t);
        if (!(f & 0x1)) njTranslate((NJS_MATRIX*)t, o->pos[0], o->pos[1], o->pos[2]);
        if (!(f & 0x2)) {
            if (f & 0x20) { njRotateY((NJS_MATRIX*)t, o->ang[1]); njRotateX((NJS_MATRIX*)t, o->ang[0]); njRotateZ((NJS_MATRIX*)t, o->ang[2]); }
            else { njRotateZ((NJS_MATRIX*)t, o->ang[2]); njRotateY((NJS_MATRIX*)t, o->ang[1]); njRotateX((NJS_MATRIX*)t, o->ang[0]); }
        }
        if (!(f & 0x4)) njScale((NJS_MATRIX*)t, o->scl[0], o->scl[1], o->scl[2]);
        if (o == target) { memcpy(out, t, sizeof t); return 1; }
        if (!(f & 0x10) && o->child && tree_world(o->child, target, t, out)) return 1;
    }
    return 0;
}

/* p (row vector, w = 1) through the matrix m (translation in 12 .. 14) */
static void mat_point(const float* m, const float p[3], float out[3])
{
    for (int k = 0; k < 3; k++) out[k] = m[k] * p[0] + m[4 + k] * p[1] + m[8 + k] * p[2] + m[12 + k];
}

/* the reason the zone is not known, in recvx.log, the first times */
static int slide_fail(const char* why, float a, float b)
{
    static int told;
    if (told++ < 6) {
        char ln[200];
        snprintf(ln, sizeof ln, "vr: slide zone: not known (%s %.3f %.3f): the old point is used", why, a, b);
        vr_log_str(ln);
    }
    return -1;
}

int pc_slide_zone(const float P2[2][3], float along[2])
{
    O_WRK* op = sys ? (O_WRK*)sys->obwp : NULL;
    ML_WORK* ml;
    NJS_CNK_OBJECT* so;
    NJS_CNK_MODEL* md;
    float W[16], iW[16], N[16], iN[16], o_w[3], q0[3], xd[3], l, r;
    int k, t, i, in[2] = { 0, 0 }, src = 0;
    if (!pc_vr_weapon_frame(W)) return slide_fail("no weapon frame", 0, 0);
    W[3] = W[7] = W[11] = 0.0f; W[15] = 1.0f;   /* (a proper 4x4 to invert) */
    if (!vr_mat_invert(iW, W)) return slide_fail("weapon frame not invertible", 0, 0);
    if (!op || !(ml = op->mlwP) || !ml->objP || !ml->owP || ml->obj_num < 3) return slide_fail("no weapon object with a slide node", 0, 0);

    /* the slide's piece: its node, or the first node under it with a model */
    so = &ml->objP[2];
    for (; so && !so->model; so = so->child) ;
    if (!so || !(md = so->model)) return slide_fail("node 2 has no model", 0, 0);
    r = md->r;
    if (!(r >= SLIDE_R_MIN && r <= SLIDE_R_MAX)) return slide_fail("radius out of a slide's range", r, 0);

    /* its matrix: the game's when it lies where a slide can, else from the tree */
    memcpy(N, ml->owP[2].mtx, sizeof N);
    N[3] = N[7] = N[11] = 0.0f; N[15] = 1.0f;
    o_w[0] = N[12]; o_w[1] = N[13]; o_w[2] = N[14];
    mat_point(iW, o_w, q0);
    if (q0[0] * q0[0] + q0[1] * q0[1] + q0[2] * q0[2] < 25.0f) src = 1;   /* (within 50 cm of the weapon's origin) */
    if (!src) {
        if (!tree_world(ml->objP, &ml->objP[2], W, N)) return slide_fail("slide node not in the tree", 0, 0);
        N[3] = N[7] = N[11] = 0.0f; N[15] = 1.0f;
        o_w[0] = N[12]; o_w[1] = N[13]; o_w[2] = N[14];
        mat_point(iW, o_w, q0);
        if (!(q0[0] * q0[0] + q0[1] * q0[1] + q0[2] * q0[2] < 25.0f)) return slide_fail("slide node far from the weapon", q0[0], q0[1]);
        src = 2;
    }
    if (!vr_mat_invert(iN, N)) return slide_fail("slide matrix not invertible", 0, 0);

    /* the slide's own x axis in the world, and the hand along it */
    xd[0] = N[0]; xd[1] = N[1]; xd[2] = N[2];
    l = sqrtf(xd[0] * xd[0] + xd[1] * xd[1] + xd[2] * xd[2]);
    if (!(l > 1e-6f)) return slide_fail("slide axis empty", 0, 0);
    for (k = 0; k < 3; k++) xd[k] /= l;

    /* the zone: the box round the model's centre in the slide's own axes, each point tried 0 .. 4 cm lower in the world */
    for (i = 0; i < 2; i++) {
        const float* P = P2[i];
        along[i] = P[0] * xd[0] + P[1] * xd[1] + P[2] * xd[2];
        for (t = 0; t <= SLIDE_ABOVE_CM && !in[i]; t++) {
            float ph[3] = { P[0], P[1] - 0.1f * t, P[2] }, pl[3];   /* 0.1 unit = 1 cm; the game's y is up */
            mat_point(iN, ph, pl);
            in[i] = fabsf(pl[0] - md->center.x) <= r + SLIDE_MARGIN &&
                    fabsf(pl[1] - md->center.y) <= SLIDE_HALF_SECTION + SLIDE_MARGIN &&
                    fabsf(pl[2] - md->center.z) <= SLIDE_HALF_SECTION + SLIDE_MARGIN;
        }
    }

    {   /* what it is, every 3 s and when the hand goes in or out of it */
        static int was = -1;
        static uint64_t last;
        uint64_t now = SDL_GetTicks();
        if (now - last > 3000 || (in[0] | in[1] << 1) != was) {
            float pl[3], pg[3];
            char ln[360];
            mat_point(iN, P2[0], pl);
            mat_point(iN, P2[1], pg);
            last = now; was = in[0] | in[1] << 1;
            snprintf(ln, sizeof ln, "vr: slide zone (matrix %s): radius %.2f centre %.2f %.2f %.2f; the controller in the slide's axes %.2f %.2f %.2f (%s), the holding point %.2f %.2f %.2f (%s)",
                     src == 1 ? "the game's" : "from the tree", r, md->center.x, md->center.y, md->center.z, pl[0], pl[1], pl[2], in[0] ? "IN" : "out", pg[0], pg[1], pg[2], in[1] ? "IN" : "out");
            vr_log_str(ln);
        }
    }
    return in[0] | in[1] << 1;
}
