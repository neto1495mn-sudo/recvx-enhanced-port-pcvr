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
 * tps_input.c (vr_tps_keys), pad.c (vr_pad_select, vr_quick_knife).
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_Vu1Strip.h"
#include "../src/host/xr.h"
#include "../src/gs/stream.h"
#include "../../decomp/include/ps2/veronica/prog/player.h"
#include "../../decomp/include/ps2/veronica/prog/weapon.h"
#include "../../decomp/include/ps2/veronica/prog/sdfunc.h"
#include "../../decomp/include/ps2/veronica/prog/pwksub.h"
#include "../../decomp/include/ps2/veronica/prog/item.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaMath.h"
#include "../../decomp/include/ps2/veronica/prog/sub1.h"
#include <stdint.h>
extern uint64_t SDL_GetTicks(void);     /* SDL3 (the game glue does not see its headers) */
extern uint64_t SDL_GetTicksNS(void);
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "vr_watch.h"   /* vr108: the VR watch */

int g_vr_scale = 100;        /* ini vr_scale: game units per metre, in tenths (100 = 10 units/m: the character is ~17 tall) */
int g_vr_eye_height = 155;   /* ini vr_eye_height: the camera's height above the player's feet, tenths of units */
int g_vr_fov = 120;          /* ini vr_fov: the game camera's horizontal field of view (degrees): what it culls against */
int g_vr_turn = 30;          /* ini vr_turn: right-stick turn in degrees per flick; 0 = smooth turning */
int g_vr_gun_min = 15;       /* ini vr_gun_min: the hand/weapon never comes closer than this to the eyes (cm) */
int g_vr_slash_speed = 180;  /* ini vr_slash_speed: the knife cuts when the right hand moves faster than this (cm/s) */
extern int g_vr_danger_fx;    /* gs_gl.c, ini vr_danger_fx */
int g_vr_hit_tilt = 5;       /* ini vr_hit_tilt: the view tilts this many degrees away from a hit; 0 = off */
int g_vr_left_mirror = 1;    /* ini vr_left_mirror: the left hand without a weapon holds its controller as the right does, mirrored (1/2/3: the bone axis mirrored x/y/z; 0 = the game's own pose) */
int g_vr_dmg_head = 115, g_vr_dmg_body = 75, g_vr_dmg_body_max = 100, g_vr_dmg_arm = 50, g_vr_dmg_leg = 50;   /* ini vr_dmg_*: damage by the body part hit (zombies), percent; the body's between _body and _body_max */
int g_vr_wake_shots = 1;     /* ini vr_wake_shots: a zombie still asleep can be shot from afar (and wakes) */
int g_vr_precise_hits = 1;   /* ini vr_precise_hits: thin bullets hitting the enemy's body parts (its 3D), not the game's wide aim assist */
int g_vr_barrel_pitch = 0;   /* ini vr_barrel_pitch: the barrel line turned up (+) or down (-), tenths of a degree */   /* ini vr_dmg_*: damage by the body part hit (zombies), percent */
int g_vr_aim_walk = 1;       /* ini vr_aim_walk: the left stick walks while aiming, at the measured walking/running speed */
int g_vr_roomscale = 1;      /* ini vr_roomscale: walking in the room moves Claire (with the game's walls) */
int g_vr_manual_reload = 1;  /* ini vr_manual_reload: B drops the magazine, the left hand takes one at the holster and puts it in */
int g_vr_prompts = 1;        /* ini vr_prompts: one "A" over what the action button would use, when right in front of it */
int g_vr_holster = 1;        /* ini vr_holster: the ammo in the boxes, shown at the belt */
int g_vr_sight = 1;          /* ini vr_sight: a small white ring where the aimed gun's shot would hit */
int g_vr_recoil = 1;         /* ini vr_recoil: the weapon kicks in the hand at each shot */
int g_vr_haptics = 1;        /* ini vr_haptics: the controllers vibrate (shots, cuts) */
extern const short EneDamNear[31][22];   /* weapon.c */
static void vr_log(const char* fmt, ...);
int g_vr_hand_offset = -6;   /* ini vr_hand_offset: the hands moved forward (+) or back (-) from the palm, cm */
int g_vr_fx_fix = 1;         /* ini vr_fx_fix: 1 = 2D effects with a depth (fire, glows) are placed in the world */
int g_vr_hand_aim = 1;       /* ini vr_hand_aim: 1 = shots go where the right controller points, 0 = where the head looks */

static int vr_frame;
static int last_vr_frame = 1;   /* vr86: the last drawn frame was in the head (else the cinema screen) */
static int box_mode;             /* vr87: the ammo box this frame (0 none, 1 holster, 2 left hand) and its corners (world) */
static float box_c[8][3];
static int box_item = -1;       /* vr131: the ammo item of the gun in hand (holster_update) */
#define BOX_W 0.09f              /* its size, metres: wide, high, deep (vr131: the handgun bullets' box as the game's, 202:120:68) */
#define BOX_H (box_item == 12 ? 0.054f : 0.06f)
#define BOX_D (box_item == 12 ? 0.031f : 0.05f)         /* the VR camera set this frame's view (until vr_after_draw) */
static int vr_active;        /* the VR camera drew the last frame */
static float frame_W[16];    /* this frame's local -> world */
static float frame_Wg[16];   /* this frame's recentred tracking space -> world */
static float vr_yaw;         /* the view's heading (radians, forward (-sin, 0, -cos)) */
static unsigned rec_seen;    /* xr_recenter_gen at the last heading reset */
volatile int g_vr_recenter_on_start;   /* RECVX_TEST_SAVESPOT: set when the test save loads; the first game frame recentres (heading_update) */
static int snap_held;        /* the right stick is still out after a snap turn */
static uint64_t turn_ms;     /* vr116: the last snap or smooth turn (the comfort vignette) */
static int sprint_on;         /* vr133: running by one click of the left stick while walking forward (as Call of Duty) */
extern int  vr_turn_total(void);             /* vr141: the smooth turn so far, by the headset's clock (gs_gl.c) */
extern void vr_turn_take(int taken, int allow);
static int turn_taken;                        /* ... of it, what vr_yaw has taken */
static float smooth_turn;    /* vr120: this frame's smooth turn (radians) - the renderer turns the view in between frames */
int vr_playing(void);
static uint64_t cam_ms;      /* when the VR camera last ran (SDL ticks): it runs only in play, not in menus */
static float vr_za, vr_zb;   /* the GS depth constants the frame is drawn with (Ps2_zbuff_a/b after tps_camera's njClipZ) */

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
/* vr186: she is on stairs (the game's stair walk, mode2 14/15, or its stair flags) - no knife drawn, no aiming there */
int vr_on_stairs(void)
{
    return plp && plp->mode0 == 1 && ((plp->mode1 == 0 && (plp->mode2 == 14 || plp->mode2 == 15)) ||
                                      ((plp->stflg & 0x80000000u) && (plp->flg & 0x400)));
}
static float frame_follow[2];   /* this frame's move of the player by the room walking (body_follow) */
static float frame_aimwalk[2];  /* vr76: this frame's move by the VR walking while aiming (aim_walk_update) */
extern float walk_spd, run_spd;
int g_vr_cam_steady = 1;
static int steady_reset;   /* vr91: the steady camera starts again where she is (after the stairs) */
int g_vr_move_direct = 1;       /* ini vr_move_direct: VR - she faces the stick's way at once, no turning arc (vr85) */        /* ini vr_cam_steady: off the stairs, the camera moves with the stick's walking only (vr75) */

/* vr75: the camera's place off the stairs. The game moves the player also by itself - the slide
   of the stop, the aim stance, turning on the spot - and the camera went along (a lurch). Steady:
   the camera takes the player's moves only while the stick walks (and the room walking at once);
   what the game added meanwhile is caught up while walking, or at once if it is large (a hit, a push). */
static void body_shift(float dx, float dz);
static int shift_src;             /* vr92 diagnosis: who moves her now (1 room walk, 2 aim walk, 3 under the camera) */
static float shift_f[4][2];        /* ... this frame, by source */
static void steady_anchor(float t[3])
{
    extern int g_vr_debug_log;
    extern void host_log(const char* fmt, ...);
    static float anc[3], last[3];
    static int ok;
    static unsigned n, nsnap, nshuffle, nback, nahead, nwallcam, nbrought, nexact;
    static int stance;
    static float brought;
    static float omax, sdir[2];
    float cur[3] = { plp->px, plp->py, plp->pz }, o[3], ol, s = g_vr_scale / 10.0f;
    VrInput in;
    int walk = xr_input(&in) && in.lx * in.lx + in.ly * in.ly > 0.04f;
    if (!g_vr_cam_steady || !ok || steady_reset) {
        if (steady_reset && g_vr_debug_log) host_log("vr-diag: steady camera starts again where she is (after the stairs)\n");
        steady_reset = 0; stance = 0;
        memcpy(anc, cur, sizeof(anc)); memcpy(last, cur, sizeof(last)); ok = g_vr_cam_steady; memcpy(t, cur, sizeof(cur)); return;
    }
    if (plp->mode1 == 1) stance = 1;   /* vr91: knife / aim - she may be brought back under the camera after it */
    {   /* vr76: what the camera takes - the room walking always; aiming, only the VR's own walking (not the
           stance turning the body about its feet); walking, only whole steps (not the feet shuffling at a wall) */
        float dx = cur[0] - last[0], dz = cur[2] - last[2], d = sqrtf(dx * dx + dz * dz), fl = sqrtf(frame_follow[0] * frame_follow[0] + frame_follow[1] * frame_follow[1]);
        float stepmin = 0.5f * (walk_spd < run_spd ? walk_spd : run_spd);
        anc[0] += frame_follow[0]; anc[2] += frame_follow[1];
        dx -= frame_follow[0]; dz -= frame_follow[1]; d = sqrtf(dx * dx + dz * dz); (void)fl;
        if (plp->mode1 == 1) { anc[0] += frame_aimwalk[0]; anc[2] += frame_aimwalk[1]; walk = 0; nshuffle += d > 1e-3f; }
        else if (walk && d >= stepmin) {   /* vr77: a ratchet - forward only; a wall pushing back is left out */
            float ux = dx / d, uz = dz / d, sl;
            {   /* vr97: "back" is against the way the stick points NOW (not the way she walked before): walking back with the
                   stick back is followed; only a move against the stick (a wall pushing her back) is left out */
                float a = (float)cam.ay * (3.14159265f / 32768.0f);
                sdir[0] = sinf(a) * in.ly + cosf(a) * in.lx; sdir[1] = -cosf(a) * in.ly + sinf(a) * in.lx;
                sl = sqrtf(sdir[0] * sdir[0] + sdir[1] * sdir[1]);
                if (sl > 1e-4f) { sdir[0] /= sl; sdir[1] /= sl; sl = 1.0f; }
            }
            if (sl > 1e-3f && ux * sdir[0] + uz * sdir[1] < -0.2f * sl) { nback++; walk = 0; }
            else {
                float ox, oz, along;
                ox = cur[0] - anc[0]; oz = cur[2] - anc[2];
                along = ox * sdir[0] + oz * sdir[1];
                if (along > 0) {   /* (vr99: vr98's exact following taken back - it made the jolts sharper) */
                    float mx = 1.5f * run_spd, a2 = along > mx ? mx : along;
                    anc[0] += sdir[0] * a2 + (ox - sdir[0] * along) * 0.15f;
                    anc[2] += sdir[1] * a2 + (oz - sdir[1] * along) * 0.15f;
                }
                walk = 0;   /* (caught up here) */
            }
        }
        else { if (d > 1e-3f) nshuffle++; walk = 0; if (!xr_input(&in) || in.lx * in.lx + in.ly * in.ly < 0.04f) sdir[0] = sdir[1] = 0.0f; }
    }
    {   /* vr79: never ahead of her the way the stick walks (a wall stops the camera where it stops her) */
        VrInput i2;
        if (xr_input(&i2) && i2.lx * i2.lx + i2.ly * i2.ly > 0.04f) {
            float a = (float)cam.ay * (3.14159265f / 32768.0f), wx = sinf(a) * i2.ly + cosf(a) * i2.lx, wz = -cosf(a) * i2.ly + sinf(a) * i2.lx;
            float wl = sqrtf(wx * wx + wz * wz), ahead;
            if (wl > 1e-4f) {
                wx /= wl; wz /= wl;
                ahead = (anc[0] - cur[0]) * wx + (anc[2] - cur[2]) * wz;
                if (ahead > 0) { anc[0] -= ahead * wx; anc[2] -= ahead * wz; nahead++; }
            }
        }
    }
    {   /* vr80: the camera is where she could stand - from her place to its own through the game's wall check */
        extern int vr_game_wall(float x0, float z0, float* x, float* z);
        float x = anc[0], z = anc[2];
        if (fabsf(x - cur[0]) + fabsf(z - cur[2]) > 1e-4f && vr_game_wall(cur[0], cur[2], &x, &z)) { anc[0] = x; anc[2] = z; nwallcam++; }
    }
    anc[1] = cur[1];
    memcpy(last, cur, sizeof(last));
    o[0] = cur[0] - anc[0]; o[2] = cur[2] - anc[2];
    ol = sqrtf(o[0] * o[0] + o[2] * o[2]) / (s > 0 ? s : 1.0f);   /* metres */
    if (ol > omax) omax = ol;
    if (ol > 3.0f) { memcpy(anc, cur, sizeof(anc)); nsnap++; }   /* a teleport: at once */
    else if (ol > 0.3f && plp->mode1 == 0 && !stance) {   /* vr97: left behind for any reason: caught up at her walking pace, no jump */
        float k = walk_spd / (ol * (s > 0 ? s : 1.0f));
        if (k > 1.0f) k = 1.0f;
        anc[0] += o[0] * k; anc[2] += o[2] * k; nsnap++;
    }
    else if (plp->mode1 == 0 && (xr_input(&in) && in.lx * in.lx + in.ly * in.ly > 0.04f)) stance = 0;   /* walking: the ratchet takes over */
    else if (plp->mode1 == 0 && stance && ol <= 0.02f) stance = 0;
    else if (plp->mode1 == 0 && stance && ol > 0.02f) {   /* vr88: off the knife / the aim (the game moved her): she is brought
                                                           back under the camera - the camera does not jump to her */
        extern int vr_game_wall(float x0, float z0, float* x, float* z);
        float x = anc[0], z = anc[2];
        vr_game_wall(cur[0], cur[2], &x, &z);
        if (fabsf(x - cur[0]) + fabsf(z - cur[2]) > 1e-4f) { shift_src = 3; body_shift(x - cur[0], z - cur[2]); shift_src = 0; nbrought++; brought += ol; }
        anc[0] = x; anc[2] = z;
        memcpy(last, anc, sizeof(last)); last[1] = cur[1];
    }
    else if (walk) { anc[0] += o[0] * 0.15f; anc[2] += o[2] * 0.15f; }   /* caught up while walking */
    if (g_vr_debug_log && ++n >= 300) {
        host_log("vr-diag: steady camera: off the player by %.2f m now, at most %.2f m; caught up %u; small/aim moves left out %u; wall push-backs left out %u; held back from ahead of her %u; stopped by a wall %u; she was brought under the camera %u times (%.2f m); exact steps %u\n", ol, omax, nsnap, nshuffle, nback, nahead, nwallcam, nbrought, brought, nexact);
        nbrought = 0; brought = 0; nexact = 0;
        n = 0; omax = 0; nsnap = 0; nshuffle = 0; nback = 0; nahead = 0; nwallcam = 0;
    }
    memcpy(t, anc, sizeof(anc));
}

