/*
 * VR stereo, game side (stage 2): during normal play, while the headset shows our frames
 * (xr_stereo_ready), the third-person camera hook hands the view to the head: the camera sits
 * at the player's eyes and looks where the head looks, turned with the character (the
 * character's facing is the body; the head turns on top of it). The renderer re-projects each
 * eye from this camera at display time, so the head moving between game frames is corrected
 * there (gs/gs_gl.c present_vr_stereo).
 *
 * Per frame the renderer gets (stream_vr_view): G, world -> the game's view space (cam.mtx as
 * drawn), and W, tracking space -> world (where the play space sits around the player this
 * frame). The player's own model is not drawn while the VR camera is in use (the camera is
 * inside the head).
 *
 * Stage 3, with Touch controllers: the view's heading (vr_yaw) is the player's own - the right
 * stick turns it (snap or smooth), the character turning to walk or aim does not. The left stick
 * walks relative to where the head looks (the third-person "camera A" walking, its camera yaw
 * being the head's), the right hand aims (vr_hand_ray, free aim). Without controllers (a gamepad)
 * the heading follows the character's facing as in stage 2.
 *
 * Game thread only. Hooks: tps_camera() end (vr_camera), tps_after_draw() (vr_after_draw),
 * game.c around bhPutModel(plp) (vr_hide_player), tps_glue.c (vr_on, vr_hand_ray),
 * tps_input.c (vr_tps_keys), pad.c (vr_pad_start, vr_quick_knife).
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_Vu1Strip.h"
#include "../src/host/xr.h"
#include "../src/gs/stream.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

int g_vr_scale = 100;        /* ini vr_scale: game units per metre, in tenths (100 = 10 units/m: the character is ~17 tall) */
int g_vr_eye_height = 155;   /* ini vr_eye_height: the camera's height above the player's feet, tenths of units */
int g_vr_fov = 120;          /* ini vr_fov: the game camera's horizontal field of view (degrees): what it culls against */
int g_vr_turn = 30;          /* ini vr_turn: right-stick turn in degrees per flick; 0 = smooth turning */
int g_vr_gun_min = 15;       /* ini vr_gun_min: the hand/weapon never comes closer than this to the eyes (cm) */
int g_vr_fx_fix = 1;         /* ini vr_fx_fix: 1 = 2D effects with a depth (fire, glows) are placed in the world */
int g_vr_hand_aim = 1;       /* ini vr_hand_aim: 1 = shots go where the right controller points, 0 = where the head looks */

static int vr_frame;         /* the VR camera set this frame's view (until vr_after_draw) */
static int vr_active;        /* the VR camera drew the last frame */
static float frame_W[16];    /* this frame's local -> world */
static float frame_Wg[16];   /* this frame's recentred tracking space -> world */
static float vr_yaw;         /* the view's heading (radians, forward (-sin, 0, -cos)) */
static unsigned rec_seen;    /* xr_recenter_gen at the last heading reset */
static int snap_held;        /* the right stick is still out after a snap turn */

void tps_vr_view(float yaw, float pitch);   /* tps_glue.c: its camera yaw/pitch (game angle units) */

#define TO_ANGLE(r) ((r) * (32768.0f / 3.14159265f))   /* radians -> game angle units (65536 = 360 deg) */

int vr_hide_player(void) { return vr_frame; }

static void quat_to_rows(float m[9], const VrPose* p)
{
    float x = p->qx, y = p->qy, z = p->qz, w = p->qw;
    m[0] = 1 - 2 * (y * y + z * z); m[1] = 2 * (x * y - z * w);     m[2] = 2 * (x * z + y * w);
    m[3] = 2 * (x * y + z * w);     m[4] = 1 - 2 * (x * x + z * z); m[5] = 2 * (y * z - x * w);
    m[6] = 2 * (x * z - y * w);     m[7] = 2 * (y * z + x * w);     m[8] = 1 - 2 * (x * x + y * y);
}

