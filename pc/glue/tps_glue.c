/* Third-person mode (docs/THIRDPERSON.md): an over-the-shoulder camera that replaces the
   room's camera cuts during normal play. The cut system keeps running underneath (doors,
   events, lights and fog per cut); only the view written into `cam` before
   bhControlCamera() is ours. All hooks run on the game thread and only while tps_on. */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaView.h"
#include "../../decomp/include/ps2/veronica/prog/pwksub.h"
#include "../../decomp/include/ps2/veronica/prog/player.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../decomp/include/ps2/veronica/prog/light.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NinjaCnk.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaFog.h"
#include "../src/host/tps.h"

TpsConfig g_tps = { .fov = 75, .aim_fov = 65, .dist = 140, .aim_dist = 100, .height = 165, .shoulder = 35,
                    .sens_x = 100, .sens_y = 100, .aim_sens = 70, .crosshair = 1, .atmos = 1, .key = 62 /* F5 */ };
int tps_on;            /* hooks active (changes only at the frame hand-off) */
int tps_cam_active;    /* our camera is the view of the current frame */

extern float fNaViwClipNear, fNaViwClipFar;
void bhCalcModel(BH_PWORK* ewP);   /* MdlPut.h */
void actor_aim_offset(const BH_PWORK* p, float* out);   /* actor_glue.c */
int actor_aim_hand(const BH_PWORK* p, float* hand);
float actor_cam_scale(void);
uint64_t gs_frame_count(void); /* the presented frame (RECVX_INPUT, frame dumps) */

static float yaw, pitch;          /* camera, game angle units (65536 = 360 deg); pitch > 0 looks up */
static float mouse_dx, mouse_dy;  /* this frame's mouse motion, pixels */
static int mouse_moved;           /* the mouse moved this frame (pc_input_active) */
static float clip_near, clip_far; /* the game's clip range while we draw with ours */
static int trace;                 /* RECVX_TPS_TRACE=1: log the free aim */
static NJS_POINT3 aim_t;          /* free aim: the crosshair target (tps_calc_model) */
static int aim_h;                 /* free aim: the aim height bit given to the game (at_flg 4 up, 8 down), 0: none */
static float bend_a;   /* the last free-aim bend (radians per node), 0: none */
static float aim_corr;            /* free aim with a model: the drawn weapon's pitch minus the hand's (radians), eased */
static int no_cam_walls;          /* test aid (RECVX_TPS_ORBIT): the eye goes through walls */
static unsigned move_keys;        /* camera A (g_tps.free_cam): the WASD keys held, walked relative to the camera */
static int pad_steer;             /* the game rows (arrows, gamepad) press a direction: tank controls, the camera follows */

#define DEG(a) ((a) * (65536.0f / 360.0f))
#define TURN_PER_PIXEL DEG(0.12f)   /* mouse at 100% sensitivity */
#define RAD(a) ((a) * (3.14159265f / 32768.0f))

static float wrap(float a)
{
    a = fmodf(a, 65536.0f);
    return a > 32768.0f ? a - 65536.0f : a < -32768.0f ? a + 65536.0f : a;
}

/* normal play: no event or fixed camera, first person, scope, door transition or event.
   Not sp_flg 0x40 (the cut camera may move): messages, examine texts, pick-up prompts, the
   fade to/from the inventory, map and options clear it, which only freezes the cut camera
   (bhCheckCut does nothing) - the view stays ours, so we stay active and hold still (frozen);
   dropping out there showed our view with the cut's hidden room parts and clip range. */
static int camera_allowed(void)
{
    return sys && plp && plp->mlwP && !(sys->st_flg & 0x800001) &&
           !(sys->gm_flg & 0x49) && !(sys->cb_flg & 0x5) && plp->mode0 != 7;
}

static void atmos_off(void);

/* player.c bhControlPlayer, before the mode handler (and when third person stops): the weapon
   modes pick motions by the aim height, so they keep the straight one free aim plays */
void tps_player_mode(void)
{
    if (aim_h && plp && (plp->at_flg & 0xE) == aim_h) plp->at_flg = (plp->at_flg & ~0xE) | 2;
    aim_h = 0;
}

static void deactivate(void)
{
    atmos_off();
    tps_player_mode();
    if (!tps_cam_active) return;
    tps_cam_active = g_tps.cam_active = g_tps.aiming = 0;
    /* the room camera resumes: the game's own re-initialisation of the current cut */
    if (sys && !(sys->st_flg & 0x1)) sys->gm_flg |= 0x800;
}

static int aiming(void) { return tps_cam_active && plp->mode0 == 1 && plp->mode1 == 1; }

/* VR (vr_glue.c): the head's heading/pitch as this camera's, walking relative to it (camera A) */
int vr_on(void);
int vr_hand_ray(NJS_POINT3* o, NJS_POINT3* d);
void tps_vr_view(float y, float p) { yaw = wrap(y); pitch = p; }
void vr_hand(BH_PWORK* p);

int vr_playing(void);
void vr_pad_frame(void);
#define FREE_CAM (g_tps.free_cam || vr_playing())   /* VR: in play only - in menus the stick is the d-pad */

static float cur_dist, cur_fov;   /* eased towards the walk/aim values */

/* b moved towards a so that it stays 1.5 units in front of the first wall between them (the
   game's bullet ray test; its hit globals are preserved) */
static void clear_of_walls(const NJS_POINT3* a, NJS_POINT3* b)
{
    NJS_LINE line;
    NJS_POINT3 apos = sys->apos;
    ATR_WORK* ahtp = sys->ahtp;
    float total, len;
    line.px = a->x; line.py = a->y; line.pz = a->z;
    line.vx = b->x - a->x; line.vy = b->y - a->y; line.vz = b->z - a->z;
    total = sqrtf(line.vx * line.vx + line.vy * line.vy + line.vz * line.vz);
    if (total > 0.01f && bhCheckL2Wall(&line, 0x400, &len)) {
        float k = (len - 1.5f) / total;
        if (k < 0) k = 0;
        b->x = a->x + line.vx * k; b->y = a->y + line.vy * k; b->z = a->z + line.vz * k;
    }
    sys->apos = apos; sys->ahtp = ahtp;
}

/* camera for the current yaw/pitch: dir = viewing direction, sh = the shoulder point (the aim
   ray starts there), eye = behind it on the ray through it. Walls pull a camera shoulder point
   in sideways (only if the shoulder itself is in a wall) and slide the eye forward along the
   ray; `sh` stays the wall-free one, so camera collision never moves the aim or the facing.
   Depends on the player's position only. */
static void view(NJS_POINT3* eye, NJS_POINT3* dir, NJS_POINT3* sh)
{
    float s = sinf(RAD(yaw)), c = cosf(RAD(yaw)), sp = sinf(RAD(pitch)), cp = cosf(RAD(pitch));
    float k = actor_cam_scale();   /* "You appear as" a taller or shorter model: its height (1: the character's) */
    float side = (g_tps.left ? -g_tps.shoulder : g_tps.shoulder) / 10.0f * k;
    NJS_POINT3 at, csh;

    /* forward (-sin, 0, -cos), right (cos, 0, -sin) */
    dir->x = -s * cp; dir->y = sp; dir->z = -c * cp;
    at.x = plp->px; at.y = plp->py + g_tps.height / 10.0f * k; at.z = plp->pz;
    sh->x = at.x + c * side; sh->y = at.y; sh->z = at.z - s * side;
    csh = *sh;
    clear_of_walls(&at, &csh);
    eye->x = csh.x - dir->x * cur_dist; eye->y = csh.y - dir->y * cur_dist; eye->z = csh.z - dir->z * cur_dist;
    if (!no_cam_walls) clear_of_walls(&csh, eye);
}

