/*
 * VR output through OpenXR (pc/src/host/xr.c).
 *
 * Stage 1, "cinema mode": the finished game image (what the window shows, overlay included)
 * is copied every present into a quad layer floating in front of the player. The headset's
 * frame loop (xrWaitFrame) paces presentation while VR is active; the desktop window keeps
 * mirroring without vsync. Stereo rendering and head tracking of the game camera build on this.
 *
 * Enabled with --vr or vr = 1 in recvx.ini. Windows only for now (OpenGL on WGL); elsewhere
 * xr_init() reports VR as unavailable and the game runs as usual.
 */
#ifndef RECVX_XR_H
#define RECVX_XR_H

extern int g_vr_request;        /* --vr / ini vr: try to start VR at boot */
extern int g_vr_screen_dist;    /* ini vr_dist: screen distance in cm (default 250) */
extern int g_vr_screen_width;   /* ini vr_width: screen width in cm (default 400) */

int    xr_init(void);           /* after the GL context is current; 1 if a VR session was created */
int    xr_active(void);         /* a VR session exists (it may not be visible yet) */
double xr_display_hz(void);     /* the headset's refresh rate (0 if unknown) */
void   xr_submit_backbuffer(int w, int h);   /* render thread, before the buffer swap: one headset frame */
void   xr_recenter(void);       /* put the screen in front of where the player looks now */
void   xr_shutdown(void);

#endif