/* end of tps_camera(), the third-person camera already in `cam`: replace it with the head's */
void vr_camera(void)
{
    VrPose h;
    float s = g_vr_scale / 10.0f, yb, cb, sb, hr[9], R[9], ax[3];
    float Wg[16], rec[16];
    vr_frame = 0;
    {
        static int last = -1;
        int why = !plp ? 1 : !xr_stereo_ready() ? 2 : !xr_head_pose(&h) ? 3 : 0;
        if (why != last) {
            extern void host_log(const char* fmt, ...);
            static const char* names[] = { "on", "off (no player)", "off (headset not showing us yet)", "off (no head pose yet)" };
            host_log("vr: camera %s\n", names[why]);
            last = why;
        }
        if (why) { vr_active = 0; return; }
    }

    {   /* heading: the player's own with controllers (right stick), else the character's facing */
        VrInput in;
        float facing = (float)(short)plp->ay * (3.14159265f / 32768.0f);
        int ctl = xr_input(&in);
        if (!vr_active || !ctl || rec_seen != xr_recenter_gen()) { vr_yaw = facing; rec_seen = xr_recenter_gen(); }
        if (ctl) {
            if (g_vr_turn > 0) {
                if (fabsf(in.rx) > 0.7f && !snap_held) { vr_yaw -= (in.rx > 0 ? 1.0f : -1.0f) * g_vr_turn * (3.14159265f / 180.0f); snap_held = 1; }
                else if (fabsf(in.rx) < 0.3f) snap_held = 0;
            } else if (fabsf(in.rx) > 0.15f) vr_yaw -= in.rx * (3.14159265f / 180.0f) * 4.0f;   /* ~120 deg/s at 30 fps */
        }
    }
    yb = vr_yaw;
    cb = cosf(yb); sb = sinf(yb);

    /* Wg: recentred tracking space (metres) -> world: scaled, turned with the body, the
       recentred origin (the head at recentring) at the player's eyes */
    vr_mat_identity(Wg);
    Wg[0] = cb * s;  Wg[8] = sb * s;
    Wg[5] = s;
    Wg[2] = -sb * s; Wg[10] = cb * s;
    Wg[12] = plp->px; Wg[13] = plp->py + g_vr_eye_height / 10.0f; Wg[14] = plp->pz;
    xr_recenter_matrix(rec);
    vr_mat_mul(frame_W, Wg, rec);   /* local -> world */
    memcpy(frame_Wg, Wg, sizeof(Wg));

    /* camera: the head's position and orientation in the world */
    cam.px = Wg[0] * h.px + Wg[4] * h.py + Wg[8] * h.pz + Wg[12];
    cam.py = Wg[1] * h.px + Wg[5] * h.py + Wg[9] * h.pz + Wg[13];
    cam.pz = Wg[2] * h.px + Wg[6] * h.py + Wg[10] * h.pz + Wg[14];
    quat_to_rows(hr, &h);
    for (int i = 0; i < 3; i++) {   /* R = Ry(yb) * head */
        R[0 * 3 + i] = cb * hr[0 * 3 + i] + sb * hr[2 * 3 + i];
        R[1 * 3 + i] = hr[1 * 3 + i];
        R[2 * 3 + i] = -sb * hr[0 * 3 + i] + cb * hr[2 * 3 + i];
    }
    /* R = Ry(yaw) Rx(pitch) Rz(roll): the order bhControlCamera applies (inverted) */
    ax[1] = asinf(fmaxf(-1.0f, fminf(1.0f, -R[5])));   /* pitch */
    ax[0] = atan2f(R[2], R[8]);                        /* yaw */
    ax[2] = atan2f(R[3], R[4]);                        /* roll */
    cam.ay = (int)-TO_ANGLE(ax[0]);
    cam.ax = (int)-TO_ANGLE(ax[1]);
    cam.az = (int)-TO_ANGLE(ax[2]);
    cam.ln = 0;
    cam.pers = (int)(g_vr_fov * (65536.0f / 360.0f));
    tps_vr_view(TO_ANGLE(ax[0]), TO_ANGLE(ax[1]));   /* walking and head aim relative to where the head looks */
    vr_frame = 1;
    vr_active = 1;
}

