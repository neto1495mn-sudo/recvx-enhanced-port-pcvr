/* Third-person mode (docs/THIRDPERSON.md): settings shared between the overlay/host (main
   thread), the input layer and the game thread (pc/glue/tps_glue.c). */
#ifndef RECVX_TPS_H
#define RECVX_TPS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct TpsConfig {
    volatile int on;          /* user toggle (ini "tps", --third-person, toggle key) */
    int fov, aim_fov;         /* degrees, horizontal over the 4:3 picture (hor+ widens it) */
    int dist, aim_dist;       /* camera distance behind the shoulder, tenths of game units */
    int height;               /* pivot height above the player's feet, tenths */
    int shoulder;             /* sideways offset, tenths */
    int left;                 /* 1 = over the left shoulder */
    int sens_x, sens_y;       /* mouse sensitivity, percent */
    int aim_sens;             /* sensitivity while aiming, percent of the above */
    int invert_y, crosshair;
    int auto_aim;             /* 1 = keep the game's lock-on when raising the weapon */
    int atmos;                /* light and fog: 1 = smooth (fades between the cuts' settings), 0 = per cut */
    int free_cam;             /* mouse/keyboard camera: 0 = B (behind the character, the mouse turns it),
                                 1 = A (free: the mouse orbits the camera, WASD walk relative to it) */
    int key;                  /* SDL scancode of the toggle key */
    /* published by the game thread */
    volatile int cam_active;  /* the third-person camera drew the last frame */
    volatile int aiming;      /* weapon raised (crosshair) */
} TpsConfig;

extern TpsConfig g_tps;

/* input (pc/src/input/tps_input.c), called on the game thread */
enum { TPS_K_FWD = 1, TPS_K_BACK = 2, TPS_K_LEFT = 4, TPS_K_RIGHT = 8, TPS_K_RUN = 16, TPS_K_ACTION = 32,
       TPS_K_STATUS = 64, TPS_K_MAP = 128, TPS_K_AIM = 256, TPS_K_FIRE = 512, TPS_K_CANCEL = 1024,
       TPS_K_QUICKTURN = 2048 /* with BACK (and RUN): the quick-turn row */ };
unsigned tps_input_keys(void);
void tps_take_mouse(float* dx, float* dy);   /* pixels moved since the last call */
/* main thread */
void tps_input_event(const void* sdl_event);
void tps_update_mouse_mode(void* window, int overlay_open);
void tps_frame(void);                         /* game thread, frame hand-off */

#ifdef __cplusplus
}
#endif
#endif
