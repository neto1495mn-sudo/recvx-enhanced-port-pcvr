/* User settings: <save_dir>/recvx.ini, "key = value" lines. */
#include "xr.h"
extern int g_vr_scale, g_vr_eye_height, g_vr_fov, g_vr_turn, g_vr_hand_aim, g_vr_fx_fix, g_vr_gun_min, g_vr_hand_offset, g_vr_slash_speed, g_vr_hit_tilt, g_vr_danger_fx, g_vr_eye_shape, g_vr_screen_angle, g_vr_left_mirror, g_vr_dmg_head, g_vr_dmg_body, g_vr_dmg_arm, g_vr_dmg_leg, g_vr_recoil, g_vr_haptics, g_vr_hud_dist, g_vr_hud_angle, g_vr_sight, g_vr_sight_size, g_vr_holster, g_vr_hud_y, g_vr_roomscale, g_vr_manual_reload, g_vr_prompts, g_vr_dmg_body_max, g_vr_wake_shots, g_vr_precise_hits, g_vr_barrel_pitch, g_vr_hud_dist, g_vr_hud_angle, g_vr_aim_walk, g_vr_cam_steady, g_vr_move_direct, g_vr_heart_sound, g_vr_close_push, g_vr_bite_push, g_vr_comfort, g_update_check, g_tex_hd_per_frame, g_tex_loaders, g_tex_upload, g_vr_dyn_res, g_launcher, g_launcher_lang;
extern int g_vr_savespot;
extern int g_tex_hd, g_tex_hd_log, g_tex_hd_mb, g_tex_hd_max, g_fast_doors, g_vr_debug_log, g_vr_door_fade, g_vr_shadows, g_vr_debug_cores, g_vr_interpolate, g_vr_interp_body;   /* gs/texreplace.c */
static int g_vr_cfg_ver;   /* ini vr_cfg_ver: the VR settings' version, to move older saved values to new defaults */   /* pc/glue/vr_glue.c */
#include "../ps2_platform.h"
#include "coop.h"
#include "actors.h"
#include "tps.h"
#include "menu_mouse.h"
#include "enemies.h"
#include "bindings.h"
#include "modern.h"
#include "../net/rdv_proto.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static const struct { const char* key; int* val; int lo, hi; } keys[] = {
    { "window_w", &g_config.window_w, 320, 16384 },
    { "window_h", &g_config.window_h, 240, 16384 },
    { "fullscreen", &g_config.fullscreen, 0, 1 },
    { "vsync", &g_config.vsync, 0, 1 },
    { "fps_limit", &g_config.fps_limit, 0, 5 },
    { "scale", &g_config.scale, 0, 8 },
    { "aspect", &g_config.aspect, 0, 2 },
    { "show_fps", &g_config.show_fps, 0, 1 },
    { "show_ping", &g_config.show_ping, 0, 1 },
    { "music_volume", &g_config.music_volume, 0, 100 },
    { "sfx_volume", &g_config.sfx_volume, 0, 100 },
    { "screen_filter", &g_config.screen_filter, 0, 2 },
    { "cinema_bars", &g_config.cinema_bars, 0, 2 },
    { "screen_effects", &g_config.screen_effects, 0, 2 },
    { "brightness", &g_config.brightness, -50, 50 },
    { "contrast", &g_config.contrast, 50, 150 },
    { "gamma", &g_config.gamma, 5, 20 },
    { "skip_doors", &g_config.skip_doors, 0, 1 },
    { "fast_doors", &g_fast_doors, 0, 1 },            /* ps2/kernel.c */
    { "vr_door_fade", &g_vr_door_fade, 0, 1 },
    { "vr_shadows", &g_vr_shadows, 0, 1 },
    { "vr_debug_cores", &g_vr_debug_cores, 0, 1 },            /* glue/vr_glue.c */        /* gs/gs_gl.c: room changes fade in the headset */
    { "vr_debug_log", &g_vr_debug_log, 0, 1 },
    { "vr_savespot", &g_vr_savespot, 0, 1 },    /* glue/lobby_glue.c: the game starts in save 15 (hold X 2 s saves here, hold Y 2 s goes back to it) */
    { "vr_interp_body", &g_vr_interp_body, 0, 1 },   /* gs/gs_gl.c: the stick walking smoothed to the headset's rate */
    { "vr_interpolate", &g_vr_interpolate, 0, 1 },    /* gs/gs_gl.c: the characters smoothed to the headset's rate */        /* glue/vr_glue.c: the enemies' diary */
    { "quick_knife", &g_config.quick_knife, 0, 1 },
    { "model_cull", &g_config.model_cull, 0, 1 },
    { "skip_intro", &g_config.skip_intro, 0, 1 },
    { "battle_unlocked", &g_config.battle_unlocked, 0, 1 },
    { "no_ink_ribbon", &g_config.no_ink_ribbon, 0, 1 },   /* typewriters save without an ink ribbon (co-op: the host's) */
    { "save_system", &g_config.save_system, 0, 1 },     /* 0 native files, 1 memory card */
    { "native_3d", &g_config.native_3d, 0, 1 },
    { "native_tex", &g_config.native_tex, 0, 1 },
    { "interpolate", &g_config.interpolate, 0, 1 },
    { "frame_blend", &g_config.frame_blend, -1, 1 },
    { "coop_port", &g_coop.port, 1, 65535 },
    { "coop_char", &g_coop.chr, 0, COOP_CHR_MAX },
    { "appear_gender_anim", &g_appear_anim, 0, 1 },
    { "appear_cutscene_face", &g_appear_face, 0, 1 },
    { "coop_loot", &g_coop.loot, 0, 1 },
    { "coop_follow", &g_coop.follow, 0, 1 },
    { "coop_upnp", &g_coop.upnp, 0, 1 },
    { "coop_conn", &g_coop.conn, 0, 2 },                 /* COOP_CONN_*: Automatic, Direct, Via server */
    { "coop_nametag", &g_coop.nametag, 0, 1 },
    { "coop_trade_freeze", &g_coop.trade_freeze, 0, 1 },  /* host: enemies stand still while both trade */
    { "coop_items", &g_coop.items, COOP_ITEMS_CURRENT, 999999 },   /* COOP_ITEMS_*, else a save (lobby.h: card slot, native number - 1) */
    { "coop_remember", &g_coop.remember, 0, 1 },          /* host: saved partner inventories */
    { "coop_important", &g_coop.important, 0, 1 },        /* host, shared loot: weapons and ink ribbons for both */
    { "coop_skip_sync", &g_coop.skip_sync, 0, 1 },        /* host: a cutscene is skipped when both press skip */
    { "coop_start", &g_coop.start, 0, 1000002 },
    { "menu_mouse", (int*)&g_menu_mouse.on, 0, 1 },
    { "input_source", &g_input_source, 0, 2 },            /* INPUT_*: Auto, Keyboard/mouse only, Gamepad only */
    { "input_pad", &g_input_pad, 0, 4 },                  /* 0 any gamepad, n player n's (bindings.c) */
    { "fixed_cam_controls", &g_modern_on, 0, 1 },        /* fixed cameras: 0 classic (tank), 1 modern (camera-relative) */
    { "enemy_extra", &g_enemy_opts.extra, 0, 3 },        /* enemy options (docs/ENEMIES.md "Enemy options") */
    { "enemy_place", &g_enemy_opts.place, 0, 1 },
    { "enemy_random", &g_enemy_opts.rand, 0, 1 },
    { "enemy_seed", &g_enemy_opts.seed, 0, 0x7FFFFFFF },
    { "vr", &g_vr_request, 0, 1 },                        /* VR output (host/xr.c) */
    { "vr_dist", &g_vr_screen_dist, 50, 2000 },
    { "vr_width", &g_vr_screen_width, 50, 3000 },
    { "vr_stereo", &g_vr_stereo, 0, 1 },
    { "vr_scale", &g_vr_scale, 20, 500 },
    { "vr_eye_height", &g_vr_eye_height, 50, 400 },
    { "vr_fov", &g_vr_fov, 60, 170 },
    { "vr_turn", &g_vr_turn, 0, 90 },
    { "vr_hand_aim", &g_vr_hand_aim, 0, 1 },
    { "vr_fx_fix", &g_vr_fx_fix, 0, 1 },
    { "vr_res", &g_vr_res, 25, 200 },
    { "vr_gun_min", &g_vr_gun_min, 0, 60 },
    { "vr_hand_offset", &g_vr_hand_offset, -30, 30 },
    { "vr_slash_speed", &g_vr_slash_speed, 0, 1000 },
    { "vr_hit_tilt", &g_vr_hit_tilt, 0, 20 },
    { "vr_danger_fx", &g_vr_danger_fx, 0, 1 },
    { "vr_heart_sound", &g_vr_heart_sound, 0, 100 },   /* vr112: the heartbeat with the Danger / poison edge, volume; 0 = none */
    { "vr_close_push", &g_vr_close_push, 0, 60 },
    { "vr_bite_push", &g_vr_bite_push, 0, 15 },
    { "vr_comfort", &g_vr_comfort, 0, 3 },
    { "update_check", &g_update_check, 0, 1 },
    { "launcher_lang", &g_launcher_lang, 0, 2 },     /* vr134: the launcher's language (0 English, 1 Portuguese, 2 Spanish) */
    { "launcher", &g_launcher, 0, 1 },                /* vr120: the launcher window at start (host.c, overlay.cpp) */        /* vr119: at start, ask GitHub whether a newer VR release is out (host/update.c) */
    { "vr_dyn_res", &g_vr_dyn_res, 0, 1 },            /* vr162: dynamic resolution in heavy places */
    { "tex_loaders", &g_tex_loaders, 4, 6 },          /* vr160: threads reading the HD files */
    { "tex_upload_thread", &g_tex_upload, 0, 2 },     /* vr160: HD upload on a thread of its own: 0 by the card, 1 on, 2 off */
    { "tex_hd_per_frame", &g_tex_hd_per_frame, 1, 4 }, /* vr119: HD textures converted (mipmaps) per frame - fewer: lighter, longer stutter */            /* vr116: dark edges while walking with the stick / turning: 0 off, 1 weak, 2 medium, 3 strong */       /* vr115: each bite pushes the camera back this many cm; 0 = off */     /* vr112: the camera keeps an enemy's body this many cm from the eyes; 0 = off */
    { "vr_eye_shape", &g_vr_eye_shape, 0, 1 },
    { "vr_screen_angle", &g_vr_screen_angle, 20, 120 },
    { "vr_left_mirror", &g_vr_left_mirror, 0, 3 },
    { "vr_dmg_head", &g_vr_dmg_head, 0, 1000 },
    { "vr_dmg_body", &g_vr_dmg_body, 0, 1000 },
    { "vr_dmg_body_max", &g_vr_dmg_body_max, 0, 1000 },
    { "vr_wake_shots", &g_vr_wake_shots, 0, 1 },
    { "vr_cfg_ver", &g_vr_cfg_ver, 0, 1000 },
    { "vr_precise_hits", &g_vr_precise_hits, 0, 1 },
    { "vr_barrel_pitch", &g_vr_barrel_pitch, -300, 300 },
    { "vr_dmg_arm", &g_vr_dmg_arm, 0, 1000 },
    { "vr_dmg_leg", &g_vr_dmg_leg, 0, 1000 },
    { "vr_recoil", &g_vr_recoil, 0, 1 },
    { "vr_sight", &g_vr_sight, 0, 3 },   /* 1 ring, 2 laser, 3 both (vr77) */
    { "vr_sight_size", &g_vr_sight_size, 5, 400 },
    { "vr_holster", &g_vr_holster, 0, 1 },
    { "vr_roomscale", &g_vr_roomscale, 0, 1 },
    { "vr_aim_walk", &g_vr_aim_walk, 0, 1 },
    { "vr_cam_steady", &g_vr_cam_steady, 0, 1 },
    { "vr_move_direct", &g_vr_move_direct, 0, 1 },   /* glue/tps_glue.c: VR turns at once to the stick's way (vr85) */   /* glue/vr_glue.c: the camera off the game's own small moves (vr75) */
    { "vr_manual_reload", &g_vr_manual_reload, 0, 1 },
    { "vr_prompts", &g_vr_prompts, 0, 1 },
    { "vr_hud_y", &g_vr_hud_y, -100, 100 },
    { "vr_haptics", &g_vr_haptics, 0, 1 },
    { "vr_hud_dist", &g_vr_hud_dist, 30, 1000 },
    { "vr_hud_angle", &g_vr_hud_angle, 0, 120 },
    { "tex_hd", &g_tex_hd, 0, 1 },                        /* HD texture packs for PCSX2 (gs/texreplace.c) */
    { "tex_hd_log", &g_tex_hd_log, 0, 1 },
    { "tex_hd_mb", &g_tex_hd_mb, 64, 4000 },
    { "tex_hd_max", &g_tex_hd_max, 64, 4096 },
    { "tps", (int*)&g_tps.on, 0, 1 },
    { "tps_fov", &g_tps.fov, 30, 130 },
    { "tps_aim_fov", &g_tps.aim_fov, 20, 130 },
    { "tps_distance", &g_tps.dist, 0, 600 },
    { "tps_aim_distance", &g_tps.aim_dist, 0, 600 },
    { "tps_height", &g_tps.height, 0, 300 },
    { "tps_shoulder", &g_tps.shoulder, 0, 200 },
    { "tps_left_shoulder", &g_tps.left, 0, 1 },
    { "tps_sens_x", &g_tps.sens_x, 1, 1000 },
    { "tps_sens_y", &g_tps.sens_y, 1, 1000 },
    { "tps_aim_sens", &g_tps.aim_sens, 1, 300 },
    { "tps_invert_y", &g_tps.invert_y, 0, 1 },
    { "tps_crosshair", &g_tps.crosshair, 0, 1 },
    { "tps_auto_aim", &g_tps.auto_aim, 0, 1 },
    { "tps_atmos", &g_tps.atmos, 0, 1 },
    { "tps_free_camera", &g_tps.free_cam, 0, 1 },
    { "tps_key", &g_tps.key, 1, 511 },
};

