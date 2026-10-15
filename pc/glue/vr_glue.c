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
#include "../../decomp/include/ps2/veronica/prog/player.h"
#include "../../decomp/include/ps2/veronica/prog/weapon.h"
#include "../../decomp/include/ps2/veronica/prog/sdfunc.h"
#include <stdint.h>
extern uint64_t SDL_GetTicks(void);     /* SDL3 (the game glue does not see its headers) */
extern uint64_t SDL_GetTicksNS(void);
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

int g_vr_scale = 100;        /* ini vr_scale: game units per metre, in tenths (100 = 10 units/m: the character is ~17 tall) */
int g_vr_eye_height = 155;   /* ini vr_eye_height: the camera's height above the player's feet, tenths of units */
int g_vr_fov = 120;          /* ini vr_fov: the game camera's horizontal field of view (degrees): what it culls against */
int g_vr_turn = 30;          /* ini vr_turn: right-stick turn in degrees per flick; 0 = smooth turning */
int g_vr_gun_min = 15;       /* ini vr_gun_min: the hand/weapon never comes closer than this to the eyes (cm) */
int g_vr_slash_speed = 180;  /* ini vr_slash_speed: the knife cuts when the right hand moves faster than this (cm/s) */
extern int g_vr_danger_fx;    /* gs_gl.c, ini vr_danger_fx */
int g_vr_hit_tilt = 5;       /* ini vr_hit_tilt: the view tilts this many degrees away from a hit; 0 = off */
int g_vr_hand_offset = -6;   /* ini vr_hand_offset: the hands moved forward (+) or back (-) from the palm, cm */
int g_vr_fx_fix = 1;         /* ini vr_fx_fix: 1 = 2D effects with a depth (fire, glows) are placed in the world */
int g_vr_hand_aim = 1;       /* ini vr_hand_aim: 1 = shots go where the right controller points, 0 = where the head looks */

static int vr_frame;         /* the VR camera set this frame's view (until vr_after_draw) */
static int vr_active;        /* the VR camera drew the last frame */
static float frame_W[16];    /* this frame's local -> world */
static float frame_Wg[16];   /* this frame's recentred tracking space -> world */
static float vr_yaw;         /* the view's heading (radians, forward (-sin, 0, -cos)) */
static unsigned rec_seen;    /* xr_recenter_gen at the last heading reset */
static int snap_held;        /* the right stick is still out after a snap turn */
static int cam_ran, playing; /* the VR camera ran since the last pad read / it ran in the last game frame (in play, not a menu) */

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

/* Where the play space sits under the player this frame (the eyes are g_vr_eye_height above
   it). Normally the player's position. On stairs the game moves the model by its animation and
   the position only jumps forward at the end of each step (player.c bhCPM2_act_kdu), so there
   the base follows the model's root node (node 0; the game itself takes its x/z as the position
   when the stairs end) and, vertically, the root less its usual height above the feet, lightly
   smoothed. Changes between the two are eased out over a few frames. Computed once per game
   frame (the first of vr_hand / vr_camera to ask). */
static unsigned base_gen, cam_gen = 1;
static float base[3], base_res[3], root_ref, sm_y;
static int root_ok;
static int on_stairs = -1;