static void look(float sens)
{
    pitch += (g_tps.invert_y ? mouse_dy : -mouse_dy) * TURN_PER_PIXEL * g_tps.sens_y / 100.0f * sens;
    mouse_dy = 0;
    if (pitch > DEG(60.0f)) pitch = DEG(60.0f);
    if (pitch < DEG(-60.0f)) pitch = DEG(-60.0f);
}

/* game.c bhMainSequence, before bhControlCamera() */
void tps_camera(void)
{
    NJS_POINT3 eye, dir, sh;
    int aim;

    if (!camera_allowed()) { deactivate(); return; }
    if (!tps_cam_active) { yaw = plp->ay; pitch = DEG(-8.0f); cur_dist = g_tps.dist / 10.0f; cur_fov = (float)g_tps.fov; }
    tps_cam_active = g_tps.cam_active = 1;

    aim = aiming() && !g_tps.auto_aim;
    g_tps.aiming = aiming();
    if (!(sys->sp_flg & 0x40)) mouse_dx = mouse_dy = 0;   /* frozen (above): no mouse look */
    if (g_tps.free_cam && !aiming()) {   /* camera A: the mouse orbits the camera (aiming: tps_player, as B) */
        yaw = wrap(yaw - mouse_dx * TURN_PER_PIXEL * g_tps.sens_x / 100.0f);
        mouse_dx = 0;
        if (pad_steer) yaw = wrap(yaw + wrap(plp->ay - yaw) * 0.4f);
    } else if (!aim) yaw = wrap(yaw + wrap(plp->ay - yaw) * 0.4f);   /* free aim: the mouse steers the camera */
    look(1.0f);
    cur_dist += ((aiming() ? g_tps.aim_dist : g_tps.dist) / 10.0f * actor_cam_scale() - cur_dist) * 0.35f;
    cur_fov += ((float)(aiming() ? g_tps.aim_fov : g_tps.fov) - cur_fov) * 0.35f;

    {   /* test aid RECVX_TPS_ORBIT=degrees: the view only turned about the player (side views for
           contact sheets); the aim keeps the unturned view */
        static float orbit = -1;
        float y0 = yaw;
        if (orbit < 0) no_cam_walls = (orbit = getenv("RECVX_TPS_ORBIT") ? DEG(atof(getenv("RECVX_TPS_ORBIT"))) : 0) != 0;
        yaw += orbit;
        view(&eye, &dir, &sh);
        cam.ay = (short)(int)-yaw;
        yaw = y0;
    }
    cam.px = eye.x; cam.py = eye.y; cam.pz = eye.z;
    cam.ln = 0;
    cam.ax = (short)(int)-pitch;
    cam.az = 0;
    cam.pers = (int)DEG(cur_fov);
    if (trace)
        printf("tps: cam %c frame %u yaw %.1f pitch %.1f | ay %.1f at (%.1f %.1f) mode %d/%d/%d keys %x\n", g_tps.free_cam ? 'A' : 'B',
               (unsigned)gs_frame_count(), yaw * (360.0f / 65536.0f), pitch * (360.0f / 65536.0f), (short)plp->ay * (360.0f / 65536.0f),
               plp->px, plp->pz, plp->mode0, plp->mode1, plp->mode2, move_keys);

    /* the room's near plane can be far out (up to 22 units); the camera is close */
    clip_near = fNaViwClipNear; clip_far = fNaViwClipFar;
    if (clip_near != -1.0f || clip_far != -20000.0f) njClipZ(-1.0f, -20000.0f);
    { extern void vr_camera(void); vr_camera(); }   /* VR: the head takes the view (vr_glue.c) */
}

/* our view as bhControlCamera derived it: position, angles, view vector, eye */
static struct { float px, py, pz, vx, vy, vz, wpx, wpy, wpz; int ax, ay, az; float ln; int pers; } view_now;

/* game.c bhMainSequence, after the frame is drawn */
void tps_after_draw(void)
{
    if (!tps_cam_active) return;
    if (clip_near != fNaViwClipNear || clip_far != fNaViwClipFar) njClipZ(clip_near, clip_far);
    view_now.px = cam.px; view_now.py = cam.py; view_now.pz = cam.pz;
    view_now.vx = cam.vx; view_now.vy = cam.vy; view_now.vz = cam.vz;
    view_now.wpx = cam.wpx; view_now.wpy = cam.wpy; view_now.wpz = cam.wpz;
    view_now.ax = cam.ax; view_now.ay = cam.ay; view_now.az = cam.az;
    view_now.ln = cam.ln; view_now.pers = cam.pers;
    { extern void vr_after_draw(void); vr_after_draw(); }
}

/* game.c bhMainSequence, after bhCheckCut: the cut system (still running underneath) leaves
   its own camera in `cam` - position and angles of a tracking cut every frame, the view
   vector on a cut change (bhSetCut), the eye for cuts with distance fog
   (bhControlActiveCamera). The next frame's game logic runs before tps_camera and reads
   them: effects offset towards the camera (gore sprites, explosions, splashes: cam.v*,
   cam.wp*), 3D sound (cam.ay). Put ours back while our camera stays (not when this frame
   switched to an event or fixed camera). */
void tps_after_cut(void)
{
    if (!tps_cam_active || !camera_allowed()) return;
    sys->gm_flg &= ~0x20;   /* "the view cut" (bhSetCut): ours did not; it wipes blood off the lens (effect 301) */
    cam.px = view_now.px; cam.py = view_now.py; cam.pz = view_now.pz;
    cam.vx = view_now.vx; cam.vy = view_now.vy; cam.vz = view_now.vz;
    cam.wpx = view_now.wpx; cam.wpy = view_now.wpy; cam.wpz = view_now.wpz;
    cam.ax = view_now.ax; cam.ay = view_now.ay; cam.az = view_now.az;
    cam.ln = view_now.ln; cam.pers = view_now.pers;
}

/* ---- Light and fog (docs/THIRDPERSON.md "Light and fog") ----
   Each cut carries its own fog (on/off, colour, range, optionally shifted by the camera's
   distance from the cut camera, flg 0x40/0x80), light set (hidlgt: which of lights 4.. are on)
   and screen filter (fil_no/fil_rt). With the fixed cameras a change hides in the cut; with
   ours it pops while walking. "Smooth" (g_tps.atmos) keeps the game's per-cut values as the
   target and fades to them: changes in a frame where the cut changed fade, others (scripts,
   muzzle flash, lightning, a linked actor dying) apply at once, as in the game. */
#define FADE (1.0f / 24.0f)   /* light/filter weights and fog density: 0 -> 1 in 24 game frames (0.8 s) */
#define EASE 0.08f             /* fog range and colour, per game frame (95% in 36 frames) */
#define MAXL 64                /* lights per room: 44 at most [data] */
extern float fNaFogTbl[128];   /* ps2_NaFog.c */

static int atmos_reset = 1, atmos_trace = -1, last_cut = -1;
static float lw[MAXL];                      /* weight of each room light */
static unsigned char lon[MAXL];             /* on (flg & 3 == 3) in the previous frame */
static struct { float a, nr, fr, c[3]; int on; unsigned col; float tnr, tfr; } fog;   /* shown; on..tfr: last target */
static float filw[4], fila[4];              /* screen filters (effects 90..92): weight, alpha */

