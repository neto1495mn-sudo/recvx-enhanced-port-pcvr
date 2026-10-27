/* Control bindings (pc/src/input/bindings.c, docs/ARCHITECTURE.md "Input"): which keyboard keys,
   mouse buttons and gamepad inputs press the DualShock 2 buttons the game reads (pad.c), the
   third-person keys (tps_input.c) and the port's hotkeys. The game's own key type (KEY CONFIG
   A-D) then turns the DS2 buttons into game actions, so a row is named after what its button
   does under the current key type. Two sets: keyboard/mouse and gamepad, two slots per row.
   Edited by the F1 overlay (main thread) and the native OPTION menu (game thread). */
#ifndef RECVX_BINDINGS_H
#define RECVX_BINDINGS_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* DS2 buttons, in the bit order of the pad report (pad.c PAD_*) */
    B_SELECT, B_L3, B_R3, B_START, B_UP, B_RIGHT, B_DOWN, B_LEFT,
    B_L2, B_R2, B_L1, B_R1, B_TRIANGLE, B_CIRCLE, B_CROSS, B_SQUARE,
    B_QUICKTURN,                       /* back, then the run button (the game's quick turn) */
    B_QUICKKNIFE,                      /* hold: knife drawn and readied, the weapon before back on release (knife_glue.c) */
    B_TRADE,                           /* co-op: trade request / accept with the partner next to you (coop_glue.c, trade_glue.c) */
    /* third person (keyboard and mouse; a gamepad drives it through the DS2 rows) */
    B_TPS_FWD, B_TPS_BACK, B_TPS_LEFT, B_TPS_RIGHT, B_TPS_RUN, B_TPS_ACTION, B_TPS_CANCEL,
    B_TPS_STATUS, B_TPS_MAP, B_TPS_AIM, B_TPS_FIRE, B_TPS_QUICKTURN,
    /* port hotkeys (keyboard) */
    B_TPS_TOGGLE, B_SETTINGS, B_CHEATS, B_COOP, B_FULLSCREEN, B_NOCLIP_UP, B_NOCLIP_DOWN,
    B_VR_RECENTER,
    B_COUNT
};
enum { BIND_KB, BIND_PAD };            /* device sets */
enum { BG_GAME, BG_TPS, BG_HOTKEY };   /* row groups */
#define BIND_SLOTS 2
/* codes: keyboard set 1..511 SDL scancode, BIND_MOUSE + n mouse button n; gamepad set
   1 + SDL_GamepadButton, BIND_AXIS + axis * 2 + (0 positive, 1 negative); 0 = none */
#define BIND_MOUSE 512
#define BIND_AXIS 64

typedef struct BindRow { const char* ini; const char* name; int group; int pad_ok; } BindRow;
extern const BindRow bind_rows[B_COUNT];
extern const int bind_order[B_COUNT];  /* display order */

int  bind_get(int row, int dev, int slot);
void bind_set(int row, int dev, int slot, int code);   /* a code another row of the same context has moves there (swap) */
void bind_reset(int dev);                               /* defaults of one set */
int  bind_conflict(int row, int dev);                   /* another row of the same context shares a code: that row + 1, else 0 */
int  bind_row_shown(int row, int dev);                  /* the row exists for this device */
const char* bind_code_name(int dev, int code, char* buf, int n);   /* for menus, by the connected pad's type */
const char* bind_row_label(int row, char* buf, int n);  /* game rows: the actions of the button under the key type */
const char* bind_pad_desc(void);                        /* the connected gamepad, or "No gamepad" */
const char* bind_swapped(void);                         /* the row the last bind_set moved a code from, or NULL */

/* capture ("press a key"): main-thread events; any thread starts/polls */
void bind_capture_start(int dev);
int  bind_capture_poll(void);          /* -1 waiting, -2 cancelled (Esc, time-out), -3 clear (Delete), else the code */
int  bind_capture_left_ms(void);
int  bind_event(const void* sdl_event); /* main thread, first: 1 = consumed by a capture */

/* state, game thread */
int  bind_held(int row, int dev, const bool* keys, int mouse);  /* any slot of the row is down */
int  bind_hotkey(const void* sdl_event, int row);               /* a key-down event of this hotkey */
void* bind_gamepad(void);              /* the open SDL_Gamepad (pad.c) */
void bind_set_gamepad(void* gamepad);
const bool* bind_keys(void);           /* SDL keyboard state plus scripted keys (RECVX_KEYS) */
int  bind_quickturn(int which, int held);   /* which 0 game, 1 third person: 0 off, 1 hold back, 2 back + run (pad.c) */
int  pad_quick_knife(void);            /* the quick-knife row is held (pad.c) */

/* Input source of this window (F1 Controls, ENHANCED SETTINGS -> CONTROLS; ini input_source, input_pad;
   docs/USER_GUIDE.md "Input source (per window)"). Auto: keyboard/mouse and the gamepad, while focused. Gamepad only:
   keyboard/mouse no longer play (hotkeys, F1/F2/F3 windows and lobby text fields still work) and
   the gamepad keeps working in the background. */
enum { INPUT_AUTO, INPUT_KBM, INPUT_PAD };
extern int g_input_source;             /* INPUT_* */
extern int g_input_pad;                /* 0 any gamepad, n: the one SDL calls player n - 1 (XInput slot, PS/Switch player light) */
int  input_accept(int source, int want_pad, int dev, int player);   /* pure: may a device of set dev (a pad: its player index) play */
unsigned bind_pick_pad(void);          /* the SDL_JoystickID of the gamepad this window reads, 0 none */
const char* bind_pad_choice(int want, char* buf, int n);   /* "Any", "2: <name>" (setting value) */
void bind_apply_source(void);          /* background gamepad events on for Gamepad only (any time, no restart) */

/* recvx.ini (config.c): "key_<row>[_2] = <key name>", "pad_<row>[_2] = <gamepad input>" */
int  bind_ini_line(const char* key, const char* value);        /* 1 = a binding line */
void bind_ini_write(void* FILE_ptr);
void bind_test_frame(void);            /* main thread, per pumped frame: RECVX_KEYS / RECVX_VPAD */

/* game thread glue (pc/glue/options_glue.c): the key type's mask of virtual bit `vbit` (0..17)
   in DS2 report bits, -1 = no game running */
int  pc_keytype_mask(int vbit);
int  pc_keytype(void);

#ifdef __cplusplus
}
#endif
#endif
