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
 * Game thread only. Hooks: tps_camera() end (vr_camera), tps_after_draw() (vr_after_draw),
 * game.c around bhPutModel(plp) (vr_hide_player).
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_Vu1Strip.h"
#include "../src/host/xr.h"
#include "../src/gs/stream.h"
#include <math.h>
#include <string.h>

int g_vr_scale = 100;        /* ini vr_scale: game units per metre, in tenths (100 = 10 units/m: the character is ~17 tall) */
int g_vr_eye_height = 155;   /* ini vr_eye_height: the camera's height above the player's feet, tenths of units */
int g_vr_fov = 120;          /* ini vr_fov: the game camera's horizontal field of view (degrees): what it culls against */

static int vr_frame;         /* the VR camera set this frame's view (until vr_after_draw) */
static float frame_W[16];    /* this frame's local -> world */

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
        if (why) return;
    }

    /* body: the character's facing, the same angle the third-person camera follows (forward
       is (-sin, 0, -cos)); Ry(yb) turns the recentred play space to it */
    yb = (float)(short)plp->ay * (3.14159265f / 32768.0f);
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
    vr_frame = 1;
}

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
    stream_vr_view(&v);
    vr_frame = 0;
}