int vr_on(void) { return vr_active && xr_stereo_ready(); }

/* free aim: the right controller's ray in the world (origin, unit direction); 0 if not aiming by hand */
int vr_hand_ray(NJS_POINT3* o, NJS_POINT3* d)
{
    VrInput in;
    float r[9], f[3], c = cosf(vr_yaw), s = sinf(vr_yaw), l;
    if (!g_vr_hand_aim || !vr_on() || !xr_input(&in) || !in.aim_valid) return 0;
    o->x = frame_Wg[0] * in.aim.px + frame_Wg[4] * in.aim.py + frame_Wg[8] * in.aim.pz + frame_Wg[12];
    o->y = frame_Wg[1] * in.aim.px + frame_Wg[5] * in.aim.py + frame_Wg[9] * in.aim.pz + frame_Wg[13];
    o->z = frame_Wg[2] * in.aim.px + frame_Wg[6] * in.aim.py + frame_Wg[10] * in.aim.pz + frame_Wg[14];
    quat_to_rows(r, &in.aim);
    f[0] = -r[2]; f[1] = -r[5]; f[2] = -r[8];          /* -z of the hand */
    d->x = c * f[0] + s * f[2]; d->y = f[1]; d->z = -s * f[0] + c * f[2];   /* turned by the heading */
    l = sqrtf(d->x * d->x + d->y * d->y + d->z * d->z);
    if (l < 1e-4f) return 0;
    d->x /= l; d->y /= l; d->z /= l;
    return 1;
}

/* Touch controllers as the third-person keys (tps_input.c; key-type independent):
   left stick walk (relative to the head in play, the d-pad in menus), left trigger run,
   A action/confirm, B cancel, X map, Y inventory, right grip ready weapon, right trigger fire */
unsigned vr_tps_keys(void)
{
    VrInput in;
    unsigned k = 0;
    if (!xr_input(&in)) return 0;
    if (in.ly > 0.4f) k |= 1;            /* TPS_K_FWD */
    if (in.ly < -0.4f) k |= 2;           /* TPS_K_BACK */
    if (in.lx < -0.4f) k |= 4;           /* TPS_K_LEFT */
    if (in.lx > 0.4f) k |= 8;            /* TPS_K_RIGHT */
    if (in.ltrig > 0.5f || (in.buttons & VRB_LSTICK)) k |= 16;   /* TPS_K_RUN */
    if (in.buttons & VRB_A) k |= 32;     /* TPS_K_ACTION */
    if (in.buttons & VRB_Y) k |= 64;     /* TPS_K_STATUS */
    if (in.buttons & VRB_X) k |= 128;    /* TPS_K_MAP */
    if (in.rgrip > 0.5f) k |= 256;       /* TPS_K_AIM */
    if (in.rtrig > 0.5f) k |= 512;       /* TPS_K_FIRE */
    if (in.buttons & VRB_B) k |= 1024;   /* TPS_K_CANCEL */
    return k;
}

/* the play space in the world now: recentred tracking space (metres) -> world */
static void make_Wg(float Wg[16])
{
    float s = g_vr_scale / 10.0f, c = cosf(vr_yaw), sn = sinf(vr_yaw);
    vr_mat_identity(Wg);
    Wg[0] = c * s;  Wg[8] = sn * s;
    Wg[5] = s;
    Wg[2] = -sn * s; Wg[10] = c * s;
    Wg[12] = plp->px; Wg[13] = plp->py + g_vr_eye_height / 10.0f; Wg[14] = plp->pz;
}

static void xform(const float m[16], float x, float y, float z, float o[3])
{
    o[0] = m[0] * x + m[4] * y + m[8] * z + m[12];
    o[1] = m[1] * x + m[5] * y + m[9] * z + m[13];
    o[2] = m[2] * x + m[6] * y + m[10] * z + m[14];
}

