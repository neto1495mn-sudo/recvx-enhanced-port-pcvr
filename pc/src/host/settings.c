/* The settings table shared by the native OPTION menu (ENHANCED SETTINGS) and the F1 overlay.
   docs/ARCHITECTURE.md "Settings": to add a setting, add its variable's ini key in config.c and
   one entry here; both menus show it (F1 on the CO-OP and EXTRAS pages only with SET_F1). Rows
   are in menu order, .group starts a heading. */
#include "../ps2_platform.h"
#include "settings.h"
#include "coop.h"
#include "actors.h"
#include "tps.h"
#include "cheat.h"
#include "menu_mouse.h"
#include "bindings.h"
#include "modern.h"
#include "enemies.h"
#include "lobby.h"
#include "savestore.h"
#include "../audio/audio.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern SDL_Window* g_window;

const char* const settings_pages[SP_COUNT] = { "VR", "DISPLAY", "GAMEPLAY", "CONTROLS", "THIRD PERSON", "AUDIO", "CO-OP", "EXTRAS" };

/* ---------------------------------------------------------------- appliers (main thread) */
void settings_apply_window(void)
{
    SDL_SetWindowFullscreen(g_window, g_config.fullscreen != 0);
    if (!g_config.fullscreen) SDL_SetWindowSize(g_window, g_config.window_w, g_config.window_h);
}
static void apply_vsync(void) { SDL_GL_SetSwapInterval(g_config.vsync ? 1 : 0); }
static void apply_volume(void) { audio_apply_volumes(); }

/* ---------------------------------------------------------------- special values */
static const struct { int w, h; } sizes[] = { { 1280, 960 }, { 1440, 1080 }, { 1920, 1080 }, { 2560, 1440 }, { 3440, 1440 }, { 3840, 2160 } };
static int size_get(void)
{
    for (int i = 0; i < (int)SDL_arraysize(sizes); i++)
        if (sizes[i].w == g_config.window_w && sizes[i].h == g_config.window_h) return i;
    return -1;
}
static void size_set(int v) { if (v >= 0 && v < (int)SDL_arraysize(sizes)) { g_config.window_w = sizes[v].w; g_config.window_h = sizes[v].h; } }
static const char* size_name(int v, char* b, int n)
{
    if (v < 0) snprintf(b, n, "%d x %d", g_config.window_w, g_config.window_h);
    else snprintf(b, n, "%d x %d", sizes[v].w, sizes[v].h);
    return b;
}
static const char* fullscreen_on(void) { return g_config.fullscreen ? "Fullscreen is on." : NULL; }
static const char* scale_name(int v, char* b, int n)
{
    if (v) snprintf(b, n, "%dx (%d x %d)", v, 640 * v, 448 * v);
    else snprintf(b, n, "Auto (%dx)", (int)gs_wanted_scale());
    return b;
}
static const char* tenths(int v, char* b, int n) { snprintf(b, n, "%d.%d", v / 10, v % 10); return b; }
/* picture calibration (applied in gs_gl.c draw_display); the test pattern both menus show
   while a SET_CALIB row is chosen: squares on a black and on a white strip, the faint ones at the ends */
const unsigned char settings_calib[2][CALIB_STEPS] = { { 5, 10, 16, 24, 32 }, { 223, 231, 239, 245, 250 } };
static void picture_reset(void) { g_config.brightness = 0; g_config.contrast = 100; g_config.gamma = 10; settings_save_later(); }
/* "You appear as": positions in the menus' list (actors.c, without event doubles) */
static int chr_get(void) { return appear_pos(g_coop.chr); }
static void chr_set(int v) { g_coop.chr = appear_at(v); }
static const char* chr_name(int v, char* b, int n) { snprintf(b, n, "%s", appear_name(appear_at(v))); return b; }
/* saves: the memory card's copied into the native folder (as the first native start does) */
static void import_card(void)
{
    int n = savestore_import_card(g_config.save_dir, save_native_dir());
    host_log("saves: imported %d memory card save%s into %s (the card keeps them)\n", n, n == 1 ? "" : "s", save_native_dir());
}
static const char* in_session(void) { return g_coop.state != COOP_OFF ? "Not during a co-op session." : NULL; }

