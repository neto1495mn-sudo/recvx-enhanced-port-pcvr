/*
 * Force-included (-include) into every decomp translation unit of the PC build.
 *
 * The decomp targets MWCC/ee-gcc for the PS2 EE, where:
 *   - long is 64 bits (GS/GIF packet code relies on it; see override_katana.h)
 *   - 128-bit integer types exist
 * Host system headers are pulled in first, then 'long' is widened so the
 * decomp sees the same data model it was written for.
 */
#pragma once
#ifndef TARGET_PC
#define TARGET_PC 1
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#ifdef __ANDROID__
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#endif

/* CRI xpt: pre-empt the EE-only mode(TI) typedefs. */
#define _TYPEDEF_Uint64
#define _TYPEDEF_Sint64
#define _TYPEDEF_Uint128
#define _TYPEDEF_Sint128
typedef uint64_t Uint64;
typedef int64_t  Sint64;
typedef struct { uint32_t w[4]; } __attribute__((aligned(16))) Uint128;
typedef Uint128 Sint128;

/* EE 128-bit ints only appear in movie/DMA structs.
   ponytail: keyword macro, revisit if a 64-bit target needs real __int128 */
#define __int128 Uint128

/* decomp function names that collide with libc symbols */
#define index recvx_index
#ifdef _WIN32
/* The game assumes RAND_MAX 0x7fffffff (macros.h: njRandom() = -rand() / 2^31); UCRT's is
   0x7fff. Use glibc's generator instead: same range and the same sequence as the Linux build. */
int recvx_rand(void);
void recvx_srand(unsigned seed);
#define rand recvx_rand
#define srand recvx_srand
#endif

/* MWCC cast-as-lvalue idioms (see macros.h) */
static inline void* ps2_postinc(void** pp, size_t n) { void* p = *pp; *pp = (char*)p + n; return p; }
#define PS2_POSTINC(T, X) (*(T*)ps2_postinc((void**)&(X), sizeof(T)))
#define PS2_PTRADD(X, n) ((X) = (void*)((char*)(X) + (n)))

/* hor+ widescreen: horizontal extent of the view relative to 4:3 (1.0 = original) */
float pc_hor_scale(void);
float pc_clip_guard(void);
int   pc_is_room_object(const void* obj);   /* part of the current room's model */
/* rendering rework: chunk-model strips drawn natively (pc/glue/native_glue.c) */
int   pc_native_3d(void);
struct VU1_STRIP_BUF;
struct VU1_PRIM_BUF;
unsigned int pc_native_prim3d(unsigned int prim, const struct VU1_PRIM_BUF* in, int n, int deferred, float* zavg);
unsigned int pc_native_prim(unsigned int prim, const unsigned int* dp, int n, int clip_3d_on, int deferred, float* last_sz);
unsigned int pc_native_prim_raw(unsigned int prim, const unsigned int* dp, int n, int clip_3d_on, float* last_sz);
void  Ps2AddPrim3DExNative(unsigned long long prim, void* dp, unsigned int num, int mode);   /* 0 Ex, 1 Ex1P, 2 ExI */
#define NATIVE_MARKER_REG 0x7F
#define STREAM_TAG_REG 0x7E   /* gs/native.h */
/* native textures (pc/glue/texture_glue.c) */
int   pc_native_tex(void);
unsigned int pc_tex_begin(const void* tim2, const void* phead, unsigned int image_adr, unsigned int clut_adr, int* is_new);
void  pc_tex_end(unsigned int id, unsigned int image_adr, unsigned int clut_adr, int is_new);
int   pc_native_rtex(void);   /* StoreRenderTex on the GPU (RECVX_NATIVE_RTEX, default on) */
void  pc_rtex_capture(void* tim2, void* image, unsigned int w, unsigned int h);
extern int pc_rtex_wide;      /* set before StoreRenderTex: capture the hor+ widened view (the full-screen render texture) */
int   pc_rtex_bind(const void* image, unsigned int image_adr, unsigned int clut_adr);
void  pc_native_strip(const unsigned short* idx_uv, int n, const struct VU1_STRIP_BUF* vbuf, unsigned int prim,
                      int color_mode, int single, float uv_scale);   /* enhanced mode: clip volume enlarged into the GS guard band */
int   pc_screen_filter_mode(void);
int   pc_skip_door_demo(void);
/* mods folder (pc/src/audio/adx.c, docs/modding/README.md "The mods folder"): size of <mods>/name or -1; ADXF handle or NULL */
int   pc_mod_file_size(const char* name);
void* pc_mod_open(const char* name);
int   pc_model_cull(void);      /* actors/objects off-screen are skipped (bhCheckClipModel/Point) */
/* languages (pc/glue/lang_glue.c, docs/LANGUAGES.md): the language font's width of a glyph (< 0: the game's own);
   bhSetRoom's message table in the language */