/* Both hands follow the controllers, each on its own: node 9 (right hand: the weapon, the
   muzzle flash and the shot's origin hang on it) the right controller, node 13 (left hand) the
   left one. The hand objects the game links to these nodes (sys->obwp, drawn from owP[lkono])
   follow with them. How a hand holds each weapon relative to the controller is taken from the
   game's own pose: when it aims, the hand's rotation relative to the aim direction Ry(ay) Rx(wax)
   (the gun pointing where the controller points); until the weapon was first aimed, relative to
   the body Ry(ay). The hand is put where the controller is, never closer than vr_gun_min to the
   eyes. The body is not turned by the hands (its facing is the head's, tps_glue.c).
   tps_calc_model(), after the player's model is computed, before the linked objects. */
static void vr_log(const char* fmt, ...);

static int  hand_state[2] = { -1, -1 };   /* last logged: 0 following, 1 no controller pose, 2 no hand node */
static float hand_pos[2][3];               /* where each hand was put this frame (world) */
static int  hand_ok[2];                    /* ... and whether it was */

static void hand_log(int side, int st, int wn)
{
    static const char* names[] = { "following the controller", "no controller pose (hand stays in the game's pose)", "model has no hand node" };
    if (hand_state[side] == st) return;
    hand_state[side] = st;
    vr_log("vr: %s hand %s (weapon %d)\n", side ? "left" : "right", names[st], wn);
}

static void rows_from_angles(float A[9], float ya, float pa)   /* Ry(ya) Rx(pa), rows */
{
    float c1 = cosf(ya), s1 = sinf(ya), c2 = cosf(pa), s2 = sinf(pa);
    A[0] = c1;  A[1] = s1 * s2; A[2] = s1 * c2;
    A[3] = 0;   A[4] = c2;      A[5] = -s2;
    A[6] = -s1; A[7] = c1 * s2; A[8] = c1 * c2;
}

static void learn(float out[9], const float A[9], const float* H)   /* out = A^T * Hrot (Hrot[r][c] = H[c * 4 + r]) */
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            out[i * 3 + j] = A[0 * 3 + i] * H[j * 4 + 0] + A[1 * 3 + i] * H[j * 4 + 1] + A[2 * 3 + i] * H[j * 4 + 2];
}

static void log_hand_objects(void)
{
    static unsigned last;
    unsigned sig = 0;
    char buf[256];
    int n = 0;
    if (!sys || !sys->obwp || !rom) return;
    buf[0] = 0;
    for (int i = 0; i < rom->obj_n && i < 64; i++) {
        O_WRK* o = &sys->obwp[i];
        if (!(o->flg & 0x1) || !(o->flg & 0x80) || o->lkwkp != (unsigned char*)plp) continue;
        sig = sig * 31 + o->id * 64 + o->lkono + 1;
        if (n < 200) n += snprintf(buf + n, sizeof(buf) - n, " %d(id %d, node %d)", i, o->id, o->lkono);
    }
    if (sig != last) { last = sig; vr_log("vr: objects on the player:%s\n", buf[0] ? buf : " none"); }
}