/* cheats (F2): the request bits are taken by the game thread at the frame hand-off */
/* VR: the game has no way out from its menus (vr111) - quit as the window's close does: settings saved, exit handlers run */
static void quit_desktop(void) { SDL_Event e; SDL_zero(e); e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e); }
static void heal(void) { __atomic_or_fetch(&g_cheat.request, CHEAT_HEAL, __ATOMIC_ACQ_REL); }
static void give(void) { if (g_cheat.give_id) __atomic_or_fetch(&g_cheat.request, CHEAT_GIVE, __ATOMIC_ACQ_REL); }
static void spawn(void) { __atomic_or_fetch(&g_cheat.request, CHEAT_SPAWN, __ATOMIC_ACQ_REL); }
static void teleport(void) { __atomic_or_fetch(&g_cheat.request, CHEAT_TELEPORT, __ATOMIC_ACQ_REL); }
static void here(void) { g_cheat.tp_stg = g_cheat.stg; g_cheat.tp_rom = g_cheat.rom; g_cheat.tp_pos = g_cheat.pos; }
static int item_get(void) { return g_cheat.give_id; }
static void item_set(int v)   /* on to the next id with a name, in the direction of the step */
{
    int dir = v < g_cheat.give_id ? -1 : 1;
    for (int i = 0; i < CHEAT_ITEMS && !g_cheat.names[v][0]; i++) v = v + dir < 1 ? CHEAT_ITEMS - 1 : v + dir >= CHEAT_ITEMS ? 1 : v + dir;
    g_cheat.give_id = v;
}
static const char* item_name(int v, char* b, int n)
{
    snprintf(b, n, "%d %s", v, v > 0 && v < CHEAT_ITEMS && g_cheat.names[v][0] ? g_cheat.names[v] : "-");
    return b;
}
static int spawn_get(void) { return g_cheat.spawn_idx; }
static void spawn_set(int v) { g_cheat.spawn_idx = v; }
static const char* spawn_nm(int v, char* b, int n) { snprintf(b, n, "%s", spawn_name(v)); return b; }

static const char* const onoff_auto[] = { "Auto", "Off", "On" };
static const char* const modes3[] = { "Original (4:3)", "Stretch to width", "Off" };
static const char* const fps_limits[] = { "Off (display rate)", "30", "60", "90", "120", "144" };   /* render.c present_hz */
static const char* const aspects[] = { "4:3 (original)", "Stretch to window", "Widescreen (hor+)" };
static const char* const save_systems[] = { "Native files", "Memory card (original)" };
static const char* const loots[] = { "Shared", "Individual" };
static const char* const conns[] = { "Automatic", "Direct (P2P)", "Via server" };
static const char* const atmos_names[] = { "Per cut (original)", "Smooth" };
static const char* const free_cam_names[] = { "B (behind character)", "A (free)" };
static const char* const fixed_cam_names[] = { "Classic (tank)", "Modern (camera-relative)" };
static const char* const noclips[] = { "Off", "Through walls", "Walls + fly" };
static const char* const sources[] = { "Auto", "Keyboard/mouse only", "Gamepad only" };
static const char* const extras[] = { "Off", "Up to 1 per room", "Up to 2 per room", "Up to 3 per room" };

