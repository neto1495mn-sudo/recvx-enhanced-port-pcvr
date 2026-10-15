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

int g_vr_scale = 100;        /* ini vr_scale: game units per metre, in tenths (100 = 10 units/m: the character is ~17 tall) */
int g_vr_eye_height = 155;   /* ini vr_eye_height: the camera's height above the player's feet, tenths of units */
int g_vr_fov = 120;          /* ini vr_fov: the game camera's horizontal field of view (degrees): what it culls against */
int g_vr_turn = 30;          /* ini vr_turn: right-stick turn in degrees per flick; 0 = smooth turning */
int g_vr_gun_min = 15;       /* ini vr_gun_min: the hand/weapon never comes closer than this to the eyes (cm) */
int g_vr_slash_speed = 180;  /* ini vr_slash_speed: the knife cuts when the right hand moves faster than this (cm/s) */
extern int g_vr_danger_fx;    /* gs_gl.c, ini vr_danger_fx */
int g_vr_hit_tilt = 5;       /* ini vr_hit_tilt: the view tilts this many degrees away from a hit; 0 = off */
int g_vr_left_mirror = 1;    /* ini vr_left_mirror: the left hand without a weapon holds its controller as the right does, mirrored (1/2/3: the bone axis mirrored x/y/z; 0 = the game's own pose) */
int g_vr_dmg_head = 125, g_vr_dmg_body = 75, g_vr_dmg_body_max = 100, g_vr_dmg_arm = 50, g_vr_dmg_leg = 50;   /* ini vr_dmg_*: damage by the body part hit (zombies), percent; the body's between _body and _body_max */
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

static int vr_frame;         /* the VR camera set this frame's view (until vr_after_draw) */
static int vr_active;        /* the VR camera drew the last frame */
static float frame_W[16];    /* this frame's local -> world */
static float frame_Wg[16];   /* this frame's recentred tracking space -> world */
static float vr_yaw;         /* the view's heading (radians, forward (-sin, 0, -cos)) */
static unsigned rec_seen;    /* xr_recenter_gen at the last heading reset */
static int snap_held;        /* the right stick is still out after a snap turn */
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
static int hand_ok[2];              /* ... and whether it was */

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

/* the holster: in front of the belly at the belt (about 62 cm below the eyes, 15 cm forward of
   them), turned with the head's heading smoothed - the box ammo of the gun in hand */