static void default_name(void);
static int player_id(void);

/* hex text <-> bytes (the player id and secret) */
static int unhex(const char* s, unsigned char* out, int n)
{
    for (int i = 0; i < n; i++) {
        unsigned v;
        if (!SDL_isxdigit((unsigned char)s[2 * i]) || !SDL_isxdigit((unsigned char)s[2 * i + 1]) || sscanf(s + 2 * i, "%2x", &v) != 1) return 0;
        out[i] = (unsigned char)v;
    }
    return 1;
}
static void hex(FILE* f, const unsigned char* b, int n) { for (int i = 0; i < n; i++) fprintf(f, "%02x", b[i]); }
static int nonzero(const unsigned char* b, int n) { while (n--) if (*b++) return 1; return 0; }

static char* trim(char* s)
{
    size_t n = strlen(s);
    while (n && s[n - 1] == ' ') s[--n] = 0;
    return s;
}

static void path(char* out, size_t n) { SDL_snprintf(out, n, "%s/recvx.ini", g_config.save_dir); }
const char* pc_save_dir(void) { return g_config.save_dir; }   /* the settings/save folder (vr_glue.c keeps its hand grip there) */

void config_load(void)
{
    char p[1100], line[1100], k[64], val[128];
    int v;
    FILE* f;
    path(p, sizeof(p));
    if (!(f = pc_fopen(p, "r"))) { g_vr_cfg_ver = 26; default_name(); player_id(); return; }
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, " coop_join = %127s", g_coop.join) == 1) continue;
        else if (sscanf(line, " coop_player_id = %127s", val) == 1) { if (!unhex(val, g_coop.pid, 16)) memset(g_coop.pid, 0, 16); continue; }
        else if (sscanf(line, " coop_player_key = %127s", val) == 1) { if (!unhex(val, g_coop.pkey, 32)) memset(g_coop.pkey, 0, 32); continue; }
        else if (!strncmp(line, "coop_server =", 13)) { if (sscanf(line + 13, " %127s", g_coop.server) != 1) g_coop.server[0] = 0; continue; }   /* empty: no server */
        else if (sscanf(line, " coop_name = %15[^\r\n]", g_coop.name) == 1) continue;
        else if (sscanf(line, " iso = %1023[^\r\n]", g_config.iso_path) == 1) continue;
        else if (sscanf(line, " %63[a-z_0-9] = %127[^\r\n]", k, val) == 2 && bind_ini_line(k, trim(val))) continue;
        else if (sscanf(line, " %63[a-z_0-9] = %d", k, &v) == 2)
            for (size_t i = 0; i < SDL_arraysize(keys); i++)
                if (!strcmp(k, keys[i].key) && v >= keys[i].lo && v <= keys[i].hi) *keys[i].val = v;
    fclose(f);
    if (g_vr_cfg_ver < 25) {   /* VR defaults changed in vr25: damage by part and the text panel (an older file saved the old ones) */
        g_vr_dmg_head = 115; g_vr_dmg_body = 75; g_vr_dmg_body_max = 100; g_vr_dmg_arm = 50; g_vr_dmg_leg = 50;
        g_vr_hud_dist = 200; g_vr_hud_angle = 42;
    }
    if (g_vr_cfg_ver < 26 && g_vr_dmg_head == 125) g_vr_dmg_head = 115;   /* vr217: the head's damage 115% (was 125: the old default moves) */
    g_vr_cfg_ver = 26;
    g_coop.chr = appear_canon(g_coop.chr);   /* an event double no longer listed: its listed twin (actors.c) */
    default_name();
    player_id();
}