/* vr116: the VR comfort page - the few VR options to set before playing (the rest stay in recvx.ini) */
extern int g_vr_turn, g_vr_sight, g_vr_comfort, g_vr_res;
static const int vr_turns[] = { 0, 15, 30, 45, 60, 90 };
static int vr_turn_get(void) { int b = 0; for (int i = 0; i < (int)SDL_arraysize(vr_turns); i++) if (abs(vr_turns[i] - g_vr_turn) < abs(vr_turns[b] - g_vr_turn)) b = i; return b; }
static void vr_turn_set(int v) { if (v >= 0 && v < (int)SDL_arraysize(vr_turns)) g_vr_turn = vr_turns[v]; }
static const char* vr_turn_name(int v, char* b, int n) { if (v <= 0) return "Smooth"; SDL_snprintf(b, n, "%d degrees", vr_turns[v]); return b; }
/* vr118: three sights - Laser, Dot, Off (vr_sight 2, 1, 0; 3, both, shows as Laser) - and three vignettes - Off, Medium,
   Strong (vr_comfort 0, 2, 3; 1, weak, shows as Medium) */
static const int vr_sight_v[] = { 2, 1, 0 }, vr_comfort_v[] = { 0, 2, 3 };
static const char* const vr_sights[] = { "Laser", "Dot", "Off" };
static const char* const vr_comforts[] = { "Off", "Medium", "Strong" };
static int vr_sight_get(void) { return g_vr_sight >= 2 ? 0 : g_vr_sight == 1 ? 1 : 2; }
static void vr_sight_set(int v) { if (v >= 0 && v < 3) g_vr_sight = vr_sight_v[v]; }
static int vr_comfort_get(void) { return g_vr_comfort <= 0 ? 0 : g_vr_comfort <= 2 ? 1 : 2; }
static void vr_comfort_set(int v) { if (v >= 0 && v < 3) g_vr_comfort = vr_comfort_v[v]; }