static void play_base(float out[3])
{
    if (base_gen != cam_gen) {
        float t[3];
        int st = 0;
        base_gen = cam_gen;
        if (plp && plp->mlwP && plp->mlwP->obj_num > 0) {
            const float* r = plp->mlwP->owP[0].mtx;
            /* stairs: the game's stair states (player.c bhCPM2_act_kdu / kdd: mode2 14/15) or its stair flags */
            st = (plp->mode0 == 1 && plp->mode1 == 0 && (plp->mode2 == 14 || plp->mode2 == 15)) ||
                 ((plp->stflg & 0x80000000u) && (plp->flg & 0x400));
            if (!st) {   /* the root's usual height above the feet (can be 0 or below), averaged */
                float h = r[13] - plp->py;
                root_ref = !root_ok ? h : root_ref + (h - root_ref) * 0.05f;
                root_ok = 1;
            }
            if (st && root_ok) {
                float y = r[13] - root_ref;
                if (on_stairs != 1) {
                    extern void host_log(const char* fmt, ...);
                    host_log("vr: stairs: body %.1f %.1f %.1f, position %.1f %.1f %.1f\n", r[12], r[13], r[14], plp->px, plp->py, plp->pz);
                    sm_y = y;
                }
                sm_y += (y - sm_y) * 0.5f;   /* step by step with the body, without the bob */
                t[0] = r[12]; t[1] = sm_y; t[2] = r[14];
            } else { t[0] = plp->px; t[1] = plp->py; t[2] = plp->pz; st = 0; }
        } else if (plp) { t[0] = plp->px; t[1] = plp->py; t[2] = plp->pz; }
        else { t[0] = t[1] = t[2] = 0; }
        if (on_stairs >= 0 && st != on_stairs)   /* switching: keep the old base, ease into the new */
            for (int k = 0; k < 3; k++) base_res[k] = base[k] - t[k];
        if (st != on_stairs && on_stairs >= 0) {
            extern void host_log(const char* fmt, ...);
            host_log("vr: stairs %s\n", st ? "start (camera follows the body)" : "end");
        }
        on_stairs = st;
        for (int k = 0; k < 3; k++) {
            base_res[k] *= 0.75f;
            if (fabsf(base_res[k]) > 30.0f) base_res[k] = 0;   /* a teleport (door, cut): no easing */
            base[k] = t[k] + base_res[k];
        }
    }
    out[0] = base[0]; out[1] = base[1]; out[2] = base[2];
}

/* Hit reaction: when the player loses life the view tilts g_vr_hit_tilt degrees away from the
   hit (rolled away from the nearest enemy's side, pushed back from a hit in front) and eases
   back in about half a second. Applied to the play space about the head (make_Wg), so the
   world, the camera and the hands tilt together and the renderer's re-projection keeps it. */
static float tilt_roll, tilt_pitch;   /* now (radians) */
static float hit_side, hit_front;     /* the last hit's direction relative to the view */
static uint64_t hit_ms;               /* when (SDL ticks), 0 = none */
static int last_hp = -1;

static void hit_update(void)
{
    uint64_t now = SDL_GetTicks();
    float env = 0, a = g_vr_hit_tilt * (3.14159265f / 180.0f);
    if (plp && last_hp >= 0 && plp->hp < last_hp && last_hp - plp->hp < 400) {
        float best = 1e9f, dx = 0, dz = 0;
        for (int i = 0; i < sys->ewk_n; i++) {   /* the nearest live enemy is the one that hit */
            BH_PWORK* e = &ene[i];
            float x, z, d;
            if (!(e->flg & 0x1) || (e->flg & 0x2)) continue;
            x = e->px - plp->px; z = e->pz - plp->pz; d = x * x + z * z;
            if (d < best) { best = d; dx = x; dz = z; }
        }
        if (best < 60.0f * 60.0f && best > 1e-4f) {
            float l = sqrtf(best), c = cosf(vr_yaw), sn = sinf(vr_yaw);
            dx /= l; dz /= l;
            hit_side = dx * c - dz * sn;      /* right (cos, 0, -sin) */
            hit_front = -dx * sn - dz * c;    /* forward (-sin, 0, -cos) */
        } else { hit_side = 0; hit_front = 1; }
        hit_ms = now ? now : 1;
        {
            extern void host_log(const char* fmt, ...);
            host_log("vr: hit (life %d -> %d), from %s\n", last_hp, plp->hp, hit_side > 0.3f ? "the right" : hit_side < -0.3f ? "the left" : hit_front > 0 ? "the front" : "behind");
        }
    }
    if (plp) last_hp = plp->hp;
    if (hit_ms) {
        float t = (now - hit_ms) / 1000.0f;
        env = t < 0.08f ? t / 0.08f : expf(-(t - 0.08f) / 0.15f);   /* quick in, ~0.5 s out */
        if (t > 1.0f) { hit_ms = 0; env = 0; }
    }
    tilt_roll = a * env * hit_side;               /* positive: the head rolls left (away from a hit on the right) */
    tilt_pitch = a * 0.6f * env * hit_front;      /* positive: the head goes back / looks up (away from a hit in front) */
}

