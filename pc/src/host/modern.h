/* Modern controls for the fixed cameras (pc/glue/modern_glue.c, docs/USER_GUIDE.md "Modern
   controls"): the stick or d-pad points where to walk on screen. The direction math is pure
   (pc/src/input/modern.c, ctest "modern"). Angles in game units (65536 = 360 degrees; a
   character at angle a faces (-sin a, 0, -cos a)). */
#ifndef RECVX_MODERN_H
#define RECVX_MODERN_H
#ifdef __cplusplus
extern "C" {
#endif

extern int g_modern_on;   /* "Fixed camera controls": 0 classic (tank), 1 modern (ini fixed_cam_controls) */

#define MODERN_DEG(a) ((a) * (65536.0f / 360.0f))

float modern_wrap(float a);   /* into -32768..32768 */

/* the angle to face for a screen direction (sx right, sy up) under a camera whose right axis
   on the ground is (rx, rz) */
float modern_target(float rx, float rz, float sx, float sy);

/* camera cuts while the direction is held: the camera of the cut the direction was given in
   stays until the direction turns by more than MODERN_RELEASE or is let go */
#define MODERN_RELEASE MODERN_DEG(35.0f)
typedef struct ModernHold { int held, cut; float rx, rz, ref; } ModernHold;
/* stick: the screen direction's angle; cut: the current cut; rx/rz: in the current camera's
   right axis, out the one to use */
void modern_hold(ModernHold* h, float stick, int cut, float* rx, float* rz);

/* one game frame's turn from facing `ay` towards `target`: a third of the way, at most 30
   degrees (180 degrees in about 9 frames, 0.3 s) */
float modern_turn(float ay, float target);

#ifdef __cplusplus
}
#endif
#endif
