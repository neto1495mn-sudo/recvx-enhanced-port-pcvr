/*
 * VR output through OpenXR (pc/src/host/xr.c).
 *
 * Stage 1, "cinema mode": the finished game image is shown on a quad floating in front of the
 * player (cutscenes, menus, doors and anything the VR camera does not draw).
 * Stage 2, stereo: during normal play the game camera follows the head (pc/glue/vr_glue.c)
 * and the renderer draws each eye by re-projecting the frame's 3D (gs/gs_gl.c), submitted as
 * a projection layer.
 *
 * Enabled with --vr or vr = 1 in recvx.ini. Windows only for now (OpenGL on WGL).
 */
#include <stdint.h>
#ifndef RECVX_XR_H
#define RECVX_XR_H
#include "vr_view.h"

extern int g_vr_request;        /* --vr / ini vr: try to start VR at boot */
extern int g_vr_screen_dist;    /* ini vr_dist: cinema screen distance in cm (default 250) */
extern int g_vr_screen_width;   /* ini vr_width: cinema screen width in cm (default 400) */
extern int g_vr_stereo;
extern int g_vr_res;            /* ini vr_res: eye resolution, percent of the headset's recommended size (default 100) */
int xr_recommended_eye_height(void);
int xr_recommended_eye_width(void);
extern int g_vr_screen_angle;   /* ini vr_screen_angle (degrees) */         /* ini vr_stereo: 1 = first-person stereo in normal play (default), 0 = cinema only */

int    xr_init(void);           /* after the GL context is current; 1 if a VR session was created */
int    xr_active(void);         /* a VR session exists (it may not be visible yet) */
int64_t xr_display_time_ns(void);   /* vr142: when the headset shows the frame begun last (OpenXR time, ns; 0 if unknown) */
double xr_display_hz(void);     /* the headset's refresh rate (0 if unknown) */
void   xr_recenter(void);       /* the screen in front of the head, the game's forward where the head looks */
void   xr_shutdown(void);

/* any thread: the headset is showing our frames and stereo is wanted (the game thread uses
   the VR camera only then) */
int    xr_stereo_ready(void);
/* any thread: the latest predicted head pose relative to the recentred reference (metres;
   at the reference the head is at the origin looking down -z); 0 if none yet */
int    xr_head_pose(VrPose* out);
/* the transform recentring applies: recentred = R * (local - origin), as a matrix
   (local -> recentred); vr_glue.c folds it into the frame's W */
void   xr_recenter_matrix(float m[16]);

/* Touch controllers (any thread): the latest state, read each headset frame. Buttons: */
enum { VRB_A = 1, VRB_B = 2, VRB_X = 4, VRB_Y = 8, VRB_MENU = 16, VRB_LSTICK = 32, VRB_RSTICK = 64 };
typedef struct {
    unsigned buttons;
    float lx, ly, rx, ry;          /* thumbsticks, -1..1 (y up) */
    float ltrig, rtrig, lgrip, rgrip;
    int   aim_valid;               /* the right hand's aim pose is tracked */
    VrPose aim;                    /* right hand aim pose (forward = -z), recentred like xr_head_pose */
    int   aiml_valid;              /* the left hand's aim pose is tracked */
    VrPose aiml;                   /* left hand aim pose */
    int   grip_valid, gripl_valid;  /* the grip poses (the palm) are tracked */
    VrPose grip, gripl;            /* right / left grip pose: where the hand holds the controller */
} VrInput;
int    xr_input(VrInput* out);     /* 0 if there is no controller input (no session / not focused) */
unsigned xr_recenter_gen(void);    /* changes at every recentring */
void   xr_haptic(int hand, float amplitude, int ms);   /* any thread: vibrate a controller (0 right, 1 left; amplitude 0..1) */

/* render thread, one headset frame per present:
   xr_frame_begin: -1 no frame (not running), 0 frame begun but nothing to draw, 1 draw */
int          xr_frame_begin(void);
#ifdef __ANDROID__
int          xr_frame_ready(void);              /* the headset wants an image now (xrWaitFrame on its own thread returned) */
void         xr_frame_wait_ready(int ms);       /* wait for that up to ms */
void         xr_timing_log(void);               /* once a second: where the headset images waited */
#endif
const VrEye* xr_frame_eye(int e);                 /* after a begin returning 1; poses in local space */
unsigned int xr_eye_target(int e, int w, int h);  /* acquire eye e's image (w x h); GL texture, 0 on failure */
void         xr_eye_release(int e);
void         xr_frame_end_stereo(void);           /* both eye images as a projection layer */
int          xr_have_held(void);                  /* a stereo frame was shown (its poses kept) */
void         xr_frame_end_held(void);             /* the eye images at the last stereo frame's poses */
void         xr_frame_end_quad(int w, int h);     /* the window's back buffer (w x h) on the cinema screen */
void         xr_frame_end_quad_rect(int x, int y, int w, int h);   /* a part of it (the game's picture) */
void         xr_frame_end_none(void);
void         xr_screen_lock(int on, int angle);   /* the cinema screen stays where it appears (angle: its width in degrees, 0 = the usual); the launcher */
void         xr_quad_source(unsigned fbo);   /* the cinema screen shows this framebuffer (0: the window's back buffer) */
const char*  xr_headset_name(void);   /* the runtime's name for the headset ("" before xr_init) */
int          xr_quad_pointer(float* u, float* v, int* down);   /* the right controller's ray on the cinema screen (0..1 from the top left); the trigger */

#endif