#define V(x) ((int*)&(x))
const Setting g_settings[] = {
    { SP_VR, "Snap turn", NULL, 0, 5, 1, .get = vr_turn_get, .set = vr_turn_set, .name = vr_turn_name, .group = "COMFORT",
      .help = "Right stick: turn in steps of this many degrees, or Smooth for a continuous turn." },
    { SP_VR, "Comfort vignette", NULL, 0, 2, 1, vr_comforts, .get = vr_comfort_get, .set = vr_comfort_set,
      .help = "Darkens the edges of the view only while you walk with the stick or turn; walking in your room does not." },
    { SP_VR, "Sight", NULL, 0, 2, 1, vr_sights, .get = vr_sight_get, .set = vr_sight_set, .group = "WEAPONS",
      .help = "Laser: a faint red beam from the gun to where the shot hits. Dot: a small ring there. Off: no sight." },
    { SP_VR, "Resolution", V(g_vr_res), 50, 200, 10, .fmt = "%d%%", .group = "PICTURE",
      .help = "Each eye's resolution, in percent of what the headset recommends. Higher is sharper and heavier for the GPU." },

    { SP_DISPLAY, "Fullscreen", V(g_config.fullscreen), 0, 1, 1, .help = "Also F11.", .apply = settings_apply_window, .group = "WINDOW" },
    { SP_DISPLAY, "Window size", NULL, 0, 5, 1, .get = size_get, .set = size_set, .name = size_name, .locked = fullscreen_on, .apply = settings_apply_window },
    { SP_DISPLAY, "VSync", V(g_config.vsync), 0, 1, 1, .apply = apply_vsync },
    { SP_DISPLAY, "Frame rate limit", V(g_config.fps_limit), 0, 5, 1, fps_limits, .help = "Caps the images shown per second (enhanced in-between frames included) below the display rate, e.g. 60 on a 175 Hz monitor. The game itself always runs at 30." },
    { SP_DISPLAY, "Show frame rate", V(g_config.show_fps), 0, 1, 1 },
    { SP_DISPLAY, "Aspect ratio", V(g_config.aspect), 0, 2, 1, aspects, .help = "Widescreen: 3D fills the window, 2D and menus stay 4:3 in the middle.", .group = "PICTURE" },
    { SP_DISPLAY, "Internal resolution", V(g_config.scale), 0, 8, 1, .name = scale_name, .help = "The resolution the game is drawn at; Auto follows the window." },
    { SP_DISPLAY, "Display-rate frames", V(g_config.interpolate), 0, 1, 1, .help = "In-between frames blended from the last two game frames. The game still runs at 30 FPS; half a frame of latency." },
    { SP_DISPLAY, "Frame blend (trail)", V(g_config.frame_blend), -1, 1, 1, onoff_auto, .help = "As on the PS2: each frame mixed over the previous one. Auto: on in vanilla, off with display-rate frames." },
    { SP_DISPLAY, "Cutscene bars", V(g_config.cinema_bars), 0, 2, 1, modes3, .help = "Original keeps them 4:3 in widescreen; fades always fill the screen." },
    { SP_DISPLAY, "Camera vignette", V(g_config.screen_filter), 0, 2, 1, modes3, .help = "Original keeps it 4:3 in widescreen." },
    { SP_DISPLAY, "Screen effects", V(g_config.screen_effects), 0, 2, 1, modes3, .help = "Full-screen effect layers. Original keeps them 4:3 in widescreen." },
    { SP_DISPLAY, "Brightness", V(g_config.brightness), -50, 50, 1, .fmt = "%+d", .flags = SET_CALIB,
      .help = "Lifts or lowers the dark end of the game's picture. Black strip: the left square should be barely visible." },
    { SP_DISPLAY, "Contrast", V(g_config.contrast), 50, 150, 1, .fmt = "%d%%", .flags = SET_CALIB,
      .help = "Spreads or narrows the tones around mid-grey. White strip: the right square should be barely visible." },
    { SP_DISPLAY, "Gamma", V(g_config.gamma), 5, 20, 1, .name = tenths, .flags = SET_CALIB,
      .help = "Higher brightens the mid-tones, lower darkens them; black and white stay." },
    { SP_DISPLAY, "Reset picture calibration", NULL, .flags = SET_ACTION | SET_NOW, .apply = picture_reset,
      .help = "Brightness, contrast and gamma back to neutral: the original picture." },
    { SP_DISPLAY, "Native 3D renderer", V(g_config.native_3d), 0, 1, 1, .help = "3D models projected and clipped by the GPU instead of the emulated VU1 and GS.", .group = "RENDERER" },
    { SP_DISPLAY, "Native textures", V(g_config.native_tex), 0, 1, 1, .help = "Textures uploaded once and bound afterwards instead of re-uploaded on every switch." },
    { SP_DISPLAY, "Cull off-screen models", V(g_config.model_cull), 0, 1, 1, .help = "As on the PS2. Off: models no longer pop at the screen edges. Third person never culls." },

    { SP_GAMEPLAY, "Exit to desktop", NULL, .flags = SET_ACTION, .apply = quit_desktop, .group = "GENERAL",
      .help = "Closes the game now. Progress since the last save is lost." },
    { SP_GAMEPLAY, "Skip intro", V(g_config.skip_intro), 0, 1, 1, .flags = SET_RESTART, .help = "Boot straight into the title menu: no warning screen, logos or intro movie." },
    { SP_GAMEPLAY, "Skip door animations", V(g_config.skip_doors), 0, 1, 1, .help = "Room transitions use the game's load screen instead of the door, stairs or ladder animation." },
    { SP_GAMEPLAY, "Battle Game unlocked", V(g_config.battle_unlocked), 0, 1, 1,
      .help = "The title menu offers BATTLE GAME without a finished playthrough. Saves and records stay as they are; off: only after a cleared game, as the original." },
    { SP_GAMEPLAY, "Quick knife", V(g_config.quick_knife), 0, 1, 1,
      .help = "Hold the quick-knife key (Space, right trigger; see CONTROLS) to draw the knife without the inventory; release it for the weapon before." },
    { SP_GAMEPLAY, "Save system", V(g_config.save_system), 0, 1, 1, save_systems,
      .help = "Native files: each save is its own file in the saves folder, as many as you like, no memory card screens. Memory card: the original 15 slots. From the next save screen.", .group = "SAVES" },
    { SP_GAMEPLAY, "Import memory card saves", NULL, .flags = SET_ACTION, .apply = import_card,
      .help = "Copies the memory card's saves into the saves folder as new files (ones already there are skipped). The card keeps them." },
    { SP_GAMEPLAY, "No ink ribbons needed", V(g_config.no_ink_ribbon), 0, 1, 1,
      .help = "Typewriters save without an ink ribbon and use none up. In co-op the host's setting counts (lobby GAME RULES)." },
    { SP_GAMEPLAY, "Extra enemies", V(g_enemy_opts.extra), 0, 3, 1, extras, .help = "More enemies of the types a room already has (also in single player). In co-op the host's setting counts. Next room.", .group = "ENEMIES" },
    { SP_GAMEPLAY, "Rearranged enemies", V(g_enemy_opts.place), 0, 1, 1, .help = "Free-roaming enemies start elsewhere in their room (from the seed). Story, cutscene and boss enemies stay. Next room." },
    { SP_GAMEPLAY, "Enemy randomizer", V(g_enemy_opts.rand), 0, 1, 1, .help = "Free-roaming enemies become other types (from the seed), within each room's limits. Story, cutscene and boss enemies stay. Next room." },
    { SP_GAMEPLAY, "Enemy seed", V(g_enemy_opts.seed), 0, 99999, 1, .fmt = "%d", .help = "Placements and randomizer: the same seed gives the same rooms. In co-op both use the host's." },

    { SP_CONTROLS, "Input source", V(g_input_source), 0, 2, 1, sources, .apply = bind_apply_source,
      .help = "Which devices play in this window. Auto: keyboard/mouse and gamepad, while the window has focus. Keyboard/mouse "
              "only: no gamepads. Gamepad only: no keys or mouse (hotkeys still work), and the gamepad works without focus: two "
              "windows side by side for local co-op." },
    { SP_CONTROLS, "Gamepad", V(g_input_pad), 0, 4, 1, .name = bind_pad_choice,
      .help = "Any: the first one connected. 1-4: the gamepad the system numbers so (Xbox: its slot / light, PlayStation and "
              "Switch pads show it on their player lights); give each window its own number for local co-op." },
    { SP_CONTROLS, "Mouse in menus", V(g_menu_mouse.on), 0, 1, 1, .help = "Inventory, item box, map, files and these menus: point, left click selects, right click goes back." },
    { SP_CONTROLS, "Fixed camera controls", V(g_modern_on), 0, 1, 1, fixed_cam_names,
      .help = "Modern: the left stick or the d-pad (arrow keys) walks where it points on screen, pushed far or with run it runs; "
              "aiming and the quick turn as Classic. Not in third person." },

    { SP_TPS, "Over-the-shoulder camera", V(g_tps.on), 0, 1, 1, .help = "Third person: mouse/keyboard controls and free aim. Events, doors and menus keep the original camera.", .group = "CAMERA" },
    { SP_TPS, "Mouse/keyboard camera", V(g_tps.free_cam), 0, 1, 1, free_cam_names,
      .help = "B: the camera stays behind the character, the mouse turns it. A: the mouse turns the camera around the character, WASD walk towards their direction on screen; aiming turns the character to the camera. Not the KEY CONFIG type." },
    { SP_TPS, "Field of view", V(g_tps.fov), 40, 120, 5, .fmt = "%d deg", .help = "Horizontal, over the 4:3 picture; widescreen shows more at the sides." },
    { SP_TPS, "Field of view aiming", V(g_tps.aim_fov), 30, 120, 5, .fmt = "%d deg" },
    { SP_TPS, "Camera distance", V(g_tps.dist), 0, 400, 5, .name = tenths },
    { SP_TPS, "Camera distance aiming", V(g_tps.aim_dist), 0, 400, 5, .name = tenths },
    { SP_TPS, "Camera height", V(g_tps.height), 0, 300, 5, .name = tenths },
    { SP_TPS, "Shoulder offset", V(g_tps.shoulder), 0, 150, 5, .name = tenths },
    { SP_TPS, "Left shoulder", V(g_tps.left), 0, 1, 1 },
    { SP_TPS, "Light and fog", V(g_tps.atmos), 0, 1, 1, atmos_names,
      .help = "Smooth: lights, fog and screen filter fade over a second when the room's camera cut changes, fog measured from your camera. Per cut: they switch with the cut, as the fixed cameras do." },
    { SP_TPS, "Mouse sensitivity X", V(g_tps.sens_x), 5, 400, 5, .fmt = "%d%%", .group = "MOUSE AND AIM" },
    { SP_TPS, "Mouse sensitivity Y", V(g_tps.sens_y), 5, 400, 5, .fmt = "%d%%" },
    { SP_TPS, "Aiming sensitivity", V(g_tps.aim_sens), 10, 200, 5, .fmt = "%d%%", .help = "Mouse sensitivity while aiming, relative to the above." },
    { SP_TPS, "Invert Y", V(g_tps.invert_y), 0, 1, 1 },
    { SP_TPS, "Crosshair", V(g_tps.crosshair), 0, 1, 1 },
    { SP_TPS, "Game auto-aim", V(g_tps.auto_aim), 0, 1, 1, .help = "Off: free aim, the shot goes where the crosshair points. On: the game's lock-on." },

    { SP_AUDIO, "Music and voice", V(g_config.music_volume), 0, 100, 5, .fmt = "%d%%", .apply = apply_volume },
    { SP_AUDIO, "Sound effects", V(g_config.sfx_volume), 0, 100, 5, .fmt = "%d%%", .apply = apply_volume },

    { SP_COOP, "You appear as", NULL, 0, 0, 1, .get = chr_get, .set = chr_set, .name = chr_name, .count = appear_count, .help = "Your model, also in single player and cutscenes; in co-op your partner sees it too.", .group = "YOU" },
    { SP_COOP, "Use gender-matched animations", V(g_appear_anim), 0, 1, 1, .flags = SET_F1, .help = "A man shown while you play Claire walks, aims and fights with Chris's motions; a woman shown while you play a man, with Claire's." },
    { SP_COOP, "Talking faces in cutscenes (experimental)", V(g_appear_face), 0, 1, 1, .flags = SET_F1, .help = "Your model in cutscenes moves its mouth with the lines, with the story cast's own faces (a player file shown there becomes the same character's cutscene model). From the next room with a cutscene on." },
    { SP_COOP, "Partner's name tag", V(g_coop.nametag), 0, 1, 1, .help = "The partner's name above its head, or at the screen edge when it is out of view." },
    { SP_COOP, "Show ping", V(g_config.show_ping), 0, 1, 1, .flags = SET_F1, .help = "In co-op, while connected: the round trip to the partner next to the frame rate." },
    { SP_COOP, "Follow host (joining player)", V(g_coop.follow), 0, 1, 1, .help = "The joining player's choice. On: you go through every door the host takes. Off: you stay and may explore other rooms; story cutscenes and character switches still bring you to the host." },
    { SP_COOP, "Loot (host)", V(g_coop.loot), 0, 1, 1, loots, .locked = in_session, .help = "Shared: each item lies there once, whoever takes it has it (split the supplies). Individual: each player takes their own copy. Key items go to both.", .group = "GAME RULES (HOST)" },
    { SP_COOP, "Important items for both (host)", V(g_coop.important), 0, 1, 1, .locked = in_session,
      .help = "Shared loot: weapons and ink ribbons lie there for each player (taking one leaves the partner's). Off: they are one pickup like ammo and herbs." },
    { SP_COOP, "Synchronized cutscene skip (host)", V(g_coop.skip_sync), 0, 1, 1,
      .help = "A cutscene both of you watch is skipped once both pressed skip; the partner sees that you want to." },
    { SP_COOP, "Freeze enemies while trading (host)", V(g_coop.trade_freeze), 0, 1, 1, .help = "While both players are in the trade screen the room's enemies stand still (the host's setting counts)." },
    { SP_COOP, "Remember partner inventories (host)", V(g_coop.remember), 0, 1, 1, .flags = SET_F1,
      .help = "Keep each partner's items when it leaves, kept with your saves: it can join again with them (ITEMS: Saved with the host), and gets them back after its game crashed." },
    { SP_COOP, "Connection", V(g_coop.conn), 0, 2, 1, conns, .locked = in_session,
      .help = "Automatic: with a co-op server set (F3 or the lobby), hosting also opens a room and joins by room code try a direct path first, then the server's relay; without a server, as Direct. Direct: address and join code only. Via server: always through the relay.", .group = "CONNECTION" },
    { SP_COOP, "Host port", V(g_coop.port), 1, 65535, 1, .fmt = "%d", .locked = in_session, .help = "Hosting and joining: MULTIPLAYER in the title menu, or F3." },
    { SP_COOP, "Open port (UPnP)", V(g_coop.upnp), 0, 1, 1, .locked = in_session, .help = "While you host, ask the router to forward the port so a friend can join over the internet." },

    { SP_EXTRAS, "Infinite health", V(g_cheat.inf_health), 0, 1, 1, .group = "CHEATS" },
    { SP_EXTRAS, "Infinite ammo", V(g_cheat.inf_ammo), 0, 1, 1 },
    { SP_EXTRAS, "Noclip", V(g_cheat.noclip), 0, 2, 1, noclips, .help = "Walls and enemies no longer block you. Fly: noclip up/down keys (R/F, Page Up/Down) or R2/L2." },
    { SP_EXTRAS, "Heal now", NULL, .flags = SET_ACTION | SET_GAME_ONLY, .apply = heal },
    { SP_EXTRAS, "Item", NULL, 1, CHEAT_ITEMS - 1, 1, .get = item_get, .set = item_set, .name = item_name, .group = "ITEMS" },
    { SP_EXTRAS, "Count", V(g_cheat.give_count), 1, 999, 1, .fmt = "%d" },
    { SP_EXTRAS, "Add to inventory", NULL, .flags = SET_ACTION | SET_GAME_ONLY, .apply = give },
    { SP_EXTRAS, "Enemy", NULL, 0, 0, 1, .get = spawn_get, .set = spawn_set, .name = spawn_nm, .count = spawn_count,
      .help = "Bosses and scripted enemies may not work outside their room; spawned enemies are gone after leaving it.", .group = "SPAWN" },
    { SP_EXTRAS, "Spawn in front of me", NULL, .flags = SET_ACTION | SET_GAME_ONLY, .apply = spawn },
    { SP_EXTRAS, "Teleport stage", V(g_cheat.tp_stg), 0, 9, 1, .fmt = "%d", .help = "Uses the game's door transition; an invalid room or position may crash.", .group = "TELEPORT" },
    { SP_EXTRAS, "Teleport room", V(g_cheat.tp_rom), 0, 99, 1, .fmt = "%d" },
    { SP_EXTRAS, "Teleport position", V(g_cheat.tp_pos), 0, 31, 1, .fmt = "%d" },
    { SP_EXTRAS, "Current room", NULL, .flags = SET_ACTION | SET_GAME_ONLY | SET_NOW, .apply = here, .help = "Fills in the room you are in." },
    { SP_EXTRAS, "Teleport", NULL, .flags = SET_ACTION | SET_GAME_ONLY, .apply = teleport },
};
const int g_settings_n = (int)SDL_arraysize(g_settings);