static void make_Wg(float Wg[16]);

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
        if (why) { extern volatile int g_vr_danger; g_vr_danger = 0; vr_active = 0; return; }
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
    (void)cb; (void)sb;
    hit_update();
    if (g_vr_danger_fx) {   /* the renderer's danger vignette: life in Danger (the game's limp) */
        extern volatile int g_vr_danger;
        g_vr_danger = plp->exp0 && ((EXP_WORK*)plp->exp0)->dmlvl >= 2 && plp->hp > 0;
    }

    /* Wg: recentred tracking space (metres) -> world: scaled, turned with the body, the
       recentred origin (the head at recentring) at the player's eyes, tilted by a hit */
    make_Wg(Wg);
    xr_recenter_matrix(rec);
    vr_mat_mul(frame_W, Wg, rec);   /* local -> world */
    memcpy(frame_Wg, Wg, sizeof(Wg));

    /* camera: the head's position and orientation in the world */
    cam.px = Wg[0] * h.px + Wg[4] * h.py + Wg[8] * h.pz + Wg[12];
    cam.py = Wg[1] * h.px + Wg[5] * h.py + Wg[9] * h.pz + Wg[13];
    cam.pz = Wg[2] * h.px + Wg[6] * h.py + Wg[10] * h.pz + Wg[14];
    quat_to_rows(hr, &h);
    for (int i = 0; i < 3; i++)       /* R = (Wg's rotation) * head */
        for (int j = 0; j < 3; j++)
            R[i * 3 + j] = (Wg[0 * 4 + i] * hr[0 * 3 + j] + Wg[1 * 4 + i] * hr[1 * 3 + j] + Wg[2 * 4 + i] * hr[2 * 3 + j]) / s;
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
    cam_ran = 1;
    cam_gen++;   /* the next game frame computes its base anew */
}

int vr_on(void) { return vr_active && xr_stereo_ready(); }

/* tps_pad(), once per game frame at the pad read: whether the last frame was played with the
   VR camera (normal play) - in menus, the inventory and the map it does not run */
void vr_pad_frame(void) { playing = cam_ran; cam_ran = 0; }
int vr_playing(void) { return vr_on() && playing; }

