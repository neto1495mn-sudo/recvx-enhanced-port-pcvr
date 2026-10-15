/* Third-person mode input: mouse (relative mode, main thread) and the keyboard layout
   (read on the game thread like pad.c). docs/THIRDPERSON.md */
#include "../host/tps.h"
#include "../host/menu_mouse.h"
#include "../host/bindings.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>

int pad_game_input_enabled(void);

static int mouse_x, mouse_y;   /* 1/64 pixel, atomic */
static int buttons;            /* SDL button mask, atomic */
static int relative;

void tps_input_event(const void* e)
{
    const SDL_Event* ev = (const SDL_Event*)e;
    if (!relative) return;
    switch (ev->type) {
    case SDL_EVENT_MOUSE_MOTION:
        __atomic_add_fetch(&mouse_x, (int)(ev->motion.xrel * 64.0f), __ATOMIC_RELAXED);
        __atomic_add_fetch(&mouse_y, (int)(ev->motion.yrel * 64.0f), __ATOMIC_RELAXED);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        __atomic_or_fetch(&buttons, SDL_BUTTON_MASK(ev->button.button), __ATOMIC_RELAXED);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        __atomic_and_fetch(&buttons, ~SDL_BUTTON_MASK(ev->button.button), __ATOMIC_RELAXED);
        break;
    }
}

/* the cursor is captured while the window has focus, third person is on, the mouse plays (not
   "Gamepad only") and no overlay window or menu with mouse support (menu_mouse.c) is open */
void tps_update_mouse_mode(void* window, int overlay_open)
{
    int want = g_tps.on && !overlay_open && !menu_mouse_cursor() && g_input_source != INPUT_PAD
               && (SDL_GetWindowFlags((SDL_Window*)window) & SDL_WINDOW_INPUT_FOCUS);   /* alt-tab releases */
    if (want == relative) return;
    relative = want;
    SDL_SetWindowRelativeMouseMode((SDL_Window*)window, want);
    if (!want) __atomic_store_n(&buttons, 0, __ATOMIC_RELAXED);
}

/* Test automation: RECVX_TPS_MOUSE="<frame>:<dx>:<dy>,..." moves the mouse by dx/dy pixels
   per game frame from that presented frame until the next entry. */
uint64_t gs_frame_count(void);
static void scripted_mouse(float* dx, float* dy)
{
    static const char* script; static int checked;
    char* e;
    if (!checked++) script = SDL_getenv("RECVX_TPS_MOUSE");
    for (const char* p = script; p && *p; p = *e == ',' ? e + 1 : "") {
        unsigned long at = strtoul(p, &e, 10);
        float x = *e == ':' ? strtof(e + 1, &e) : 0, y = *e == ':' ? strtof(e + 1, &e) : 0;
        if (at > gs_frame_count()) break;
        *dx = x;
        *dy = y;
    }
}

void tps_take_mouse(float* dx, float* dy)
{
    *dx = __atomic_exchange_n(&mouse_x, 0, __ATOMIC_RELAXED) / 64.0f;
    *dy = __atomic_exchange_n(&mouse_y, 0, __ATOMIC_RELAXED) / 64.0f;
    if (!pad_game_input_enabled()) *dx = *dy = 0;
    scripted_mouse(dx, dy);
}

/* the bindings' third-person rows (bindings.c; by default W/S forward/back, A/D turn, Shift run,
   E/Space action, Q cancel, Tab status, M map, right mouse ready weapon, left mouse fire) */
unsigned tps_input_keys(void)
{
    static const struct { int row; unsigned bit; } map[] = {
        { B_TPS_FWD, TPS_K_FWD }, { B_TPS_BACK, TPS_K_BACK }, { B_TPS_LEFT, TPS_K_LEFT }, { B_TPS_RIGHT, TPS_K_RIGHT },
        { B_TPS_RUN, TPS_K_RUN }, { B_TPS_ACTION, TPS_K_ACTION }, { B_TPS_CANCEL, TPS_K_CANCEL },
        { B_TPS_STATUS, TPS_K_STATUS }, { B_TPS_MAP, TPS_K_MAP }, { B_TPS_AIM, TPS_K_AIM }, { B_TPS_FIRE, TPS_K_FIRE },
    };
    const bool* k = bind_keys();
    int b = __atomic_load_n(&buttons, __ATOMIC_RELAXED), qt;
    unsigned r = 0;
    if (!pad_game_input_enabled()) return 0;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (bind_held(map[i].row, BIND_KB, k, b)) r |= map[i].bit;
    qt = bind_quickturn(1, bind_held(B_TPS_QUICKTURN, BIND_KB, k, b));
    if (qt) r |= TPS_K_BACK | TPS_K_QUICKTURN | (qt > 1 ? TPS_K_RUN : 0);
    { extern unsigned vr_tps_keys(void); r |= vr_tps_keys(); }   /* VR Touch controllers (vr_glue.c) */
    return r;
}

/* mouse buttons held while the mouse is captured (bindings to mouse buttons) */
int tps_mouse_buttons(void) { return __atomic_load_n(&buttons, __ATOMIC_RELAXED); }