static void holster_update(const float Wg[16], const VrPose* h)
{
    static float yaw_s;
    static int have;
    float hr[9], fx, fz, yaw, pl[3], pw[3];
    int n;
    holster_ok = 0;
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
    if (g_vr_holster) mk_add(VRMK_NUMBER, pw, n > 999 ? 999 : n);
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
static int body_move(float dx, float dz)
{
    if (body_free(dx, dz)) { body_shift(dx, dz); return 3; }
    if (body_free(dx, 0)) { body_shift(dx, 0); return 1; }
    if (body_free(0, dz)) { body_shift(0, dz); return 2; }
    return 0;
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

/* Walking while aiming: the game holds Claire still with a weapon raised; in VR the left stick
   moves her where the head looks at the speed she walks (runs, with the run trigger) - measured
   from her own walking (not stairs, not the room's walking) - through the game's walls. */
static float walk_spd = 0.55f, run_spd = 1.1f;   /* units per game frame, until measured */
static float last_px, last_pz;
static int last_ok;

static void aim_walk_update(void)
{
    VrInput in;
    float dx = plp->px - last_px, dz = plp->pz - last_pz, d = sqrtf(dx * dx + dz * dz);
    int stick, run;
    if (!xr_input(&in)) { last_ok = 0; return; }
    stick = in.lx * in.lx + in.ly * in.ly > 0.36f;
    run = in.ltrig > 0.5f || (in.buttons & VRB_LSTICK);
    if (last_ok && stick && plp->mode0 == 1 && plp->mode1 == 0 && plp->mode2 != 14 && plp->mode2 != 15 && d > 0.05f && d < 5.0f) {
        if (run) run_spd += (d - run_spd) * 0.05f;   /* the game's own steps, averaged */
        else walk_spd += (d - walk_spd) * 0.05f;
    }
    {   /* diagnosis (vr: aim walk), twice a second while aiming with the stick out */
        static int n, nlog;
        if (stick && plp->mode1 == 1 && (n++ % 15) == 0 && nlog < 200) {
            nlog++;
            vr_log("vr: aim walk: mode %d/%d/%d msg %d stick %.2f %.2f speed %.2f/%.2f at %.1f %.1f, %.2f since last frame\n",
                   plp->mode0, plp->mode1, plp->mode2, (sys->st_flg & 0x200) != 0, in.lx, in.ly, walk_spd, run_spd, plp->px, plp->pz, last_ok ? d : -1.0f);
        }
    }
    if (g_vr_aim_walk && stick && plp->mode0 == 1 && plp->mode1 == 1 && !(sys->st_flg & 0x200)) {
        float a = (float)cam.ay * (3.14159265f / 32768.0f), fx = sinf(a), fz = -cosf(a), rx = cosf(a), rz = sinf(a);
        float m = sqrtf(in.lx * in.lx + in.ly * in.ly), mx, mz, sp = (run && !dual_equipped()) ? run_spd : walk_spd;
        if (m > 1.0f) m = 1.0f;
        mx = (fx * in.ly + rx * in.lx); mz = (fz * in.ly + rz * in.lx);
        d = sqrtf(mx * mx + mz * mz);
        if (d > 1e-4f) {
            mx = mx / d * sp * m; mz = mz / d * sp * m;
            {
                static int n2, nlog2;
                int how = body_move(mx, mz);
                if ((n2++ % 15) == 0 && nlog2 < 200) { nlog2++; vr_log("vr: aim walk: step %.2f %.2f %s\n", mx, mz, how == 3 ? "moved" : how ? "slid along a wall" : "blocked by a wall"); }
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
int vr_block_autoreload(void) { return g_vr_manual_reload && vr_on() && !reloading; }   /* sub1.c bhSearchBullet */
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
    if (!g_vr_manual_reload || !xr_input(&in)) return;
    b = (in.buttons & VRB_B) != 0; gr = in.lgrip > 0.5f;
    g = gun_slot();
    if (mag_out >= 0 && (!g || (int)(g - swork.pip) != mag_out)) {   /* the gun changed: its rounds go back */
        if (swork.pip && mag_out > 0) { swork.pip[mag_out] = (swork.pip[mag_out] & 0xFFFF0000u) | (unsigned)mag_kept; }
        mag_out = -1; mag_kept = 0;
    }
    if (g && b && !b_was && mag_out < 0) {   /* B: the magazine out */
        mag_out = (int)(g - swork.pip);
        mag_kept = (int)(*g & 0xFFFF);
        *g &= 0xFFFF0000u;
        CallPlayerWeaponSeEx((NJS_POINT3*)&plp->mlwP->owP[9].mtx[12], 263, 0);
        if (g_vr_haptics) xr_haptic(0, 0.4f, 40);
        vr_log("vr: magazine out (%d rounds kept)\n", mag_kept);
    }
    if (gr && !g_was && near_holster() && g && box_ammo() > 0) {   /* a magazine from the holster */
        mag_held = 1;
        if (g_vr_haptics) xr_haptic(1, 0.5f, 40);
    }
    if (!gr) mag_held = 0;
    if (mag_held && g && hand_ok[0] && hand_ok[1] && dist3(hand_pos[0], hand_pos[1]) < 2.0f * (g_vr_scale / 100.0f) &&
        (mag_out >= 0 || (*g & 0xFFFF) == 0)) {   /* the left hand at the gun: in */
        if (mag_out >= 0) { *g = (*g & 0xFFFF0000u) | (unsigned)mag_kept; mag_out = -1; mag_kept = 0; }
        reloading = 1;
        bhSearchBullet();
        reloading = 0;
        g = gun_slot();   /* (the game sorts the inventory after a reload) */
        if (g && (*g & 0xFFFF)) sys->gm_flg &= ~0x40000;
        mag_held = 0;
        CallPlayerWeaponSeEx((NJS_POINT3*)&plp->mlwP->owP[9].mtx[12], 264, 0);
        if (g_vr_haptics) { xr_haptic(0, 0.6f, 50); xr_haptic(1, 0.6f, 50); }
        vr_log("vr: reloaded (%d rounds)\n", g ? (int)(*g & 0xFFFF) : 0);
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
    int etc_n;
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
    mk_add(VRMK_BUTTON, p, 0);
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
        if (why) { extern volatile int g_vr_danger; g_vr_danger = 0; vr_active = 0; mk_n = 0; return; }
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
    aim_walk_update();
    body_follow(&h);
    last_px = plp->px; last_pz = plp->pz; last_ok = 1;
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
    cam_gen++;   /* the next game frame computes its base anew */
}

int vr_on(void) { return vr_active && xr_stereo_ready(); }


/* in play: the VR camera ran in the last moments - in menus, the inventory, the map and the
   title it does not run (tps_pad, pad.c) */
void vr_pad_frame(void) {}
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

/* the trigger of a hand (0 right, 1 left) is pulled, or was within the last 150 ms (the game
   fires the left gun two frames after the right) */
static int trigger_recent(int side)
{
    static uint64_t last_ms[2];
    VrInput in;
    uint64_t now = SDL_GetTicks();
    if (xr_input(&in) && (side ? in.ltrig : in.rtrig) > 0.5f) last_ms[side] = now ? now : 1;
    return last_ms[side] && now - last_ms[side] < 150;
}

/* pwksub.c bhSetGunFire / bhSetYakkyou: with twin guns in VR each hand fires with its own
   trigger - no muzzle flash or shell from the hand whose trigger is not pulled */
int vr_dual_block(void* pp, int jno)
{
    if (pp != (void*)plp || !vr_on() || !dual_equipped() || (jno != 9 && jno != 13)) return 0;
    return !trigger_recent(jno == 13);
}

/* recoil and vibration of a shot from a hand */
static uint64_t recoil_ms[2];
static float recoil_k[2];
static void shot_feedback(int side, int wno)
{
    float k = (wno >= 0 && wno < 22) ? EneDamNear[1][wno] / 60.0f : 0.5f;   /* by the weapon's power (zombie damage) */
    k = k < 0.35f ? 0.35f : k > 1.5f ? 1.5f : k;
    recoil_ms[side] = SDL_GetTicks() | 1;
    recoil_k[side] = k;
    if (g_vr_haptics) xr_haptic(side, k > 1.0f ? 1.0f : 0.3f + 0.6f * k, k > 1.0f ? 90 : 50);
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
    dx = pp->px - plp->px; dz = pp->pz - plp->pz;
    if (dx * dx + dz * dz < 30.0f * 30.0f * (g_vr_scale / 100.0f) * (g_vr_scale / 100.0f)) return 0;
    {
        static int nlog;
        if (nlog++ < 100) vr_log("vr: shot at a sleeping zombie (enemy %d, flg %x, stflg %x, mode %d/%d/%d)\n", (int)(pp - ene), pp->flg, pp->stflg, pp->mode0, pp->mode1, pp->mode2);
    }
    return 1;
}

/* weapon.c bhCheckGunAtari, after the damage of a player's hit is added (and the knife's):
   in VR, scaled by the part of a zombie's body hit - the hit point (dpx..) against the enemy:
   the top of it the head, the lower part the legs, away from the body capsule's axis the arms */
int vr_part_damage(BH_PWORK* pp, int kno, int d0, int d1)
{
    static const char* names[] = { "body", "head", "arm", "leg" };
    float top = 0, h, rel, mult;
    int part = 0, add = d1 - d0, out;
    (void)kno;
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
    out = (int)(add * mult + 0.5f);
    if (out < 1) out = 1;
    { static int nlog; if (nlog++ < 300) vr_log("vr: hit zombie %s x%.2f (%d -> %d)\n", names[part], mult, add, out); }
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
    {
        int dual_aim = vr_playing() && dual_equipped() && in.rgrip > 0.5f;   /* twin guns raised: the left trigger fires the left gun */
        if ((in.ltrig > 0.5f && !dual_aim) || (in.buttons & VRB_LSTICK)) k |= 16;   /* TPS_K_RUN */
        if (dual_aim && (in.ltrig > 0.5f || in.rtrig > 0.5f)) k |= 512;           /* TPS_K_FIRE: either gun */
    }
    if (in.buttons & VRB_A) k |= 32;     /* TPS_K_ACTION */
    if ((in.buttons & VRB_Y) && in.ltrig <= 0.5f) k |= 64;   /* TPS_K_STATUS (LT + Y is Select, vr_pad_select) */
    if (in.buttons & VRB_X) k |= 128;    /* TPS_K_MAP */
    if (in.rgrip > 0.5f) k |= 256;       /* TPS_K_AIM */
    if (in.rtrig > 0.5f && !(vr_playing() && knife_equipped() && g_vr_slash_speed > 0)) k |= 512;   /* TPS_K_FIRE (the knife cuts by swinging) */
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
        if (g_vr_haptics) xr_haptic(0, 0.5f, 40);
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
    mk_add(VRMK_DOT, p, 0);
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
    char pth[512];
    FILE* f;
    int n = 0;
    grip_loaded = 1;
    grip_path(pth, sizeof(pth));
    if (!(f = fopen(pth, "r"))) return;
    while (n < 9 && fscanf(f, "%f", &grip_ref[n]) == 1) n++;
    fclose(f);
    grip_ok = n == 9;
    if (grip_ok) vr_log("vr: hand pose loaded (%s)\n", pth);
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
    barrel_ok[0] = barrel_ok[1] = 0;
    if (!vr_on() || !p || !p->mlwP) return;
    if (!hooked) { hooked = 1; vr_log("vr: hand hook running (player model %d nodes)\n", p->mlwP->obj_num); }
    if (!grip_loaded) grip_load();
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
        if (g_vr_recoil && recoil_ms[side]) {   /* the kick of a shot: up about the controller's x axis, back along it */
            float t = (SDL_GetTicks() - recoil_ms[side]) / 1000.0f, e = t < 0.02f ? t / 0.02f : expf(-(t - 0.02f) / 0.07f);
            if (t > 0.4f) recoil_ms[side] = 0;
            else {
                float a = 9.0f * (3.14159265f / 180.0f) * recoil_k[side] * e, c = cosf(a), sn = sinf(a), tmp[9];
                for (int i = 0; i < 3; i++) {   /* ra = ra * Rx(a) */
                    tmp[i * 3 + 0] = ra[i * 3 + 0];
                    tmp[i * 3 + 1] = ra[i * 3 + 1] * c + ra[i * 3 + 2] * sn;
                    tmp[i * 3 + 2] = -ra[i * 3 + 1] * sn + ra[i * 3 + 2] * c;
                }
                memcpy(ra, tmp, sizeof(tmp));
                kick = 0.03f * recoil_k[side] * e;   /* metres back */
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
                if (g_vr_barrel_pitch) {   /* turned towards up */
                    float a = g_vr_barrel_pitch / 10.0f * (3.14159265f / 180.0f), u[3] = { -d[0] * d[1], 1.0f - d[1] * d[1], -d[2] * d[1] }, ul;
                    ul = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
                    if (ul > 1e-4f) for (int k = 0; k < 3; k++) d[k] = d[k] * cosf(a) + u[k] / ul * sinf(a);
                }
                for (int k = 0; k < 3; k++) muzzle_w[side][k] = H[0 * 4 + k] * m->x + H[1 * 4 + k] * m->y + H[2 * 4 + k] * m->z + H[12 + k];
            }
        }
        memcpy(hand_pos[side], P, sizeof(P));
        hand_ok[side] = 1;
        hand_log(side, 0, wn);
        if (side == 0) vr_slash(p, hp, H);
        if (g_vr_sight && aiming && !knife_equipped() && (side == 0 || dual_equipped())) sight_add(H, side, p->wpnr_no);
    }
}

/* weapon.c bhCheckGunAtari(), first thing: a shot of the player's (the game builds it from the
   body: origin at the chest, direction Rx(wax) Ry(way)) leaves the muzzle of the weapon in the
   hand that fired it (the game's own muzzle point gx, on the hand node, which follows the
   controller) and goes where that controller points. Everything else about the shot (damage,
   walls, enemies, effects) is the game's. */
int vr_gun_ray(GA_WORK* gap)
{
    static int last = -1;
    NJS_POINT3 o, d;
    VrInput in;
    float r[9], f[3], c = cosf(vr_yaw), s = sinf(vr_yaw), l;
    int side, st;
    if (!gap || !plp || !vr_on()) return 0;
    if (gap->wpn_no != plp->wpnr_no || gap->ax != plp->wax || gap->ay != plp->way) return 0;   /* not the player's shot */
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
    if (dual_equipped() && !trigger_recent(side)) return 1;   /* twin guns: this hand's trigger is not pulled */
    shot_feedback(side, gap->wpn_no);
    if (!g_vr_hand_aim) return 0;
    st = 1;
    if (xr_input(&in) && (side ? in.aiml_valid : in.aim_valid) && hand_ok[side]) {
        const VrPose* hp = side ? &in.aiml : &in.aim;
        float bd[3];
        quat_to_rows(r, hp);
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
int vr_quick_knife(void) { VrInput in; return xr_input(&in) && in.lgrip > 0.5f && !mag_held && !(g_vr_manual_reload && near_holster()); }             /* left grip: quick knife */

/* tps_after_draw(): the frame is drawn with `cam`; hand its camera to the renderer */
void vr_after_draw(void)
{
    VrView v;
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