float pc_font_width(unsigned short code);
float pc_font_space(void);      /* a space's (0xFF01) advance at scale 1 */
int   pc_font_glyph(int code, void** texlist, float* dx);   /* bhDispFont: the cell to draw (-1 none); *texlist another font's; *dx its shift */
const unsigned short* pc_lang_mes(int which, const unsigned short* us);   /* the executable's short texts (lang.h LM_*) */
int   pc_lang_file_page(float x, float y, int file, int page);   /* a file page as the language's picture (1 drawn) */
int   pc_lang_page_end(int us);   /* PlayPageCheck: the playing manual's hidden pages in the language's layout */
unsigned int* pc_lang_room_mes(unsigned int* mesp);
int   lang_picture(int adv);    /* the language of the ADV.AFS picture read last (0: English) */
int   lang_eu_picture(int adv); /* ... is a European one (its title layout) */
extern int pc_enemy_refused;       /* eneset.c: the enemy's code runs on a player another enemy holds (its changes are undone) */
int   pc_frame_blend(void);     /* PS2_swap blends the new frame 75% over the previous one (motion trail) */
unsigned int pc_noclip(void);   /* F2 cheat: player flags whose enemy/wall push bhControlPlayer skips */
int   pc_inf_ammo(void);      /* F2 cheat (pc/glue/cheat_glue.c): bhCheckBullet/bhCountBullet as for an unlimited weapon */
int   pc_skip_intro(void);               /* option: boot straight to the title menu */
int   pc_skip_intro_press_start(void);   /* ... and past "press start", once */     /* option: room transitions without the door/stairs animation */   /* camera filter/vignette: 0 original, 1 stretched, 2 off */
/* full-screen 2D overlays (cinematic bars, fades, screen effects) in hor+ widescreen:
   begin returns 0 if the overlay is switched off; inside, 2D polygon x is stretched if set */
enum { PC_OVL_CINEMA, PC_OVL_FADE, PC_OVL_EFFECT };
int   pc_overlay_begin(int kind);
void  pc_overlay_end(void);
float pc_overlay_x(float x);
/* enhanced frame interpolation: GS output between begin and end belongs to this mesh */
void  pc_mesh_begin(const void* model);
void  pc_mesh_end(void);
void  pc_mesh_strip(const void* strip);   /* within a mesh: the following output is this strip */
void  pc_effect_tag(const void* op);      /* effect.c: the following output is this effect (O_WRK*, NULL: none) */
unsigned long long pc_effect_poly(void); /* ps2_dummy.c Ps2AddOT: STREAM_TAG_REG value for an effect polygon (0: none) */
/* online co-op (pc/glue/coop_glue.c, docs/COOP.md): the hooks are only called while coop_on */
extern int coop_on;
void  coop_control_remote(void);
void  coop_draw_remote(void);
void  coop_textures_released(void);
struct BH_PWORK;
int   coop_enemy_begin(struct BH_PWORK* ep);   /* 1 = step done (client puppet) */
void  coop_enemy_end(struct BH_PWORK* ep);
void  coop_scd1(void);                         /* event.c bhControlEvent: runs the room's per-frame script */
void  coop_fx_player(void);                    /* game.c, before bhControlPlayer: our step's effects to the partner */
int   coop_fx_step(int i, int after);          /* effect.c bhControlEffect, around effect i's step (1 before: call after) */
void  coop_item_granted(int item);            /* sub1.c GetItem: an event gave us this item without asking */
void  coop_item_taken(int bit, int etc);      /* event.c bhItmCk: we took the item of trigger etc, its it_flg bit */
void  coop_item_late(int bit);                /* event.c bhItmCk: we took an item whose it_flg bit was set already (the partner was first) */
int   pc_no_ink_ribbon(void);                 /* typewriters save without an ink ribbon (setting; in co-op the host's rule) */
void  coop_fx_particle(struct BH_PWORK* ewP, int lnk, const void* offP, const void* dirP, unsigned int col, unsigned int dst, int typ, int mk2);   /* effsub3.c bhSetEffParticle(Mk2) */
/* "You appear as" (pc/glue/actor_glue.c): another model drawn in place of the player / of the
   played character's cutscene double; appear_on: in use without co-op (coop_control_remote runs) */
extern int actor_on, appear_on;
int   actor_put_player(void);                 /* 1 = drawn */
int   actor_put_enemy(struct BH_PWORK* ep);   /* 1 = drawn */
void  actor_hands_back(void);                 /* game.c, before bhControlPlayer: the source's own head and hands back */
/* PS2 budgets raised on the PC (docs/ENEMIES.md "Budgets on the PC"): the texture heap (the EE copies of the
   TIM2s the GS loads from; 10 MB on the PS2) and its entry table (256). Vanilla rooms use the same
   first entries and bytes as before; only what used to exit(0) now fits. */