static float st_x, st_z, st_vx, st_vz, st_lx, st_lz;   /* the stairs' horizontal camera: smoothed root, its speed, the last root */
static int st_ok;
static int anchor_live;          /* the steady camera's anchor is current (play_base worked the camera out with it last) */
static uint64_t last_play_ms;
static void play_base(float out[3])
{
    if (base_gen != cam_gen) {
        float t[3];
        int st = 0;
        {   /* this function not called for a while (a cut-scene, a menu, a door, the inventory): the anchor is stale */
            uint64_t now = SDL_GetTicks();
            if (last_play_ms && now - last_play_ms > 300) anchor_live = 0;
            last_play_ms = now;
        }
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
                anchor_live = 0;   /* (the stairs: the steady camera's anchor is not kept up to date meanwhile) */
                if (on_stairs != 1) {
                    extern void host_log(const char* fmt, ...);
                    host_log("vr: stairs: body %.1f %.1f %.1f, position %.1f %.1f %.1f\n", r[12], r[13], r[14], plp->px, plp->py, plp->pz);
                    sm_y = y;
                    st_ok = 0;   /* (the stairs' horizontal smoothing starts again at the root) */
                }
                sm_y += (y - sm_y) * 0.5f;   /* step by step with the body, without the bob */
                {   /* the root's way up the stairs is uneven (the log, stairs-trace): nearly still for ~5 frames at the start of each cycle of the climb
                       animation, then a jump of 0.2 m in one frame (the usual step 0.05) - the camera copied it: the pull forward. Followed with the
                       speed predicted (the average step) and the error taken a fifth a frame: steady climbing, no lag; the jump spread over ~8 frames. */
                    if (!st_ok) { st_x = r[12]; st_z = r[14]; st_vx = st_vz = 0.0f; st_lx = r[12]; st_lz = r[14]; st_ok = 1; }
                    else {
                        float sx = r[12] - st_lx, sz = r[14] - st_lz;
                        st_vx += (sx - st_vx) * 0.12f; st_vz += (sz - st_vz) * 0.12f;
                        st_lx = r[12]; st_lz = r[14];
                        st_x += st_vx; st_z += st_vz;
                        st_x += (r[12] - st_x) * 0.2f; st_z += (r[14] - st_z) * 0.2f;
                    }
                    t[0] = st_x; t[1] = sm_y; t[2] = st_z;
                }
            } else {
                /* The rule for EVERY mode change: the steady camera's anchor is only kept up to date while it is the one in use (here). Whenever the camera was worked out some other way
                   (the stairs, no model, a cut-scene or menu, a door: this function not called for a while) the anchor is stale; coming back, it starts at her NOW, before this frame's
                   target is worked out - else the base eases toward a place she left long ago and then jumps back (stairs' end measured +0.5 / -1.7 m). */
                if (!anchor_live) steady_reset = 1;
                anchor_live = 1;
                steady_anchor(t); st = 0;
            }
        } else if (plp) { anchor_live = 0; t[0] = plp->px; t[1] = plp->py; t[2] = plp->pz; }
        else { anchor_live = 0; t[0] = t[1] = t[2] = 0; }
        if (on_stairs >= 0 && st != on_stairs)   /* switching: keep the old base, ease into the new */
            for (int k = 0; k < 3; k++) base_res[k] = base[k] - t[k];
        if (st != on_stairs && on_stairs >= 0) {
            extern void host_log(const char* fmt, ...);
            host_log("vr: stairs %s\n", st ? "start (camera follows the body)" : "end");
            /* (vr91: the steady camera starts again where she is: done above, before the target is worked out) */
        }
        on_stairs = st;
        for (int k = 0; k < 3; k++) {
            base_res[k] *= 0.75f;
            if (fabsf(base_res[k]) > 8.0f) base_res[k] = 0;   /* a teleport (door, cut) or a base that differs by more than 0.8 m: no easing - easing a big difference showed as the camera thrown ahead and back (the stairs' end measured +0.5 / -1.7 m); one clean step instead (was 3 m) */
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

/* the nearest live enemy (the one that hit or holds her): its index, and its way relative to the
   view (side: +1 right, -1 left; front: +1 ahead, -1 behind); -1 if none within 6 m */
extern int g_vr_debug_log;
static void make_Wg(float Wg[16]);
int vr_game_wall(float x0, float z0, float* x, float* z);
static float close_push[2];       /* vr112: the camera moved back from an enemy at the eyes (world) */
static float bite_off[2];         /* vr115: and the small push of each bite (world) */
static uint64_t bite_ms;          /* vr115: the holder's last bite (vr_enemy_sound) */
int g_vr_bite_push = 0;           /* ini vr_bite_push: each bite pushes the camera back this many cm; 0 = off */
int g_vr_close_push = 35;         /* ini vr_close_push: keep an enemy's body this many cm from the eyes; 0 = off */
static int enemy_way(float* side, float* front)
{
    float best = 1e9f, dx = 0, dz = 0;
    int bi = -1;
    for (int i = 0; i < sys->ewk_n; i++) {
        BH_PWORK* e = &ene[i];
        float x, z, d;
        if (!(e->flg & 0x1) || (e->flg & 0x2)) continue;
        x = e->px - plp->px; z = e->pz - plp->pz; d = x * x + z * z;
        if (d < best) { best = d; dx = x; dz = z; bi = i; }
    }
    if (bi >= 0 && best < 60.0f * 60.0f && best > 1e-4f) {
        float l = sqrtf(best), c = cosf(vr_yaw), sn = sinf(vr_yaw);
        dx /= l; dz /= l;
        *side = dx * c - dz * sn;      /* right (cos, 0, -sin) */
        *front = -dx * sn - dz * c;    /* forward (-sin, 0, -cos) */
        return bi;
    }
    *side = 0; *front = 1;
    return -1;
}

/* vr110: a vibration from where the hit came - split between the hands as a sound between two
   speakers (only the right hand for a hit fully on the right, both alike in front or behind),
   one hard knock from the front, two quick ones from behind, stronger for more damage */
static uint64_t hap_second_ms;
static float hap_second[2];
static void hit_haptics(int dmg, float side, float front)
{
    float k = 0.4f + dmg / 60.0f, w[2];
    if (!g_vr_haptics) return;
    k = k > 1.0f ? 1.0f : k;
    side = side > 1.0f ? 1.0f : side < -1.0f ? -1.0f : side;
    w[0] = sqrtf((1.0f + side) * 0.5f) * 1.3f; w[1] = sqrtf((1.0f - side) * 0.5f) * 1.3f;   /* 0 right, 1 left */
    for (int h = 0; h < 2; h++) {
        float amp = k * (w[h] > 1.0f ? 1.0f : w[h]);
        if (amp < 0.08f) continue;
        if (front >= 0) xr_haptic(h, amp, 130);
        else { xr_haptic(h, amp, 60); hap_second[h] = amp; }
    }
    if (front < 0) { uint64_t t = SDL_GetTicks(); hap_second_ms = (t ? t : 1) + 150; }
}

/* vr113: held (mode0 4/5), the holder's sounds are felt as they play (sdfunc.c CallEnemySeMain) - its
   bites' "check, check" a short hard knock on its side, its grab and letting go a softer one (vr114: the
   numbers from the vr113 log; the other enemies' sounds not) */
static uint64_t hold_sound_ms;
int vr_on(void);
void vr_enemy_sound(int enemy_no, int se_no)
{
    float s, f, w0, w1, amp;
    int n = se_no & 0xFF;
    if (!plp || !(plp->mode0 == 4 || plp->mode0 == 5) || !vr_on()) return;
    if (enemy_no != enemy_way(&s, &f)) return;   /* vr114: only the one holding her (the others' groans did too, vr113 log) */
    if (n == 0) amp = 0.9f;                                            /* the bite (bank 3, 0: each followed by her life falling) */
    else if (n == 42 || n == 34 || n == 40 || n == 32) amp = 0.5f;     /* its grab and its letting go */
    else return;
    w0 = sqrtf((1.0f + s) * 0.5f); w1 = sqrtf((1.0f - s) * 0.5f);
    if (n == 0) bite_ms = SDL_GetTicks();   /* vr115: the bite's push back */
    if (!g_vr_haptics) return;
    if (w0 > 0.15f) xr_haptic(0, amp * w0, n == 0 ? 45 : 60);
    if (w1 > 0.15f) xr_haptic(1, amp * w1, n == 0 ? 45 : 60);
    hold_sound_ms = SDL_GetTicks();
}

static void hit_update(void)
{
    uint64_t now = SDL_GetTicks();
    float env = 0, a = g_vr_hit_tilt * (3.14159265f / 180.0f);
    if (plp && last_hp >= 0 && plp->hp < last_hp && last_hp - plp->hp <= 2 && (plp->stflg & 0x280000)) {   /* vr154: the poison's drain, a point at a time: not a hit (the tilt each point swayed the view) */
        static unsigned nl;
        if (g_vr_debug_log && nl++ < 5) host_log("vr-diag: poison drain (life %d -> %d), no tilt\n", last_hp, plp->hp);
    } else if (plp && last_hp >= 0 && plp->hp < last_hp && last_hp - plp->hp < 400) {
        enemy_way(&hit_side, &hit_front);   /* the nearest live enemy is the one that hit */
        hit_ms = now ? now : 1;
        hit_haptics(last_hp - plp->hp, hit_side, hit_front);
        {   /* vr112: a hit in Caution - one flash in the life's colour (the game's status screen, sub1.c:
               60-119 yellow, 30-59 orange; poisoned, Danger: their own pulsing edge) */
            extern volatile int g_vr_flash_col;
            extern volatile uint64_t g_vr_flash_ms;
            if (!(plp->stflg & 0x280000) && plp->hp >= 30 && plp->hp < 120) { g_vr_flash_col = plp->hp >= 60 ? 1 : 2; g_vr_flash_ms = now; }
        }
        {
            extern void host_log(const char* fmt, ...);
            host_log("vr: hit (life %d -> %d), from %s\n", last_hp, plp->hp, hit_side > 0.3f ? "the right" : hit_side < -0.3f ? "the left" : hit_front > 0 ? "the front" : "behind");
        }
    }
    if (hap_second_ms && now >= hap_second_ms) {   /* the second knock of a hit from behind */
        for (int h = 0; h < 2; h++) if (hap_second[h] > 0) xr_haptic(h, hap_second[h], 60);
        hap_second[0] = hap_second[1] = 0; hap_second_ms = 0;
    }
    {   /* vr110: held by an enemy (mode0 5, player.c bhCPM0_enedam): a pulse from its side every
           0.4 s; and the diagnosis of the hold (for the escape, vr111 - out once it has told us):
           which enemy, its states as they change, how long, what ended it */
        static int held, hold_e = -1, e_last[5];
        static uint64_t hold_ms, pulse_ms;
        static unsigned frames;
        int now_held = plp && (plp->mode0 == 4 || plp->mode0 == 5);   /* vr112: held is 4 (vr111 log: every zombie hold), 5 too */
        if (now_held) {
            float s, f;
            int ei = enemy_way(&s, &f);
            if (!held) {
                hold_ms = now; frames = 0; hold_e = ei; e_last[0] = -1;
                if (g_vr_debug_log) vr_log("vr-diag: hold: start - enemy %d (id %d) %s, her life %d, her mode %d/%d/%d\n", ei, ei >= 0 ? ene[ei].id : -1,
                                           s > 0.3f ? "on the right" : s < -0.3f ? "on the left" : f > 0 ? "in front" : "behind", plp->hp, plp->mode0, plp->mode1, plp->mode2);
            }
            frames++;
            if (g_vr_haptics && now >= pulse_ms && now - hold_sound_ms > 1200) {   /* (vr113: only when its sounds are not felt) */
                float w0 = sqrtf((1.0f + s) * 0.5f), w1 = sqrtf((1.0f - s) * 0.5f);
                if (w0 > 0.15f) xr_haptic(0, 0.55f * w0, 70);
                if (w1 > 0.15f) xr_haptic(1, 0.55f * w1, 70);
                pulse_ms = now + 400;
            }
            if (g_vr_debug_log && hold_e >= 0) {
                BH_PWORK* e = &ene[hold_e];
                if (e->mode0 != e_last[0] || e->mode1 != e_last[1] || e->mode2 != e_last[2] || e->mode3 != e_last[3] || e->hp != e_last[4]) {
                    vr_log("vr-diag: hold: frame %u: enemy mode %d/%d/%d/%d life %d (motion %d frame %d), her life %d mode %d/%d/%d\n", frames, e->mode0, e->mode1, e->mode2, e->mode3,
                           e->hp, e->mtn_no, e->frm_no >> 16, plp->hp, plp->mode0, plp->mode1, plp->mode2);
                    e_last[0] = e->mode0; e_last[1] = e->mode1; e_last[2] = e->mode2; e_last[3] = e->mode3; e_last[4] = e->hp;
                }
            }
        } else if (held && g_vr_debug_log) {
            BH_PWORK* e = hold_e >= 0 ? &ene[hold_e] : NULL;
            vr_log("vr-diag: hold: end after %u frames (%.1f s) - her life %d mode %d/%d/%d; enemy mode %d/%d/%d/%d life %d\n", frames, (now - hold_ms) / 1000.0f,
                   plp ? plp->hp : -1, plp ? plp->mode0 : -1, plp ? plp->mode1 : -1, plp ? plp->mode2 : -1,
                   e ? e->mode0 : -1, e ? e->mode1 : -1, e ? e->mode2 : -1, e ? e->mode3 : -1, e ? e->hp : -1);
        }
        held = now_held;
    }
    {   /* vr112: an enemy's body at the eyes - an attack or a hold puts a zombie's head where the VR eyes
           are (the game's camera was far; vr111 log: 3-15 cm from the eyes for ~3 s of a hold). The camera
           moves back from it to vr_close_push cm, smoothly and not through the game's walls, and the edge
           darkens if it is still within 15 cm. Only the camera: the game is not touched.
           (vr116: the vr111 diagnosis line is out - the move back was found right, vr115) */
        extern volatile float g_vr_close_dark;
        VrPose h;
        static float away[2];
        float Wg[16], eye[3], nb[3] = { 0, 0, 0 }, s = g_vr_scale / 10.0f, best = 1e9f, want[2] = { 0, 0 }, keep = g_vr_close_push / 100.0f, dark = 0;
        int bi = -1, bn = -1;
        if (plp && xr_head_pose(&h)) {
            make_Wg(Wg);
            for (int k = 0; k < 3; k++) eye[k] = Wg[k] * h.px + Wg[4 + k] * h.py + Wg[8 + k] * h.pz + Wg[12 + k];
            eye[0] -= close_push[0] + bite_off[0]; eye[2] -= close_push[1] + bite_off[1];   /* (where the eyes are without the move back) */
            for (int i = 0; i < sys->ewk_n; i++) {
                BH_PWORK* e = &ene[i];
                float x = e->px - eye[0], z = e->pz - eye[2];
                if (!(e->flg & 0x1) || (e->flg & 0x2) || !e->mlwP || !e->mlwP->owP) continue;
                if (x * x + z * z > (2.0f * s) * (2.0f * s)) continue;   /* (only enemies within 2 m) */
                for (int j = 0; j < (int)e->mlwP->obj_num && j < 64; j++) {
                    const float* m = e->mlwP->owP[j].mtx;
                    float dx = m[12] - eye[0], dy = m[13] - eye[1], dz = m[14] - eye[2], d = dx * dx + dy * dy + dz * dz;
                    if (d < best) { best = d; bi = i; bn = j; nb[0] = m[12]; nb[1] = m[13]; nb[2] = m[14]; }
                }
            }
            best = bi >= 0 ? sqrtf(best) / s : 1e9f;
            if (bi >= 0) {   /* vr115: the way back is away from the enemy's body (the way the vibration feels it), not from
                                the nearest part: a bite's head is at her neck, beside or behind the eyes - the camera went sideways */
                float dx = eye[0] - ene[bi].px, dz = eye[2] - ene[bi].pz, l = sqrtf(dx * dx + dz * dz);
                if (l > 0.02f * s) { away[0] = dx / l; away[1] = dz / l; }
                else { away[0] = sinf(vr_yaw); away[1] = cosf(vr_yaw); }   /* (right on her: back from the view) */
            }
            if (keep > 0.0f && bi >= 0 && best < keep) {   /* back, to keep its body that many cm from the eyes */
                float m = (keep - best) * s;
                want[0] = away[0] * m; want[1] = away[1] * m;
            }
            for (int k = 0; k < 2; k++) close_push[k] += (want[k] - close_push[k]) * 0.12f;   /* vr114: slower (~0.3 s), out and back - easier on the stomach */
            {   /* vr115: each bite of the one holding her a small push back, quickly in, ~0.2 s out (vr_bite_push) */
                uint64_t tb = SDL_GetTicks() - bite_ms;
                float env = !bite_ms || g_vr_bite_push <= 0 || tb > 800 ? 0.0f : tb < 50 ? tb / 50.0f : expf(-(tb - 50.0f) / 180.0f);
                float m = g_vr_bite_push / 100.0f * s * env;
                bite_off[0] = away[0] * m; bite_off[1] = away[1] * m;
            }
            if (fabsf(close_push[0] + bite_off[0]) + fabsf(close_push[1] + bite_off[1]) > 1e-3f) {   /* not into a wall: through the game's own wall check, from her */
                float x = plp->px + close_push[0] + bite_off[0], z = plp->pz + close_push[1] + bite_off[1];
                vr_game_wall(plp->px, plp->pz, &x, &z);
                close_push[0] = x - plp->px - bite_off[0]; close_push[1] = z - plp->pz - bite_off[1];
            }
            if (bi >= 0) {   /* still close with the move back: the edge darkens */
                float dx = eye[0] + close_push[0] + bite_off[0] - nb[0], dy = eye[1] - nb[1], dz = eye[2] + close_push[1] + bite_off[1] - nb[2];
                float de = sqrtf(dx * dx + dy * dy + dz * dz) / s;
                dark = de < 0.15f ? (0.15f - de) / 0.10f : 0.0f;
                if (dark > 1.0f) dark = 1.0f;
            }
        } else { close_push[0] *= 0.88f; close_push[1] *= 0.88f; bite_off[0] = bite_off[1] = 0; }
        g_vr_close_dark += (dark - g_vr_close_dark) * 0.4f;
        if (g_vr_close_dark < 0.01f && dark <= 0.0f) g_vr_close_dark = 0.0f;
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
static void mk_add(int kind, const float p[3], int val);
static int mk_n;
static float mk_pt[12][3];
static int mk_kind[12], mk_val[12];
static int knife_equipped(void);
static int dual_equipped(void);
int vr_on(void);
static float holster_w[3];          /* the holster (world), when a gun is in hand */
static int holster_ok;
static float hand_pos[2][3];        /* where each hand was put this frame (world) */
static int hand_ok[2];
static VrPose hand_aim[2];
static void aim_rows(float r[9], const VrPose* p);   /* vr196: the right controller's rows, turned for the two-handed hold */
static float frame_body[3];
static int laser_ok[2];                       /* vr77: the laser sight of each hand's gun (world): muzzle -> hit */
static float laser_a[2][3], laser_b[2][3];   /* vr71: the body's place this frame, and the room-walking part of its move */              /* ... and whether it was */

/* The ammo in the boxes for the gun in hand: the inventory items the game would reload it
   from (as bhSearchBullet: ammo items whose combination list names the weapon), summed;
   -1 if no gun is equipped */
static int box_ammo(void)
{
    S_WORK* st = &swork;
    int max, wpn, n = 0;
    unsigned short bt;
    if (!st->pip) return -1;
    wpn = (st->pip[*st->pip] >> 16) & 0xFF;
    bt = (st->pip[*st->pip] >> 16) & 0xFF00;
    if (wpn == 0) return -1;
    if (wpn == 7 && (bt & 0x2000)) wpn = 155;
    max = (sys->st_flg & 0x8000000) ? 10 : 8;
    for (int i = 0; i < max; i++) {
        unsigned int y = (st->pip[i + 2] >> 16) & 0xFF;
        if (y >= 156 || !(itemdata[y].type & 0x2)) continue;
        {
            int z = combidata[itemdata[y].cmb], it = itemdata[y].cmb + 1;
            for (; z > 0; z--, it += 3)
                if (wpn == combidata[it] && (combidata[it + 2] & 0xF000)) { n += st->pip[i + 2] & 0xFFFF; break; }
        }
    }
    return n;
}

/* vr131: the ammo item the gun in hand takes (the first in the inventory, as box_ammo counts them); -1 none */
static int ammo_item(void)
{
    S_WORK* st = &swork;
    int max, wpn;
    unsigned short bt;
    if (!st->pip) return -1;
    wpn = (st->pip[*st->pip] >> 16) & 0xFF;
    bt = (st->pip[*st->pip] >> 16) & 0xFF00;
    if (wpn == 0) return -1;
    if (wpn == 7 && (bt & 0x2000)) wpn = 155;
    max = (sys->st_flg & 0x8000000) ? 10 : 8;
    for (int i = 0; i < max; i++) {
        unsigned int y = (st->pip[i + 2] >> 16) & 0xFF;
        if (y >= 156 || !(itemdata[y].type & 0x2) || !(st->pip[i + 2] & 0xFFFF)) continue;
        {
            int z = combidata[itemdata[y].cmb], it = itemdata[y].cmb + 1;
            for (; z > 0; z--, it += 3)
                if (wpn == combidata[it] && (combidata[it + 2] & 0xF000)) return (int)y;
        }
    }
    return -1;
}

static int mag_out, mag_kept;     /* (vr132: defined below) */
static unsigned* gun_slot(void);

/* the holster: in front of the belly at the belt (about 62 cm below the eyes, 15 cm forward of
   them), turned with the head's heading smoothed - the box ammo of the gun in hand */
static void holster_update(const float Wg[16], const VrPose* h)
{
    static float yaw_s;
    static int have;
    float hr[9], fx, fz, yaw, pl[3], pw[3];
    int n;
    holster_ok = 0;
    box_item = ammo_item();   /* vr131 */
    if (knife_equipped() || (n = box_ammo()) < 0) return;
    quat_to_rows(hr, h);
    fx = -hr[2]; fz = -hr[8];
    if (fx * fx + fz * fz < 1e-4f) return;
    yaw = atan2f(fx, fz);
    if (!have) { yaw_s = yaw; have = 1; }
    {
        float d = yaw - yaw_s;
        while (d > 3.14159265f) d -= 6.2831853f;
        while (d < -3.14159265f) d += 6.2831853f;
        if (fabsf(d) > 0.6f) yaw_s += d - (d > 0 ? 0.6f : -0.6f);   /* follows when the head turns far, not every glance down */
        yaw_s += d * 0.02f;
    }
    pl[0] = h->px + sinf(yaw_s) * 0.15f; pl[1] = h->py - 0.62f; pl[2] = h->pz + cosf(yaw_s) * 0.15f;
    pw[0] = Wg[0] * pl[0] + Wg[4] * pl[1] + Wg[8] * pl[2] + Wg[12];
    pw[1] = Wg[1] * pl[0] + Wg[5] * pl[1] + Wg[9] * pl[2] + Wg[13];
    pw[2] = Wg[2] * pl[0] + Wg[6] * pl[1] + Wg[10] * pl[2] + Wg[14];
    memcpy(holster_w, pw, sizeof(pw)); holster_ok = 1;
    if (g_vr_holster) {   /* vr132: "in the magazine / in the boxes" (the magazine out: 0 / its rounds with the boxes'), in front of the box, towards her */
        unsigned* g = gun_slot();
        int mag = mag_out >= 0 ? 0 : g ? (int)(*g & 0xFFFF) : 0, box = n + (mag_out >= 0 ? mag_kept : 0);
        float fw[3] = { sinf(yaw_s), 0.0f, cosf(yaw_s) }, q[3], nw[3];
        for (int k = 0; k < 3; k++) q[k] = pl[k] - fw[k] * (0.5f * BOX_D + 0.012f) + (k == 1 ? -0.06f : 0.0f);
        for (int k = 0; k < 3; k++) nw[k] = Wg[k] * q[0] + Wg[4 + k] * q[1] + Wg[8 + k] * q[2] + Wg[12 + k];
        mk_add(VRMK_NUMBER, nw, 1000000 + (mag > 999 ? 999 : mag) * 1000 + (box > 999 ? 999 : box));
    }
    if (n > 0 && box_mode == 0) {   /* vr87: the ammo box at the holster (none left: none shown) */
        float ax[3] = { cosf(yaw_s), 0.0f, -sinf(yaw_s) }, fw[3] = { sinf(yaw_s), 0.0f, cosf(yaw_s) };
        for (int c = 0; c < 8; c++) {
            float q[3];
            for (int k = 0; k < 3; k++)
                q[k] = pl[k] + ax[k] * ((c & 1) ? 0.5f : -0.5f) * BOX_W + (k == 1 ? ((c & 2) ? 0.5f : -0.5f) * BOX_H - 0.06f : 0.0f) + fw[k] * ((c & 4) ? 0.5f : -0.5f) * BOX_D;   /* (vr90: below the ammo count) */
            for (int k = 0; k < 3; k++) box_c[c][k] = Wg[k] * q[0] + Wg[4 + k] * q[1] + Wg[8 + k] * q[2] + Wg[12 + k];
        }
        box_mode = 1;
    }
}


/* Walking in the room: the head's movement in the room (since the last frame, turned into the
   world) moves Claire, through the game's walls - the camera stays over her (make_Wg), so a
   wall stops both and the view is held back at it. Only in her normal states (walking,
   aiming), not on stairs or in the game's own moves. */
static int body_free(float dx, float dz)
{
    NJS_LINE line;
    NJS_POINT3 apos = sys->apos;
    ATR_WORK* ahtp = sys->ahtp;
    float l = sqrtf(dx * dx + dz * dz), len, R = 2.5f;
    int hit;
    if (l < 1e-5f) return 1;
    line.px = plp->px; line.py = plp->py + 6.0f; line.pz = plp->pz;
    line.vx = dx / l * (l + R); line.vy = 0; line.vz = dz / l * (l + R);
    hit = bhCheckL2Wall(&line, 0x400, &len) && len < l + R;
    sys->apos = apos; sys->ahtp = ahtp;
    return !hit;
}

/* move Claire by (dx, dz): her position and the copies the game keeps of it (the frame's start,
   used by its wall test, and its body/stand points), so nothing pulls her back */
static void body_shift(float dx, float dz)
{
    EXP_WORK* ex = (EXP_WORK*)plp->exp0;
    shift_f[shift_src & 3][0] += dx; shift_f[shift_src & 3][1] += dz;
    plp->px += dx; plp->pz += dz;
    plp->pxb += dx; plp->pzb += dz;
    if (ex) { ex->bpx += dx; ex->bpz += dz; ex->bpxb += dx; ex->bpzb += dz; ex->spx += dx; ex->spz += dz; }
    /* the skeleton along: the aim stance pins the feet where they were drawn (bhFixPosition,
       player.c) and would pull her back by the step */
    if (plp->mlwP && plp->mlwP->owP) {
        unsigned i, n = plp->mlwP->obj_num;
        if (n > 64) n = 64;
        for (i = 0; i < n; i++) { plp->mlwP->owP[i].mtx[12] += dx; plp->mlwP->owP[i].mtx[14] += dz; }
    }
}

/* move with the game's walls: whole, else sliding along x or z; which way it went (0 none) */
/* vr80: from (x0, z0) towards (*x, *z) through the game's own wall check (bhCheckWallEx, the player's
   radius and height, as her walking): *x, *z where the game would stop her; 1 if a wall was met */
int vr_game_wall(float x0, float z0, float* x, float* z)
{
    extern int bhCheckWallEx(BH_PWORK* pw, NJS_POINT3* npos, NJS_POINT3* opos, float par, float pah);
    NJS_POINT3 np, op, apos = sys->apos;
    ATR_WORK* ahtp = sys->ahtp;
    int hit;
    op.x = x0; op.y = plp->py; op.z = z0;
    np.x = *x; np.y = plp->py; np.z = *z;
    hit = bhCheckWallEx(plp, &np, &op, plp->ar, plp->ah) != 0;
    sys->apos = apos; sys->ahtp = ahtp;   /* (the game's own state kept) */
    *x = np.x; *z = np.z;
    return hit;
}

static int body_move(float dx, float dz)
{
    float x = plp->px + dx, z = plp->pz + dz;   /* vr80: the room walking stops where the game's walls stop her */
    int hit = vr_game_wall(plp->px, plp->pz, &x, &z);
    if (fabsf(x - plp->px) + fabsf(z - plp->pz) > 1e-5f) { shift_src = 1; body_shift(x - plp->px, z - plp->pz); shift_src = 0; }
    return hit ? 0 : 3;
}

static void body_follow(const VrPose* h)
{
    static float lx, lz;
    static int have;
    static unsigned rec;
    float s = g_vr_scale / 10.0f, c = cosf(vr_yaw), sn = sinf(vr_yaw), dx, dz, l;
    int ok;
    if (!g_vr_roomscale) { have = 0; return; }
    if (!have || rec != xr_recenter_gen() || !vr_active) { lx = h->px; lz = h->pz; have = 1; rec = xr_recenter_gen(); return; }
    dx = (c * (h->px - lx) + sn * (h->pz - lz)) * s;
    dz = (-sn * (h->px - lx) + c * (h->pz - lz)) * s;
    lx = h->px; lz = h->pz;
    ok = plp->mode0 == 1 && plp->mode1 <= 1 && plp->mode2 != 14 && plp->mode2 != 15 && plp->mode2 != 20 && plp->mode2 != 21;
    l = sqrtf(dx * dx + dz * dz);
    if (!ok || l < 1e-4f || l > 3.0f) return;   /* (a jump of the tracking is not a step) */
    body_move(dx, dz);   /* (sliding along a wall) */
}

/* a room change is under way (the game's door/stairs transition, or its loading): the renderer
   holds the last 3D picture and fades it out and back in instead of the cinema screen */
int vr_door_transition(void)
{
    extern unsigned char NowLoadDisp;   /* (system.c) */
    return sys && ((sys->ts_flg & 0x100000) || NowLoadDisp);
}

int vr_session(void) { return xr_stereo_ready(); }   /* the headset shows the game (cutscenes included) */

int g_vr_shadows = 0;   /* ini vr_shadows: the game's screen-space shadows in VR (they float) */
int vr_hide_shadows(void) { return vr_on() && !g_vr_shadows; }

/* ini vr_debug_log: the enemies' diary - every change of an enemy's state, life, hittability,
   scene control and position jumps (or walks through a wall), with the time and room, so a
   misbehaving zombie can be followed in recvx.log. Only changes are written. */
int g_vr_debug_log = 1;
static void enemy_diary(void)
{
    static struct { int seen, m, hp, flg, type; float x, z; } last[128];
    static int room = -1;
    static const char* m0[] = { "init", "move", "grab", "damage", "die", "event", "6", "7" };
    int r = sys->stg_no * 1000 + sys->rom_no * 10 + sys->rcase;
    double t = SDL_GetTicks() / 1000.0;
    if (!g_vr_debug_log) return;
    if (r != room) { room = r; memset(last, 0, sizeof(last)); vr_log("diary %.1f: room RM_%04d, %d enemy slots\n", t, r, sys->ewk_n); }
    for (int i = 0; i < sys->ewk_n && i < 128; i++) {
        BH_PWORK* e = &ene[i];
        int m = e->mode0 << 24 | e->mode1 << 16 | e->mode2 << 8 | e->mode3, f = (int)(e->flg & 0x40023);
        if (!(e->flg & 1)) { if (last[i].seen) { vr_log("diary %.1f: enemy %d (id %d) gone\n", t, i, e->id); last[i].seen = 0; } continue; }
        if (!last[i].seen) {
            vr_log("diary %.1f: enemy %d id %d type %d at (%.0f %.0f), mode %s %d/%d/%d, life %d, %s%s\n", t, i, e->id, e->type, e->px, e->pz,
                   m0[e->mode0 & 7], e->mode1, e->mode2, e->mode3, e->hp, (e->flg & 0x20) ? "hittable" : "unhittable", (e->flg & 0x40000) ? ", held by a scene" : "");
        } else {
            float dx = e->px - last[i].x, dz = e->pz - last[i].z, d = sqrtf(dx * dx + dz * dz);
            if (m != last[i].m) vr_log("diary %.1f: enemy %d mode %s %d/%d/%d (was %s %d/%d/%d), life %d\n", t, i, m0[e->mode0 & 7], e->mode1, e->mode2, e->mode3,
                                       m0[(last[i].m >> 24) & 7], (last[i].m >> 16) & 0xFF, (last[i].m >> 8) & 0xFF, last[i].m & 0xFF, e->hp);
            if (e->hp != last[i].hp) vr_log("diary %.1f: enemy %d life %d -> %d (mode %s)\n", t, i, last[i].hp, e->hp, m0[e->mode0 & 7]);
            if ((f ^ last[i].flg) & 0x20) vr_log("diary %.1f: enemy %d now %s (mode %s %d/%d/%d)\n", t, i, (f & 0x20) ? "hittable" : "unhittable", m0[e->mode0 & 7], e->mode1, e->mode2, e->mode3);
            if ((f ^ last[i].flg) & 0x40000) vr_log("diary %.1f: enemy %d %s a scene\n", t, i, (f & 0x40000) ? "held by" : "let go by");
            if (e->type != last[i].type) vr_log("diary %.1f: enemy %d type %d -> %d\n", t, i, last[i].type, e->type);
            if (d > 15.0f) vr_log("diary %.1f: enemy %d jumped %.0f units to (%.0f %.0f)\n", t, i, d, e->px, e->pz);
            else if (d > 0.3f) {   /* a step across a wall (the game's own wall test, from where it stood) */
                NJS_LINE line;
                NJS_POINT3 apos = sys->apos;
                ATR_WORK* ahtp = sys->ahtp;
                float len;
                line.px = last[i].x; line.py = e->py + 6.0f; line.pz = last[i].z;
                line.vx = dx; line.vy = 0; line.vz = dz;
                if (bhCheckL2Wall(&line, 0x400, &len) && len < d) vr_log("diary %.1f: enemy %d stepped through a wall at (%.0f %.0f), mode %s %d/%d/%d\n", t, i, e->px, e->pz, m0[e->mode0 & 7], e->mode1, e->mode2, e->mode3);
                sys->apos = apos; sys->ahtp = ahtp;
            }
        }
        last[i].seen = 1; last[i].m = m; last[i].hp = e->hp; last[i].flg = f; last[i].type = e->type; last[i].x = e->px; last[i].z = e->pz;
    }
}

/* Walking while aiming: the game holds Claire still with a weapon raised; in VR the left stick
   moves her where the head looks at the speed she walks (runs, with the run trigger) - measured
   from her own walking (not stairs, not the room's walking) - through the game's walls. */
float walk_spd = 0.55f, run_spd = 1.1f;   /* units per game frame, until measured (also steady_anchor) */
static float last_px, last_pz;
static int last_ok;

static unsigned aw_nwall;   /* vr78 diagnosis: aim walk steps the game's wall check stopped */

static float end_px, end_pz;   /* vr95: where she was at the end of the last frame */
static int end_ok, end_aim;
static void aim_walk_update(void)
{
    VrInput in;
    frame_aimwalk[0] = frame_aimwalk[1] = 0.0f;
    if (end_ok && end_aim && plp->mode1 == 1 && plp->mode0 == 1) {   /* vr95: aiming (or the knife) the VR owns her place - the game's own
                                                                        moves in it (the stance pinning her feet back where the aim began) undone */
        float gx = plp->px - end_px, gz = plp->pz - end_pz;
        if (gx * gx + gz * gz > 1e-6f && gx * gx + gz * gz < 9.0f) {
            static unsigned n; static float tot;
            shift_src = 2; body_shift(-gx, -gz); shift_src = 0;
            if (g_vr_debug_log) vw_undo(sqrtf(gx * gx + gz * gz) / (g_vr_scale / 10.0f), plp->mode2);   /* vr108 watch */
            tot += sqrtf(gx * gx + gz * gz);
            if (g_vr_debug_log && (n++ % 150) == 0) vr_log("vr-diag: aiming: the game's own moves undone (%.2f m so far)\n", tot / (g_vr_scale / 10.0f));
        }
    }
    float dx = plp->px - last_px, dz = plp->pz - last_pz, d = sqrtf(dx * dx + dz * dz);
    int stick, run;
    if (!xr_input(&in)) { last_ok = 0; return; }
    stick = in.lx * in.lx + in.ly * in.ly > 0.36f;
    run = in.ltrig > 0.5f || sprint_on;
    if (last_ok && stick && plp->mode0 == 1 && plp->mode1 == 0 && plp->mode2 != 14 && plp->mode2 != 15 && d > 0.05f && d < 5.0f) {
        if (run) run_spd += (d - run_spd) * 0.05f;   /* the game's own steps, averaged */
        else walk_spd += (d - walk_spd) * 0.05f;
    }
    if (g_vr_aim_walk && stick && plp->mode0 == 1 && plp->mode1 == 1 && !(sys->st_flg & 0x200)) {
        float a = (float)cam.ay * (3.14159265f / 32768.0f), fx = sinf(a), fz = -cosf(a), rx = cosf(a), rz = sinf(a);
        float m = sqrtf(in.lx * in.lx + in.ly * in.ly), mx, mz, sp;
        {   /* vr96: drawing / putting away a weapon or the knife (64 up, 68 down, the knife) keeps her walking speed;
               only aiming slows her to 80% - eased over about half a second, no jolt */
            static float k = 1.0f;
            int slow = (plp->mode2 == 65 || plp->mode2 == 66 || plp->mode2 == 67 || plp->mode2 == 69) && !knife_equipped();
            k += ((slow ? 0.8f : 1.0f) - k) * 0.12f;
            sp = k * walk_spd;
            {   static int last = -1; if (g_vr_debug_log && last != plp->mode2) { last = plp->mode2; vr_log("vr-diag: aim walk: state %d, speed %.0f%%\n", plp->mode2, k * 100.0f); } }
        }
        if (m > 1.0f) m = 1.0f;
        mx = (fx * in.ly + rx * in.lx); mz = (fz * in.ly + rz * in.lx);
        d = sqrtf(mx * mx + mz * mz);
        if (d > 1e-4f) {
            mx = mx / d * sp * m; mz = mz / d * sp * m;
            {   /* vr78: the step through the game's own wall check (bhCheckWallEx, her radius and height, as her
                   walking without aiming): she stops where the game stops her */
                extern int bhCheckWallEx(BH_PWORK* pw, NJS_POINT3* npos, NJS_POINT3* opos, float par, float pah);
                NJS_POINT3 np, op;
                float px0 = plp->px, pz0 = plp->pz;
                op.x = plp->px; op.y = plp->py; op.z = plp->pz;
                np.x = plp->px + mx; np.y = plp->py; np.z = plp->pz + mz;
                {   NJS_POINT3 apos = sys->apos; ATR_WORK* ahtp = sys->ahtp;   /* (as body_free: the game's own state kept) */
                    if (bhCheckWallEx(plp, &np, &op, plp->ar, plp->ah)) aw_nwall++;
                    sys->apos = apos; sys->ahtp = ahtp; }
                if (fabsf(np.x - px0) + fabsf(np.z - pz0) > 1e-5f) { shift_src = 2; body_shift(np.x - px0, np.z - pz0); shift_src = 0; }
                frame_aimwalk[0] = plp->px - px0; frame_aimwalk[1] = plp->pz - pz0;
            }
            {
                extern int g_vr_debug_log; extern void host_log(const char* fmt, ...);
                static unsigned nl;
                if (g_vr_debug_log && (nl++ % 150) == 0) host_log("vr-diag: aim walk: the game's wall check stopped the step %u times so far\n", aw_nwall);
            }
        }
    }
}

/* Manual reload: B drops the magazine of the gun in hand (its rounds are kept aside, the gun
   is empty and only clicks - the game's own reload is off), the left grip at the holster takes
   a magazine, the left hand brought to the gun puts it in: the rounds back plus the boxes, as
   the game's own reload (bhSearchBullet) fills it. */
static int mag_out = -1, mag_kept;       /* the inventory slot whose magazine is out, its rounds */
static int mag_held, reloading;
static int grip_latch;          /* vr130: the left grip took the ammo box: no quick knife until it is let go */
static uint64_t tec_ms;         /* vr130: the second tap of the "loaded" haptic on the gun hand */
volatile int g_vr_inv_item = -1;   /* vr130 diagnosis: the item the inventory shows (itemview.c), and when */
volatile uint64_t g_vr_inv_ms;
void vr_inv_item(int id)
{
    static int nl;
    if (id != g_vr_inv_item && nl < 300) { nl++; vr_log("vr-diag: inventory shows item %d\n", id); }
    g_vr_inv_item = id; g_vr_inv_ms = SDL_GetTicks();
}
static unsigned* mag_inv;                 /* vr107: and whose inventory (the character), and which item */
static unsigned mag_item;
static const char* inv_who;               /* vr107 diagnosis: who changed the inventory this frame (inv_watch) */
#define ITEM_OF(v) (((v) >> 16) & 0xFF)

/* vr107: the rounds of a magazine taken out go back to that gun only - the same item in the same
   character's inventory, wherever the game moved it; else nowhere. Put back by slot number alone,
   they could land on another item after the game changed the inventory (a character switch, a
   scene, a sort): a gun that has no ammo was seen with 200 rounds after Steve's part */
static void mag_return(const char* why)
{
    unsigned* p = swork.pip;
    int slot = -1, max = (sys->st_flg & 0x8000000) ? 10 : 8;
    if (mag_out < 0) return;
    if (p && p == mag_inv) {
        if (mag_out > 0 && ITEM_OF(p[mag_out]) == mag_item) slot = mag_out;
        else for (int i = 2; i < max + 2 && slot < 0; i++) if (ITEM_OF(p[i]) == mag_item) slot = i;
    }
    if (slot >= 0) { p[slot] = (p[slot] & 0xFFFF0000u) | (unsigned)mag_kept; inv_who = "VR: magazine rounds back"; }
    vr_log("vr: magazine back (%s): %d rounds of item %u %s slot %d (taken from slot %d%s)\n", why, mag_kept, mag_item,
           slot >= 0 ? "to" : "dropped - not found, was", slot >= 0 ? slot : mag_out, mag_out, p == mag_inv ? "" : ", another character's inventory");
    mag_out = -1; mag_kept = 0;
}
static unsigned* gun_slot(void)
{
    S_WORK* st = &swork;
    if (!st->pip || !*st->pip || knife_equipped()) return NULL;
    if (!((st->pip[*st->pip] >> 16) & 0xFF) || ((st->pip[*st->pip] >> 16) & 0x800)) return NULL;   /* none / endless ammo */
    return &st->pip[*st->pip];
}
static float dist3(const float* a, const float* b) { float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2]; return sqrtf(x * x + y * y + z * z); }
static int near_holster(void) { return holster_ok && hand_ok[1] && dist3(hand_pos[1], holster_w) < 2.5f * (g_vr_scale / 100.0f); }

static void reload_update(void)
{
    static int b_was, g_was;
    VrInput in;
    unsigned* g;
    int b, gr;
    if (tec_ms && SDL_GetTicks() >= tec_ms) { tec_ms = 0; if (g_vr_haptics) xr_haptic(0, 0.35f, 30); }   /* vr130: "tec-tec" */
    if (!g_vr_manual_reload || !xr_input(&in)) return;
    b = (in.buttons & VRB_B) != 0; gr = in.lgrip > 0.5f;
    g = gun_slot();
    if (mag_out >= 0 && (!g || (int)(g - swork.pip) != mag_out || swork.pip != mag_inv || ITEM_OF(*g) != mag_item))   /* the gun changed: its rounds go back */
        mag_return("the gun in hand changed");
    if (g && b && !b_was && mag_out >= 0 && box_ammo() <= 0) {   /* vr111: B again with no ammo to reload: the magazine back in */
        *g = (*g & 0xFFFF0000u) | (unsigned)mag_kept;
        vr_log("vr: magazine back in with B (%d rounds - no ammo to reload with)\n", mag_kept);
        mag_out = -1; mag_kept = 0; inv_who = "VR: B again, magazine back in";
        b_was = b; g_was = gr;
        return;
    }
    if (g && b && !b_was && mag_out < 0 && box_ammo() <= 0 && g_vr_debug_log)
        vr_log("vr: B - magazine kept in: no ammo in the inventory for this gun (e.g. Steve's Lugers)\n");
    if (g && b && !b_was && mag_out < 0 && box_ammo() > 0) {   /* B: the magazine out (vr111: only when it can be reloaded - the game's ammo list) */
        mag_out = (int)(g - swork.pip);
        mag_kept = (int)(*g & 0xFFFF);
        mag_inv = swork.pip; mag_item = ITEM_OF(*g);
        *g &= 0xFFFF0000u;
        inv_who = "VR: B, magazine out";
        CallPlayerWeaponSeEx((NJS_POINT3*)&plp->mlwP->owP[9].mtx[12], 263, 0);
        if (g_vr_haptics) xr_haptic(0, 0.4f, 40);
        vr_log("vr: magazine out (%d rounds kept)\n", mag_kept);
        if (plp->wpnr_no >= 0 && plp->wpnr_no < 23 && (WpnTab[plp->wpnr_no].flg & 0x80)) {   /* vr129: the game's own falling magazine, as its
                                                                                              reload drops it (player.c bhCPM2_act_rld): from the right hand */
            bhSetMagazine(plp, plp->wpnr_no, 9, 0, plp->ay);
            CallPlayerWeaponSeEx((NJS_POINT3*)&plp->mlwP->owP[9].mtx[12], 257, 0);
        }
    }
    if (gr && !g_was && g_vr_debug_log)   /* VR diagnosis: each left grip with a gun in hand */
        vr_log("vr-diag: left grip: near holster %d (holster shown %d) box ammo %d rounds %d mag_out %d\n", near_holster(), holster_ok, box_ammo(), g ? (int)(*g & 0xFFFF) : -1, mag_out);
    if (gr && !g_was && near_holster() && g && box_ammo() > 0) {   /* a magazine from the holster */
        mag_held = 1;
        grip_latch = 1;
        if (g_vr_haptics) xr_haptic(1, 0.5f, 40);
    }
    if (!gr) mag_held = grip_latch = 0;
    if (mag_held && g && hand_ok[0] && hand_ok[1] && dist3(hand_pos[0], hand_pos[1]) < 2.0f * (g_vr_scale / 100.0f)) {   /* the left hand at the gun: in
                                                                    (vr130: also with the magazine in - topped up, as the game's reload; full: the box just put away) */
        int was_out = mag_out >= 0, before;
        if (mag_out >= 0) { *g = (*g & 0xFFFF0000u) | (unsigned)mag_kept; mag_out = -1; mag_kept = 0; }
        before = (int)(*g & 0xFFFF);
        reloading = 1;
        for (int pass = 0; pass < 8; pass++) {   /* the game's reload takes ONE stack of ammo a call: a small first stack (what an earlier
                                                    reload left) filled the gun by a round or two and a second reload was needed */
            unsigned* gg = gun_slot();
            int r0 = gg ? (int)(*gg & 0xFFFF) : 0, r1;
            int did = bhSearchBullet();
            gg = gun_slot();
            r1 = gg ? (int)(*gg & 0xFFFF) : 0;
            if (g_vr_debug_log) vr_log("vr-diag: reload pass %d: %d -> %d rounds (game says %d)\n", pass, r0, r1, did);
            if (!did || r1 <= r0) break;
        }
        reloading = 0;
        inv_who = "VR: hand reload (the game's reload from the boxes)";
        g = gun_slot();   /* (the game sorts the inventory after a reload) */
        if (g && (*g & 0xFFFF)) sys->gm_flg &= ~0x40000;
        mag_held = 0;
        if (was_out || (g && (int)(*g & 0xFFFF) > before)) {
            CallPlayerWeaponSeEx((NJS_POINT3*)&plp->mlwP->owP[9].mtx[12], 264, 0);
            if (g_vr_haptics) { xr_haptic(1, 0.4f, 30); xr_haptic(0, 0.35f, 30); tec_ms = SDL_GetTicks() + 100; }   /* vr130: the box's hand a touch, the gun's "tec-tec" */
            vr_log("vr: reloaded (%d rounds)\n", g ? (int)(*g & 0xFFFF) : 0);
        } else vr_log("vr: the box at a full gun: put away\n");
    }
    b_was = b; g_was = gr;
}

/* One "A" over what the action button would use now: the area the game's own action test
   (hitchk.c bhCheckExmAtari: its probe points ahead of Claire, a door's side) picks; or, from a
   little further (vr_prompt_door cm) a door ahead of the head */
static void prompts_update(void)
{
    float pxn[5], pzn[5], sx, cz, p[3];
    EXP_WORK* ex = (EXP_WORK*)plp->exp0;
    ATR_WORK* found = NULL;
    int etc_n, near;
    if (!g_vr_prompts || !rom || !ex) return;
    njSinCos(plp->ay, &sx, &cz);
    pxn[0] = ex->fpx - sx * (1.5f + (3.0f - ex->arn)); pzn[0] = ex->fpz - cz * (1.5f + (3.0f - ex->arn));
    pxn[1] = plp->px - 0.75f * sx; pzn[1] = plp->pz - 0.75f * cz;
    pxn[2] = plp->px - 4.5f * sx;  pzn[2] = plp->pz - 4.5f * cz;
    pxn[3] = ex->fpx - sx * (3.0f + (3.0f - ex->arn)); pzn[3] = ex->fpz - cz * (3.0f + (3.0f - ex->arn));
    pxn[4] = plp->px - 2.0f * sx;  pzn[4] = plp->pz - 2.0f * cz;
    etc_n = rom->etc_n + sys->metc_n;
    for (int i = 0; i < etc_n && !found; i++) {
        ATR_WORK* e = i < rom->etc_n ? &rom->etcp[i] : &sys->metcp[i - rom->etc_n];
        float x, z;
        if (!(e->flg & 0x1) || e->type > 4 || e->flr_no != plp->flr_no) continue;
        x = pxn[e->type]; z = pzn[e->type];
        if (x < e->px || x > e->px + e->w || z < e->pz || z > e->pz + e->d) continue;
        if (e->type == 0) {   /* a door only from its open side */
            int ang = plp->ay + 8192;
            if ((((ang & 0xC000) == 0x8000) && (e->attr & 0x400)) || (((ang & 0xC000) == 0x4000) && (e->attr & 0x800)) ||
                (!(ang & 0xC000) && (e->attr & 0x1000)) || (((ang & 0xC000) == 0xC000) && (e->attr & 0x2000))) continue;
        }
        found = e;
    }
    near = found != NULL;   /* vr127: the game's own action test - the button works now: the "A" */
    if (!found) {   /* a door a little further, ahead of the head, not behind a wall */
        float best = 20.0f * (g_vr_scale / 100.0f), hy = (float)cam.ay * (3.14159265f / 32768.0f);
        float fx = sinf(hy), fz = -cosf(hy);   /* cam.ay is minus the heading: forward (-sin(-a), -cos(-a)) */
        for (int i = 0; i < etc_n; i++) {
            ATR_WORK* e = i < rom->etc_n ? &rom->etcp[i] : &sys->metcp[i - rom->etc_n];
            float cx, cz2, dx, dz, d;
            if (!(e->flg & 0x1) || e->type != 0 || e->flr_no != plp->flr_no) continue;
            cx = e->px + e->w * 0.5f; cz2 = e->pz + e->d * 0.5f;
            dx = cx - plp->px; dz = cz2 - plp->pz; d = sqrtf(dx * dx + dz * dz);
            if (d > best || d < 1e-3f || (dx * fx + dz * fz) / d < 0.82f) continue;   /* within ~35 degrees of the gaze */
            best = d; found = e;
        }
        if (found) {
            NJS_LINE line;
            NJS_POINT3 apos = sys->apos;
            ATR_WORK* ahtp = sys->ahtp;
            float len, cx = found->px + found->w * 0.5f, cz2 = found->pz + found->d * 0.5f;
            line.px = cam.px; line.py = cam.py; line.pz = cam.pz;
            line.vx = cx - cam.px; line.vy = 0; line.vz = cz2 - cam.pz;
            if (bhCheckL2Wall(&line, 0x400, &len) && len < sqrtf(line.vx * line.vx + line.vz * line.vz) - 3.0f) found = NULL;
            sys->apos = apos; sys->ahtp = ahtp;
        }
    }
    if (!found) return;
    p[0] = found->px + found->w * 0.5f; p[2] = found->pz + found->d * 0.5f;
    p[1] = found->py + (found->h > 0 ? found->h : 0) + 1.5f;
    if (p[1] < plp->py + 4.0f) p[1] = plp->py + 4.0f;
    if (p[1] > plp->py + 15.0f) p[1] = plp->py + 15.0f;
    if (found->type == 0) p[1] = plp->py + 10.5f;   /* vr127: a door's at the middle of a door (~1.05 m) */
    else {   /* vr128: over the thing itself, not over the floor in front of it where she stands */
        O_WRK* op = NULL;
        if (found->type == 4) op = !(found->attr & 0x2) ? &sys->itwp[found->prm0] : found->prm0 != 0xFF ? &sys->obwp[found->prm0] : NULL;
        if (op && (op->flg & 0x1)) { p[0] = op->px; p[2] = op->pz; }   /* the item or object the game would use (hitchk.c) */
        p[1] = plp->py + 10.5f;   /* vr132: every icon at the middle of a door's height (~1.05 m), near the eyes; examined things: the middle of the game's area for it (fixed) */
    }
    mk_add(VRMK_BUTTON, p, near);
}

/* end of tps_camera(), the third-person camera already in `cam`: replace it with the head's */
/* the heading: the player's own with controllers (right stick), else the character's facing; once a game frame */
static void heading_update(void)
{
    static unsigned done_gen;
    VrInput in;
    float facing;
    int ctl;
    if (done_gen == cam_gen) return;
    done_gen = cam_gen;
    smooth_turn = 0.0f;   /* vr120 */
    facing = (float)(short)plp->ay * (3.14159265f / 32768.0f);
    if (g_vr_recenter_on_start && vr_active) {   /* the test save spot has just loaded (lobby_glue.c pc_quickload_take): recentre now - the headset's forward is where the head looks, the body keeps the saved facing */
        g_vr_recenter_on_start = 0;
        xr_recenter();
        vr_log("vr: recentred at the start of the game; saved facing %.0f degrees\n", (double)facing * (180.0 / 3.14159265));
    }
    ctl = xr_input(&in);
    if (!vr_active || !ctl || rec_seen != xr_recenter_gen()) {
        /* the body's facing (plp->ay) is the view's world yaw, vr_yaw PLUS the head's own turn in the recentred space (vr_camera: R = Wg * head,
           tps_vr_view -> plp->ay): resetting vr_yaw to the facing counted the head's turn twice, so a reset (back from the Start menu, a recentre,
           no controller input) with the head turned - looking at the fire, say - turned the view by that angle again (180 degrees behind him).
           The head's turn comes off: the view then stays where the body already faces. */
        VrPose hp;
        float hrow[9], rel = 0.0f, old = vr_yaw;
        if (xr_head_pose(&hp)) { quat_to_rows(hrow, &hp); rel = atan2f(hrow[2], hrow[8]); }
        vr_yaw = facing - rel;
        rec_seen = xr_recenter_gen();
        { static unsigned nl; if (nl++ < 60) vr_log("vr: heading reset (%s): facing %.0f, head turn %.0f, vr_yaw %.0f -> %.0f degrees\n",
                                                  !vr_active ? "camera was off" : !ctl ? "no controller" : "recentred", (double)facing * (180.0 / 3.14159265),
                                                  (double)rel * (180.0 / 3.14159265), (double)old * (180.0 / 3.14159265), (double)vr_yaw * (180.0 / 3.14159265)); }
    }
    if (ctl && on_stairs == 1) {   /* vr91: on the stairs the stick does not turn (the stairs go straight); the head still looks around */
        static unsigned nl;
        if (fabsf(in.rx) > 0.7f && g_vr_debug_log && nl++ < 20) vr_log("vr-diag: turn held on the stairs\n");
        if (fabsf(in.rx) > 0.7f) snap_held = 1;   /* (no turn when she steps off with the stick still out) */
        ctl = 0;
    }
    if (ctl) {
        if (g_vr_turn > 0) {
            if (fabsf(in.rx) > 0.7f && !snap_held) {
                vr_yaw -= (in.rx > 0 ? 1.0f : -1.0f) * g_vr_turn * (3.14159265f / 180.0f); snap_held = 1;
                turn_ms = SDL_GetTicks();   /* vr116: the comfort vignette */
                if (g_vr_debug_log) { static unsigned n; if (n++ < 30) vr_log("vr-diag: snap turn before the hands (frame gen %u)\n", cam_gen); }
            }
            else if (fabsf(in.rx) < 0.3f) snap_held = 0;
        } else {   /* vr141: the renderer turns by the headset's clock (gs_gl.c); here only what it has turned since */
            float d = (float)(int)((unsigned)vr_turn_total() - (unsigned)turn_taken) * 1e-6f;
            if (fabsf(d) < 0.5f && fabsf(d) > 1e-6f) { vr_yaw += d; smooth_turn += d; turn_ms = SDL_GetTicks(); }
        }
    }
    turn_taken = vr_turn_total();   /* vr141: also when not turning - nothing left over for later */
    vr_turn_take(turn_taken, ctl && g_vr_turn == 0 && vr_active);
}

/* vr92 diagnosis (vr_debug_log): "walking on the spot" - the stick pushed, the game walking her, but she does not
   get anywhere: who moved her each frame (the game, the room walk, the aim walk, the steady camera's pull),
   logged when it happens (half a second without getting anywhere), at most once a second */
static void stuck_check(void)
{
    static float px0, pz0, wx[15], wz[15], gm[15], src[15][4], gdx[15], gdz[15];
    static short ays[15];
    static float slx[15], sly[15];
    static unsigned char m2[15];
    static unsigned stf[15];
    static int have, i, nlog;
    static uint64_t last_ms;
    VrInput in;
    float s = g_vr_scale / 10.0f, m = 0.0f, game, sx = 0, sz = 0;
    int k;
    memset(&in, 0, sizeof(in));
    if (!plp) return;
    if (xr_input(&in)) m = sqrtf(in.lx * in.lx + in.ly * in.ly);
    for (k = 1; k < 4; k++) { sx += shift_f[k][0]; sz += shift_f[k][1]; }
    if (have) {
        float gx = plp->px - px0 - sx, gz = plp->pz - pz0 - sz;   /* the game's own move this frame */
        game = sqrtf(gx * gx + gz * gz);
        wx[i] = plp->px; wz[i] = plp->pz; gm[i] = game; gdx[i] = gx; gdz[i] = gz; ays[i] = (short)plp->ay; m2[i] = (unsigned char)plp->mode2; slx[i] = in.lx; sly[i] = in.ly; stf[i] = (unsigned)plp->stflg;
        for (k = 1; k < 4; k++) src[i][k] = sqrtf(shift_f[k][0] * shift_f[k][0] + shift_f[k][1] * shift_f[k][1]);
        i = (i + 1) % 15;
        {   /* the micro stops while walking (the stick held firm and the game's step short), by cause, every 10 s: turning = the stick's direction moved
               more than 25 degrees since the last frame, wall = the game's wall hit, state = the game left its walk/run, other */
            static float plx2, ply2;
            static unsigned nwalk, nslow, c_turn, c_wall, c_state, c_other;
            static uint64_t t10;
            float stepm = game / (s > 0 ? s : 1.0f), pm = sqrtf(plx2 * plx2 + ply2 * ply2), cs = pm > 0.1f && m > 0.1f ? (in.lx * plx2 + in.ly * ply2) / (m * pm) : 1.0f;
            if (g_vr_debug_log && m > 0.7f && plp->mode0 == 1 && plp->mode1 == 0 && !on_stairs) {
                nwalk++;
                if (stepm < 0.06f) {
                    nslow++;
                    if (cs < 0.906f) c_turn++; else if (plp->stflg & 1) c_wall++; else if (plp->mode2 != 6 && plp->mode2 != 3) c_state++; else c_other++;
                }
            }
            plx2 = in.lx; ply2 = in.ly;
            if (g_vr_debug_log && SDL_GetTicks() > t10 + 10000) {
                if (t10 && nwalk) vr_log("vr-diag: walk summary (10 s): %u game frames walking with the stick held, %u with a short step (%u while the stick turned, %u at a wall, %u the game left its walk, %u other)%s", nwalk, nslow, c_turn, c_wall, c_state, c_other, "\n");
                nwalk = nslow = c_turn = c_wall = c_state = c_other = 0; t10 = SDL_GetTicks();
            }
        }
        if (g_vr_debug_log && m > 0.5f && plp->mode0 == 1 && nlog < 60) {
            float gsum = 0, s1 = 0, s2 = 0, s3 = 0, nx = plp->px - wx[i], nz = plp->pz - wz[i], net = sqrtf(nx * nx + nz * nz);
            for (k = 0; k < 15; k++) { gsum += gm[k]; s1 += src[k][1]; s2 += src[k][2]; s3 += src[k][3]; }
            if (gsum > 2.0f && net < 0.3f * gsum && SDL_GetTicks() > last_ms + 1000) {
                last_ms = SDL_GetTicks(); nlog++;
                vr_log("vr-diag: WALKING ON THE SPOT: last 0.5 s the game moved her %.2f m but she got %.2f m; moved back by room walk %.2f, aim walk %.2f, camera pull %.2f m; mode %d/%d/%d flg %x stflg %x, stairs %d, stick %.2f %.2f\n",
                       gsum / s, net / s, s1 / s, s2 / s, s3 / s, plp->mode0, plp->mode1, plp->mode2, (unsigned)plp->flg, (unsigned)plp->stflg, on_stairs, in.lx, in.ly);
                for (k = 0; k < 15; k++) {   /* vr94: frame by frame, oldest first - where she was, the game's own step, her facing */
                    int j = (i + k) % 15;
                    vr_log("vr-diag:   frame %2d: at %.2f %.2f, game step %+.3f %+.3f m, facing %d, mode2 %d, stick %+.2f %+.2f, wall %s, ours room %.3f aim %.3f pull %.3f\n",
                           k, wx[j] / s, wz[j] / s, gdx[j] / s, gdz[j] / s, (int)ays[j], (int)m2[j], slx[j], sly[j], (stf[j] & 1) ? "HIT" : "-", src[j][1] / s, src[j][2] / s, src[j][3] / s);
                }
            }
        }
    }
    px0 = plp->px; pz0 = plp->pz; have = 1;
    memset(shift_f, 0, sizeof(shift_f));
}

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
        if (why) { extern volatile int g_vr_danger, g_vr_poison; extern volatile float g_vr_close_dark, g_vr_move_dark; g_vr_danger = g_vr_poison = 0; g_vr_close_dark = g_vr_move_dark = 0; vr_active = 0; mk_n = 0; return; }
    }

    heading_update();   /* (done already this frame by vr_pad_frame, before the hands) */
    yb = vr_yaw;
    cb = cosf(yb); sb = sinf(yb);
    (void)cb; (void)sb;
    hit_update();
    if (g_vr_danger_fx) {   /* the renderer's danger vignette: life in Danger (the game's limp) */
        extern volatile int g_vr_danger;
        g_vr_danger = plp->exp0 && ((EXP_WORK*)plp->exp0)->dmlvl >= 2 && plp->hp >= 0;   /* vr135: life 0 is alive */
        { extern volatile int g_vr_poison; g_vr_poison = (plp->stflg & 0x280000) && plp->hp >= 0; }   /* vr112: poisoned (the game's own flags, sub1.c) */
    }
    if (g_vr_debug_log) {   /* vr135 log preventivo: at life 0 the VR walking must stay on (it went to the tank controls) */
        static int was0, nl;
        extern int vr_playing(void);
        if (plp->hp == 0 && !was0 && nl++ < 20)
            vr_log("vr-diag: life 0: still alive (mode %d/%d/%d), VR walking %s\n", plp->mode0, plp->mode1, plp->mode2, vr_playing() ? "on" : "OFF");
        was0 = plp->hp == 0;
    }
    {   /* vr116: the comfort vignette (gs_gl.c) - how much she moves by the stick or turns now, 0..1: in in ~0.1 s, out in
           ~0.2 s; walking in the room is not counted (the body's own move does not make one sick) */
        extern volatile float g_vr_move_dark;
        extern int g_vr_comfort;
        VrInput in;
        float want = 0.0f;
        if (g_vr_comfort && xr_input(&in) && vr_playing() && plp->mode0 == 1) {
            float st = sqrtf(in.lx * in.lx + in.ly * in.ly);
            if (st > 0.2f) want = st > 1.0f ? 1.0f : st;
            if (SDL_GetTicks() - turn_ms < 250) want = 1.0f;
        }
        g_vr_move_dark += (want - g_vr_move_dark) * (want > g_vr_move_dark ? 0.35f : 0.2f);
        if (want <= 0.0f && g_vr_move_dark < 0.01f) g_vr_move_dark = 0.0f;
    }

    /* Wg: recentred tracking space (metres) -> world: scaled, turned with the body, the
       recentred origin (the head at recentring) at the player's eyes, tilted by a hit */
    enemy_diary();
    aim_walk_update();
    {   /* vr74 diagnosis (vr_debug_log): the slide after the stick is let go (the game's stop) */
        extern int g_vr_debug_log;
        static int held, n;
        static float sx, sz, dist, lx, lz;
        VrInput in;
        float mag = xr_input(&in) ? sqrtf(in.lx * in.lx + in.ly * in.ly) : 0.0f;
        if (mag > 0.5f) { held = 1; n = 0; }
        else if (held && mag < 0.2f) { held = 0; n = 1; sx = lx = plp->px; sz = lz = plp->pz; dist = 0; }
        else if (n > 0) {
            float dx = plp->px - lx, dz = plp->pz - lz, s = g_vr_scale / 10.0f;
            dist += sqrtf(dx * dx + dz * dz);
            lx = plp->px; lz = plp->pz;
            if (sqrtf(dx * dx + dz * dz) > 1e-3f) n++;
            else {
                if (g_vr_debug_log && n > 1) host_log("vr-diag: stop slide: %.2f m along, %.2f m in a line, over %d game frames (mode %d/%d/%d)\n",
                                                     dist / (s > 0 ? s : 1.0f), sqrtf((plp->px - sx) * (plp->px - sx) + (plp->pz - sz) * (plp->pz - sz)) / (s > 0 ? s : 1.0f),
                                                     n - 1, plp->mode0, plp->mode1, plp->mode2);
                n = 0;
            }
            if (n > 60) n = 0;
        }
    }
    {
        float px0 = plp->px, pz0 = plp->pz;
        body_follow(&h);
        frame_follow[0] = plp->px - px0; frame_follow[1] = plp->pz - pz0;
    }
    last_px = plp->px; last_pz = plp->pz; last_ok = 1;
    make_Wg(Wg);
    xr_recenter_matrix(rec);
    vr_mat_mul(frame_W, Wg, rec);   /* local -> world */
    memcpy(frame_Wg, Wg, sizeof(Wg));

    {   /* A permanent camera jump detector (vr_debug_log): the head's place in the world moving more than 0.3 m in ONE game frame (a run is 0.105 m) -
           logged with the state around it, at most 80 times: the next camera problem shows up in the log without a special trace. */
        extern int g_vr_debug_log;
        static float lx, lz;
        static int have, nj;
        float sc = g_vr_scale / 10.0f, cx = Wg[0] * h.px + Wg[4] * h.py + Wg[8] * h.pz + Wg[12], cz = Wg[2] * h.px + Wg[6] * h.py + Wg[10] * h.pz + Wg[14];
        if (g_vr_debug_log && have && plp && nj < 80) {
            float dx = (cx - lx) / sc, dz = (cz - lz) / sc, d = sqrtf(dx * dx + dz * dz);
            if (d > 0.3f) {
                nj++;
                vr_log("vr-diag: CAMERA JUMP %.2f m in one game frame (%+.2f %+.2f): stairs %d, mode %d/%d/%d, base %.2f %.2f, player %.2f %.2f, follow %+.3f %+.3f, steady_reset %d\n",
                       d, dx, dz, on_stairs, plp->mode0, plp->mode1, plp->mode2, base[0] / sc, base[2] / sc, plp->px / sc, plp->pz / sc, frame_follow[0] / sc, frame_follow[1] / sc, steady_reset);
            }
        }
        lx = cx; lz = cz; have = 1;
    }
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
    holster_update(Wg, &h);
    reload_update();
    prompts_update();
    vr_frame = 1;
    vr_active = 1;
    cam_ms = SDL_GetTicks();
    {   /* the depth constants now, while the frame's near/far (njClipZ in tps_camera) are set:
           tps_after_draw puts the room's own back before vr_after_draw runs */
        extern float Ps2_zbuff_a, Ps2_zbuff_b;
        vr_za = Ps2_zbuff_a; vr_zb = Ps2_zbuff_b;
    }
    stuck_check();
    end_px = plp->px; end_pz = plp->pz; end_ok = 1; end_aim = plp->mode1 == 1;   /* vr95 */
    cam_gen++;   /* the next game frame computes its base anew */
}