static int knife_equipped(void)
{
    return plp && plp->wpnr_no >= 0 && plp->wpnr_no < 23 && (WpnTab[plp->wpnr_no].flg & 0x40000000);
}

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
    if ((in.buttons & VRB_Y) && in.ltrig <= 0.5f) k |= 64;   /* TPS_K_STATUS (LT + Y is Select, vr_pad_select) */
    if (in.buttons & VRB_X) k |= 128;    /* TPS_K_MAP */
    if (in.rgrip > 0.5f) k |= 256;       /* TPS_K_AIM */
    if (in.rtrig > 0.5f && !(vr_playing() && knife_equipped() && g_vr_slash_speed > 0)) k |= 512;   /* TPS_K_FIRE (the knife cuts by swinging) */
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
    {
        float b[3];
        play_base(b);
        Wg[12] = b[0]; Wg[13] = b[1] + g_vr_eye_height / 10.0f; Wg[14] = b[2];
    }
    if ((tilt_roll != 0 || tilt_pitch != 0)) {   /* the hit tilt about the head, in its own heading */
        VrPose h;
        if (xr_head_pose(&h)) {
            float hr[9], fx, fz, l, M[16], T[16], X[16], k;
            float axes[2][3], ang[2] = { -tilt_roll, tilt_pitch };
            quat_to_rows(hr, &h);
            fx = -hr[2]; fz = -hr[8]; l = sqrtf(fx * fx + fz * fz);
            if (l > 1e-3f) {
                fx /= l; fz /= l;
                axes[0][0] = fx; axes[0][1] = 0; axes[0][2] = fz;     /* forward: roll */
                axes[1][0] = -fz; axes[1][1] = 0; axes[1][2] = fx;    /* right: pitch */
                vr_mat_identity(M);
                for (int q = 0; q < 2; q++) {   /* M = M * Rot(axis, angle) (Rodrigues, column-major) */
                    float c = cosf(ang[q]), sn = sinf(ang[q]), *u = axes[q], Rm[16];
                    vr_mat_identity(Rm);
                    for (int i = 0; i < 3; i++)
                        for (int j = 0; j < 3; j++) {
                            k = u[i] * u[j] * (1 - c) + (i == j ? c : 0);
                            if (i != j) {
                                int m = 3 - i - j;   /* the third index */
                                float sg = ((i + 1) % 3 == j) ? -1.0f : 1.0f;
                                k += sg * u[m] * sn;
                            }
                            Rm[j * 4 + i] = k;
                        }
                    vr_mat_mul(M, M, Rm);
                }
                /* X = T(h) M T(-h) */
                vr_mat_identity(T); T[12] = h.px; T[13] = h.py; T[14] = h.pz;
                vr_mat_mul(X, T, M);
                vr_mat_identity(T); T[12] = -h.px; T[13] = -h.py; T[14] = -h.pz;
                vr_mat_mul(X, X, T);
                vr_mat_mul(Wg, Wg, X);
            }
        }
    }
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
   game's own pose, once per weapon: the first time it aims and the pose holds still, the hand's
   rotation relative to the aim direction Ry(ay) Rx(wax) (the gun pointing where the controller
   points); until then, relative to the body Ry(ay). After that the weapon stays fixed in the
   hand (the game's swing / recoil animations no longer move it). The hand is put at the palm (the grip pose, plus vr_hand_offset), never closer than vr_gun_min to the
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

/* The knife cuts by swinging the right hand: faster than vr_slash_speed starts a cut, which
   for a few game frames tests the blade (the hand to the knife's tip, wp_fps1 on node 9) with
   the game's own knife hit (damage, sound, sparks, the bounce off walls); the next cut can
   start half a second later. */
static void vr_slash(BH_PWORK* p, const VrPose* hp, const float* H)
{
    static float lp[3];
    static uint64_t lt, next_ms;
    static int have, swing;
    uint64_t now = SDL_GetTicksNS();
    float v[3] = { 0, 0, 0 }, sp = 0;
    if (have && now > lt) {
        float dt = (now - lt) / 1e9f;
        v[0] = (hp->px - lp[0]) / dt; v[1] = (hp->py - lp[1]) / dt; v[2] = (hp->pz - lp[2]) / dt;
        sp = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (dt > 0.2f) sp = 0;   /* a pause, not a swing */
    }
    lp[0] = hp->px; lp[1] = hp->py; lp[2] = hp->pz; lt = now; have = 1;
    if (!knife_equipped() || g_vr_slash_speed <= 0 || p->mode0 != 1 || !vr_playing()) { swing = 0; return; }
    if (!swing && sp * 100.0f > g_vr_slash_speed && now / 1000000 >= next_ms) {
        swing = 4;
        next_ms = now / 1000000 + 500;
        vr_log("vr: knife cut (hand %.1f m/s)\n", sp);
    }
    if (swing > 0) {
        GA_WORK gap;
        NJS_POINT3 tip;
        const NJS_POINT3* tp = &WpnTab[p->wpnr_no].wp_fps1;
        float c = cosf(vr_yaw), sn = sinf(vr_yaw), wx = c * v[0] + sn * v[2], wz = -sn * v[0] + c * v[2];
        tip.x = H[0] * tp->x + H[4] * tp->y + H[8] * tp->z + H[12];
        tip.y = H[1] * tp->x + H[5] * tp->y + H[9] * tp->z + H[13];
        tip.z = H[2] * tp->x + H[6] * tp->y + H[10] * tp->z + H[14];
        memset(&gap, 0, sizeof(gap));
        gap.at_flg = p->at_flg;
        gap.wpn_no = p->wpnr_no;
        gap.r = WpnTab[p->wpnr_no].r;
        gap.ax = p->wax;
        gap.ay = (wx != 0 || wz != 0) ? (int)(atan2f(-wx, -wz) * (32768.0f / 3.14159265f)) & 0xFFFF : p->ay;   /* the cut's direction */
        gap.px = tip.x; gap.py = tip.y; gap.pz = tip.z;
        gap.gx = H[12]; gap.gy = H[13]; gap.gz = H[14];
        gap.vx = gap.px - gap.gx; gap.vy = gap.py - gap.gy; gap.vz = gap.pz - gap.gz;
        if (swing == 4) CallPlayerWeaponSeEx(&tip, 276, 0);
        bhCheckKnifeAtari(&gap);
        swing--;
    }
}

void vr_hand(BH_PWORK* p)
{
    static float rrel[2][2][64][9];          /* [hand][0 aim grip, 1 body grip][weapon] */
    static unsigned char rok[2][2][64];
    static float prev[2][9];
    static int still[2];
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
        /* the grip is learned once per weapon, when the pose has held still for a few frames
           (not while raising, firing, recoiling or a knife swing moves the hand) */
        if (aiming && !rok[side][0][wn]) {
            float cand[9], dmax = 0;
            rows_from_angles(A, ya, pa);
            learn(cand, A, H);
            for (int k = 0; k < 9; k++) { float dd = fabsf(cand[k] - prev[side][k]); if (dd > dmax) dmax = dd; }
            memcpy(prev[side], cand, sizeof(cand));
            still[side] = dmax < 0.02f ? still[side] + 1 : 0;
            if (still[side] >= 6) {
                memcpy(rrel[side][0][wn], cand, sizeof(cand));
                rok[side][0][wn] = 1;
                vr_log("vr: %s hand grip learned for weapon %d\n", side ? "left" : "right", wn);
            }
        } else if (!aiming) still[side] = 0;
        if (!aiming && !rok[side][1][wn]) { rows_from_angles(A, ya, 0); learn(rrel[side][1][wn], A, H); rok[side][1][wn] = 1; }
        hp = side ? &in.aiml : &in.aim;
        if (!ctl || !(side ? in.aiml_valid : in.aim_valid)) { hand_log(side, 1, wn); continue; }
        R = rok[side][0][wn] ? rrel[side][0][wn] : rrel[side][1][wn];
        if (side == 0 && knife_equipped()) {   /* the knife: its blade along the controller (cut by swinging) */
            static float Rk[9];
            const NJS_POINT3* tp = &WpnTab[p->wpnr_no].wp_fps1;
            float b[3] = { tp->x, tp->y, tp->z }, l = sqrtf(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
            if (l > 1e-3f) {
                float u[3], t[3] = { 0, 0, -1 }, ax[3], c, sn, Q[9];
                for (int i = 0; i < 3; i++) b[i] /= l;
                for (int i = 0; i < 3; i++) u[i] = R[i * 3 + 0] * b[0] + R[i * 3 + 1] * b[1] + R[i * 3 + 2] * b[2];   /* the blade in the controller's frame */
                ax[0] = u[1] * t[2] - u[2] * t[1]; ax[1] = u[2] * t[0] - u[0] * t[2]; ax[2] = u[0] * t[1] - u[1] * t[0];
                c = u[0] * t[0] + u[1] * t[1] + u[2] * t[2];
                sn = sqrtf(ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2]);
                if (sn > 1e-4f) {   /* Q: the shortest turn taking the blade to the controller's forward */
                    for (int i = 0; i < 3; i++) ax[i] /= sn;
                    for (int i = 0; i < 3; i++)
                        for (int j = 0; j < 3; j++) {
                            float k = ax[i] * ax[j] * (1 - c) + (i == j ? c : 0);
                            if (i != j) { int m = 3 - i - j; k += (((i + 1) % 3 == j) ? -1.0f : 1.0f) * ax[m] * sn; }
                            Q[i * 3 + j] = k;
                        }
                    for (int i = 0; i < 3; i++)
                        for (int j = 0; j < 3; j++) Rk[i * 3 + j] = Q[i * 3 + 0] * R[0 * 3 + j] + Q[i * 3 + 1] * R[1 * 3 + j] + Q[i * 3 + 2] * R[2 * 3 + j];
                    R = Rk;
                }
            }
        }
        quat_to_rows(ra, hp);
        for (int j = 0; j < 3; j++) {   /* C = Ry(vr_yaw) * Rhand */
            C[0 * 3 + j] = cy * ra[0 * 3 + j] + sy * ra[2 * 3 + j];
            C[1 * 3 + j] = ra[1 * 3 + j];
            C[2 * 3 + j] = -sy * ra[0 * 3 + j] + cy * ra[2 * 3 + j];
        }
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)   /* Hrot = C * R, written back as columns */
                H[j * 4 + i] = C[i * 3 + 0] * R[0 * 3 + j] + C[i * 3 + 1] * R[1 * 3 + j] + C[i * 3 + 2] * R[2 * 3 + j];
        {   /* at the palm (the grip pose), moved along the pointing direction by vr_hand_offset */
            const VrPose* gp = side ? &in.gripl : &in.grip;
            int gv = side ? in.gripl_valid : in.grip_valid;
            float o = g_vr_hand_offset / 100.0f;
            float bx = gv ? gp->px : hp->px, by = gv ? gp->py : hp->py, bz = gv ? gp->pz : hp->pz;
            xform(Wg, bx - ra[2] * o, by - ra[5] * o, bz - ra[8] * o, P);
        }
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
        if (side == 0) vr_slash(p, hp, H);
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

int vr_pad_select(void) { VrInput in; return xr_input(&in) && in.ltrig > 0.5f && (in.buttons & VRB_Y); }   /* LT + Y: Select */
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
