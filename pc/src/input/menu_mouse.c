/* Mouse for the inventory screens (pc/glue/menu_glue.c): the cursor in the game's 2D screen
   space, clicks and wheel; the OS cursor is shown only while such a screen (or an overlay
   window) is open. Main thread. */
#include "../host/menu_mouse.h"
#include "../host/bindings.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

MenuMouse g_menu_mouse = { .on = 1 };

void gs_window_to_screen(float px, float py, float* x, float* y);
int pad_game_input_enabled(void);
uint64_t gs_frame_count(void);

#define ADD(field, n) __atomic_add_fetch(&g_menu_mouse.field, n, __ATOMIC_RELAXED)

static void move_to(float px, float py)   /* window pixels */
{
    float x, y;
    gs_window_to_screen(px, py, &x, &y);
    __atomic_store_n(&g_menu_mouse.x, (int)(x * 16.0f), __ATOMIC_RELAXED);
    __atomic_store_n(&g_menu_mouse.y, (int)(y * 16.0f), __ATOMIC_RELAXED);
}

/* the option, and the mouse plays (not "Gamepad only": a stale cursor would point at items) */
int menu_mouse_on(void) { return g_menu_mouse.on && g_input_source != INPUT_PAD; }
int menu_mouse_cursor(void) { return menu_mouse_on() && g_menu_mouse.active; }

void menu_mouse_event(const void* e)
{
    const SDL_Event* ev = (const SDL_Event*)e;
    static float notches;
    if (!menu_mouse_cursor() || !pad_game_input_enabled()) return;
    switch (ev->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (ev->button.button == SDL_BUTTON_LEFT) ADD(left, 1);
        if (ev->button.button == SDL_BUTTON_RIGHT) ADD(right, 1);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (ev->button.button == SDL_BUTTON_LEFT) ADD(left_up, 1);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        notches += ev->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -ev->wheel.y : ev->wheel.y;
        if (notches >= 1.0f || notches <= -1.0f) {
            ADD(wheel, (int)notches);
            notches -= (int)notches;
        }
        break;
    }
}

/* Test automation: RECVX_MENU_MOUSE="<frame>:<x>:<y>[:<act>],..." puts the cursor at window
   pixel x/y at that presented frame; act: l / r = left / right click, L / U = left button
   down / up, w / s = wheel up / down */
static int scripted(void)
{
    static const char* p; static int checked;
    char* e;
    if (!checked++) p = SDL_getenv("RECVX_MENU_MOUSE");
    if (!p || !*p) return p != NULL;
    while (*p && strtoul(p, NULL, 10) <= gs_frame_count()) {
        float x, y;
        strtoul(p, &e, 10);
        x = *e == ':' ? strtof(e + 1, &e) : 0;
        y = *e == ':' ? strtof(e + 1, &e) : 0;
        move_to(x, y);
        if (*e == ':') {
            switch (e[1]) {
            case 'l': ADD(left, 1); ADD(left_up, 1); break;
            case 'r': ADD(right, 1); break;
            case 'L': ADD(left, 1); g_menu_mouse.held |= 1; break;
            case 'U': ADD(left_up, 1); g_menu_mouse.held &= ~1; break;
            case 'w': ADD(wheel, 1); break;
            case 's': ADD(wheel, -1); break;
            }
            e += 2;
        }
        p = *e == ',' ? e + 1 : e + strlen(e);
    }
    return 1;
}

void menu_mouse_update(void* window, int overlay_open)
{
    static int shown = -1;
    /* unfocused (alt-tab): always show the cursor over the window */
    int focused = (SDL_GetWindowFlags((SDL_Window*)window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    int show = !focused || !menu_mouse_on() || overlay_open || g_menu_mouse.active;
    if (!scripted() && menu_mouse_cursor() && !overlay_open) {
        float x, y, d = SDL_GetWindowPixelDensity((SDL_Window*)window);
        SDL_MouseButtonFlags b = SDL_GetMouseState(&x, &y);
        move_to(x * d, y * d);
        g_menu_mouse.held = (b & SDL_BUTTON_LMASK ? 1 : 0) | (b & SDL_BUTTON_RMASK ? 2 : 0);
    }
    if (show != shown) {
        shown = show;
        if (show) SDL_ShowCursor(); else SDL_HideCursor();
    }
}