int vr_on(void) { return vr_active && xr_stereo_ready(); }
/* player.c bhCPM2_act_wre: the aim let go with the stick pushed - she walks at once (vr79) */
int vr_walk_at_once(void)
{
    VrInput in;
    int r = vr_on() && xr_input(&in) && in.lx * in.lx + in.ly * in.ly > 0.09f;
    if (r && g_vr_debug_log) { static unsigned n; if (n++ < 20) vr_log("vr-diag: aim let go while walking: walking at once\n"); }
    return r;
}


/* in play: the VR camera ran in the last moments - in menus, the inventory, the map and the
   title it does not run (tps_pad, pad.c) */
/* vr86: the frame's input, before the game moves her and the hands are put (bhControlPlayer): the
   heading turned here, so the hands and the world of a frame use the same one (a snap turn showed the
   weapons a frame at the old heading - a ghost) */
/* vr133: running as Call of Duty - one click of the left stick while walking forward runs until the stick stops
   pushing forward (let go, sideways or back) or it is clicked again */
#define RUN_STOP_MS 500   /* the run is let go after this long really stopped (the controller's stick dips last 12 ms median, 57 ms at most in the log; the user asked for half of the first 1 s) */
static void sprint_update(void)
{
    static int was;
    VrInput in;
    int click, fwd;
    if (!xr_input(&in)) { sprint_on = 0; return; }
    click = (in.buttons & VRB_LSTICK) != 0;
    fwd = in.ly > 0.3f;
    if (click && !was) {
        sprint_on = fwd ? !sprint_on : 0;
        if (g_vr_debug_log) vr_log("vr-diag: run %s (stick %.2f %.2f)\n", sprint_on ? "on" : "off", in.lx, in.ly);
    }
    {   /* the run lasts while the stick pushes: it is let go only after she has really stopped for a second - the stick not forward for a second, or
           (pushing) she has not got anywhere for a second (a wall): then it takes a click again. Not at once at a dip of the controller's stick (a log:
           forward 0.98 -> 0.31 -> 0.15 -> back cancelled it - she walked slowly, 0.04 m a step against 0.105, until the next click). */
        static uint64_t t_nofwd, t_still;
        static float lx, lz;
        uint64_t now = SDL_GetTicks();
        float s = g_vr_scale / 10.0f, mv = plp ? sqrtf((plp->px - lx) * (plp->px - lx) + (plp->pz - lz) * (plp->pz - lz)) / (s > 0 ? s : 1.0f) : 1.0f;
        if (plp) { lx = plp->px; lz = plp->pz; }
        if (fwd) t_nofwd = 0; else if (!t_nofwd) t_nofwd = now;
        if (!sprint_on || mv > 0.02f) t_still = 0; else if (!t_still) t_still = now;
        if ((t_nofwd && now - t_nofwd >= RUN_STOP_MS) || (t_still && now - t_still >= RUN_STOP_MS)) {
            if (sprint_on && g_vr_debug_log) vr_log("vr-diag: run off: %s for half a second%s", t_nofwd && now - t_nofwd >= RUN_STOP_MS ? "the stick not forward" : "she did not move", "\n");
            sprint_on = 0; t_still = 0;
        }
    }
    was = click;
}
void vr_pad_frame(void) { sprint_update(); if (plp && vr_active) heading_update(); }
int vr_playing(void) { return vr_on() && SDL_GetTicks() - cam_ms < 100 && !(sys && (sys->st_flg & 0x200)); }   /* (a message or a question on screen: its own controls) */