static float step(float w, int on) { w += on ? FADE : -FADE; return w < 0 ? 0 : w > 1 ? 1 : w; }

/* RECVX_ATMOS_TRACE=1: one line per game frame - the cut, the game's fog (on, colour, range as
   set, the cut's own range, camera-relative bits), the filter, the lights on, the light slots
   as drawn; with "smooth" also the fog and filters shown */
static void atmos_log(int smooth_on)
{
    char buf[640];
    int len = 0;
    CAM_WRK* ci = &rom->cutp[cam.ncut].cam[cam.camver];
    for (int i = 4; i < rom->lgt_n && i < MAXL && len < 200; i++)
        if ((rom->lgtp[i].flg & 3) == 3) len += snprintf(buf + len, sizeof buf - len, " %d", i);
    len += snprintf(buf + len, sizeof buf - len, " | dir %.2f %.2f %.2f pts", (sys->st_flg & 0x10000) ? sys->lg_r : 0,
                    (sys->st_flg & 0x10000) ? sys->lg_g : 0, (sys->st_flg & 0x10000) ? sys->lg_b : 0);
    for (int i = 0; i < sys->lg_pnt && i < 3; i++)
        len += snprintf(buf + len, sizeof buf - len, " (%.0f %.0f %.0f: %.2f %.2f %.2f)", sys->lg_ptb[i].ps.x, sys->lg_ptb[i].ps.y,
                        sys->lg_ptb[i].ps.z, sys->lg_ptb[i].cr, sys->lg_ptb[i].cg, sys->lg_ptb[i].cb);
    printf("atmos: frame %u room %d/%d cut %d.%d pos %.1f %.1f tps %d smooth %d | game fog %d %06x %.1f %.1f cut %.1f %.1f rel %x"
           " | fil %d %d %02x | eff110 %d | lights%s",
           (unsigned)gs_frame_count(), sys->stg_no, sys->rom_no, cam.ncut, cam.camver, plp->px, plp->pz, tps_cam_active, smooth_on,
           (ci->flg & 8) != 0, cam.fog_col & 0xFFFFFF, cam.fog_nr, cam.fog_fr, ci->fog_nr, ci->fog_fr, ci->flg & 0xC0,
           (cam.flg & 8) != 0, sys->fil_no, sys->fil_rt & 0xFF, (cam.flg & 0x20) != 0, buf);
    if (smooth_on)
        printf(" | shown fog %.2f %02x%02x%02x %.1f %.1f fil %.2f %.2f %.2f", fog.a, (int)(fog.c[0] + 0.5f), (int)(fog.c[1] + 0.5f),
               (int)(fog.c[2] + 0.5f), fog.nr, fog.fr, filw[0], filw[1], filw[2]);
    printf("\n");
}

/* fog amount 0..1 of the range (near, far) (njGenerateFogTable3: linear from -n to -f) at 50
   units, a typical distance in view */
static float fog_amount(float n, float f)
{
    float a = (50.0f + n) / fmaxf(n - f, 1.0f);
    return a < 0 ? 0 : a > 1 ? 1 : a;
}

static void atmos_fog(int cut_changed)
{
    CAM_WRK* ci = &rom->cutp[cam.ncut].cam[cam.camver];
    int on = (ci->flg & 0x8) && !(sys->gm_flg & 0x200000), snap;
    unsigned col = ((sys->st_flg & 0x100000) ? sys->fog_col : cam.fog_col) & 0xFFFFFF;
    float c[3] = { (float)(col >> 16 & 0xFF), (float)(col >> 8 & 0xFF), (float)(col & 0xFF) };
    /* the cut's own range, measured from our camera: not the camera-relative shift of
       bhControlActiveCamera, which measures our eye against the cut camera's base position
       along our view, so the fog came and went as the camera turned */
    float nr = ci->fog_nr, fr = ci->fog_fr;

    snap = atmos_reset || (!cut_changed && (on != fog.on || col != fog.col || nr != fog.tnr || fr != fog.tfr));
    fog.a = snap ? on : step(fog.a, on);
    if (snap || (on && fog.a <= FADE)) {   /* at once, or fading in from no fog */
        fog.nr = nr; fog.fr = fr;
        for (int i = 0; i < 3; i++) fog.c[i] = c[i];
    } else if (on) {
        /* the colour follows the denser fog: towards a denser one it changes faster, towards a
           thinner one slower (else a thin bright fog and a dense dark one mix to a grey haze) */
        float k = EASE * fog_amount(nr, fr) / fmaxf(fog_amount(fog.nr, fog.fr), 0.01f);
        k = k < EASE / 4 ? EASE / 4 : k > EASE * 4 ? EASE * 4 : k;
        fog.nr += (nr - fog.nr) * EASE; fog.fr += (fr - fog.fr) * EASE;
        for (int i = 0; i < 3; i++) fog.c[i] += (c[i] - fog.c[i]) * k;
    }
    fog.on = on; fog.col = col; fog.tnr = nr; fog.tfr = fr;

    if (fog.a <= 0) { njFogDisable(); return; }
    njSetFogColor((unsigned)(fog.c[0] + 0.5f) << 16 | (unsigned)(fog.c[1] + 0.5f) << 8 | (unsigned)(fog.c[2] + 0.5f));
    njGenerateFogTable3(rom->fog, fog.nr, fog.fr);
    for (int i = 0; i < 128; i++) fNaFogTbl[i] = 255.0f - fog.a * (255.0f - fNaFogTbl[i]);   /* 255 = no fog */
    njSetFogTable(rom->fog);
    njFogEnable();
}

/* the light slots as bhControlLight fills them (one directional, three point lights; the
   lights' values of this frame are in lp->w*), each light scaled by its weight. A light that
   fades in waits for a free point-light slot, so slots never change hands mid-fade. */