/* Co-op: this installation's player id and secret (docs/COOP.md "Saved partner inventories"),
   drawn once from the OS's randomness and written to recvx.ini at once. The host keeps a
   partner's items under its id; the secret makes the rejoin tags (rdv_rejoin_tag) that let only
   this player take its own place back. (Two copies of the game sharing a save folder are one player.) */
static int player_id(void)
{
    if (nonzero(g_coop.pid, 16) && nonzero(g_coop.pkey, 32)) return 1;
    if (!rdv_random(g_coop.pid, 16) || !rdv_random(g_coop.pkey, 32)) { host_log("config: no randomness for the co-op player id\n"); return 0; }
    config_save();
    return 1;
}

/* the co-op name before one is chosen: the login name; only what the game's font can show */
static void default_name(void)
{
    const char* u = SDL_getenv("USER");
    if (!u || !*u) u = SDL_getenv("USERNAME");
    if (!g_coop.name[0]) SDL_snprintf(g_coop.name, sizeof(g_coop.name), "%s", u && *u ? u : "Player");
    for (char* d = g_coop.name; *d; d++)
        if (!(SDL_isalnum((unsigned char)*d) || strchr(" -_.!?'()+", *d))) *d = '_';
}

static int write_ini(FILE* f, void* ud)
{
    (void)ud;
    for (size_t i = 0; i < SDL_arraysize(keys); i++) fprintf(f, "%s = %d\n", keys[i].key, *keys[i].val);
    fprintf(f, "coop_join = %s\n", g_coop.join);
    fprintf(f, "coop_server = %s\n", g_coop.server);
    if (g_coop.name[0]) fprintf(f, "coop_name = %s\n", g_coop.name);
    if (nonzero(g_coop.pid, 16)) {   /* who we are in co-op: made once; the key never leaves this file */
        fprintf(f, "coop_player_id = "); hex(f, g_coop.pid, 16);
        fprintf(f, "\ncoop_player_key = "); hex(f, g_coop.pkey, 32);
        fprintf(f, "\n");
    }
    if (g_config.iso_path[0]) fprintf(f, "iso = %s\n", g_config.iso_path);
    bind_ini_write(f);
    return 0;
}

/* Main thread only (settings_poll, and main() before the game thread starts); everyone else
   asks with settings_save_later(). Written to a temporary file and renamed over recvx.ini, so a
   crash or full disk never leaves a truncated file. */
void config_save(void)
{
    char p[1100];
    SDL_CreateDirectory(g_config.save_dir);
    path(p, sizeof(p));
    if (pc_save_file(p, "w", write_ini, NULL) != 0) host_log("config: cannot write %s\n", p);
}