static int knife_equipped(void)
{
    return plp && plp->wpnr_no >= 0 && plp->wpnr_no < 23 && (WpnTab[plp->wpnr_no].flg & 0x40000000);
}

/* a weapon in each hand (the game's twin guns, playpch.c) */
static int dual_equipped(void)
{
    return plp && plp->wpnr_no >= 0 && plp->wpnr_no < 23 && (WpnTab[plp->wpnr_no].flg & 0x800);
}

/* vr102: a pull of a twin-gun trigger (0 right, 1 left) is kept until that hand's gun fires, at most
   700 ms: the game takes no new shot during the last one's animation (playpch.c bhCPM2_act_atk_pch),
   a quick pull there was lost; and a shot the game fires late is still that hand's (vr101 log: a
   150 ms window hid both flashes - sound and a round, no shot) */
static uint64_t pull_ms[2], decided_ms[2];
static int vw_fire_vr, vw_fire_finger;   /* vr108 watch: the fire button sent, a trigger pulled */
static int pull_was[2], hand_fired[2];
static void trigger_pulls(const VrInput* in)
{
    uint64_t now = SDL_GetTicks();
    for (int s = 0; s < 2; s++) {
        int t = (s ? in->ltrig : in->rtrig) > 0.5f;
        if (t && !pull_was[s]) pull_ms[s] = now ? now : 1;   /* (now | 1 was one ms ahead: a pull dropped at once) */
        pull_was[s] = t;
        if (pull_ms[s] && now - pull_ms[s] > 700) { if (g_vr_debug_log && dual_equipped()) { unsigned* g = gun_slot(); vw_pull_dropped(s, g ? (int)(*g & 0xFFFF) : -1); } pull_ms[s] = 0; }
    }
}

