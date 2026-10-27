/* Mouse in the inventory and its sub-screens (pc/glue/menu_glue.c, game thread) fed by the
   host (pc/src/input/menu_mouse.c, main thread). */
#ifndef RECVX_MENU_MOUSE_H
#define RECVX_MENU_MOUSE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MenuMouse {
    volatile int on;        /* option (ini "menu_mouse", F1 Gameplay), default on */
    volatile int active;    /* game thread: a screen with mouse support is open */
    /* main thread -> game thread */
    volatile int x, y;      /* cursor in the game's 2D screen space (njDrawSprite2D: 640x480, 4:3 part), 1/16 px */
    volatile int held;      /* 1 left, 2 right */
    volatile int left, right, left_up;   /* button presses / left releases since the game last looked */
    volatile int wheel;     /* notches, > 0 = away from the player */
} MenuMouse;

extern MenuMouse g_menu_mouse;

/* main thread */
void menu_mouse_event(const void* sdl_event);
void menu_mouse_update(void* window, int overlay_open);   /* once per presented frame */
int  menu_mouse_on(void);       /* any thread: the "menu_mouse" option and the input source lets the mouse play */
int  menu_mouse_cursor(void);   /* the OS cursor belongs to a menu (third person releases the mouse) */
/* game thread (pc/glue/lobby_glue.c): what the mouse did since this screen last looked - the pointer in
   game 2D px, moved, button presses and wheel notches (none when the mouse is off); each screen keeps its
   own MouseTake (zeroed) */
typedef struct MouseTake { float x, y; int moved, left, right, wheel, px, py, seen; } MouseTake;
void menu_mouse_take(MouseTake* t);

#ifdef __cplusplus
}
#endif
#endif