void vr_hand(BH_PWORK* p)
{
    static float rrel[2][2][64][9];          /* [hand][0 aim grip, 1 body grip][weapon] */
    static unsigned char rok[2][2][64];
    static int hooked;
    VrInput in;
    VrPose h;
    float Wg[16], E[3], A[9], cy = cosf(vr_yaw), sy = sinf(vr_yaw);
    float ya, pa;
    int wn, aiming, ctl;
    hand_ok[0] = hand_ok[1] = 0;
    if (!vr_on() || !p || !p->mlwP) return;
    if (!hooked) { hooked = 1; vr_log("vr: hand hook running (player model %d nodes)\n", p->mlwP->obj_num); }
    log_hand_objects();
    wn = p->wpnr_no & 63;
    aiming = p->mode0 == 1 && p->mode1 == 1;
    ya = (float)(short)p->ay * (3.14159265f / 32768.0f);
    pa = (float)(short)p->wax * (3.14159265f / 32768.0f);
    ctl = xr_input(&in) && xr_head_pose(&h);
    if (ctl) { make_Wg(Wg); xform(Wg, h.px, h.py, h.pz, E); }
    for (int side = 0; side < 2; side++) {
        int node = side ? 13 : 9;
        float* H;
        const float* R;
        float ra[9], C[9], P[3];
        const VrPose* hp;
        if (p->mlwP->obj_num <= node) { hand_log(side, 2, wn); continue; }
        H = p->mlwP->owP[node].mtx;
        if (aiming) { rows_from_angles(A, ya, pa); learn(rrel[side][0][wn], A, H); rok[side][0][wn] = 1; }
        else if (!rok[side][1][wn]) { rows_from_angles(A, ya, 0); learn(rrel[side][1][wn], A, H); rok[side][1][wn] = 1; }
        hp = side ? &in.aiml : &in.aim;
        if (!ctl || !(side ? in.aiml_valid : in.aim_valid)) { hand_log(side, 1, wn); continue; }
        R = rok[side][0][wn] ? rrel[side][0][wn] : rrel[side][1][wn];
        quat_to_rows(ra, hp);
        for (int j = 0; j < 3; j++) {   /* C = Ry(vr_yaw) * Rhand */
            C[0 * 3 + j] = cy * ra[0 * 3 + j] + sy * ra[2 * 3 + j];
            C[1 * 3 + j] = ra[1 * 3 + j];
            C[2 * 3 + j] = -sy * ra[0 * 3 + j] + cy * ra[2 * 3 + j];
        }
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)   /* Hrot = C * R, written back as columns */
                H[j * 4 + i] = C[i * 3 + 0] * R[0 * 3 + j] + C[i * 3 + 1] * R[1 * 3 + j] + C[i * 3 + 2] * R[2 * 3 + j];
        xform(Wg, hp->px, hp->py, hp->pz, P);
        {   /* not into the face */
            float d[3] = { P[0] - E[0], P[1] - E[1], P[2] - E[2] }, l = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            float mn = g_vr_gun_min / 100.0f * (g_vr_scale / 10.0f);
            if (l < mn) {
                if (l < 1e-4f) { d[0] = -sy; d[1] = 0; d[2] = -cy; l = 1.0f; }   /* straight ahead */
                for (int k = 0; k < 3; k++) P[k] = E[k] + d[k] / l * mn;
            }
        }
        H[12] = P[0]; H[13] = P[1]; H[14] = P[2];
        memcpy(hand_pos[side], P, sizeof(P));
        hand_ok[side] = 1;
        hand_log(side, 0, wn);
    }
}

/* weapon.c bhCheckGunAtari(), first thing: a shot of the player's (the game builds it from the
   body: origin at the chest, direction Rx(wax) Ry(way)) leaves the muzzle of the weapon in the
   hand that fired it (the game's own muzzle point gx, on the hand node, which follows the
   controller) and goes where that controller points. Everything else about the shot (damage,
   walls, enemies, effects) is the game's. */