/* the shot the game fires from a hand now is that hand's: its trigger is held or a pull is kept.
   One answer per shot (the hit test and the flash of a shot come within a few frames) */
static int hand_fires(int side)
{
    VrInput in;
    uint64_t now = SDL_GetTicks();
    int held;
    if (decided_ms[side] && now - decided_ms[side] < 100) return hand_fired[side];
    held = xr_input(&in) && (side ? in.ltrig : in.rtrig) > 0.5f;
    hand_fired[side] = held || pull_ms[side];
    pull_ms[side] = 0;
    decided_ms[side] = now ? now : 1;
    return hand_fired[side];
}

/* pwksub.c bhSetGunFire: with twin guns in VR each hand fires with its own trigger - no muzzle
   flash from the hand whose trigger is not pulled */
int vr_dual_block(void* pp, int jno)
{
    if (pp != (void*)plp || !vr_on() || !dual_equipped() || (jno != 9 && jno != 13)) return 0;
    return !hand_fires(jno == 13);
}

/* pwksub.c bhSetYakkyou: the shell of a hand's shot, frames later - as its shot was */
int vr_dual_block_shell(void* pp, int jno)
{
    if (pp != (void*)plp || !vr_on() || !dual_equipped() || (jno != 9 && jno != 13)) return 0;
    return !hand_fired[jno == 13];
}

/* recoil and vibration of a shot from a hand */
static uint64_t recoil_ms[2];
static float recoil_deg[2], recoil_back[2], recoil_tau[2];
static int two_on, two_steer;   /* (vr196/199, below) */
/* vr205: the assault rifle (weapon 12) held in one hand climbs and sways as it fires: each shot adds 3.5 degrees up and up
   to 2.5 to a side, settling with 0.18 s (a burst builds to about 8 up, at most 12); held with both hands it does not move */
#define RIFLE_WPN 12
static float shake_p, shake_y;   /* degrees at shake_ms */
static uint64_t shake_ms;
static float shake_decay(void) { return shake_ms ? expf(-(float)(SDL_GetTicks() - shake_ms) / 180.0f) : 0.0f; }   /* vr132: each shot's kick: degrees up, metres back, how fast it settles (s) */
/* vr132: the gun in hand takes the handgun bullets (item 12): a 9 mm pistol (the game's combination list) */
static int handgun_equipped(void)
{
    S_WORK* st = &swork;
    int wpn, z, it;
    if (!st->pip || !*st->pip || knife_equipped()) return 0;
    wpn = (st->pip[*st->pip] >> 16) & 0xFF;
    if (!wpn) return 0;
    z = combidata[itemdata[12].cmb]; it = itemdata[12].cmb + 1;
    for (; z > 0; z--, it += 3) if (wpn == combidata[it] && (combidata[it + 2] & 0xF000)) return 1;
    return 0;
}
static void shot_kick(int side, int wno)
{
    float k = (wno >= 0 && wno < 22) ? EneDamNear[1][wno] / 60.0f : 0.5f;   /* by the weapon's power (zombie damage) */
    int pistol = handgun_equipped();
    k = k < 0.35f ? 0.35f : k > 1.5f ? 1.5f : k;
    recoil_ms[side] = SDL_GetTicks() | 1;
    {   /* vr132 preventive log: the first shot of each weapon - which kick it got */
        static unsigned char seen[32];
        if (wno >= 0 && wno < 32 && !seen[wno]) {
            seen[wno] = 1;
            vr_log("vr-diag: recoil: weapon %d (item %d): %s\n", wno, swork.pip && *swork.pip ? (int)((swork.pip[*swork.pip] >> 16) & 0xFF) : -1,
                   pistol ? "the handgun's own (6 deg, 2 cm)" : "as before (by the weapon's power)");
        }
    }
    if (wno == RIFLE_WPN && side == 0) {   /* vr205 */
        static int logged;
        float e = shake_decay();
        recoil_deg[side] = 0; recoil_back[side] = 0; recoil_tau[side] = 0.07f;
        if (two_on && two_steer) {
            shake_p = shake_y = 0; shake_ms = 0;
            if (!(logged & 1)) { logged |= 1; vr_log("vr: rifle fired with both hands - no recoil\n"); }
        } else {
            shake_p = shake_p * e + 3.5f;
            shake_y = shake_y * e + ((rand() % 2001) / 1000.0f - 1.0f) * 2.5f;
            if (shake_p > 12.0f) shake_p = 12.0f;
            if (shake_y > 6.0f) shake_y = 6.0f; else if (shake_y < -6.0f) shake_y = -6.0f;
            shake_ms = SDL_GetTicks() | 1;
            recoil_back[side] = 0.015f;
            if (!(logged & 2)) { logged |= 2; vr_log("vr: rifle fired with one hand - it climbs and sways\n"); }
        }
        if (g_vr_haptics) xr_haptic(side, two_on && two_steer ? 0.4f : 0.7f, 40);
        return;
    }
    if (wno == 11 && side == 0) {   /* vr223: the shotgun - the muzzle thrown 65 degrees up one-handed; with both hands 15 */
        int both = two_on && two_steer;
        recoil_deg[side] = both ? 15.0f : 30.0f;   /* vr223: 15 with both hands (owner); vr226: 30 one-handed */ recoil_back[side] = both ? 0.015f : 0.06f; recoil_tau[side] = both ? 0.09f : 0.15f;
        if (g_vr_haptics) xr_haptic(side, 1.0f, both ? 60 : 90);
        return;
    }
    if (wno >= 15 && wno <= 17 && side == 0) {   /* vr208: the grenade launcher - the muzzle thrown up; with both hands only a little */
        int both = two_on && two_steer;
        recoil_deg[side] = both ? 3.0f : 60.0f;   /* vr210: 60 */   /* vr209: 45 one-handed (owner: the muzzle well up) */
        recoil_back[side] = both ? 0.015f : 0.05f; recoil_tau[side] = both ? 0.09f : 0.15f;
        if (g_vr_haptics) xr_haptic(side, both ? 0.6f : 1.0f, 80);
        return;
    }
    if (pistol) {   /* vr132: a 9 mm's kick - a sharp snap of the muzzle up and the hand back, settled in about a tenth of a second */
        /* vr208: stronger one-handed (8 deg, 3 cm, settles slower); with the left hand on it, 3 deg, 1.2 cm */
        int both = two_on && side == 0;
        recoil_deg[side] = both ? 3.96f : 10.56f; recoil_back[side] = both ? 0.0158f : 0.0396f;   /* vr210: 10% more */   /* vr209: 20% more */
        recoil_tau[side] = both ? 0.05f : 0.075f;
        if (g_vr_haptics) xr_haptic(side, 0.8f, 35);
        return;
    }
    recoil_deg[side] = 9.0f * k; recoil_back[side] = 0.03f * k; recoil_tau[side] = 0.07f;
    if (g_vr_haptics) xr_haptic(side, k > 1.0f ? 1.0f : 0.3f + 0.6f * k, k > 1.0f ? 90 : 50);
}
static void shot_feedback(int side, int wno)
{
    static int nlog;
    shot_kick(side, wno);
    if (nlog++ < 400)
        vr_log("vr: shot %u ms, weapon %d %s hand, %s: kick %.1f deg up, %.1f cm back%s\n", (unsigned)SDL_GetTicks(), wno, side ? "left" : "right",
               two_on ? (two_steer ? "both hands (aiming)" : "both hands (holding)") : "one hand", recoil_deg[side], recoil_back[side] * 100.0f,
               g_vr_recoil ? "" : " - recoil off in the ini");
}

static int player_shot(const GA_WORK* gap)
{
    return gap && plp && gap->wpn_no == plp->wpnr_no && gap->ax == plp->wax && gap->ay == plp->way;
}

/* weapon.c bhCheckGunAtari: in VR the player's shots (but the shotgun's spread) hit the
   enemies' body parts (their capsules on the skeleton, as the 3D is), not one body capsule */
int vr_precise_parts(GA_WORK* gap)
{
    return vr_on() && g_vr_precise_hits && player_shot(gap) && gap->wpn_no >= 0 && gap->wpn_no < 23 && !(WpnTab[gap->wpn_no].flg & 0x80000000);
}

/* weapon.c bhCheckGunAtari: a zombie the game has not woken yet (not hittable until it does)
   can be shot from afar by the player in VR - the hit wakes it as the game's own reaction to
   damage. Not within 3 m: the game's grabs and bites are its own. */
int vr_shot_wakes(BH_PWORK* pp, GA_WORK* gap)
{
    float dx, dz;
    if (!vr_on() || !g_vr_wake_shots || !player_shot(gap) || !pp || (pp->id != 1 && pp->id != 26)) return 0;
    if (pp->mode0 != 1 || pp->hp < 0) return 0;   /* (not one falling or dying: the game keeps those unhittable on purpose) */
    dx = pp->px - plp->px; dz = pp->pz - plp->pz;
    if (dx * dx + dz * dz < 30.0f * 30.0f * (g_vr_scale / 100.0f) * (g_vr_scale / 100.0f)) return 0;
    if (pp->mlwP && pp->mlwP->owP && pp->mlwP->obj_num > 1) {
        /* its body capsule where it stands now: the game does not move it while frozen, and the shot
           test starts from that capsule's middle (a stale one is "behind the gun": shots missed) */
        O_WORK* o = pp->mlwP->owP;
        float best = -1;
        int far = 0;
        for (int j = 1; j < (int)pp->mlwP->obj_num && j < 64; j++) {
            float ex = o[j].mtx[12] - o[0].mtx[12], ey = o[j].mtx[13] - o[0].mtx[13], ez = o[j].mtx[14] - o[0].mtx[14], l2 = ex * ex + ey * ey + ez * ez;
            if (l2 > best) { best = l2; far = j; }
        }
        pp->watr.c1.x = o[far].mtx[12]; pp->watr.c1.y = o[far].mtx[13]; pp->watr.c1.z = o[far].mtx[14];
        pp->watr.c2.x = o[0].mtx[12]; pp->watr.c2.y = o[0].mtx[13]; pp->watr.c2.z = o[0].mtx[14];
        if (pp->watr.r < 2.0f) pp->watr.r = 2.5f;
    }
    return 1;
}

/* weapon.c bhCheckGunAtari, after the damage of a player's hit is added (and the knife's):
   in VR, scaled by the part of a zombie's body hit - the hit point (dpx..) against the enemy:
   the top of it the head, the lower part the legs, away from the body capsule's axis the arms */
static int vr_knife_cut;          /* vr156: our swinging knife's hit test is running (vr_slash) */
static int vr_knife_hits;         /* ... and what it hit this cut (damage added) */
int vr_part_damage(BH_PWORK* pp, int kno, int d0, int d1)
{
    static const char* names[] = { "body", "head", "arm", "leg" };
    float top = 0, h, rel, mult;
    int part = 0, add = d1 - d0, out;
    if (!vr_on() || !pp || add <= 0 || (pp->id != 1 && pp->id != 26) || !pp->mlwP) return d1;   /* zombies */
    for (int j = 1; j < (int)pp->mlwP->obj_num; j++) if (pp->mlwP->owP[j].mtx[13] - pp->py > top) top = pp->mlwP->owP[j].mtx[13] - pp->py;
    h = top > 1.0f ? top : 17.0f;
    rel = (pp->dpy - pp->py) / h;
    if (rel >= 0.84f) part = 1;
    else if (rel <= 0.45f) part = 3;
    else {   /* away from the body capsule's axis: an arm */
        const NJS_CAPSULE* c = &pp->watr;
        float ax = c->c2.x - c->c1.x, ay = c->c2.y - c->c1.y, az = c->c2.z - c->c1.z, l2 = ax * ax + ay * ay + az * az, t = 0;
        float px = pp->dpx - c->c1.x, py = pp->dpy - c->c1.y, pz = pp->dpz - c->c1.z, dx, dy, dz;
        if (l2 > 1e-6f) { t = (px * ax + py * ay + pz * az) / l2; t = t < 0 ? 0 : t > 1 ? 1 : t; }
        dx = px - ax * t; dy = py - ay * t; dz = pz - az * t;
        if (dx * dx + dy * dy + dz * dz > (c->r * 0.85f) * (c->r * 0.85f)) part = 2;
    }
    if (part == 0) {   /* the body: anywhere between vr_dmg_body and vr_dmg_body_max */
        int lo = g_vr_dmg_body, hi = g_vr_dmg_body_max > g_vr_dmg_body ? g_vr_dmg_body_max : g_vr_dmg_body;
        mult = (lo + (hi > lo ? rand() % (hi - lo + 1) : 0)) / 100.0f;
    } else mult = (part == 1 ? g_vr_dmg_head : part == 2 ? g_vr_dmg_arm : g_vr_dmg_leg) / 100.0f;
    if (vr_knife_cut) mult *= 0.7f;   /* vr156: the swinging knife at 70% */
    out = (int)(add * mult + 0.5f);
    if (out < 1) out = 1;
    if (vr_knife_cut) vr_knife_hits += out;
    { static int nlog; if (nlog++ < 300 || g_vr_debug_log) vr_log("vr: hit zombie %d %s x%.2f (%d -> %d), joint %d\n", (int)(pp - ene), names[part], mult, add, out, kno); }
    if (part == 1 && pp->id == 1 && pp->exp0 && kno >= 0 && kno < 64) {
        /* vr215: the common zombie (en01.c) - a head shot that kills it is a critical: its head bursts, as the game's own
           (bhEne01_CheckExpHead: the head's damage count, exp0 + 0x78, above 0; -1 = no head left) */
        int sum = 0, *hc = (int*)((char*)pp->exp0 + 0x78);
        for (int i = 0; i < 64; i++) sum += i == kno ? d0 + out : pp->dam[i];
        if (pp->hp - sum <= 0 && *hc != -1) {
            if (*hc <= 0) *hc = 1;
            { extern unsigned char vr_crit_mark[64]; int ix = (int)(pp - ene); if (ix >= 0 && ix < 64) vr_crit_mark[ix] = 1; }   /* vr216 */
            vr_log("vr: critical - zombie %d killed by a head shot (hp %d, damage %d): its head bursts\n", (int)(pp - ene), pp->hp, sum);
        }
    }
    return d0 + out;
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
    aim_rows(r, &in.aim);
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
    trigger_pulls(&in);
    if (in.ly > 0.4f) k |= 1;            /* TPS_K_FWD */
    if (in.ly < -0.4f) k |= 2;           /* TPS_K_BACK */
    if (in.lx < -0.4f) k |= 4;           /* TPS_K_LEFT */
    if (in.lx > 0.4f) k |= 8;            /* TPS_K_RIGHT */
    {
        int dual_aim = vr_playing() && dual_equipped() && in.rgrip > 0.5f;   /* twin guns raised: the left trigger fires the left gun */
        if ((in.ltrig > 0.5f && !dual_aim) || sprint_on) k |= 16;   /* TPS_K_RUN (vr133: the stick click toggles it) */
        static uint64_t kick_until;
        static int kicked;
        int fire = in.ltrig > 0.5f || in.rtrig > 0.5f;
        if (!(dual_aim && (pull_ms[0] || pull_ms[1]))) kicked = 0;   /* (each kept pull its own press) */
        else {
            /* vr103: a kept pull is a fresh press when the game can take the next shot (aiming, or the
               end of the last shot's animation, playpch.c): the game fires only on fire let go and
               pressed again (at_flg 0x40) - held through, the vr102 kept pulls were all dropped */
            uint64_t now = SDL_GetTicks();
            int ready = plp && ((plp->mode2 >= 65 && plp->mode2 <= 67) || (plp->mode2 == 169 && (plp->at_flg & 0x10)));
            if (!ready) { fire = 0; kicked = 0; }
            else if (!kicked) {   /* let go for over a game frame, then pressed */
                kicked = 1; kick_until = now + 40;
            }
            if (ready) fire = now >= kick_until;
        }
        if (dual_aim && fire) k |= 512;   /* TPS_K_FIRE: either gun */
        if (!dual_aim) pull_ms[0] = pull_ms[1] = 0;
    }
    if (in.buttons & VRB_A) k |= 32;     /* TPS_K_ACTION */
    if (in.buttons & VRB_Y) k |= 64;   /* TPS_K_STATUS (vr155: LT + Y is no longer Start) */
    if ((in.buttons & VRB_RSTICK) || (in.buttons & VRB_X)) k |= 128;   /* TPS_K_MAP: the right stick's click (vr197), and X again (vr228: the knife is back on the left grip) */
    if (in.rgrip > 0.5f && !vr_on_stairs()) k |= 256;       /* TPS_K_AIM (vr186: not on stairs - raising a weapon there takes her out of the stair walk, off its path, through the wall) */
    else if (in.rgrip > 0.5f) { static uint64_t lm; uint64_t t = SDL_GetTicksNS() / 1000000; if (t > lm + 1000) { lm = t; vr_log("vr: aim held on the stairs - not passed to the game (mode %d/%d)\n", plp->mode1, plp->mode2); } }
    if (in.rtrig > 0.5f && !(vr_playing() && knife_equipped() && g_vr_slash_speed > 0)) k |= 512;   /* TPS_K_FIRE (the knife cuts by swinging) */
    vw_fire_vr = (k & 512) != 0; vw_fire_finger = in.ltrig > 0.5f || in.rtrig > 0.5f;   /* vr108 watch */
    if ((in.buttons & VRB_B) && !(g_vr_manual_reload && vr_playing() && gun_slot())) k |= 1024;   /* TPS_K_CANCEL (with a gun in hand B drops the magazine) */
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
        frame_body[0] = Wg[12]; frame_body[1] = Wg[13]; frame_body[2] = Wg[14];
        Wg[12] += close_push[0] + bite_off[0]; Wg[14] += close_push[1] + bite_off[1];   /* vr112: back from an enemy at the eyes (vr115: and each bite) */
    }
    if (g_vr_roomscale) {   /* the head over the body: the room's walking is the body's (body_follow) */
        VrPose h;
        if (xr_head_pose(&h)) { Wg[12] -= (c * h.px + sn * h.pz) * s; Wg[14] -= (-sn * h.px + c * h.pz) * s; }
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
        if (n < 200) n += snprintf(buf + n, sizeof(buf) - n, " %d(id %d, node %d, %d parts)", i, o->id, o->lkono, o->mlwP ? (int)o->mlwP->obj_num : 0);
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
    static uint64_t lt;
    static int have, swing, armed = 1;
    static uint64_t last_cut;   /* vr174 */
    static uint64_t bounce_watch;   /* vr185 */
    uint64_t now = SDL_GetTicksNS();
    float v[3] = { 0, 0, 0 }, sp = 0;
    if (have && now > lt) {
        float dt = (now - lt) / 1e9f;
        v[0] = (hp->px - lp[0]) / dt; v[1] = (hp->py - lp[1]) / dt; v[2] = (hp->pz - lp[2]) / dt;
        sp = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (dt > 0.2f) sp = 0;   /* a pause, not a swing */
    }
    lp[0] = hp->px; lp[1] = hp->py; lp[2] = hp->pz; lt = now; have = 1;
    if (p->mode0 == 1 && p->mode1 == 1 && p->mode2 == 72 && !(p->at_flg & 0xE)) {
        /* vr184: stuck in the knife's wall bounce (mode 72). The game leaves it only for an aiming stance (at_flg
           2/4/8, bhCPM2_act_knf); our swing hits the wall without one (or the knife is put away during it), so
           the bounce restarted forever - she walked on the spot (vr183 log) */
        /* vr185: not by setting the modes ourselves (vr184's mode 0/0/0 left her half out of it - mode 1/0/3,
           stflg 0x10000 still set, no door or item would answer A): the aiming stance the game wants is given
           instead, and the game's own end of the bounce (to stance 65, mid) does the rest */
        vr_log("vr: stuck in the knife's wall bounce (at_flg %x, flg %x, stflg %x) - given the mid aiming stance, the game ends it\n", p->at_flg, p->flg, p->stflg);
        p->at_flg |= 2;
        bounce_watch = now;
    }
    if (bounce_watch && now - bounce_watch < 3000000000ull) {   /* vr185 log: her state the 3 s after, at each change */
        static int lm1 = -1, lm2 = -1; static unsigned lst;
        if (p->mode1 != lm1 || p->mode2 != lm2 || p->stflg != lst) {
            lm1 = p->mode1; lm2 = p->mode2; lst = p->stflg;
            vr_log("vr: after the bounce: mode %d/%d/%d at_flg %x flg %x stflg %x\n", p->mode0, p->mode1, p->mode2, p->at_flg, p->flg, p->stflg);
        }
    }
    if (!knife_equipped() || g_vr_slash_speed <= 0 || p->mode0 != 1 || !vr_playing()) { swing = 0; return; }
    if (sp * 100.0f < 0.5f * g_vr_slash_speed) armed = 1;   /* vr91: the hand slowed down - the next swing may cut (vr183: vr180's 0.15 s slow wait undone, it held back real swings; the burst was vr182's wall fix) */
    if (!swing && sp * 100.0f > g_vr_slash_speed && (!armed || now - last_cut <= 350000000ull)) {   /* vr183 log: a fast swing that did not cut, and why */
        static uint64_t last_ms;
        if (now / 1000000 > last_ms + 300) { last_ms = now / 1000000; vr_log("vr: knife swing held back (hand %.1f m/s, %s)\n", sp, !armed ? "hand not slowed since the last cut" : "within 0.35 s of the last cut"); }
    }
    if (!swing && armed && sp * 100.0f > g_vr_slash_speed && now - last_cut > 350000000ull) {   /* vr179: at least 0.35 s apart (back from vr176 1 s, the wall bounce fixed in vr178) (vr174: 0.35 s was still too close) (one swing of the arm made 2-3 cuts) */
        /* vr89: only the edge cuts - the knife's +z (measured, vr88: it led 33 of 34 right cuts) ahead of the hand's move */
        float c = cosf(vr_yaw), sn = sinf(vr_yaw), mw[3] = { c * v[0] + sn * v[2], v[1], -sn * v[0] + c * v[2] };
        float ml = sqrtf(mw[0] * mw[0] + mw[1] * mw[1] + mw[2] * mw[2]), zl = sqrtf(H[8] * H[8] + H[9] * H[9] + H[10] * H[10]);
        float edge = (ml > 1e-4f && zl > 1e-6f) ? (mw[0] * H[8] + mw[1] * H[9] + mw[2] * H[10]) / (ml * zl) : 0.0f;
        if (edge >= 0.5f) {   /* vr93: within ~60 degrees of the edge (45 was too tight, vr91 log) */
            swing = 4;
            armed = 0;
            uint64_t prev_cut = last_cut;
            last_cut = now;
            vr_log("vr: knife cut t=%u ms (%u ms after the last, hand %.1f m/s, edge %+.2f, mode %d/%d)\n", (unsigned)(now / 1000000), (unsigned)((now - prev_cut) / 1000000), sp, edge, p->mode1, p->mode2);   /* vr181 log */
            if (g_vr_haptics) xr_haptic(0, 0.5f, 40);
        } else if (g_vr_debug_log) {
            static uint64_t last_ms;
            if (now / 1000000 > last_ms + 300) { last_ms = now / 1000000; vr_log("vr-diag: knife swing not on the edge (edge %+.2f): no cut\n", edge); }
        }
    }
    {   /* vr181 log: the game's own knife stab (mode 69) - not ours */
        static int was;
        int is = p->mode1 == 1 && p->mode2 == 0x45;
        if (is && !was) vr_log("vr: game's own knife stab t=%u ms\n", (unsigned)(now / 1000000));
        was = is;
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
        int m2 = plp->mode2, wall;
        /* vr182: a wall hit sets mode2 to 72 (the bounce); a cut started while still bouncing (mode2 already 72)
           did not see its hit and tested the wall again each frame - 4 clanks at once (vr181 log: those cuts had
           no "hit a wall"). A mark in mode2 first: 72 after the test is a hit whatever it was before */
        plp->mode2 = 0xEE;
        vr_knife_cut = 1; vr_knife_hits = 0;
        bhCheckKnifeAtari(&gap);
        vr_knife_cut = 0;
        wall = plp->mode2 == 72;
        if (plp->mode2 == 0xEE) plp->mode2 = m2;
        vr_log("vr: knife test frame %d (mode before %d): %s\n", 5 - swing, m2, wall ? "wall" : vr_knife_hits > 0 ? "enemy" : "nothing");   /* vr182 log */
        if (wall) {   /* vr178: the blade hit a wall (the game's bounce, mode 72) - one hit, the cut ends */
            vr_log("vr: knife cut hit a wall, cut ends\n");
            swing = 0;
        } else if (vr_knife_hits > 0) {   /* vr156: one hit a cut - the blade still in the body the next frames hit again (up to 4) */
            vr_log("vr: knife cut hit (%d damage), cut ends\n", vr_knife_hits);
            swing = 0;
        } else swing--;
    }
}