/* ---------------------------------------------------------------- access
   Both menus write the variables directly (aligned ints: a reader sees the old or the new
   value). What must not race is the hand-off: the writer's store happens before its flag
   (release), settings_poll takes the flag (acquire) before it applies and saves, so the main
   thread always sees the value that raised the flag. */
static unsigned char dirty[256];
static int dirty_any;

int settings_get(const Setting* s) { return s->get ? s->get() : s->val ? *s->val : 0; }
int settings_hi(const Setting* s) { return s->count ? s->count() - 1 : s->hi; }
int settings_is_bool(const Setting* s) { return !(s->flags & SET_ACTION) && s->lo == 0 && s->hi == 1 && !s->names && !s->name && !s->fmt; }

void settings_set(const Setting* s, int v)
{
    int hi = settings_hi(s), i = (int)(s - g_settings);
    if (v < s->lo) v = s->lo;
    if (v > hi) v = hi;
    if (v == settings_get(s)) return;
    if (s->set) s->set(v); else if (s->val) *s->val = v;
    if (i >= 0 && i < (int)sizeof(dirty)) __atomic_store_n(&dirty[i], 1, __ATOMIC_RELEASE);
    __atomic_store_n(&dirty_any, 1, __ATOMIC_RELEASE);
}

void settings_step(const Setting* s, int dir)
{
    int v = settings_get(s), hi = settings_hi(s), st = s->step > 0 ? s->step : 1;
    int choice = s->names || (s->name && s->name != tenths) || settings_is_bool(s);   /* wrap around */
    if (s->flags & SET_ACTION) return;
    if (v < s->lo) v = s->lo - (dir > 0 ? st : 0);   /* not a listed value (window size) */
    v = (v / st + dir) * st;   /* onto the step grid */
    if (choice && v > hi) v = s->lo;
    if (choice && v < s->lo) v = hi;
    settings_set(s, v);
}