static void atmos_lights(int cut_changed)
{
    LGT_WORK* lp = rom->lgtp;
    LPTB* lptb = sys->lg_ptb;
    int n = rom->lgt_n < MAXL ? rom->lgt_n : MAXL, used = 0, lct = 2, i;
    float rest = 1.0f, dv[3] = { 0, 0, 0 }, dc[3] = { 0, 0, 0 }, l;
    int wait = 0;

    /* a point light waits for a slot: the ones going out fade twice as fast, so the
       dip in between stays short */
    for (i = 0; i < n; i++) if (lp[i].lsrc == 4 && lw[i] > 0) used++;
    for (i = 4; i < n; i++) if (lp[i].lsrc == 4 && (lp[i].flg & 3) == 3 && lw[i] == 0 && used >= 3) wait = 1;
    used = 0;
    for (i = 0; i < n; i++) {
        int on = (lp[i].flg & 3) == 3;
        if (atmos_reset || i < 4 || (on != lon[i] && !cut_changed)) lw[i] = on;   /* 0..3 are global (muzzle flash, ...) */
        else if (on && lw[i] > 0) lw[i] = step(lw[i], 1);
        else if (!on) lw[i] = lw[i] > (wait ? 2 : 1) * FADE ? lw[i] - (wait ? 2 : 1) * FADE : 0;
        lon[i] = on;
        if (lp[i].lsrc == 4 && lw[i] > 0) used++;
    }
    for (i = 4; i < n; i++)   /* fading in: a point light only into a free slot */
        if (lon[i] && lw[i] == 0 && (lp[i].lsrc != 4 || used++ < 3)) lw[i] = FADE;

    for (i = 0; i < n; i++, lp++) {
        float w = lw[i];
        if (w <= 0) continue;
        if (lp->lsrc == 2) {   /* the game takes the first directional light that is on */
            float e = w * rest;
            rest *= 1.0f - w;
            dv[0] += e * lp->wvx; dv[1] += e * lp->wvy; dv[2] += e * lp->wvz;
            dc[0] += e * lp->wr; dc[1] += e * lp->wg; dc[2] += e * lp->wb;
        } else if (lp->lsrc == 4 && lct < 5) {
            njCnkSetEasyMultiLightPoint(lct, lp->wpx, lp->wpy, lp->wpz);
            njCnkSetEasyMultiLightRange(lct, lp->wnr, lp->wfr);
            njCnkSetEasyMultiLightColor(lct, lp->wr * w, lp->wg * w, lp->wb * w);
            njCnkSetEasyMultiLightSwitch(lct, 1);
            njCnkSetSimpleMultiLightPoint(lct, lp->wpx, lp->wpy, lp->wpz);
            njCnkSetSimpleMultiLightRange(lct, lp->wnr, lp->wfr);
            njCnkSetSimpleMultiLightColor(lct, lp->wr * w, lp->wg * w, lp->wb * w);
            njCnkSetSimpleMultiLightSwitch(lct, 1);
            lptb->ps.x = lp->wpx; lptb->ps.y = lp->wpy; lptb->ps.z = lp->wpz;
            lptb->nr = lp->wnr; lptb->fr = lp->wfr;
            lptb->cr = lp->wr * w; lptb->cg = lp->wg * w; lptb->cb = lp->wb * w;
            lptb++; lct++;
        }
    }
    sys->lg_pnt = lct - 2;
    for (i = lct; i <= 4; i++) { njCnkSetEasyMultiLightSwitch(i, 0); njCnkSetSimpleMultiLightSwitch(i, 0); }
    l = sqrtf(dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2]);
    if (rest < 1.0f && l > 0.0001f) {
        sys->st_flg |= 0x10000;
        sys->lg_vx = dv[0] / l; sys->lg_vy = dv[1] / l; sys->lg_vz = dv[2] / l;
        sys->lg_r = dc[0]; sys->lg_g = dc[1]; sys->lg_b = dc[2];
    } else {
        sys->st_flg &= ~0x10000;
        sys->lg_vx = sys->lg_vy = sys->lg_vz = sys->lg_r = sys->lg_g = sys->lg_b = 0;
    }
    njCnkSetEasyMultiLight(lct - 1);
    njCnkSetSimpleMultiLight(lct - 1);
    bhSetLight();   /* the directional light, the colours, the view-space matrices */
}

/* game.c bhMainSequence, after bhControlLight (before drawing) */
void tps_atmos(void)
{
    int cut, smooth_on;
    if (atmos_trace < 0) atmos_trace = getenv("RECVX_ATMOS_TRACE") != NULL;
    if (!sys || !rom || !plp) return;
    /* our camera with the room's lights (not an event's, not the inventory's) */
    smooth_on = g_tps.atmos && tps_cam_active && !(sys->gm_flg & 0x80) && !(sys->st_flg & 0x1000000) && !(cam.flg & 0x2);
    if (smooth_on) {
        cut = cam.ncut | cam.camver << 8;
        atmos_fog(cut != last_cut);
        atmos_lights(cut != last_cut);
        last_cut = cut;
        atmos_reset = 0;
    } else {
        atmos_off();
    }
    if (atmos_trace) atmos_log(smooth_on);
}

/* back to the game's own light and fog (the lights are set from scratch every frame) */
static void atmos_off(void)
{
    if (atmos_reset) return;
    atmos_reset = 1;
    last_cut = -1;
    if (sys) sys->gm_flg |= 0x10;   /* the game sets its fog again (sync.c bhControlEOR) */
}

/* effsub1.c bhEffFil (screen filters, effects 90..92), after its set-up (mode0 0): with
   "smooth" the filter fades in and out (alpha) instead of switching with the cut.
   Returns 1 if handled here. */
int tps_filter(O_WRK* op)
{
    static const float uv[4][8] = { { 0, 0, 1, 0, 0, 1, 1, 1 }, { 0, 1, 0, 0, 1, 1, 1, 0 },   /* fil_rt & 3, as bhEffFil */
                                    { 1, 1, 0, 1, 1, 0, 0, 0 }, { 1, 0, 1, 1, 0, 0, 0, 1 } };
    int k = op->id - 90, on = (cam.flg & 0x8) && sys->fil_no == k, a;
    float half;
    if (k < 0 || k > 3) return 0;
    if (atmos_reset) {   /* not smooth now: the game's state, which a fade starts from later */
        filw[k] = on;
        fila[k] = (sys->fil_rt & 0xF0) | 0xF;
        return 0;
    }
    filw[k] = step(filw[k], on);
    if (on) {
        for (int i = 0; i < 4; i++) { op->tv[i].u = uv[sys->fil_rt & 3][i * 2]; op->tv[i].v = uv[sys->fil_rt & 3][i * 2 + 1]; }
        fila[k] += (((sys->fil_rt & 0xF0) | 0xF) - fila[k]) * FADE;
    }
    if (filw[k] <= 0 || pc_screen_filter_mode() == 2) {
        op->flg |= 0x1000000;
        op->mode0 = 1;
        return 1;
    }
    a = (int)(fila[k] * filw[k] + 0.5f);
    op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col = (unsigned)a << 24 | 0xFFFFFF;
    half = 320.0f * (pc_screen_filter_mode() == 1 ? pc_hor_scale() : 1.0f);
    op->tv[0].x = op->tv[2].x = 320.0f - half;
    op->tv[1].x = op->tv[3].x = 320.0f + half;
    op->mode0 = 2;
    op->flg &= ~0x1000000;
    sys->ef_trs2d[sys->ef_trs2dn++] = op;
    return 1;
}

/* system.c bhSysCallScreenSaver: the game's idle check only reads the PS2 pad (p1per), so the
   input added on top of it - third-person keys and mouse, quick knife, inventory mouse, trade
   screen, all ending up in sys->pad_on - would let the 6-minute idle dimming start during play
   and never end it (only a pad button or pausing did, not a door). Attract demo input is not
   the player's. */
int pc_input_active(void)
{
    return (sys->pad_on && !(sys->ss_flg & 0x400000)) || (tps_on && mouse_moved);
}

/* pad.c bhSetPad, before pad_ps is derived: the keyboard/mouse layout as the game's
   virtual pad bits (independent of the controller key type) */