/* VR markers handed to the renderer with the frame (VrView.mk*): the aiming dots and the
   holster's ammo count. Filled during the game frame,
   handed over and cleared by vr_after_draw. */
static void mk_add(int kind, const float p[3], int val)
{
    if (mk_n >= 12) return;
    memcpy(mk_pt[mk_n], p, 3 * sizeof(float));
    mk_kind[mk_n] = kind; mk_val[mk_n] = val;
    mk_n++;
}

/* the barrel of the weapon in each hand (world, unit), set by vr_hand: where the game's own aim
   pose points that weapon, carried by the hand - the controller's forward through the hand
   pose, turned by how this weapon sits in the hand compared with it */
static float barrel_w[2][3], muzzle_w[2][3];

/* vr196: both hands on the gun - the left grip held with the left hand near the gun turns the right
   controller so the barrel runs from the right hand to the left one (its roll stays the right hand's).
   two_q is that turn, in the tracking space; aim_rows() gives the right controller's rows with it. */
static int two_on;
static int two_steer, two_item;
static float two_near_cm = 1e9f, two_grab_cm;
static int left_near_gun;   /* vr228: the left hand within 12 cm of the gun (last frame) */   /* two_grab_cm (vr212): how near it was when taken */
static float two_t0[3];       /* vr202: the line between the hands at the grab, in the right controller's frame */
static int two_need_ref;   /* vr201: last frame, the left hand (on its controller) from where it would hold the gun, cm */   /* vr199: the left hand turns the gun only on the big two-handed guns; on the rest it just holds on */
static int equipped_item(void) { S_WORK* st = &swork; return st->pip && *st->pip ? (int)((st->pip[*st->pip] >> 16) & 0xFF) : 0; }
/* items 1 rocket launcher, 2 assault rifle, 3 sniper rifle, 4 shotgun, 6 grenade launcher (RECVX_EQUIP 4102 = 0x1006), 7 bow gun,
   11 linear launcher (8 is the knife, knife_glue.c) */
static int big_gun(int it) { return it == 1 || it == 2 || it == 3 || it == 4 || it == 6 || it == 11; }   /* vr200: the bow gun (7) is one-handed, left out */
static float two_q[9], two_t[3];
static uint64_t two_ms;   /* vr198: when both hands took the gun (the turn and the left hand go over in 150 ms) */
static float two_w(void) { float w = (SDL_GetTicks() - two_ms) / 150.0f; return two_on ? (w > 1 ? 1 : w < 0 ? 0 : w) : 0; }

/* vr198: the game's own aim pose puts the left hand on the gun: where it is from the right hand node (inverse(H9) * H13),
   per weapon, taken while she stands aimed (mode 65..67) - the hook sees the game's pose before the hands are moved */
static float two_rel[64][16];
static int two_rel_ok[64], two_rel_near[64], two_left_logged;
static float grab_rel[16];   /* vr213: the left hand's hold taken at the grab, for a weapon with none fixed */
static int grab_on;
/* vr203: the drawn barrel of each weapon in the right hand node's frame - in the game's aim pose the gun points where she
   aims (Ry(ay) Rx(wax), the frame the hand pose is learned with), so that way, taken into the node, is its barrel. The
   node's x axis (vr96..202) is the pistol's barrel only: the grenade launcher's lies another way in the hand. */
static float two_bl[64][3];
static int two_bl_ok[64];
static void m4_mul(float* o, const float* a, const float* b)   /* affine, column-major: o = a * b */
{
    float r[16];
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 3; i++)
            r[j * 4 + i] = a[0 * 4 + i] * b[j * 4 + 0] + a[1 * 4 + i] * b[j * 4 + 1] + a[2 * 4 + i] * b[j * 4 + 2] + (j == 3 ? a[12 + i] : 0);
    r[3] = r[7] = r[11] = 0; r[15] = 1;
    memcpy(o, r, sizeof(r));
}
static int m4_inv(float* o, const float* m)   /* affine inverse */
{
    float a = m[0], b = m[4], c = m[8], d = m[1], e = m[5], f = m[9], g = m[2], h = m[6], i = m[10], det, r[16];
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (fabsf(det) < 1e-12f) return 0;
    r[0] = (e * i - f * h) / det; r[4] = (c * h - b * i) / det; r[8] = (b * f - c * e) / det;
    r[1] = (f * g - d * i) / det; r[5] = (a * i - c * g) / det; r[9] = (c * d - a * f) / det;
    r[2] = (d * h - e * g) / det; r[6] = (b * g - a * h) / det; r[10] = (a * e - b * d) / det;
    for (int k = 0; k < 3; k++) r[12 + k] = -(r[k] * m[12] + r[4 + k] * m[13] + r[8 + k] * m[14]);
    r[3] = r[7] = r[11] = 0; r[15] = 1;
    memcpy(o, r, sizeof(r));
    return 1;
}
/* vr207: what was learned of each weapon (its barrel, the left hand's hold) is kept in <save>/vr_barrel.txt and read at the
   start - learned once, not every session. The rifle's and the grenade launcher's barrels come built in (vr205's log). */
static int bar_loaded;
const char* pc_save_dir(void);   /* config.c */
static void bar_path(char* out, size_t n) { snprintf(out, n, "%s/vr_barrel.txt", pc_save_dir()); }
static void bar_save(void)
{
    char pth[512];
    FILE* f;
    bar_path(pth, sizeof(pth));
    if (!(f = fopen(pth, "w"))) return;
    for (int w = 0; w < 64; w++) {
        if (two_bl_ok[w]) fprintf(f, "b %d %.6f %.6f %.6f\n", w, two_bl[w][0], two_bl[w][1], two_bl[w][2]);
        if (two_rel_ok[w]) {
            fprintf(f, "h %d %d", w, two_rel_near[w]);
            for (int k = 0; k < 16; k++) fprintf(f, " %.6f", two_rel[w][k]);
            fprintf(f, "\n");
        }
    }
    fclose(f);
}
static void bar_load(void)
{
    /* vr213: fixed - what the owner's game learned and he confirmed (save/vr_barrel.txt of vr212): the pistol (4), the
       rifle (12), the grenade launcher (14..17: one table entry for its four rounds, 15 and 17 as 16). No longer learned
       nor read from the file. A weapon without one keeps the hand node's x axis as its barrel and the left hand holds it
       where it took it (vr213, below). */
    static const struct { int w; float b[3]; } bars[] = {
        { 4, { 0.999885f, -0.003533f, 0.014759f } }, { 12, { 0.999750f, -0.009998f, 0.019995f } },
        { 14, { 0.646480f, 0.003353f, -0.762923f } }, { 15, { 0.691905f, 0.000000f, -0.721988f } },
        { 16, { 0.691905f, 0.000000f, -0.721988f } }, { 17, { 0.691905f, 0.000000f, -0.721988f } },
    };
    static const float hold4[16] = { -0.839847f, -0.536678f, 0.081225f, 0, 0.144463f, -0.076750f, 0.986542f, 0, -0.523176f, 0.840307f, 0.142045f, 0, 0.117384f, -0.434351f, 0.450694f, 1 };
    static const float hold_gl[16] = { -0.448615f, -0.855858f, 0.257448f, 0, 0.724132f, -0.179230f, 0.666047f, 0, -0.523919f, 0.485275f, 0.700157f, 0, 2.243025f, -1.023558f, -1.827977f, 1 };
    bar_loaded = 1;
    for (unsigned k = 0; k < sizeof(bars) / sizeof(bars[0]); k++) { memcpy(two_bl[bars[k].w], bars[k].b, sizeof(bars[k].b)); two_bl_ok[bars[k].w] = 1; }
    memcpy(two_rel[4], hold4, sizeof(hold4)); two_rel_ok[4] = two_rel_near[4] = 1;
    for (int w = 14; w <= 17; w++) { memcpy(two_rel[w], hold_gl, sizeof(hold_gl)); two_rel_ok[w] = two_rel_near[w] = 1; }
    vr_log("vr: weapon barrels and left-hand holds fixed (built in)\n");
}

