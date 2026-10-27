/* The binding layer (pc/src/input/bindings.c) on its own: defaults as the old fixed layout,
   third person taking its keys over, rebinding and swapping, conflicts, recvx.ini round trip,
   the gamepad set through SDL's gamepad layer with a virtual Xbox pad, the input source filter
   and the choice of gamepad (two virtual pads, one reconnecting). */
#include "../src/host/bindings.h"
#include "../src/host/tps.h"
#include <SDL3/SDL.h>
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)
#include <stdio.h>
#include <string.h>

TpsConfig g_tps = { .key = SDL_SCANCODE_F5 };
SDL_Window* g_window;
uint64_t gs_frame_count(void) { return 0; }
void host_log(const char* fmt, ...) { (void)fmt; }
int pc_keytype_mask(int v) { (void)v; return -1; }
int pc_keytype(void) { return 0; }

static bool keys[SDL_SCANCODE_COUNT];
static int held(int row, int sc) { memset(keys, 0, sizeof(keys)); if (sc) keys[sc] = 1; return bind_held(row, BIND_KB, keys, 0); }

int main(void)
{
    char buf[64];
    /* defaults: the layout pad.c had built in */
    CHECK(held(B_CROSS, SDL_SCANCODE_Z) && held(B_CIRCLE, SDL_SCANCODE_X) && held(B_SQUARE, SDL_SCANCODE_A));
    CHECK(held(B_TRIANGLE, SDL_SCANCODE_S) && held(B_L1, SDL_SCANCODE_Q) && held(B_R1, SDL_SCANCODE_W));
    CHECK(held(B_START, SDL_SCANCODE_RETURN) && held(B_SELECT, SDL_SCANCODE_BACKSPACE) && held(B_UP, SDL_SCANCODE_UP));
    CHECK(held(B_L2, SDL_SCANCODE_1) && held(B_R2, SDL_SCANCODE_2) && !held(B_CROSS, SDL_SCANCODE_X));
    CHECK(held(B_TPS_RUN, SDL_SCANCODE_RSHIFT) && held(B_NOCLIP_UP, SDL_SCANCODE_PAGEUP));
    CHECK(bind_get(B_TPS_FIRE, BIND_KB, 0) == BIND_MOUSE + SDL_BUTTON_LEFT);
    for (int r = 0; r < B_COUNT; r++) CHECK(!bind_conflict(r, BIND_KB) && !bind_conflict(r, BIND_PAD));
    /* quick knife: Space (no longer third-person action's second key) and the right trigger (no longer R2's) */
    CHECK(held(B_QUICKKNIFE, SDL_SCANCODE_SPACE) && !held(B_TPS_ACTION, SDL_SCANCODE_SPACE) && held(B_TPS_ACTION, SDL_SCANCODE_E));
    CHECK(bind_get(B_QUICKKNIFE, BIND_PAD, 0) == BIND_AXIS + SDL_GAMEPAD_AXIS_RIGHT_TRIGGER * 2 && !bind_get(B_R2, BIND_PAD, 0));
    /* a recvx.ini from before it: its old defaults are dropped, the rest is read */
    CHECK(bind_ini_line("key_tps_action_2", "Space") && !bind_get(B_TPS_ACTION, BIND_KB, 1));
    CHECK(bind_ini_line("pad_r2", "+righttrigger") && !bind_get(B_R2, BIND_PAD, 0));
    CHECK(bind_ini_line("key_tps_action_2", "F") && bind_get(B_TPS_ACTION, BIND_KB, 1) == SDL_SCANCODE_F);
    CHECK(bind_ini_line("key_tps_action_2", "none"));
    /* third person takes W/A/S/Q from the DS2 rows, nothing else */
    g_tps.on = 1;
    CHECK(!held(B_SQUARE, SDL_SCANCODE_A) && !held(B_R1, SDL_SCANCODE_W) && held(B_CROSS, SDL_SCANCODE_Z) && held(B_TPS_FWD, SDL_SCANCODE_W));
    g_tps.on = 0;
    /* rebinding: the new key drives the row, the old one no longer does */
    bind_set(B_CROSS, BIND_KB, 0, SDL_SCANCODE_C);
    CHECK(held(B_CROSS, SDL_SCANCODE_C) && !held(B_CROSS, SDL_SCANCODE_Z) && !bind_swapped());
    /* a key of another game row swaps; a third-person key does not (they share on purpose) */
    bind_set(B_CROSS, BIND_KB, 0, SDL_SCANCODE_X);
    CHECK(held(B_CROSS, SDL_SCANCODE_X) && held(B_CIRCLE, SDL_SCANCODE_C) && bind_swapped() && !strcmp(bind_swapped(), "Circle"));
    bind_set(B_TPS_ACTION, BIND_KB, 1, SDL_SCANCODE_X);
    CHECK(!bind_swapped() && held(B_CROSS, SDL_SCANCODE_X));
    /* hotkeys share with nobody; the toggle key's first slot is g_tps.key */
    bind_set(B_TPS_TOGGLE, BIND_KB, 0, SDL_SCANCODE_F6);
    CHECK(g_tps.key == SDL_SCANCODE_F6);
    /* conflicts from a hand-edited ini */
    CHECK(bind_ini_line("key_l3", "Z") && bind_conflict(B_L3, BIND_KB) == 0);   /* Z is free now (Cross moved to X) */
    CHECK(bind_ini_line("key_l3", "Return") && bind_conflict(B_L3, BIND_KB) == B_START + 1);
    CHECK(bind_ini_line("key_l3", "none") && !bind_conflict(B_L3, BIND_KB));
    /* recvx.ini round trip */
    {
        FILE* f = tmpfile();
        char line[256], k[64], v[128];
        int n = 0;
        bind_ini_write(f);
        bind_reset(BIND_KB); bind_reset(BIND_PAD);
        CHECK(held(B_CROSS, SDL_SCANCODE_Z) && g_tps.key == SDL_SCANCODE_F5);
        rewind(f);
        while (fgets(line, sizeof(line), f))
            if (sscanf(line, " %63[a-z_0-9] = %127[^\r\n]", k, v) == 2) n += bind_ini_line(k, v);
        fclose(f);
        CHECK(n > 50 && held(B_CROSS, SDL_SCANCODE_X) && held(B_CIRCLE, SDL_SCANCODE_C) && g_tps.key == SDL_SCANCODE_F6);
        CHECK(bind_get(B_TPS_ACTION, BIND_KB, 1) == SDL_SCANCODE_X && bind_get(B_TPS_FIRE, BIND_KB, 0) == BIND_MOUSE + SDL_BUTTON_LEFT);
    }
    CHECK(bind_ini_line("pad_l2", "-lefty") && bind_get(B_L2, BIND_PAD, 0) == BIND_AXIS + SDL_GAMEPAD_AXIS_LEFTY * 2 + 1);
    CHECK(bind_ini_line("pad_l2", "+lefttrigger") && bind_ini_line("pad_cross", "south") && !bind_ini_line("coop_port", "7140"));
    /* input source (pure): keyboard/mouse in Auto and K/M only; a gamepad in Auto and Gamepad only, any or the chosen player */
    CHECK(input_accept(INPUT_AUTO, 0, BIND_KB, 0) && input_accept(INPUT_KBM, 0, BIND_KB, 0) && !input_accept(INPUT_PAD, 0, BIND_KB, 0));
    CHECK(input_accept(INPUT_AUTO, 0, BIND_PAD, 3) && !input_accept(INPUT_KBM, 0, BIND_PAD, 0) && input_accept(INPUT_PAD, 0, BIND_PAD, -1));
    CHECK(input_accept(INPUT_PAD, 2, BIND_PAD, 1) && !input_accept(INPUT_PAD, 2, BIND_PAD, 0) && !input_accept(INPUT_PAD, 1, BIND_PAD, -1));
    CHECK(input_accept(INPUT_AUTO, 1, BIND_PAD, 0) && !input_accept(INPUT_AUTO, 1, BIND_PAD, 1) && !input_accept(INPUT_KBM, 1, BIND_PAD, 0));
    /* Gamepad only: the keyboard (and mouse buttons) no longer hold any row */
    g_input_source = INPUT_PAD;
    CHECK(!held(B_CROSS, SDL_SCANCODE_X) && !held(B_TPS_FWD, SDL_SCANCODE_W) && !bind_held(B_TPS_FIRE, BIND_KB, keys, SDL_BUTTON_LMASK));
    g_input_source = INPUT_KBM;
    CHECK(held(B_CROSS, SDL_SCANCODE_X) && bind_held(B_TPS_FIRE, BIND_KB, keys, SDL_BUTTON_LMASK));
    g_input_source = INPUT_AUTO;
    /* gamepad: a virtual Xbox pad through SDL's gamepad layer */
    if (SDL_Init(SDL_INIT_GAMEPAD)) {
        SDL_VirtualJoystickDesc d;
        SDL_INIT_INTERFACE(&d);
        d.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        d.naxes = SDL_GAMEPAD_AXIS_COUNT;
        d.nbuttons = SDL_GAMEPAD_BUTTON_MISC1;
        d.button_mask = (1u << SDL_GAMEPAD_BUTTON_MISC1) - 1;
        d.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
        d.vendor_id = 0x045e; d.product_id = 0x02ea; d.name = "test pad";
        SDL_JoystickID id = SDL_AttachVirtualJoystick(&d);
        SDL_Gamepad* g = SDL_OpenGamepad(id);
        SDL_Joystick* j = SDL_GetGamepadJoystick(g);
        CHECK(g);
        bind_set_gamepad(g);
        CHECK(!strcmp(bind_code_name(BIND_PAD, bind_get(B_CROSS, BIND_PAD, 0), buf, sizeof(buf)), "A"));
        CHECK(!strcmp(bind_code_name(BIND_PAD, bind_get(B_L2, BIND_PAD, 0), buf, sizeof(buf)), "LT"));
        SDL_SetJoystickVirtualButton(j, SDL_GAMEPAD_BUTTON_SOUTH, true);
        SDL_UpdateGamepads();
        CHECK(bind_held(B_CROSS, BIND_PAD, keys, 0) && !bind_held(B_TRIANGLE, BIND_PAD, keys, 0));
        bind_set(B_CROSS, BIND_PAD, 0, 1 + SDL_GAMEPAD_BUTTON_NORTH);   /* swaps with Triangle */
        CHECK(!bind_held(B_CROSS, BIND_PAD, keys, 0) && bind_held(B_TRIANGLE, BIND_PAD, keys, 0));
        SDL_SetJoystickVirtualButton(j, SDL_GAMEPAD_BUTTON_SOUTH, false);
        SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
        SDL_UpdateGamepads();
        CHECK(bind_held(B_L2, BIND_PAD, keys, 0) && !bind_held(B_TRIANGLE, BIND_PAD, keys, 0));
        SDL_CloseGamepad(g);
        bind_set_gamepad(NULL);
        {   /* which gamepad: SDL's player numbers, kept by a pad that reconnects */
            SDL_JoystickID id2, id1;
            id2 = SDL_AttachVirtualJoystick(&d);
            CHECK(SDL_GetGamepadPlayerIndexForID(id) == 0 && SDL_GetGamepadPlayerIndexForID(id2) == 1);
            CHECK(bind_pick_pad() == id);                                   /* Auto, any: the first */
            g_input_pad = 2; CHECK(bind_pick_pad() == id2);
            CHECK(!strcmp(bind_pad_choice(2, buf, sizeof(buf)), "2: test pad")   /* the gamepad mapping's name (same VID/PID) */ && !strcmp(bind_pad_choice(0, buf, sizeof(buf)), "Any"));
            CHECK(!strcmp(bind_pad_choice(3, buf, sizeof(buf)), "3: not connected"));
            g_input_pad = 3; CHECK(bind_pick_pad() == 0);
            g_input_source = INPUT_KBM; g_input_pad = 0; CHECK(bind_pick_pad() == 0);
            g_input_source = INPUT_PAD; g_input_pad = 1; CHECK(bind_pick_pad() == id);
            SDL_DetachVirtualJoystick(id);                                  /* player 1 unplugged: nobody else takes its number */
            CHECK(bind_pick_pad() == 0);
            g_input_pad = 2; CHECK(bind_pick_pad() == id2);
            id1 = SDL_AttachVirtualJoystick(&d);                            /* back: player 1 again, though listed after pad 2 */
            g_input_pad = 1; CHECK(id1 != id && bind_pick_pad() == id1);
            g_input_pad = 0; CHECK(bind_pick_pad() == id2);                 /* any: the first listed */
            /* Gamepad only lets SDL deliver gamepad input while the window is in the background */
            bind_apply_source(); CHECK(SDL_GetHintBoolean(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, false));
            g_input_source = INPUT_AUTO;
            bind_apply_source(); CHECK(!SDL_GetHintBoolean(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, true));
        }
        SDL_Quit();
    } else printf("no SDL gamepad subsystem (%s): gamepad part skipped\n", SDL_GetError());
    printf("bindings: ok\n");
    return 0;
}