unsigned int tps_pad(unsigned int pad)
{
    unsigned k;
    vr_pad_frame();
    if (sys->ss_flg & 0x400000) return pad;   /* attract demo playback */
    k = tps_input_keys();
    pad_steer = (pad & 0xF) != 0;
    move_keys = 0;
    if (FREE_CAM && tps_cam_active && plp && plp->mode1 == 0 && !(k & TPS_K_QUICKTURN) &&
        plp->hp >= 0 && !(sys->st_flg & 0x200)) {   /* (not on the game over "Retry?" or a question: left/right choose there;
                                                       vr135: life 0 is still alive - dead is hp < 0, as the game tests) */
        /* camera A: any direction walks forward, tps_player turns the character towards it */
        move_keys = k & (TPS_K_FWD | TPS_K_BACK | TPS_K_LEFT | TPS_K_RIGHT);
        k &= ~move_keys;
        if (!!(move_keys & TPS_K_FWD) != !!(move_keys & TPS_K_BACK) || !!(move_keys & TPS_K_LEFT) != !!(move_keys & TPS_K_RIGHT))
            pad |= 0x1;
    }
    if (k & TPS_K_FWD) pad |= 0x21;   /* as the d-pad: also aim up/down (game auto-aim) */
    if (k & TPS_K_BACK) pad |= 0x42;
    if (k & TPS_K_LEFT) pad |= 0x4;
    if (k & TPS_K_RIGHT) pad |= 0x8;
    if (k & TPS_K_AIM) pad |= 0x10;
    if (k & TPS_K_RUN) pad |= 0x400;
    if (k & (TPS_K_ACTION | TPS_K_FIRE)) pad |= 0xB00;   /* fire/examine/confirm, like Cross */
    if (k & TPS_K_CANCEL) pad |= 0x1000;
    if (k & TPS_K_MAP) pad |= 0x2000;
    if (k & TPS_K_STATUS) pad |= 0x4000;
    /* free aim: always the straight aim pose; tps_calc_model bends the upper body to the pitch */
    if (g_tps.aiming && !g_tps.auto_aim && plp && plp->mode1 == 1) pad &= ~0x63;
    if (pad & 0x1) pad &= ~0x42;   /* as bhSetPad: forward wins, right wins */
    if (pad & 0x8) pad &= ~0x4;
    return pad;
}

/* distance along the ray p + t*d (|d| = 1) to the capsule, or -1 if it misses */
static float ray_capsule(const NJS_POINT3* p, const NJS_POINT3* d, const NJS_CAPSULE* c)
{
    float best = -1.0f;
    for (int i = 0; i <= 8; i++) {   /* ponytail: 9 spheres along the capsule, exact enough for aiming */
        float f = i / 8.0f, r = c->r + 0.5f;
        float cx = c->c1.x + (c->c2.x - c->c1.x) * f - p->x, cy = c->c1.y + (c->c2.y - c->c1.y) * f - p->y,
              cz = c->c1.z + (c->c2.z - c->c1.z) * f - p->z;
        float t = cx * d->x + cy * d->y + cz * d->z, q = cx * cx + cy * cy + cz * cz - t * t;
        if (t > 0 && q < r * r && (best < 0 || t < best)) best = t;
    }
    return best;
}

/* free aim: what the crosshair (screen centre) points at - the nearest enemy or wall on the
   view ray - and the character's yaw/pitch so the game's own shot ray (origin
   (px, py + 11, pz) + forward * (3 + r), direction Rx(wax) Ry(ay)) goes through it. The ray is
   cast from the shoulder point, which it passes through wherever walls put the camera. A wall
   closer than AIM_MIN counts as AIM_MIN away: a target next to the character would turn it
   (shoulder offset) and tilt the aim (shot origin below the camera) by tens of degrees. */
#define AIM_MIN 40.0f   /* ponytail: parallax limit, at most ~5 degrees yaw / 8 degrees pitch off the view */
static void aim_at_crosshair(void)
{
    NJS_POINT3 eye, dir, sh, t, o, apos;
    NJS_LINE line;
    ATR_WORK* ahtp;
    float dist = 1000.0f, wall, len, h, r = WpnTab[plp->wpnr_no].r;
    int ay;

    view(&eye, &dir, &sh);
    vr_hand_ray(&sh, &dir);   /* VR: the right controller's ray (else the head's, through yaw/pitch) */
    line.px = sh.x; line.py = sh.y; line.pz = sh.z;
    line.vx = dir.x * dist; line.vy = dir.y * dist; line.vz = dir.z * dist;
    apos = sys->apos; ahtp = sys->ahtp;
    if (bhCheckL2Wall(&line, 0x400, &len)) dist = len;
    sys->apos = apos; sys->ahtp = ahtp;
    wall = dist;
    for (int i = 0; i < sys->ewk_n; i++) {
        BH_PWORK* e = &ene[i];
        float d;
        if (!(e->flg & 0x1) || !(e->flg & 0x20) || (e->flg & 0x2) || (e->stflg & 0x1000000)) continue;
        d = ray_capsule(&sh, &dir, &e->watr);
        if (d > 0 && d < dist) dist = d;   /* in front of the shoulder */
    }
    if (dist == wall && dist < AIM_MIN) dist = AIM_MIN;
    t.x = sh.x + dir.x * dist; t.y = sh.y + dir.y * dist; t.z = sh.z + dir.z * dist;

    ay = (int)(atan2f(-(t.x - plp->px), -(t.z - plp->pz)) * (32768.0f / 3.14159265f));
    o.x = plp->px - sinf(RAD(ay)) * (3.0f + r); o.y = plp->py + 11.0f; o.z = plp->pz - cosf(RAD(ay)) * (3.0f + r);
    if (dist == wall) {   /* the crosshair is on a wall: bhCheckGunAtari tests walls from the muzzle (the hand's
                             point wp_fps1.z, owP[9]), about 3 units above o, so a ray aimed from o hit them
                             that much above the crosshair. Aimed from the muzzle as it will be after turning. */
        const float* hm = plp->mlwP->owP[9].mtx;
        float z = WpnTab[plp->wpnr_no].wp_fps1.z, mx = hm[8] * z + hm[12] - plp->px, mz = hm[10] * z + hm[14] - plp->pz;
        float tr = RAD((short)(ay - plp->ay)), c = cosf(tr), s = sinf(tr);
        o.x = plp->px + mx * c + mz * s; o.y = hm[9] * z + hm[13]; o.z = plp->pz - mx * s + mz * c;
    }
    ay = (int)(atan2f(-(t.x - o.x), -(t.z - o.z)) * (32768.0f / 3.14159265f));
    h = sqrtf((t.x - o.x) * (t.x - o.x) + (t.z - o.z) * (t.z - o.z));
    plp->ay += (short)(ay - plp->ay);
    plp->wax = plp->waxp = (short)(int)(atan2f(t.y - o.y, h) * (32768.0f / 3.14159265f));
    /* the knife slashes level at anything not below it: an enemy right in front puts the crosshair
       on its body, well above the hand at that range, which raised the slash */
    if ((WpnTab[plp->wpnr_no].flg & 0x40000000) && plp->wax > 0) plp->wax = plp->waxp = 0;
    aim_t = t;
    /* the game's aim height for what the shot goes at, read by the enemies until the next mode
       handler (tps_player_mode): zombies and the anatomist on the ground, a knocked-down Hunter,
       Cerberus and Bandersnatch in one of their states take no damage from a straight shot; a
       shotgun blows a zombie's head only aimed up. Up/down beyond 22.5 degrees (half the original
       aim's 45), and down at anything below knee height. */
    if (plp->at_flg & 0xE) {
        aim_h = plp->wax > 4096 ? 4 : plp->wax < -4096 || t.y - plp->py < 6.0f ? 8 : 2;
        plp->at_flg = (plp->at_flg & ~0xE) | aim_h;
        if (aim_h == 2) aim_h = 0;
    }
    if (trace) {   /* check: the game's shot direction Rx(wax) Ry(ay) (0, 0, -l) from o reaches t */
        NJS_MATRIX m;
        NJS_VECTOR v, w;
        float l = sqrtf(h * h + (t.y - o.y) * (t.y - o.y)), hp = 0;
        njUnitMatrix(&m);
        njRotateXYZ(&m, plp->wax, plp->ay, 0);
        v.x = v.y = 0; v.z = -l;
        njCalcVector(&m, &v, &w);
        for (int i = 0; i < sys->ewk_n; i++) if (ene[i].flg & 1) hp += ene[i].hp;
        printf("tps: mode %d/%d wpn %d ammo %u: aim yaw %.1f pitch %.1f -> ay %.1f wax %.1f, target %.1f away (aim height %d), shot misses it by %.2f, enemy hp %.0f, eye (%.1f %.1f %.1f)\n",
               plp->mode2, plp->mode3, plp->wpnr_no, sys->itm[sys->ply_id * 16 + (sys->itm[sys->ply_id * 16] & 15)] & 0xFFFF,
               yaw * (360.0f / 65536.0f), pitch * (360.0f / 65536.0f), (short)plp->ay * (360.0f / 65536.0f),
               plp->wax * (360.0f / 65536.0f), dist, plp->at_flg & 0xE,
               sqrtf((o.x + w.x - t.x) * (o.x + w.x - t.x) + (o.y + w.y - t.y) * (o.y + w.y - t.y) + (o.z + w.z - t.z) * (o.z + w.z - t.z)), hp, eye.x, eye.y, eye.z);
    }
}