#define PC_TEX_HEAP_SIZE (64 << 20)
#define PC_TEX_ENTRIES 1024
/* The room arena (freemem: persistent player data, then the room file and what it allocates;
   12.25 MB block on the PS2, ~11.7 MB of it arena) grows by this much (docs/modding/ROOM_FORMAT.md "Limits and budgets"). The
   buffers above it (njplus, vertex buffers) keep their sizes and move up; allocations from the
   bottom keep their offsets, so vanilla rooms are laid out as before and only the packed room
   file is read higher up. Costs address space only (bss, pages never touched stay free), but
   inside the PS2 address window: the whole image must end below 0x40000000 (ps2/memory.c). */
#define PC_ARENA_EXTRA (64 << 20)
/* The vertex work buffer above it (vwbmemp, njInit3D: 64-byte entries, one per vertex of the node
   being drawn): 5120 entries fit on the PS2 although njInit3D is told 8192; the PC adds the rest. */
#define PC_VERTEX_EXTRA ((8192 - 5120) * 64)
/* Ordering table of translucent/sorted primitives per frame (ps2_dummy.c Ps2AddOT): 8192 entries
   and a 1.75 MB packet buffer on the PS2 (a native-path entry takes 128 bytes of it), unchecked. */
#define PC_OT_ENTRIES 32768
#define PC_PBUFF_SIZE (4608 << 10)
/* ROM_WORK.flg bit that rdxbuild sets in "budget": "pc" rooms (retail rooms use bits 0..1): the
   player (and every actor through bhCheckWallEx) stands on type-6 slope walls, its y read from
   the slope at its x/z (hitchk.c pc_slope_ground); the camera and the co-op partner follow that
   y. Retail rooms never have it: unchanged. docs/modding/ROOM_FORMAT.md "Floors and heights". */
#define PC_ROOM_SLOPES 0x100
struct BH_PWORK;
int   pc_slope_ground(struct BH_PWORK* pw, void* pos, float ah);   /* hitchk.c: 1 = on a slope, pos y + floor set */
void  host_fatal(const char* fmt, ...);
void  host_log(const char* fmt, ...);
void  pc_et_lp_clear(void);   /* room.c: sys->et_lp rows of model versions >= 16 */
/* F2 enemy spawner (pc/glue/spawn_glue.c): its textures go with the room's / with everything */
void  spawn_release(void);
void  spawn_textures_released(void);
void  spawn_room_setup(void);      /* room.c bhSetRoom: the spawned types' data go with the room */
extern int pc_enemy_added;         /* eneset.c: the room got enemies its file does not have (helper code per type) */
void  enemy_room_finished(void);   /* room.c bhFinishRoom: enemy options (pc/glue/enemyopt_glue.c) */

/* title menu MULTIPLAYER entry and the co-op lobby screen (pc/glue/lobby_glue.c) */
void  pc_title_menu_init(void);                  /* Adv_BioCvTitle mode 0: the MULTIPLAYER row */
void  pc_title_plates(int cursor, int n, float fade, int flag);   /* DisplayGameModePlate */
void  pc_title_mouse(int level, int flag);       /* CheckButton: hover moves the cursor, a click confirms */
void  pc_title_mouse_active(int on);             /* a title menu or the lobby is up: the OS cursor is shown */
int   pc_quickload_take(void);                   /* F10: the title loads save 15 (test aid) */
int   pc_lobby(void);                            /* Adv_BioCvTitle mode 30: 1 back, 2 new game, 3 load the chosen save */
void  pc_lobby_close(void);                      /* the title ends */
void  pc_lobby_load_save(void);                  /* bhSysCallFirstmovie return code 8 */
void  pc_lobby_textures_released(void);          /* njReleaseTextureAll */
void  pc_title_leave(void);                      /* co-op: the host's game is joined from the title */
void  pc_adv_in_game(void);                      /* co-op: the guest is in a game now (AdvWork.AppMode 1) */

/* ENHANCED SETTINGS in the OPTION menu (pc/glue/options_glue.c): option level 4 */
void  pc_options_frame(void);                    /* Adv_GameOptionScreen, every frame */
void  pc_options_close(void);                    /* Adv_GameOptionScreen ends */
void  pc_options_row8(int selected);             /* DisplayOptionPlateLevel0: ENHANCED SETTINGS, right of EXIT */
void  pc_options_mouse0(int port);               /* DisplayOptionPlateLevel0: hover, click */
void  pc_options_mouse_back(int port, int level);/* levels 1-3: right click goes back */
int   pc_options_pc(int port, int flag);         /* DisplayOptionPlate level 4 */

