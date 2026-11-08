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
#include <stdlib.h>
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