/* how far the barrel of the weapon in hand matrix h (along the hand's x axis from the muzzle
   wp_fps1) passes from the crosshair target, and the angle that makes from the camera */
static float barrel_miss(const float* h, float* deg)
{
    const NJS_POINT3* f = &WpnTab[plp->wpnr_no].wp_fps1;
    float v[3], l = sqrtf(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]), k = 0, q = 0, de = 0;
    int j;
    for (j = 0; j < 3; j++) {
        v[j] = (&aim_t.x)[j] - (f->x * h[j] + f->y * h[4 + j] + f->z * h[8 + j] + h[12 + j]);
        k += v[j] * h[j] / l;
        de += ((&aim_t.x)[j] - (&cam.wpx)[j]) * ((&aim_t.x)[j] - (&cam.wpx)[j]);
    }
    for (j = 0; j < 3; j++) q += (v[j] - h[j] / l * k) * (v[j] - h[j] / l * k);
    *deg = atan2f(sqrtf(q), sqrtf(de)) * (180.0f / 3.14159265f);
    return sqrtf(q);
}

/* RECVX_TPS_TRACE (actor_glue.c, after our model is posed): the weapon as drawn (the model's hand)
   and in the real hand, against the crosshair target */
void tps_trace_hands(const float* drawn, const float* real)
{
    float d0, d1, m0, m1;
    if (!trace || !g_tps.aiming || g_tps.auto_aim || plp->mode1 != 1) return;
    m0 = barrel_miss(drawn, &d0);
    m1 = barrel_miss(real, &d1);
    printf("tps: drawn weapon vs crosshair target %.1f away: its barrel passes %.2f off (%.2f deg), the real hand's %.2f (%.2f deg); drawn hand %+.2f above the real\n",
           sqrtf((aim_t.x - plp->px) * (aim_t.x - plp->px) + (aim_t.z - plp->pz) * (aim_t.z - plp->pz)), m0, d0, m1, d1, drawn[13] - real[13]);
}

/* RECVX_TPS_TRACE: a shot fired this tick (the ammo went down), as bhCPM2 cast it: the enemy test
   from o = (px, py + 11, pz) + forward * (3 + r), the wall test from the muzzle at the hand
   (owP[9], still the matrices the fire code read), both along Rx(wax) Ry(ay); where the wall ray
   ends against the crosshair target */
static void trace_shot(void)
{
    static unsigned last = 0xFFFFFFFF;
    unsigned ammo = sys->itm[sys->ply_id * 16 + (sys->itm[sys->ply_id * 16] & 15)] & 0xFFFF;
    int shot = ammo < last && last != 0xFFFFFFFF;
    last = ammo;
    if (!shot) return;
    {
        const float* h = plp->mlwP->owP[9].mtx;
        float z = WpnTab[plp->wpnr_no].wp_fps1.z, l = WpnTab[plp->wpnr_no].l, r = WpnTab[plp->wpnr_no].r, len, g[3], o[3], w[3], miss = 0, de = 0, ang;
        NJS_MATRIX mx;
        NJS_VECTOR v = { 0, 0, -l }, vv;
        NJS_LINE line;
        NJS_POINT3 apos = sys->apos;
        ATR_WORK* ahtp = sys->ahtp;
        int j, hit;
        for (j = 0; j < 3; j++) g[j] = h[8 + j] * z + h[12 + j];
        o[0] = plp->px - sinf(RAD(plp->ay)) * (3.0f + r); o[1] = plp->py + 11.0f; o[2] = plp->pz - cosf(RAD(plp->ay)) * (3.0f + r);
        njUnitMatrix(&mx);
        njRotateXYZ(&mx, plp->wax, plp->ay, 0);
        njCalcVector(&mx, &v, &vv);
        line.px = g[0]; line.py = g[1]; line.pz = g[2]; line.vx = vv.x; line.vy = vv.y; line.vz = vv.z;
        hit = bhCheckL2Wall(&line, 0x400, &len);
        if (!hit) len = l;
        for (j = 0; j < 3; j++) {
            w[j] = g[j] + (&vv.x)[j] * len / l;
            miss += (w[j] - (&aim_t.x)[j]) * (w[j] - (&aim_t.x)[j]);
            de += ((&aim_t.x)[j] - (&cam.wpx)[j]) * ((&aim_t.x)[j] - (&cam.wpx)[j]);
        }
        sys->apos = apos; sys->ahtp = ahtp;
        miss = sqrtf(miss);
        ang = atan2f(miss, sqrtf(de)) * (180.0f / 3.14159265f);
        printf("tps: SHOT wpn %d muzzle (%.2f %.2f %.2f) = o + (%.2f %.2f %.2f), dir (%.3f %.3f %.3f); wall hit %d at (%.2f %.2f %.2f), "
               "crosshair target (%.2f %.2f %.2f): misses by %.2f (%.2f deg from the camera), height %+.2f\n",
               plp->wpnr_no, g[0], g[1], g[2], g[0] - o[0], g[1] - o[1], g[2] - o[2], vv.x / l, vv.y / l, vv.z / l, hit, w[0], w[1], w[2],
               aim_t.x, aim_t.y, aim_t.z, miss, ang, w[1] - aim_t.y);
    }
}

/* player.c bhControlPlayer, right after the mode handler (before the auto-turn, the motion
   and bhCalcModel): mouse X turns the character, or the camera while aiming freely */