void settings_act(const Setting* s) { if (s->apply) s->apply(); }

const char* settings_value(const Setting* s, char* b, int n)
{
    int v = settings_get(s);
    if (s->flags & SET_ACTION) b[0] = 0;
    else if (s->name) s->name(v, b, n);
    else if (s->names) snprintf(b, n, "%s", s->names[v - s->lo]);
    else if (s->fmt) snprintf(b, n, s->fmt, v);
    else snprintf(b, n, "%s", v ? "On" : "Off");
    return b;
}

const char* settings_locked(const Setting* s)
{
    const char* r = s->locked ? s->locked() : NULL;
    if (!r && (s->flags & SET_GAME_ONLY) && !pc_in_game()) r = "In game only.";
    return r;
}

void settings_save_later(void) { __atomic_store_n(&dirty_any, 1, __ATOMIC_RELEASE); }

/* recvx.ini is written SAVE_DELAY_MS after the last change (a slider drag changes the value
   every frame), and at exit if that is still due */
#define SAVE_DELAY_MS 500
static int save_pending;
static void save_at_exit(void) { if (__atomic_exchange_n(&save_pending, 0, __ATOMIC_ACQ_REL)) config_save(); }

/* main thread, once per pumped frame: changes from either menu are applied here (F1 also
   applies its own widgets at once) and recvx.ini is written once they stop. This and the exit
   are the only places config_save() runs once the game thread is up. */
void settings_poll(void)
{
    static int stress = -1, n;
    static uint64_t save_at;
    if (stress < 0) { stress = SDL_getenv("RECVX_STRESS_SETTINGS") != NULL; atexit(save_at_exit); }
    if (stress)   /* test aid: both volumes change every pumped frame (applied and saved below) */
        for (int i = 0; i < g_settings_n; i++)
            if (g_settings[i].val == &g_config.music_volume || g_settings[i].val == &g_config.sfx_volume)
                settings_set(&g_settings[i], ++n * 37 % 101);
    if (__atomic_exchange_n(&dirty_any, 0, __ATOMIC_ACQ_REL)) {
        for (int i = 0; i < g_settings_n && i < (int)sizeof(dirty); i++)
            if (__atomic_exchange_n(&dirty[i], 0, __ATOMIC_ACQ_REL))
                if (g_settings[i].apply && !(g_settings[i].flags & SET_ACTION)) g_settings[i].apply();
        save_at = SDL_GetTicks() + SAVE_DELAY_MS;
        __atomic_store_n(&save_pending, 1, __ATOMIC_RELEASE);
    }
    if (save_at && SDL_GetTicks() >= save_at) {
        save_at = 0;
        save_at_exit();
    }
}