void vr_gun_ray(GA_WORK* gap)
{
    static int last = -1;
    NJS_POINT3 o, d;
    VrInput in;
    float r[9], f[3], c = cosf(vr_yaw), s = sinf(vr_yaw), l;
    int side, st;
    if (!gap || !plp || !vr_on() || !g_vr_hand_aim) return;
    if (gap->wpn_no != plp->wpnr_no || gap->ax != plp->wax || gap->ay != plp->way) return;   /* not the player's shot */
    {   /* which hand: the one whose node is nearer the game's muzzle point */
        const float* h9 = plp->mlwP->owP[9].mtx;
        float d9 = (gap->gx - h9[12]) * (gap->gx - h9[12]) + (gap->gy - h9[13]) * (gap->gy - h9[13]) + (gap->gz - h9[14]) * (gap->gz - h9[14]);
        side = 0;
        if (plp->mlwP->obj_num > 13) {
            const float* h13 = plp->mlwP->owP[13].mtx;
            float d13 = (gap->gx - h13[12]) * (gap->gx - h13[12]) + (gap->gy - h13[13]) * (gap->gy - h13[13]) + (gap->gz - h13[14]) * (gap->gz - h13[14]);
            if (d13 < d9) side = 1;
        }
    }
    st = 1;
    if (xr_input(&in) && (side ? in.aiml_valid : in.aim_valid) && hand_ok[side]) {
        const VrPose* hp = side ? &in.aiml : &in.aim;
        quat_to_rows(r, hp);
        f[0] = -r[2]; f[1] = -r[5]; f[2] = -r[8];   /* -z of the controller */
        d.x = c * f[0] + s * f[2]; d.y = f[1]; d.z = -s * f[0] + c * f[2];
        l = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        if (l > 1e-4f) {
            o.x = gap->gx; o.y = gap->gy; o.z = gap->gz;   /* the muzzle, on the tracked hand */
            gap->px = o.x; gap->py = o.y; gap->pz = o.z;
            gap->vx = d.x / l * gap->l; gap->vy = d.y / l * gap->l; gap->vz = d.z / l * gap->l;
            st = 0;
        }
    }
    if (st * 2 + side != last) {
        last = st * 2 + side;
        vr_log(st ? "vr: shot from the game's aim (%s hand not tracked)\n" : "vr: shot from the %s hand's weapon\n", side ? "left" : "right");
    }
}

/* game.c, in place of the player's model while the camera is in the head: the body stays
   hidden; what is seen of the player are the hand/weapon objects linked to the hand nodes
   (drawn by the game's object pass from the tracked hand matrices) */
void vr_draw_hands(void* p) { (void)p; }

/* the left stick (forward, right), -1..1; 0 if no controllers */
int vr_move_stick(float* f, float* r)
{
    VrInput in;
    if (!vr_on() || !xr_input(&in)) return 0;
    *f = in.ly; *r = in.lx;
    return 1;
}
int vr_run_held(void) { VrInput in; return xr_input(&in) && (in.ltrig > 0.5f || (in.buttons & VRB_LSTICK)); }

int vr_pad_start(void) { VrInput in; return xr_input(&in) && (in.buttons & VRB_MENU); }        /* left menu button: Start */
int vr_quick_knife(void) { VrInput in; return xr_input(&in) && in.lgrip > 0.5f; }             /* left grip: quick knife */

/* tps_after_draw(): the frame is drawn with `cam`; hand its camera to the renderer */
void vr_after_draw(void)
{
    VrView v;
    if (!vr_frame) return;
    memset(&v, 0, sizeof(v));
    v.valid = 1;
    memcpy(v.G, cam.mtx, sizeof(v.G));
    /* Ninja matrices are 3x4 in practice: the bottom row (m[3], m[7], m[11], m[15]) is never
       maintained (CALCPOINT does not read it) and is usually zero. Complete it, or G cannot be inverted. */
    v.G[3] = v.G[7] = v.G[11] = 0.0f; v.G[15] = 1.0f;
    memcpy(v.W, frame_W, sizeof(v.W));
    v.cx = fVu1OffsetX; v.cy = fVu1OffsetY;
    v.kx = fVu1AspectW * fVu1Projection; v.ky = fVu1AspectH * fVu1Projection;
    { extern float Ps2_zbuff_a, Ps2_zbuff_b; v.za = Ps2_zbuff_a; v.zb = Ps2_zbuff_b; }
    stream_vr_view(&v);
    vr_frame = 0;
}

static void vr_log(const char* fmt, ...)
{
    extern void host_log(const char* fmt, ...);
    char b[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof(b), fmt, ap);
    va_end(ap);
    host_log("%s", b);
}