void tps_player(void)
{
    static float frac;
    float d;
    int n;
    if (trace && tps_cam_active && plp->mode1 == 1) trace_shot();
    if (!tps_cam_active || plp->mode0 != 1 || (plp->mode1 == 0 && plp->mode2 > 13)) { frac = 0; return; }
    if (trace && plp->mode1 == 1)
        printf("tps: weapon mode %d/%d motion %d frame %d/%d at (%.1f %.1f) ay %.1f hand y %.1f\n", plp->mode2, plp->mode3, plp->mtn_no, plp->frm_no >> 16,
               plp->mnwP ? plp->mnwP[plp->mtn_no].frm_num : -1, plp->px, plp->pz, (short)plp->ay * (360.0f / 65536.0f), plp->mlwP->owP[9].mtx[13] - plp->py);
    if (plp->mode1 == 1 && !g_tps.auto_aim) {
        yaw = wrap(yaw - mouse_dx * TURN_PER_PIXEL * g_tps.sens_x / 100.0f * g_tps.aim_sens / 100.0f);
        mouse_dx = 0;
        look(g_tps.aim_sens / 100.0f);
        static int lock = -1;   /* test aid (RECVX_TPS_TRACE=2): follow the enemy the game locked on to */
        if (plp->flg & 0x100000) lock = plp->src_no;
        if (trace > 1 && lock >= 0 && (ene[lock].flg & 3) == 1) {   /* at RECVX_TPS_AIM_AT along its hit capsule (0 c1 .. 1 c2) */
            NJS_CAPSULE* c = &ene[lock].watr;
            NJS_POINT3 eye, dir, sh;
            float dx, dy, dz, f = getenv("RECVX_TPS_AIM_AT") ? (float)atof(getenv("RECVX_TPS_AIM_AT")) : 0.5f;
            view(&eye, &dir, &sh);
            dx = c->c1.x + (c->c2.x - c->c1.x) * f - sh.x; dy = c->c1.y + (c->c2.y - c->c1.y) * f - sh.y; dz = c->c1.z + (c->c2.z - c->c1.z) * f - sh.z;
            yaw = atan2f(-dx, -dz) * (32768.0f / 3.14159265f);
            pitch = atan2f(dy, sqrtf(dx * dx + dz * dz)) * (32768.0f / 3.14159265f);
            look(0);   /* within the mouse's pitch range */
        }
        if (plp->flg & 0x100000) {   /* no lock-on turn (bhCPM2_act_suw / next target) */
            plp->flg &= ~0x100000;
            plp->stflg &= ~0x800;
        }
        aim_at_crosshair();
        if (vr_on()) plp->ay = (short)(int)yaw;   /* VR: the body faces where the head looks; the hands aim (vr_glue.c) */
        return;
    }
    if (FREE_CAM) {
        static int was_aiming;
        int aim = plp->mode1 == 1;
        /* game auto-aim: raised towards the camera unless locked on, then as B */
        if (aim && !was_aiming && !(plp->flg & 0x100000)) plp->ay += (short)((int)yaw - plp->ay);
        was_aiming = aim;
        if (!aim) {   /* walk towards the keys' direction seen from the camera; not during the game's own turns (quick turn) */
            float f = !!(move_keys & TPS_K_FWD) - !!(move_keys & TPS_K_BACK), r = !!(move_keys & TPS_K_RIGHT) - !!(move_keys & TPS_K_LEFT);
            {   /* vr85: VR - she faces the stick's way at once (its exact angle, from the head's heading) and walks
                   straight there: no turning arc that the camera would follow */
                extern int vr_move_stick(float* f, float* r);
                extern int g_vr_move_direct, g_vr_debug_log;
                float vf, vr;
                if (g_vr_move_direct && vr_on() && vr_move_stick(&vf, &vr) && vf * vf + vr * vr > 0.04f && (f != 0 || r != 0) && !(plp->flg & 0x100000)) {
                    static unsigned nl;
                    short want = (short)(int)wrap(yaw - atan2f(vr, vf) * (32768.0f / 3.14159265f));
                    if (g_vr_debug_log && (nl++ % 90) == 0)
                        host_log("vr-diag: turn at once: stick %.2f %.2f, turned %d to %d\n", vf, vr, (int)(short)(want - plp->ay), (int)want);
                    plp->ay = want;
                    return;
                }
            }
            if ((f != 0 || r != 0) && !(plp->flg & 0x100000)) {
                d = wrap(yaw - atan2f(r, f) * (32768.0f / 3.14159265f) - (short)plp->ay);
                plp->ay += (int)(d * 0.3f);   /* 95% in 9 game frames */
            }
            return;
        }
    }
    d = -mouse_dx * TURN_PER_PIXEL * g_tps.sens_x / 100.0f + frac;
    mouse_dx = 0;
    n = (int)d;
    frac = d - n;
    plp->ay += n;
    yaw += n;
}

/* turn a node's world matrix (rows = its axes and origin in world space, as njCalcPoint reads
   them) by angle `a` (radians) about the unit axis u through the point c */
static void turn(float* m, const float* u, const float* c, float a)
{
    float cs = cosf(a), sn = sinf(a);
    for (int r = 0; r < 4; r++) {
        float v[3], k;
        for (int j = 0; j < 3; j++) v[j] = m[r * 4 + j] - (r == 3 ? c[j] : 0);
        k = (u[0] * v[0] + u[1] * v[1] + u[2] * v[2]) * (1 - cs);   /* Rodrigues */
        m[r * 4 + 0] = v[0] * cs + (u[1] * v[2] - u[2] * v[1]) * sn + u[0] * k + (r == 3 ? c[0] : 0);
        m[r * 4 + 1] = v[1] * cs + (u[2] * v[0] - u[0] * v[2]) * sn + u[1] * k + (r == 3 ? c[1] : 0);
        m[r * 4 + 2] = v[2] * cs + (u[0] * v[1] - u[1] * v[0]) * sn + u[2] * k + (r == 3 ? c[2] : 0);
    }
}

/* player.c bhControlPlayer, in place of bhCalcModel(plp): the game has three aim poses
   (straight, 45 degrees up/down). Free aim keeps the straight one and pitches spine and
   chest (nodes 2 and 3, half each; arms, head and weapon follow) about the character's
   sideways axis so that the weapon points at the crosshair target, for every weapon and
   character. The motion state (node angles) is untouched: the two nodes' world matrices are
   turned and the tree below them recomputed from them (O_WORK flg 1). */

/* nodes 2 and 3 of p turned by a each about p's sideways axis, the tree below recomputed */
static void bend(BH_PWORK* p, float a)
{
    O_WORK* ow = p->mlwP->owP;
    float u[3], l, c[3];
    l = sqrtf((*p->mtx)[0] * (*p->mtx)[0] + (*p->mtx)[1] * (*p->mtx)[1] + (*p->mtx)[2] * (*p->mtx)[2]);
    for (int j = 0; j < 3; j++) { u[j] = (*p->mtx)[j] / l; c[j] = ow[2].mtx[12 + j]; }   /* the model's x axis */
    turn(ow[2].mtx, u, c, a);
    turn(ow[3].mtx, u, c, a);
    for (int j = 0; j < 3; j++) c[j] = ow[3].mtx[12 + j];
    turn(ow[3].mtx, u, c, a);
    ow[2].flg |= 1; ow[3].flg |= 1;
    bhCalcModel(p);
    ow[2].flg &= ~1; ow[3].flg &= ~1;
}

