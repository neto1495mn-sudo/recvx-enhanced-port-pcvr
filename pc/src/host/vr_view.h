/*
 * VR stereo (stage 2): what a game frame tells the renderer about its camera, and the pose
 * types shared by the game thread (pc/glue/vr_glue.c), the stream (gs/stream.c), the renderer
 * (gs/gs_gl.c) and OpenXR (host/xr.c). Plain C, no PS2 or Windows headers.
 *
 * Coordinate frames (column vectors, 4x4 column-major like NJS_MATRIX and GLSL):
 *   world   game units, y up (the game's world; ~10 units per metre)
 *   V       the game's view space as CALCPOINT outputs it: x right, y down, z forward
 *           (the Ninja base view matrix diag(1, -1, -1) times an OpenGL-style view)
 *   local   OpenXR LOCAL reference space, metres, y up, -z forward
 * G: world -> V of the frame (cam.mtx after bhControlCamera)
 * W: local -> world of the frame (where the tracked play space sits in the game world)
 * A vertex the game projected for V is re-projected for an eye with pose E (eye -> local) by
 *   V_eye = S * inverse(E) * inverse(W) * inverse(G) * V,   S = diag(1, -1, -1)
 */
#ifndef RECVX_VR_VIEW_H
#define RECVX_VR_VIEW_H

typedef struct {
    float px, py, pz;           /* position, metres (local) */
    float qx, qy, qz, qw;       /* orientation */
} VrPose;

typedef struct {
    VrPose pose;                /* eye -> local, at the predicted display time */
    float l, r, u, d;           /* tangents of the eye's field of view (l, d < 0 < r, u) */
} VrEye;

typedef struct {
    int   valid;                /* the frame was drawn by the VR camera */
    float G[16];                /* world -> V */
    float W[16];                /* local -> world */
    float cx, cy, kx, ky;       /* the game's projection: screen = c * Vz + k * V.xy (GS pixels, before XYOFFSET) */
    float za, zb;               /* GS depth of a view depth: z = -zb / Vz - za (Ps2_zbuff_a/b) */
    int   mk_n;                 /* VR markers drawn over the eyes (vr_glue.c): */
    float mk[12][3];            /*   their points (world) */
    int   mk_kind[12];          /*   VRMK_* */
    int   mk_val[12];           /*   VRMK_NUMBER: the number shown */
    int   hand_ok[2];           /* vr68: the controllers' aim poses (right, left) the hands were put with */
    VrPose hand[2];
    float body[3];              /* vr71: the body's place (world) the play space was put at this frame */
    float follow[2];            /* ... of which moved by the player's own steps in the room this frame (x, z) */
    int   box_mode;             /* vr87: the ammo box drawn: 0 none, 1 at the holster, 2 in the left hand */
    float box_c[8][3];          /* ... its corners (world; bit 0 x+, bit 1 y+, bit 2 z+) */
    float box_amb[3];           /* ... the room's item light */
    int   box_item;             /* vr131: ... the ammo item it is (the gun in hand's; -1 none): its own picture when known */
    int   laser_ok[2];          /* vr77: the laser sight of each hand's gun (world): muzzle -> hit */
    float laser_a[2][3], laser_b[2][3];
    float step[3], step_prev[3]; /* vr72/73: the player's own move (the stick; its position, not the swaying body) this frame and the one before (world) */
    float turn;                 /* vr120: the view's smooth turn this frame (radians, the change of vr_yaw; 0 for a snap turn) */
} VrView;
enum { VRMK_DOT = 0, VRMK_BUTTON = 1, VRMK_NUMBER = 2 };   /* aiming dot, the "A" over what the action button would use, the holster's ammo count */

/* small 4x4 helpers (column-major), vr_math.c */
void vr_mat_identity(float m[16]);
void vr_mat_mul(float out[16], const float a[16], const float b[16]);   /* out = a * b */
int  vr_mat_invert(float out[16], const float m[16]);
void vr_mat_from_pose(float m[16], const VrPose* p);                   /* pose -> local transform */
void vr_mat_rigid_inverse_pose(float m[16], const VrPose* p);         /* inverse of the above */

#endif