static void two_learn(const BH_PWORK* p, int wn)
{
    static int steady, last_wn = -1;
    const float *h9, *h13;
    float inv[16], dx, dy, dz, dist, s = g_vr_scale / 10.0f;
    int level = (short)p->wax <= 546 && (short)p->wax >= -546;   /* vr206: the aim level, within 3 degrees */
    if (!bar_loaded) bar_load();
    return;   /* vr213: nothing learned - all fixed (bar_load) */
    if (wn != last_wn) { steady = 0; last_wn = wn; }
    if (!(p->mode0 == 1 && p->mode1 == 1 && p->mode2 >= 65 && p->mode2 <= 67) || knife_equipped() || dual_equipped()) { steady = 0; return; }
    if (++steady < 10) return;
    if (!level || (two_rel_ok[wn] && two_bl_ok[wn])) return;   /* vr207: each learned once, with the aim level, and kept */
    h9 = p->mlwP->owP[9].mtx; h13 = p->mlwP->owP[13].mtx;
    if (!m4_inv(inv, h9)) return;
    if (!two_bl_ok[wn]) {   /* vr203: the aim's way in the node's frame (its columns may be scaled) */
        float A[9], a[3], bl[3], xa, el, yw;
        rows_from_angles(A, (float)(short)p->ay * (3.14159265f / 32768.0f), (float)(short)p->wax * (3.14159265f / 32768.0f));
        a[0] = -A[2]; a[1] = -A[5]; a[2] = -A[8];
        for (int j = 0; j < 3; j++) {
            float c2 = h9[j * 4 + 0] * h9[j * 4 + 0] + h9[j * 4 + 1] * h9[j * 4 + 1] + h9[j * 4 + 2] * h9[j * 4 + 2];
            bl[j] = c2 > 1e-12f ? (h9[j * 4 + 0] * a[0] + h9[j * 4 + 1] * a[1] + h9[j * 4 + 2] * a[2]) / c2 : 0;
        }
        xa = sqrtf(bl[0] * bl[0] + bl[1] * bl[1] + bl[2] * bl[2]);
        if (xa > 1e-6f) {
            for (int j = 0; j < 3; j++) bl[j] /= xa;
            /* how far from the node's x axis (the old barrel line): about z ("height" in wp_fps1) and about y */
            xa = acosf(fabsf(bl[0]) > 1 ? 1 : fabsf(bl[0])) * (180.0f / 3.14159265f);
            el = atan2f(bl[2], fabsf(bl[0])) * (180.0f / 3.14159265f);
            yw = atan2f(bl[1], fabsf(bl[0])) * (180.0f / 3.14159265f);
            vr_log("vr: weapon %d drawn barrel learned and kept (aim %.1f deg) %.1f deg from the hand's x axis (%.1f toward the barrel's height axis, %.1f to the side), in the node %.2f %.2f %.2f\n",
                   wn, (short)p->wax * (180.0f / 32768.0f), xa, el, yw, bl[0], bl[1], bl[2]);
            memcpy(two_bl[wn], bl, sizeof(bl));
            two_bl_ok[wn] = 1;
        }
    }
    if (!two_rel_ok[wn]) {
        m4_mul(two_rel[wn], inv, h13);
        dx = h13[12] - h9[12]; dy = h13[13] - h9[13]; dz = h13[14] - h9[14];
        dist = sqrtf(dx * dx + dy * dy + dz * dz) / (s > 0 ? s : 1);   /* metres (game units per metre: vr_scale / 10) */
        vr_log("vr: weapon %d aim pose - the game's left hand %.0f cm from the right (%s), kept\n", wn, dist * 100,
               dist < 0.35f ? "two-handed: the left hand will hold it" : "one-handed: the left hand stays free");
        two_rel_ok[wn] = 1;
        two_rel_near[wn] = dist < 0.35f;
    }
    bar_save();
}
static void two_hand_update(const VrInput* in, float ra[9], const float* R)
{
    float t[3], b[3], v[3], k[3], d, c, sg, q[9], m[9];
    int want = in->lgrip > 0.5f && in->grip_valid && in->gripl_valid && !knife_equipped() && !dual_equipped() && plp && plp->wpnr_no != 1 && plp->wpnr_no != 10 && plp->wpnr_no != 19 &&   /* vr199: not the lighter (weapon 1); vr200: nor the bow gun (weapons 10, 19) */
               !mag_held && !grip_latch && !(g_vr_manual_reload && near_holster());
    t[0] = in->gripl.px - in->grip.px; t[1] = in->gripl.py - in->grip.py; t[2] = in->gripl.pz - in->grip.pz;
    d = sqrtf(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
    {   /* vr198: the left grip held near the gun for 300 ms first */
        static uint64_t want_ms;
        uint64_t now = SDL_GetTicks();
        float near_cm = two_near_cm;
        two_near_cm = 1e9f;   /* measured again this frame (the left hand, after this) */
        /* vr201: near the gun itself - the left hand within 12 cm of where it holds it (vr196..200: the two
           controllers within 40 cm, which took the hand from almost anywhere in front) */
        if (two_on && !two_steer && near_cm < 1e8f && near_cm > two_grab_cm + 10.0f) {   /* vr212: the small guns let go once
                                                                                 the left hand is pulled 10 cm off (before: held on at any distance) */
            two_on = 0;
            vr_log("vr: gun back to one hand (left hand pulled %.0f cm off the gun)\n", near_cm - two_grab_cm);
        }
        if (!(want && near_cm < 12.0f)) want_ms = 0; else if (!want_ms) want_ms = now ? now : 1;
        if (!two_on && want_ms && now - want_ms >= 300) {
            two_on = 1; two_ms = now; two_item = equipped_item(); two_steer = big_gun(two_item); two_need_ref = 1; two_grab_cm = near_cm;
            vr_log("vr: gun held with both hands (left hand %.0f cm from the gun's hold, item %d weapon %d: %s)\n", near_cm, two_item, plp->wpnr_no,
                   two_steer ? "big gun - the left hand aims it too" : "the left hand only holds on, the right aims");
        }
    }
    if (two_on && (!want || (two_steer && d > 0.80f))) {   /* vr199: on the small guns the hand stays on whatever the controller does */ two_on = 0; vr_log("vr: gun back to one hand (%s, hands %.0f cm apart)\n", want ? "hands too far" : "left grip let go", d * 100); }
    if (!two_on || !two_steer || d < 0.05f) return;
    for (int i = 0; i < 3; i++) t[i] /= d;
    /* vr202: at the grab the line between the hands is kept in the right controller's frame (vr196..201 turned the barrel
       onto that line at once: the gun jumped 5..10 degrees, up when the hands were level); from then on the gun turns as
       that line turns - the left hand lifting or lowering it - and the right hand's roll stays */
    if (two_need_ref) {
        for (int j = 0; j < 3; j++) two_t0[j] = ra[0 * 3 + j] * t[0] + ra[1 * 3 + j] * t[1] + ra[2 * 3 + j] * t[2];
        two_need_ref = 0;
        vr_log("vr: both hands - the gun kept as it was at the grab, then turned by the left hand\n");
    }
    for (int i = 0; i < 3; i++) b[i] = ra[i * 3 + 0] * two_t0[0] + ra[i * 3 + 1] * two_t0[1] + ra[i * 3 + 2] * two_t0[2];
    (void)v; (void)sg; (void)R;
    d = sqrtf(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    if (d < 1e-4f) return;
    for (int i = 0; i < 3; i++) b[i] /= d;
    c = b[0] * t[0] + b[1] * t[1] + b[2] * t[2];
    if (c < -0.9f) return;   /* the left hand behind the gun */
    k[0] = b[1] * t[2] - b[2] * t[1]; k[1] = b[2] * t[0] - b[0] * t[2]; k[2] = b[0] * t[1] - b[1] * t[0];
    {   /* q = I + [k]x + [k]x^2 / (1 + c): the shortest turn from b to t */
        float K[9] = { 0, -k[2], k[1], k[2], 0, -k[0], -k[1], k[0], 0 }, f = 1.0f / (1.0f + c);
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                float kk = K[i * 3 + 0] * K[0 * 3 + j] + K[i * 3 + 1] * K[1 * 3 + j] + K[i * 3 + 2] * K[2 * 3 + j];
                q[i * 3 + j] = (i == j ? 1.0f : 0.0f) + K[i * 3 + j] + kk * f;
            }
    }
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m[i * 3 + j] = q[i * 3 + 0] * ra[0 * 3 + j] + q[i * 3 + 1] * ra[1 * 3 + j] + q[i * 3 + 2] * ra[2 * 3 + j];
    memcpy(ra, m, sizeof(m));
    memcpy(two_q, q, sizeof(q));
    memcpy(two_t, t, sizeof(t));
}
static void aim_rows(float r[9], const VrPose* p)
{
    float m[9];
    quat_to_rows(r, p);
    if (!two_on || !two_steer) return;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m[i * 3 + j] = two_q[i * 3 + 0] * r[0 * 3 + j] + two_q[i * 3 + 1] * r[1 * 3 + j] + two_q[i * 3 + 2] * r[2 * 3 + j];
    memcpy(r, m, sizeof(m));
}
static int barrel_ok[2];
static int barrel_dir(int side, float d[3])
{
    if (!barrel_ok[side]) return 0;
    memcpy(d, barrel_w[side], 3 * sizeof(float));
    return 1;
}

static float ray_capsule(const float o[3], const float d[3], const NJS_CAPSULE* c)
{
    float best = -1.0f;
    for (int i = 0; i <= 8; i++) {   /* 9 spheres along the capsule */
        float f = i / 8.0f, r = c->r + 0.3f;
        float cx = c->c1.x + (c->c2.x - c->c1.x) * f - o[0], cy = c->c1.y + (c->c2.y - c->c1.y) * f - o[1],
              cz = c->c1.z + (c->c2.z - c->c1.z) * f - o[2];
        float t = cx * d[0] + cy * d[1] + cz * d[2], q = cx * cx + cy * cy + cz * cz - t * t;
        if (t > 0 && q < r * r && (best < 0 || t < best)) best = t;
    }
    return best;
}

/* the aiming dot: where the gun in a hand would hit - the first wall or enemy along its
   barrel from its muzzle (as the shot goes, vr_gun_ray) */
static void sight_add(const float* H, int side, int wno)
{
    float d[3], o[3], dist = 1000.0f, len, p[3];
    float z = (wno >= 0 && wno < 23) ? WpnTab[wno].wp_fps1.z : 0.0f;
    NJS_LINE line;
    NJS_POINT3 apos;
    ATR_WORK* ahtp;
    if (wno == 13) {   /* vr220: the sniper rifle (the scope weapon, WpnTab flag 0x20) - no laser (vr222: weapon 20 back to a laser) */
        static unsigned seen;
        if (!(seen & (1u << (wno & 31)))) { seen |= 1u << (wno & 31); vr_log("vr: no laser on weapon %d (item %d)\n", wno, equipped_item()); }
        return;
    }
    if (!barrel_dir(side, d)) return;
    o[0] = muzzle_w[side][0]; o[1] = muzzle_w[side][1]; o[2] = muzzle_w[side][2];
    (void)z; (void)H;   /* from the muzzle (vr_hand) */
    line.px = o[0]; line.py = o[1]; line.pz = o[2];
    line.vx = d[0] * dist; line.vy = d[1] * dist; line.vz = d[2] * dist;
    apos = sys->apos; ahtp = sys->ahtp;
    if (bhCheckL2Wall(&line, 0x400, &len)) dist = len;
    sys->apos = apos; sys->ahtp = ahtp;
    for (int i = 0; i < sys->ewk_n; i++) {
        BH_PWORK* e = &ene[i];
        float t;
        if (!(e->flg & 0x1) || !(e->flg & 0x20) || (e->flg & 0x2) || (e->stflg & 0x1000000)) continue;
        t = ray_capsule(o, d, &e->watr);
        if (t > 0 && t < dist) dist = t;
    }
    for (int k = 0; k < 3; k++) p[k] = o[k] + d[k] * dist;
    if (wno == 11) { mk_add(VRMK_RING, p, 0); return; }   /* vr223: the shotgun - RE4's round reticle, no laser nor dot */
    if (g_vr_sight & 1) mk_add(VRMK_DOT, p, 0);
    if (g_vr_sight & 2) {   /* the beam from the muzzle; a muzzle point behind the hand (the mirrored left gun) is put as far in front of it */
        float v[3] = { o[0] - hand_pos[side][0], o[1] - hand_pos[side][1], o[2] - hand_pos[side][2] };
        float along = v[0] * d[0] + v[1] * d[1] + v[2] * d[2], l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        (void)l;
        if (along < 0.0f) for (int k = 0; k < 3; k++) o[k] -= 2.0f * along * d[k];   /* vr79: mirrored along the barrel, at the barrel's height */
        memcpy(laser_a[side], o, sizeof(o)); memcpy(laser_b[side], p, sizeof(p)); laser_ok[side] = 1;
    }
}

const char* pc_save_dir(void);   /* config.c */
static float grip_ref[9];
static int grip_ok, grip_loaded;
static void grip_path(char* out, size_t n) { snprintf(out, n, "%s/vr_grip.txt", pc_save_dir()); }
static void grip_save(void)
{
    char pth[512];
    FILE* f;
    grip_path(pth, sizeof(pth));
    if (!(f = fopen(pth, "w"))) return;
    for (int i = 0; i < 9; i++) fprintf(f, "%.6f\n", grip_ref[i]);
    fclose(f);
}
static void grip_load(void)
{
    /* vr213: fixed - the pose the owner's game learned and he confirmed (save/vr_grip.txt of vr212); no longer learned
       or read from the file, so every player gets the same */
    static const float fixed[9] = { 0.003280f, 0.999603f, 0.014959f, 0.029681f, 0.014863f, -0.999220f, -0.999326f, 0.003752f, -0.029635f };
    grip_loaded = 1;
    memcpy(grip_ref, fixed, sizeof(grip_ref));
    grip_ok = 1;
    vr_log("vr: hand pose fixed (built in)\n");
}

void vr_hand(BH_PWORK* p)
{
    static float rrel[2][2][64][9];          /* [hand][0 aim grip, 1 body grip][weapon] */
    static unsigned char rok[2][2][64];
    static float prev[2][9];
    static int still[2];
    float Rright[9];
    int right_ok = 0;
    static int hooked;
    VrInput in;
    VrPose h;
    float Wg[16], E[3], A[9], cy = cosf(vr_yaw), sy = sinf(vr_yaw);
    float ya, pa;
    int wn, aiming, ctl;
    hand_ok[0] = hand_ok[1] = 0;
    laser_ok[0] = laser_ok[1] = 0;
    box_mode = 0;
    barrel_ok[0] = barrel_ok[1] = 0;
    if (!vr_on() || !p || !p->mlwP) return;
    {   /* vr86: the last frame was not in the head (the cinema screen: a cut, a door): the hands stay hers, as the game poses them */
        static int was = -1;
        int head = SDL_GetTicks() - cam_ms < 100;   /* vr87: the VR camera ran lately (the cinema screen does not run it) */
        if (was != head && g_vr_debug_log) vr_log("vr-diag: hands %s\n", head ? "follow the controllers again" : "left on her (cinema screen)");
        was = head;
        if (!head) return;
    }
    if (!hooked) { hooked = 1; vr_log("vr: hand hook running (player model %d nodes)\n", p->mlwP->obj_num); }
    if (!grip_loaded) grip_load();
    log_hand_objects();
    wn = p->wpnr_no & 63;
    aiming = p->mode0 == 1 && p->mode1 == 1;
    ya = (float)(short)p->ay * (3.14159265f / 32768.0f);
    pa = (float)(short)p->wax * (3.14159265f / 32768.0f);
    ctl = xr_input(&in) && xr_head_pose(&h);
    if (p->mlwP->obj_num > 13) two_learn(p, wn);   /* vr198: before the hands are moved */
    if (ctl) { make_Wg(Wg); xform(Wg, h.px, h.py, h.pz, E); }
    for (int side = 0; side < 2; side++) {
        int node = side ? 13 : 9;
        float* H;
        const float* R;
        float ra[9], C[9], P[3];
        const VrPose* hp;
        float kick = 0;
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
        hand_aim[side] = *hp;   /* vr68: the pose this frame's hand was put with (the renderer follows the controller from it) */
        /* one hand pose for every weapon: the grip the right hand learned with the first gun it
           aimed (kept in <save>/vr_grip.txt); the left hand the same, mirrored */
        if (side == 0 && !grip_ok && rok[0][0][wn] && !knife_equipped() && !dual_equipped()) {
            memcpy(grip_ref, rrel[0][0][wn], sizeof(grip_ref));
            grip_ok = 1;
            grip_save();
            vr_log("vr: hand pose taken from weapon %d (one pose for every weapon, %s/vr_grip.txt)\n", wn, pc_save_dir());
        }
        R = rok[side][0][wn] ? rrel[side][0][wn] : rrel[side][1][wn];
        if (grip_ok) R = grip_ref;
        if (side == 1 && g_vr_left_mirror > 0 && (grip_ok || !dual_equipped()) && right_ok) {
            /* the left hand without a weapon: the right hand's grip, mirrored - F (left/right in
               the controller's frame) * Rright * B (the bone axis the skeleton mirrors) */
            static float Rm[9];
            int ax = g_vr_left_mirror - 1;
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++) Rm[i * 3 + j] = (i == 0 ? -1.0f : 1.0f) * Rright[i * 3 + j] * (j == ax ? -1.0f : 1.0f);
            R = Rm;
        }
        if (side == 0) { memcpy(Rright, R, sizeof(Rright)); right_ok = 1; }
        quat_to_rows(ra, hp);
        if (side == 0) two_hand_update(&in, ra, R);   /* vr196 */
        if (side == 0 && g_vr_recoil && shake_ms) {   /* vr205: the rifle's climb and sway, one-handed */
            float e = shake_decay(), a, b, ca, sa, cb, sb, tmp[9];
            if (e < 0.01f || (two_on && two_steer) || p->wpnr_no != RIFLE_WPN) { shake_ms = 0; shake_p = shake_y = 0; }
            else {
                a = shake_p * e * (3.14159265f / 180.0f); b = shake_y * e * (3.14159265f / 180.0f);
                ca = cosf(a); sa = sinf(a); cb = cosf(b); sb = sinf(b);
                for (int i = 0; i < 3; i++) {   /* ra = ra * Rx(a) (up, as the kick) */
                    tmp[i * 3 + 0] = ra[i * 3 + 0];
                    tmp[i * 3 + 1] = ra[i * 3 + 1] * ca + ra[i * 3 + 2] * sa;
                    tmp[i * 3 + 2] = -ra[i * 3 + 1] * sa + ra[i * 3 + 2] * ca;
                }
                for (int i = 0; i < 3; i++) {   /* ra = tmp * Ry(b) (to a side) */
                    ra[i * 3 + 0] = tmp[i * 3 + 0] * cb - tmp[i * 3 + 2] * sb;
                    ra[i * 3 + 1] = tmp[i * 3 + 1];
                    ra[i * 3 + 2] = tmp[i * 3 + 0] * sb + tmp[i * 3 + 2] * cb;
                }
            }
        }
        if (g_vr_recoil && recoil_ms[side]) {   /* the kick of a shot: up about the controller's x axis, back along it */
            float t, e;
            {   /* vr226: the kick's clock runs with the hand's frames, at most 1/30 s a frame - a stalled frame (the last
                   shot of the vr225 log: one frame, then the next 0.4 s later) no longer eats the whole kick */
                static uint64_t seen[2], prev[2];
                static float kt[2];
                uint64_t now = SDL_GetTicks();
                if (seen[side] != recoil_ms[side]) { seen[side] = recoil_ms[side]; kt[side] = 0; prev[side] = now; }
                else { float dt = (now - prev[side]) / 1000.0f; kt[side] += dt > 1.0f / 30.0f ? 1.0f / 30.0f : dt; prev[side] = now; }
                t = kt[side];
                e = t < 0.02f ? (t > 0 ? t / 0.02f : 0.5f) : expf(-(t - 0.02f) / recoil_tau[side]);   /* its first frame half up */
            }
            {   /* vr224 log: how much of each kick the drawn hand got (its peak), and in how many frames */
                static uint64_t shot_of[2];
                static float peak[2];
                static int frames[2], nlog;
                if (shot_of[side] != recoil_ms[side]) { shot_of[side] = recoil_ms[side]; peak[side] = 0; frames[side] = 0; }
                frames[side]++;
                if (t <= 0.4f && recoil_deg[side] * e > peak[side]) peak[side] = recoil_deg[side] * e;
                if (t > 0.4f && nlog++ < 300)
                    vr_log("vr: kick shown on the %s hand: peak %.1f of %.1f deg, over %d frames\n", side ? "left" : "right", peak[side], recoil_deg[side], frames[side]);
            }
            if (t > 0.4f) recoil_ms[side] = 0;
            else {
                float a = recoil_deg[side] * (3.14159265f / 180.0f) * e, c = cosf(a), sn = sinf(a), tmp[9];
                for (int i = 0; i < 3; i++) {   /* ra = ra * Rx(a) */
                    tmp[i * 3 + 0] = ra[i * 3 + 0];
                    tmp[i * 3 + 1] = ra[i * 3 + 1] * c + ra[i * 3 + 2] * sn;
                    tmp[i * 3 + 2] = -ra[i * 3 + 1] * sn + ra[i * 3 + 2] * c;
                }
                memcpy(ra, tmp, sizeof(tmp));
                kick = recoil_back[side] * e;   /* metres back */
            }
        }
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
            float o = g_vr_hand_offset / 100.0f - kick;
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
        {   /* the barrel as the gun is drawn: along the hand node's x axis (the guns' muzzle
               points, wp_fps1, lie along it: x the length, z the barrel's height), from the
               muzzle; the way the controller points picks its sense; vr_barrel_pitch tunes it */
            float f[3] = { -C[0 * 3 + 2], -C[1 * 3 + 2], -C[2 * 3 + 2] }, *d = barrel_w[side], l;
            const NJS_POINT3* m = (p->wpnr_no >= 0 && p->wpnr_no < 23) ? &WpnTab[p->wpnr_no].wp_fps1 : NULL;
            d[0] = H[0]; d[1] = H[1]; d[2] = H[2];
            l = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            barrel_ok[side] = l > 1e-4f && m;
            if (barrel_ok[side]) {
                float sg = (d[0] * f[0] + d[1] * f[1] + d[2] * f[2]) < 0 ? -1.0f : 1.0f;
                for (int k = 0; k < 3; k++) d[k] = d[k] / l * sg;
                if (side == 0 && !dual_equipped() && two_bl_ok[wn]) {   /* vr203: the weapon's own barrel, as the game draws it aimed */
                    const float* bl = two_bl[wn];
                    float e[3], el;
                    for (int k = 0; k < 3; k++) e[k] = H[0 * 4 + k] * bl[0] + H[1 * 4 + k] * bl[1] + H[2 * 4 + k] * bl[2];
                    el = sqrtf(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
                    if (el > 1e-6f) for (int k = 0; k < 3; k++) d[k] = e[k] / el;
                }
                if (g_vr_barrel_pitch) {   /* turned towards up */
                    float a = g_vr_barrel_pitch / 10.0f * (3.14159265f / 180.0f), u[3] = { -d[0] * d[1], 1.0f - d[1] * d[1], -d[2] * d[1] }, ul;
                    ul = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
                    if (ul > 1e-4f) for (int k = 0; k < 3; k++) d[k] = d[k] * cosf(a) + u[k] / ul * sinf(a);
                }
                for (int k = 0; k < 3; k++) muzzle_w[side][k] = H[0 * 4 + k] * m->x + H[1 * 4 + k] * m->y + H[2 * 4 + k] * m->z + H[12 + k];
            }
        }
        if (side == 1) {   /* vr201: how near the left hand (on its controller) is to the gun's hold - the game's left hand
                              in its aim pose, or the right hand's node before the gun was aimed with */
            float G0[16], sc = g_vr_scale / 10.0f, dx, dy, dz;
            const float* h9 = p->mlwP->owP[9].mtx;
            const float* tp = h9 + 12;
            if (two_rel_ok[wn] && (two_rel_near[wn] || big_gun(equipped_item()))) { m4_mul(G0, h9, two_rel[wn]); tp = G0 + 12; }
            dx = P[0] - tp[0]; dy = P[1] - tp[1]; dz = P[2] - tp[2];
            two_near_cm = sqrtf(dx * dx + dy * dy + dz * dz) / (sc > 0 ? sc : 1) * 100.0f;
            if (barrel_ok[0]) {   /* vr208: or anywhere along the gun, from the hand to the muzzle (the grenade launcher's hold
                                     is 31 cm out: before its aim pose was learned only the right hand counted - it never took) */
                float a[3] = { h9[12], h9[13], h9[14] }, ab[3], ap[3], t, l2, q[3], dd;
                for (int k = 0; k < 3; k++) { ab[k] = muzzle_w[0][k] - a[k]; ap[k] = P[k] - a[k]; }
                l2 = ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];
                t = l2 > 1e-9f ? (ap[0] * ab[0] + ap[1] * ab[1] + ap[2] * ab[2]) / l2 : 0;
                t = t < 0 ? 0 : t > 1 ? 1 : t;
                for (int k = 0; k < 3; k++) q[k] = P[k] - (a[k] + ab[k] * t);
                dd = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]) / (sc > 0 ? sc : 1) * 100.0f;
                if (dd < two_near_cm) two_near_cm = dd;
            }
            left_near_gun = two_near_cm < 12.0f;   /* vr228: the left grip here takes the gun, not the knife */
            {   /* vr208 log: the left grip pressed and the gun not taken - how far the hand was */
                static int was_held, logged;
                VrInput li;
                int held = xr_input(&li) && li.lgrip > 0.5f;
                if (held && !was_held) logged = 0;
                if (held && !two_on && !logged && SDL_GetTicks() - two_ms > 1000) {
                    static uint64_t since;
                    if (!since) since = SDL_GetTicks();
                    if (SDL_GetTicks() - since > 600) {
                        logged = 1; since = 0;
                        vr_log("vr: left grip held, gun not taken: the hand %.0f cm from the gun (needs 12), weapon %d\n", two_near_cm, p->wpnr_no);
                    }
                }
                was_held = held;
            }
        }
        {   /* vr213: a weapon with no fixed hold - the left hand kept where it took the gun (relative to the right hand's node) */
            static int grab_ok;
            float inv9[16];
            if (side == 1 && two_on && !two_rel_ok[wn] && !grab_ok && m4_inv(inv9, p->mlwP->owP[9].mtx)) {
                m4_mul(grab_rel, inv9, H);
                grab_ok = 1;
                vr_log("vr: weapon %d has no fixed hold - the left hand stays where it took it\n", wn);
            }
            if (!two_on) grab_ok = 0;
            grab_on = grab_ok;
        }
        if (side == 1 && two_on && !two_rel_ok[wn] && grab_on) {
            float G[16], w = two_w();
            m4_mul(G, p->mlwP->owP[9].mtx, grab_rel);
            for (int k = 0; k < 16; k++) H[k] = H[k] * (1 - w) + G[k] * w;
            P[0] = H[12]; P[1] = H[13]; P[2] = H[14];
        } else
        if (side == 1 && two_on && two_rel_ok[wn] && (two_rel_near[wn] || two_steer)) {   /* vr198: the left hand on the gun, as the game's aim pose has it (vr202: always on the big guns) */
            float G[16], w = two_w();
            m4_mul(G, p->mlwP->owP[9].mtx, two_rel[wn]);
            for (int k = 0; k < 16; k++) H[k] = H[k] * (1 - w) + G[k] * w;
            P[0] = H[12]; P[1] = H[13]; P[2] = H[14];
            if (!two_left_logged) { two_left_logged = 1; vr_log("vr: left hand put on weapon %d\n", wn); }
        } else if (side == 1) {
            if (two_on && !two_left_logged) {
                two_left_logged = 1;
                vr_log("vr: left hand not put on weapon %d (%s)\n", wn, two_rel_ok[wn] ? "one-handed in the game's aim" : "no aim pose seen yet for it");
            }
            if (!two_on) two_left_logged = 0;
        }
        memcpy(hand_pos[side], P, sizeof(P));
        if (side == 1 && mag_held) {   /* vr87: the ammo box in the left hand, from the holster to the gun */
            /* vr90: in the palm - along the controller (C: its axes in the world), 6 cm ahead of the hand point */
            float s = g_vr_scale / 10.0f, ctr[3];
            for (int k = 0; k < 3; k++) ctr[k] = P[k] - C[k * 3 + 2] * 0.06f * s;
            for (int c = 0; c < 8; c++) {
                float o[3] = { ((c & 1) ? 0.5f : -0.5f) * BOX_W * s, ((c & 2) ? 0.5f : -0.5f) * BOX_H * s, ((c & 4) ? 0.5f : -0.5f) * BOX_D * s };
                for (int k = 0; k < 3; k++) box_c[c][k] = C[k * 3 + 0] * o[0] + C[k * 3 + 1] * o[1] + C[k * 3 + 2] * o[2] + ctr[k];
            }
            box_mode = 2;
        }
        hand_ok[side] = 1;
        hand_log(side, 0, wn);
        if (side == 0) vr_slash(p, hp, H);
        if (g_vr_sight && aiming && !knife_equipped() && (side == 0 || dual_equipped())) {
            VrInput gin;   /* vr174: only while the right grip (aim) is held - letting go of the knife passes through the aim stance */
            if (xr_input(&gin) && gin.rgrip > 0.5f) sight_add(H, side, p->wpnr_no);
        }
    }
}

/* weapon.c bhCheckGunAtari(), first thing: a shot of the player's (the game builds it from the
   body: origin at the chest, direction Rx(wax) Ry(way)) leaves the muzzle of the weapon in the
   hand that fired it (the game's own muzzle point gx, on the hand node, which follows the
   controller) and goes where that controller points. Everything else about the shot (damage,
   walls, enemies, effects) is the game's. */