void tps_calc_model(void)
{
    static float w;   /* eased in/out when the weapon is raised/lowered */
    O_WORK* ow = plp->mlwP->owP;
    float a, dx, dz, ofs[3];
    int on = tps_cam_active && !g_tps.auto_aim && plp->mode0 == 1 && plp->mode1 == 1 && plp->mode2 != 68;

    bhCalcModel(plp);
    bend_a = 0;
    w += (on - w) * 0.35f;
    if (w < 0.01f || plp->mlwP->obj_num <= 9) { aim_corr = 0; vr_hand(plp); return; }
    /* pitch from the weapon hand of the straight pose to the target (not wax: the shot starts
       lower, at 11 units, and needs a steeper angle for close targets); with "You appear as" the
       hand where the model holds the weapon (a Tyrant's is higher: its weapon pointed over the
       crosshair target) */
    actor_aim_offset(plp, ofs);
    dx = aim_t.x - ow[9].mtx[12] - ofs[0]; dz = aim_t.z - ow[9].mtx[14] - ofs[2];
    a = atan2f(aim_t.y - ow[9].mtx[13] - ofs[1], sqrtf(dx * dx + dz * dz));
    if ((WpnTab[plp->wpnr_no].flg & 0x40000000) && a > 0) a = 0;   /* knife: level at targets above (an enemy
                                                                       right in front raised the slash to its head), still bends down */
    bend_a = a = w * (a > 1.047f ? 1.047f : a < -1.047f ? -1.047f : a) / 2;   /* at most 60 degrees */
    bend(plp, a);
    {   /* "You appear as" another model (the retargeting): the weapon is drawn in the model's hand, which
           the bend swings about the chest as well, farther the taller the model, and in a gender-matched
           pose the hand turns the weapon as that motion does. The pitch is solved on the weapon as it will
           be drawn: its barrel through the crosshair target. The barrel runs from the muzzle (wp_fps1) on to
           wp_fps2 where that point lies beyond it, else along the hand's x axis: the grenade launcher's runs at
           45 degrees to that axis (both its points on the barrel), and solved on the x axis it was held 44
           degrees up (the other weapons' second point is the muzzle, on the x axis or unset).
           actor_aim_hand is the hand as drawn last tick, in the frame of the chest now, so it is solved
           only while the aim pose stands still (weapon raised, mode2 65, no motion blend) and not for the
           knife (no barrel). Raising, shots, recoil and slashes keep the last correction, which is eased:
           solved on a moving hand the upper body jumped back and forth. */
        static int still;   /* ticks the aim pose has stood still (the hand read is last tick's) */
        float h[16], t0 = a * 2 / w, t = t0, goal = aim_corr;
        int it, steady;
        still = plp->mode2 == 65 && !plp->hokan_count && !(WpnTab[plp->wpnr_no].flg & 0x40000000) ? still + 1 : 0;
        steady = still >= 2;
        if (!actor_aim_hand(plp, h)) { aim_corr = 0; vr_hand(plp); return; }   /* no model: the hand's pitch */
        const NJS_POINT3 *f = &WpnTab[plp->wpnr_no].wp_fps1, *f2 = &WpnTab[plp->wpnr_no].wp_fps2;
        float ax[3] = { f2->x - f->x, f2->y - f->y, f2->z - f->z }, l = sqrtf(ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2]);
        if (l < 0.01f || ax[0] * f->x + ax[1] * f->y + ax[2] * f->z <= 0) ax[0] = 1, ax[1] = ax[2] = 0;   /* no second point
            beyond the muzzle (the same, unset, or behind it: the rocket launcher's back end): the x axis */
        else for (int j = 0; j < 3; j++) ax[j] /= l;
        for (it = 0; steady && it < 4 && (it == 0 || actor_aim_hand(plp, h)); it++) {
            float m[3], b[3], e, sg;
            for (int j = 0; j < 3; j++) {
                m[j] = f->x * h[j] + f->y * h[4 + j] + f->z * h[8 + j] + h[12 + j];
                b[j] = ax[0] * h[j] + ax[1] * h[4 + j] + ax[2] * h[8 + j];
            }
            dx = aim_t.x - m[0]; dz = aim_t.z - m[2];
            sg = dx * b[0] + (aim_t.y - m[1]) * b[1] + dz * b[2] < 0 ? -1.0f : 1.0f;   /* (the barrel's way along the axis) */
            e = atan2f(aim_t.y - m[1], sqrtf(dx * dx + dz * dz)) - atan2f(sg * b[1], sqrtf(b[0] * b[0] + b[2] * b[2]));
            if (fabsf(e) < 0.002f) break;
            t += e;
            t = t > 1.047f ? 1.047f : t < -1.047f ? -1.047f : t;
            bhCalcModel(plp);   /* (unbent) */
            bend(plp, w * t / 2);
        }
        if (steady) goal = t - t0;
        aim_corr += (goal - aim_corr) * 0.35f;
        t = t0 + aim_corr;
        t = t > 1.047f ? 1.047f : t < -1.047f ? -1.047f : t;
        bhCalcModel(plp);   /* (unbent) */
        bend_a = w * t / 2;
        bend(plp, bend_a);
    }
    vr_hand(plp);   /* VR: the weapon hand to the right controller */
}

/* the same bend on a display copy of the player posed this tick (actor_glue.c: a gender-matched
   pose; its base matrix is the player's) */
void tps_bend(BH_PWORK* p)
{
    if (tps_on && bend_a != 0 && p->mlwP->obj_num > 9) bend(p, bend_a);
}

/* RECVX_EFF_TRACE=1 (test aid): the live effects at the frame hand-off - id.type, an
   asterisk if queued for drawing this frame, and the view-space z of their position */
static void eff_trace(void)
{
    char buf[2048];
    int len = 0, alive = 0;
    if (!sys || !plp) return;
    O_WRK** lists[] = { sys->ef_pol, sys->ef_mdf, sys->ef_lin, sys->ef_ntx, sys->ef_trs, sys->ef_pnc, sys->ef_opq,
                        sys->ef_thl, sys->ef_ntx2d, sys->ef_trs2d, sys->ef_pnc2d, sys->ef_fnc };
    int* n = &sys->ef_poln;   /* the 12 list counts, in the order of the lists */
    for (int i = 0; i < 512; i++) {
        O_WRK* op = &eff[i];
        int drawn = 0;
        NJS_POINT3 v;
        if (!(op->flg & 1) || op->id == 0) continue;
        for (int l = 0; l < 12 && !drawn; l++)
            for (int k = 0; k < n[l] && !drawn; k++) drawn = lists[l][k] == op;
        njCalcPoint(cam.mtx, (NJS_POINT3*)&op->px, &v);
        alive++;
        if (len < (int)sizeof(buf) - 40)
            len += snprintf(buf + len, sizeof(buf) - len, " %d.%d@%d%s%s(%.0f)", op->id, op->type, i, drawn ? "*" : "",
                            (op->flg & 0x1000000) || (op->stflg & 0x1000000) ? "h" : "", v.z);
    }
    if (alive) printf("eff: gfrm %d cut %d tps %d sp %x:%s\n", sys->gfrm_ct, cam.ncut, tps_cam_active, sys->sp_flg, buf);
}

/* frame hand-off (libgraph.c), game thread */
void tps_frame(void)
{
    static int efftrace = -1;
    if (efftrace < 0) efftrace = getenv("RECVX_EFF_TRACE") != NULL;
    if (efftrace) eff_trace();
    int on = g_tps.on;
    static int init;
    if (!init++) trace = getenv("RECVX_TPS_TRACE") ? atoi(getenv("RECVX_TPS_TRACE")) : 0;
    tps_take_mouse(&mouse_dx, &mouse_dy);
    mouse_moved = mouse_dx != 0 || mouse_dy != 0;
    if (tps_on && !on) deactivate();
    tps_on = on;
}