/* native saves (pc/glue/save_glue.c, docs/ARCHITECTURE.md "Save system"): the save and load
   screens list save files instead of memory card slots and skip the card screens */
struct SELECTFILEWINDOW;
struct MEMORYCARDSTATE;
struct SYSLOAD_SCREEN;
struct SYSSAVE_SCREEN;
int   pc_saves_native(int state);                /* the open save/load screen uses native saves; its state 0 takes the setting, -1 asks */
int   pc_save_card(struct MEMORYCARDSTATE* mc);  /* native, instead of ExecuteMemoryCard: its queued read or write done with the save files; 100 */
void  pc_save_list(struct SELECTFILEWINDOW* win, struct MEMORYCARDSTATE* mc, int saving);   /* the window's rows: the saves (+ a new one) */
void  pc_save_move(struct SELECTFILEWINDOW* win, short* cursor, int dir);   /* the cursor up/down through any number of rows */
int   pc_sysload_native(struct SYSLOAD_SCREEN* p);   /* the system file: 0 memory card, else the screen's result (1 none, 2 read) */
int   pc_syssave_native(struct SYSSAVE_SCREEN* p);   /* 0 memory card, else the screen's result (1 failed, 2 written) */

/* mouse in the inventory screens (pc/glue/menu_glue.c) */
unsigned int menu_pad(unsigned int pad);
/* the co-op trade screen (pc/glue/trade_glue.c, docs/COOP.md "Trade screen") */
unsigned int selmenu_pad(unsigned int pad);   /* vr145: pad.c bhSetPad, after trade_pad: Select pauses with a menu (options_glue.c) */
int pc_selmenu_draw(void);
void pc_title_exit(void);
void pc_title_font_leave(void);   /* vr146: adv.c, a game mode chosen at the title: its font released (lobby_glue.c) */   /* vr145: adv.c, the title menu's EXIT row (lobby_glue.c) */   /* vr145: system.c, the pause screen: 1 = the menu drew there */
unsigned int trade_pad(unsigned int pad);   /* pad.c bhSetPad, after menu_pad: the screen's input, the player stands still */
void  trade_draw(void);                     /* game.c bhMainSequence, after the frame's drawing */
int   pc_trade_screen(void);                /* the screen is open (menu_pad leaves the mouse to it) */
void  trade_textures_released(void);        /* njReleaseTextureAll */
/* quick knife (pc/glue/knife_glue.c) */
unsigned int knife_pad(unsigned int pad);
/* modern controls for the fixed cameras (pc/glue/modern_glue.c): nothing while they are off */
unsigned int modern_pad(unsigned int pad);   /* pad.c bhSetPad, before knife_pad */
void  modern_player(void);                    /* player.c bhControlPlayer, after the mode handler */
struct STCAM_WRK;
void  menu_examine(struct STCAM_WRK* sc);

/* third-person mode (pc/glue/tps_glue.c, docs/THIRDPERSON.md): hooks only called while tps_on */
extern int tps_on;
extern int tps_cam_active;   /* the third-person camera is the current view */
void  tps_camera(void);
void  tps_after_draw(void);
void  tps_after_cut(void);
void  tps_atmos(void);
struct O_WRK;
int   tps_filter(struct O_WRK* op);
unsigned int tps_pad(unsigned int pad);
int   pc_input_active(void);   /* any PC-side input this frame (bhSysCallScreenSaver) */
void  tps_player_mode(void);   /* before the player's mode handler: the straight aim height back (free aim) */
void  tps_player(void);
void  tps_calc_model(void);
struct BH_PWORK;
void  tps_bend(struct BH_PWORK* p);   /* the last free-aim bend on a display copy of the player (actor_glue.c) */
#include "ps2sdk_shim.h"
#include "ps2_hw.h"
#ifndef PS2_WINDOW
/* the PS2 address window the image is loaded in (docs/ARCHITECTURE.md "Data model and memory"): the EE's
   accelerated alias addr|PS2_WINDOW is an identity on every game object. PC: linked at 0x30000000. Android (Quest):
   0x30000000 is the Java heap in an app process - loaded at 0x40000000 (android/loader.c) */
#ifdef __ANDROID__
#define PS2_WINDOW 0x40000000u
#else
#define PS2_WINDOW 0x30000000u
#endif
#define PS2_WINDOW_END (PS2_WINDOW + 0x10000000u)
#endif
#define u_long ps2_u_long
#ifdef __SSE2__
#include <emmintrin.h>   /* for vu_simd.h (vu0.h), which game files include after the define below */
#endif

/* EE data model: long is 64-bit. Must come after every host header. */
#define long long long