static unsigned diag_shots;
int vr_gun_ray(GA_WORK* gap)
{
    static int last = -1;
    NJS_POINT3 o, d;
    VrInput in;
    float r[9], f[3], c = cosf(vr_yaw), s = sinf(vr_yaw), l;
    int side, st;
    if (!gap || !plp || !vr_on()) return 0;
    if (gap->wpn_no != plp->wpnr_no || gap->ax != plp->wax || gap->ay != plp->way) return 0;   /* not the player's shot */
    diag_shots++;
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
    if (dual_equipped() && !hand_fires(side)) return 1;   /* twin guns: this hand's trigger is not pulled */
    shot_feedback(side, gap->wpn_no);
    if (!g_vr_hand_aim) return 0;
    st = 1;
    if (xr_input(&in) && (side ? in.aiml_valid : in.aim_valid) && hand_ok[side]) {
        const VrPose* hp = side ? &in.aiml : &in.aim;
        float bd[3];
        if (side) quat_to_rows(r, hp); else aim_rows(r, hp);
        f[0] = -r[2]; f[1] = -r[5]; f[2] = -r[8];   /* -z of the controller */
        d.x = c * f[0] + s * f[2]; d.y = f[1]; d.z = -s * f[0] + c * f[2];
        int along = barrel_dir(side, bd);
        if (along) { d.x = bd[0]; d.y = bd[1]; d.z = bd[2]; }   /* along the barrel in the hand */
        l = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        if (l > 1e-4f) {
            o.x = gap->gx; o.y = gap->gy; o.z = gap->gz;   /* the muzzle, on the tracked hand */
            if (along) { o.x = muzzle_w[side][0]; o.y = muzzle_w[side][1]; o.z = muzzle_w[side][2]; gap->gx = o.x; gap->gy = o.y; gap->gz = o.z; }
            gap->px = o.x; gap->py = o.y; gap->pz = o.z;
            if (g_vr_precise_hits && !(WpnTab[gap->wpn_no].flg & 0x80000000)) { gap->r = 0.3f; gap->rn = 0; gap->rmax = 0; }   /* a thin bullet (not the shotgun's spread) */
            gap->vx = d.x / l * gap->l; gap->vy = d.y / l * gap->l; gap->vz = d.z / l * gap->l;
            st = 0;
            {   /* vr210 log (the owner: shots drawn to the zombies): how near the line passes each zombie - a hit counted for one
                   it passed well clear of is the game's help, not the aim */
                static int nlog;
                float ro[3] = { o.x, o.y, o.z }, rd[3] = { d.x / l, d.y / l, d.z / l }, sc = g_vr_scale / 10.0f, best = 1e9f;
                int bi = -1;
                for (int i = 0; i < sys->ewk_n && nlog < 400; i++) {
                    BH_PWORK* e = &ene[i];
                    const NJS_CAPSULE* cp = &e->watr;
                    if (!(e->flg & 0x1) || !(e->flg & 0x20) || (e->flg & 0x2)) continue;
                    for (int k = 0; k <= 8; k++) {
                        float f = k / 8.0f, cx = cp->c1.x + (cp->c2.x - cp->c1.x) * f - ro[0], cy = cp->c1.y + (cp->c2.y - cp->c1.y) * f - ro[1],
                              cz = cp->c1.z + (cp->c2.z - cp->c1.z) * f - ro[2];
                        float t = cx * rd[0] + cy * rd[1] + cz * rd[2], q = cx * cx + cy * cy + cz * cz - t * t, dd;
                        if (t <= 0) continue;
                        dd = sqrtf(q > 0 ? q : 0) - cp->r;
                        if (dd < best) { best = dd; bi = i; }
                    }
                }
                if (bi >= 0 && nlog++ < 400)
                    vr_log("vr: shot line passes zombie %d %.0f cm %s its body\n", bi, fabsf(best) / (sc > 0 ? sc : 1) * 100.0f, best <= 0 ? "inside" : "clear of");
            }
        }
    }
    if (st * 2 + side != last) {
        last = st * 2 + side;
        vr_log(st ? "vr: shot from the game's aim (%s hand not tracked)\n" : "vr: shot from the %s hand's weapon\n", side ? "left" : "right");
    }
    return 0;
}

/* game.c, in place of the player's model while the camera is in the head: the body stays
   hidden; what is seen of the player are the hand/weapon objects linked to the hand nodes
   (drawn by the game's object pass from the tracked hand matrices) */
void vr_draw_hands(void* p) { (void)p; }

/* vr136: player.c, the bowgun's arrow (weapons 10 and 19, effect 130): it leaves the muzzle on the hand node but
   flew along the body's aim, Rx(wax) Ry(way). Its velocity v goes along the barrel in the right hand instead (as
   vr_gun_ray), same speed, and its angles ax/ay follow so it flies point first. 0 = left as the game's. */
int vr_fly_dir(NJS_POINT3* v, int* ax, int* ay, int wno)
{
    VrInput in;
    float r[9], d[3], bd[3], l, sp, c = cosf(vr_yaw), s = sinf(vr_yaw);
    if (!v || !plp || !vr_on() || !g_vr_hand_aim) return 0;
    diag_shots++;
    if (wno >= 15 && wno <= 17) shot_feedback(0, wno);   /* vr208: the grenade launcher's kick */
    if (!xr_input(&in) || !in.aim_valid || !hand_ok[0]) return 0;
    aim_rows(r, &in.aim);
    d[0] = c * -r[2] + s * -r[8]; d[1] = -r[5]; d[2] = -s * -r[2] + c * -r[8];   /* -z of the controller */
    if (barrel_dir(0, bd)) memcpy(d, bd, sizeof(d));
    l = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    sp = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (l < 1e-4f || sp < 1e-4f) return 0;
    if (g_vr_debug_log) {   /* log preventivo: the first arrow of each weapon */
        static unsigned seen;
        if (wno >= 0 && wno < 32 && !(seen & (1u << wno))) {
            float dot = (v->x * d[0] + v->y * d[1] + v->z * d[2]) / (l * sp);
            seen |= 1u << wno;
            vr_log("vr-diag: arrow: weapon %d, the game's way and the controller's %.1f deg apart (%s)\n", wno,
                   acosf(dot > 1 ? 1 : dot < -1 ? -1 : dot) * (180.0f / 3.14159265f), barrel_ok[0] ? "barrel" : "controller");
        }
    }
    v->x = d[0] / l * sp; v->y = d[1] / l * sp; v->z = d[2] / l * sp;
    *ay = (int)(atan2f(-d[0], -d[2]) * (32768.0f / 3.14159265f));
    *ax = (int)(atan2f(d[1], sqrtf(d[0] * d[0] + d[2] * d[2])) * (32768.0f / 3.14159265f));
    return 1;
}

/* the left stick (forward, right), -1..1; 0 if no controllers */
int vr_move_stick(float* f, float* r)
{
    VrInput in;
    if (!vr_on() || !xr_input(&in)) return 0;
    *f = in.ly; *r = in.lx;
    return 1;
}
int vr_run_held(void) { VrInput in; return xr_input(&in) && (in.ltrig > 0.5f || sprint_on); }

int vr_pad_select(void) { VrInput in; return xr_input(&in) && (in.buttons & VRB_MENU); }        /* vr133: left menu button: Select */
/* vr228: the quick knife back on the left grip (vr196..227: X) - but not when that grip is for something else: the left
   hand at the gun (both hands on it), holding the ammo box, or at the belt with ammo to take */
int vr_quick_knife(void)
{
    VrInput in;
    if (!xr_input(&in) || in.lgrip <= 0.5f || mag_held || grip_latch || two_on) return 0;
    if (g_vr_manual_reload && near_holster() && box_ammo() > 0) return 0;
    if (!knife_equipped() && left_near_gun) return 0;   /* (once the knife is out it stays while the grip is held) */
    if (!knife_equipped() && in.rgrip > 0.5f) return 0;   /* (vr228: not while aiming the gun - the right grip held) */
    return 1;
}

/* tps_after_draw(): the frame is drawn with `cam`; hand its camera to the renderer */
void vr_after_draw(void)
{
    VrView v;
    last_vr_frame = vr_frame;   /* vr86: whether this frame was in the head (vr_hand, next frame) */
    if (!vr_frame) { mk_n = 0; return; }
    memset(&v, 0, sizeof(v));
    v.valid = 1;
    memcpy(v.G, cam.mtx, sizeof(v.G));
    /* Ninja matrices are 3x4 in practice: the bottom row (m[3], m[7], m[11], m[15]) is never
       maintained (CALCPOINT does not read it) and is usually zero. Complete it, or G cannot be inverted. */
    v.G[3] = v.G[7] = v.G[11] = 0.0f; v.G[15] = 1.0f;
    memcpy(v.W, frame_W, sizeof(v.W));
    v.cx = fVu1OffsetX; v.cy = fVu1OffsetY;
    v.kx = fVu1AspectW * fVu1Projection; v.ky = fVu1AspectH * fVu1Projection;
    v.za = vr_za; v.zb = vr_zb;
    v.mk_n = mk_n;
    memcpy(v.mk, mk_pt, sizeof(v.mk)); memcpy(v.mk_kind, mk_kind, sizeof(v.mk_kind)); memcpy(v.mk_val, mk_val, sizeof(v.mk_val));
    for (int s = 0; s < 2; s++) { v.hand_ok[s] = hand_ok[s]; v.hand[s] = hand_aim[s]; }
    for (int s = 0; s < 2; s++) { v.laser_ok[s] = laser_ok[s]; memcpy(v.laser_a[s], laser_a[s], sizeof(v.laser_a[s])); memcpy(v.laser_b[s], laser_b[s], sizeof(v.laser_b[s])); }
    v.box_mode = box_mode; memcpy(v.box_c, box_c, sizeof(v.box_c));
    v.box_item = box_item;   /* vr131 */
    v.turn = smooth_turn;   /* vr120 */
    v.turn_at = turn_taken;   /* vr141 */
    v.box_amb[0] = rom ? rom->amb_r[(int)rom->amb_itm & 3] : 1.0f; v.box_amb[1] = rom ? rom->amb_g[(int)rom->amb_itm & 3] : 1.0f; v.box_amb[2] = rom ? rom->amb_b[(int)rom->amb_itm & 3] : 1.0f;
    {   /* VR diagnosis (vr_debug_log) */
        static int last = -1;
        if (g_vr_debug_log && box_mode != last) vr_log("vr-diag: ammo box %s (room light %.2f %.2f %.2f)\n", box_mode == 1 ? "at the holster" : box_mode == 2 ? "in the left hand" : "not shown", v.box_amb[0], v.box_amb[1], v.box_amb[2]);
        last = box_mode;
    }
    memcpy(v.body, frame_body, sizeof(v.body)); memcpy(v.follow, frame_follow, sizeof(v.follow));
    {   /* vr72: the stick's move, this frame and the last (a wall pushing back reverses it) */
        static float last_body[3], last_step[3];
        static int have;
        float pos[3] = { plp ? plp->px : 0.0f, plp ? plp->py : 0.0f, plp ? plp->pz : 0.0f };   /* vr73: the position, not the body's root (it sways with the walk) */
        float st[3] = { pos[0] - last_body[0] - frame_follow[0], pos[1] - last_body[1], pos[2] - last_body[2] - frame_follow[1] };
        if (!have) st[0] = st[1] = st[2] = 0.0f;
        memcpy(v.step, st, sizeof(st)); memcpy(v.step_prev, last_step, sizeof(last_step));
        memcpy(last_step, st, sizeof(st)); memcpy(last_body, pos, sizeof(last_body)); have = 1;
    }
    mk_n = 0;   /* as drawn (vr_camera), not the room's restored by tps_after_draw */
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

/* VR diagnosis (ini vr_debug_log = 1), called once per game frame (game.c): the state that decides
   what is drawn and how the weapons behave, one line whenever it changes, and each trigger pull with
   a gun: the weapon, its rounds, the reload state and whether a shot followed (vr_gun_ray). */
void vr_diag_frame(void)
{
    extern float BHD_ASPECT_Y;
    static char last[256];
    static int trig_was, wait = -1;
    static unsigned shots0;
    char b[256];
    VrInput in;
    if (!g_vr_debug_log || !sys || !plp) return;
    snprintf(b, sizeof(b), "room RM_%04d cam %s%s vr %s | fx: %s%s%s%s aspect %.2f | %s%s",
             sys->stg_no * 1000 + sys->rom_no * 10 + sys->rcase,
             (cam.flg & 0x2) ? "event" : "fixed", (cam.flg & 0x20) ? "+water" : "",
             vr_on() ? "head" : vr_session() ? "on (no head camera)" : "off",
             (sys->gm_flg & 0x100) ? "screen-distortion " : "",
             (sys->gm_flg & 0x200) ? "monitor " : "",
             sys->fade_an > 0 ? "fade " : "", sys->cine_an > 0 ? "cinema-bars " : "", BHD_ASPECT_Y,
             (sys->st_flg & 0x200) ? "message " : "", (sys->st_flg & 0x1) ? "event-running" : "play");
    if (strcmp(b, last)) { vr_log("vr-diag: %s\n", b); strcpy(last, b); }
    if (xr_input(&in)) {
        int t = in.rtrig > 0.5f;
        unsigned* g = gun_slot();
        if (t && !trig_was && plp->wpnr_no >= 0 && !knife_equipped()) {
            vr_log("vr-diag: trigger: weapon %d rounds %d box %d (slot %s, equipped slot %d item %d) mag_out %d mag_held %d empty-flag %d mode %d/%d/%d playing %d message %d\n",
                   plp->wpnr_no, g ? (int)(*g & 0xFFFF) : -1, box_ammo(), g ? "gun" : "none/endless", swork.pip ? (int)swork.pip[0] : -1, swork.pip && swork.pip[0] ? (int)((swork.pip[swork.pip[0]] >> 16) & 0x7FF) : -1, mag_out, mag_held, (sys->gm_flg & 0x40000) != 0,
                   plp->mode0, plp->mode1, plp->mode2, vr_playing(), (sys->st_flg & 0x200) != 0);
            shots0 = diag_shots; wait = 90;   /* vr210: 90 frames (the grenade launcher fires late in its motion) */
        }
        if (wait > 0 && --wait == 0) vr_log("vr-diag: trigger result: %u shot(s) in 90 frames\n", diag_shots - shots0);
        trig_was = t;
    }
    {   /* vr107 diagnosis: the inventory watch - each slot whose item or count changed since the last
           frame, and who changed it (our reload code names itself in inv_who; anything else is the
           game: a pickup, a scene, the item box, combining, its own reload) */
        static unsigned* last_p;
        static unsigned last_v[12];
        unsigned* p = swork.pip;
        if (p != last_p) {
            if (p) {
                char l[200];
                int n = 0;
                for (int i = 2; i < 12; i++) n += snprintf(l + n, sizeof(l) - n, " %u:%u", ITEM_OF(p[i]), p[i] & 0xFFFF);
                vr_log("vr-diag: inventory: now that of character %d (room RM_%04d, %s), item:count%s\n", sys->ply_id,
                       sys->stg_no * 1000 + sys->rom_no * 10 + sys->rcase, (sys->st_flg & 0x1) ? "event-running" : "play", l);
                for (int i = 0; i < 12; i++) last_v[i] = p[i];
            }
            last_p = p;
        } else if (p) {
            for (int i = 2; i < 12; i++)
                if (p[i] != last_v[i]) {
                    vr_log("vr-diag: inventory: slot %d item %u -> %u (flags %03x -> %03x), count %u -> %u - by %s (character %d, room RM_%04d, %s, weapon %d, mode %d)\n",
                           i, ITEM_OF(last_v[i]), ITEM_OF(p[i]), (last_v[i] >> 24) & 0xFFF, (p[i] >> 24) & 0xFFF, last_v[i] & 0xFFFF, p[i] & 0xFFFF,
                           inv_who ? inv_who : "the game", sys->ply_id, sys->stg_no * 1000 + sys->rom_no * 10 + sys->rcase,
                           (sys->st_flg & 0x1) ? "event-running" : (sys->st_flg & 0x200) ? "message" : "play", plp->wpnr_no, plp->mode2);
                    vw_inv(i, ITEM_OF(last_v[i]), ITEM_OF(p[i]), last_v[i] & 0xFFFF, p[i] & 0xFFFF, ITEM_OF(p[i]) < 156 ? (unsigned)itemdata[ITEM_OF(p[i])].type : 0,
                           inv_who, !(sys->st_flg & 0x201));   /* vr108 watch */
                    last_v[i] = p[i];
                }
            if (p[0] != last_v[0]) { vr_log("vr-diag: inventory: equipped slot %u -> %u\n", last_v[0], p[0]); last_v[0] = p[0]; }
        }
        inv_who = NULL;
    }
    {   /* vr108: the VR watch's frame (vr_watch.c) */
        VwFrame f;
        unsigned* g = gun_slot();
        float k = g_vr_scale / 10.0f;
        f.x = plp->px / k; f.z = plp->pz / k; f.ay = (unsigned short)plp->ay;
        f.m0 = plp->mode0; f.m1 = plp->mode1; f.m2 = plp->mode2;
        f.room = sys->stg_no * 1000 + sys->rom_no * 10 + sys->rcase; f.chr = sys->ply_id;
        f.event = (sys->st_flg & 0x1) != 0; f.message = (sys->st_flg & 0x200) != 0;
        f.gun_rounds = g ? (int)(*g & 0xFFFF) : -1;
        f.mag_pending = mag_out >= 0; f.mag_inv_ok = swork.pip == mag_inv;
        f.fire_vr = vw_fire_vr; f.fire_finger = vw_fire_finger;
        vw_frame(&f);
    }
}

/* sub1.c bhSearchBullet: in VR a gun is reloaded by hand (reload_update) - except when it is
   empty: then the trigger reloads it as in the game (a gun left empty with ammo in the boxes,
   e.g. the bow gun whose holster grab did not take, otherwise only clicked) */
int vr_block_autoreload(void)
{
    unsigned* g;
    if (!g_vr_manual_reload || !vr_on() || reloading) return 0;
    g = gun_slot();
    if (g && (*g & 0xFFFF) == 0 && g_vr_debug_log) {   /* vr101: empty too - it only clicks; B, the holster and the gun reload it */
        static uint64_t last;
        if (SDL_GetTicks() > last + 1000) { last = SDL_GetTicks(); vr_log("vr-diag: empty gun: the game's own reload blocked (box ammo %d) - B and the holster\n", box_ammo()); }
    }
    return 1;
}

/* vr210: effsub1.c bhEff013 - the bow gun's bolt turned towards the nearest enemy in flight (bhSearchNearEnemy, then a
   steer each frame): the owner's "shots drawn to the zombie". In VR it flies straight from the barrel. */
int vr_no_homing(void) { return vr_on(); }

/* vr210 log: effsub1.c bhEff130 (grenades) once it hit: how far it went sideways of the way it was fired (ay) - the
   game moves it straight with gravity only; this checks no turn shows */
void vr_grenade_end(int ay, float sx, float sz, float ex, float ez, int type)
{
    static float lx = 1e9f, lz;
    static int nlog;
    float dx = ex - sx, dz = ez - sz, d = sqrtf(dx * dx + dz * dz), a = (short)ay * (3.14159265f / 32768.0f), fx = -sinf(a), fz = -cosf(a), sc = g_vr_scale / 10.0f;
    float side, along;
    if (!vr_on() || (sx == lx && sz == lz) || nlog >= 100 || d < 1.0f) return;
    lx = sx; lz = sz; nlog++;
    side = dx * fz - dz * fx; along = dx * fx + dz * fz;
    vr_log("vr: grenade (type %d) flew %.0f cm, %.0f cm to the side of its firing line (%.1f deg)\n", type, along / (sc > 0 ? sc : 1) * 100.0f,
           side / (sc > 0 ? sc : 1) * 100.0f, atan2f(side, along) * (180.0f / 3.14159265f));
}

/* vr214: player.c, after vr_fly_dir: the bolt / grenade leaves the muzzle of the gun in the VR hand - where the sight's
   line starts - not the point the game took from the hand node before the hands were put this frame (the bolt was seen
   leaving off the sight, a little to the left) */
void vr_fly_from(float* p, int wno)
{
    static unsigned seen;
    float sc = g_vr_scale / 10.0f, dx, dy, dz;
    if (!p || !vr_on() || !barrel_ok[0]) return;
    dx = muzzle_w[0][0] - p[0]; dy = muzzle_w[0][1] - p[1]; dz = muzzle_w[0][2] - p[2];
    if (wno >= 0 && wno < 32 && !(seen & (1u << wno))) {
        seen |= 1u << wno;
        vr_log("vr: weapon %d shell/bolt put at the muzzle: the game's start was %.1f cm off it\n", wno, sqrtf(dx * dx + dy * dy + dz * dz) / (sc > 0 ? sc : 1) * 100.0f);
    }
    p[0] = muzzle_w[0][0]; p[1] = muzzle_w[0][1]; p[2] = muzzle_w[0][2];
}

/* vr215: en01.c bhEne01_SetBlood - a shot in the head (joint 11, whose damage is the head's count) bleeds from the head,
   as the body does from where it is hit; the game put that blood at the chest (object 9, 2.5 up) */
int vr_head_joint(int djnt) { return vr_on() && djnt == 11; }

/* vr216: en01.c bhEne01_DamageCheck - the head's burst (its effect and its sound, 4869) runs only for a weapon whose damage
   table allows it (flag 0x10): not the handguns. A critical (vr215: a head shot that kills) lets it run - the damage is
   the shot's own, the burst only shows it */
unsigned char vr_crit_mark[64];
int vr_crit_take(BH_PWORK* epw)
{
    int ix = (int)(epw - ene);
    if (!vr_on() || ix < 0 || ix >= 64 || !vr_crit_mark[ix]) return 0;
    vr_crit_mark[ix] = 0;
    vr_log("vr: critical - zombie %d's head burst (the game's effect and sound)\n", ix);
    return 1;
}

/* vr216: the head's blood where the shot hit it: the hit point (dpx..) in the head joint's frame (vr215 put it at the
   joint's origin - the neck) */
void vr_head_ofs(BH_PWORK* epp, BH_PWORK* epw, NJS_POINT3* o)
{
    float inv[16];
    const float* m = epp->mlwP->owP[11].mtx;
    o->x = o->y = o->z = 0;
    if (!m4_inv(inv, m)) return;
    o->x = inv[0] * epw->dpx + inv[4] * epw->dpy + inv[8] * epw->dpz + inv[12];
    o->y = inv[1] * epw->dpx + inv[5] * epw->dpy + inv[9] * epw->dpz + inv[13];
    o->z = inv[2] * epw->dpx + inv[6] * epw->dpy + inv[10] * epw->dpz + inv[14];
}

/* vr220: playpch2.c bhCPM2_act_scp (the sniper rifle's scope): the game's own zoom (held: 65 -> 20 degrees, cam.ppers)
   on the right stick - forward zooms in, back out; each scope starts out, as the game's */
int vr_scope_zoom(void)
{
    static int zoomed;
    static uint64_t last;
    VrInput in;
    uint64_t now = SDL_GetTicks();
    if (now - last > 500) { zoomed = 0; vr_log("vr: scope on (zoom: right stick forward / back)\n"); }
    last = now;
    if (!vr_on() || !xr_input(&in)) return 0;
    if (in.ry > 0.7f && !zoomed) { zoomed = 1; vr_log("vr: scope zoom in\n"); }
    else if (in.ry < -0.7f && zoomed) { zoomed = 0; vr_log("vr: scope zoom out\n"); }
    return zoomed;
}
