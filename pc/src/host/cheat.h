/* F2 cheat menu state, shared between the overlay (render thread) and the game thread
   (pc/glue/cheat_glue.c, applied once per game frame). */
#ifndef RECVX_CHEAT_H
#define RECVX_CHEAT_H
#ifdef __cplusplus
extern "C" {
#endif

enum { CHEAT_GIVE = 1, CHEAT_TELEPORT = 2, CHEAT_HEAL = 4, CHEAT_SPAWN = 8 };
#define CHEAT_ITEMS 156

typedef struct CheatState {
    /* set by the overlay */
    volatile int inf_health, inf_ammo;
    volatile int noclip;            /* 0 off, 1 through walls at the current height, 2 also up/down */
    int give_id, give_count, tp_stg, tp_rom, tp_pos;
    int spawn_idx;                  /* enemy spawner list entry (spawn_name) */
    volatile int request;           /* CHEAT_* bits, cleared by the game thread */
    /* published by the game thread */
    int hp, stg, rom, pos, player, slots;
    unsigned int inv[12];           /* [0] equipped slot index (0 = none), [1] knife slot, [2..] slots: (id << 16) | count */
    const char* result;             /* last action's outcome */
    char names[CHEAT_ITEMS][32];    /* item names from the game's message data, "" until loaded */
} CheatState;

extern CheatState g_cheat;
void cheat_frame(void);             /* game thread */
/* enemy spawner (pc/glue/spawn_glue.c): the list (constant, any thread) and the game-thread calls */
int spawn_count(void);
const char* spawn_name(int i);
const char* spawn_enemy(int i);     /* result message */
void spawn_frame(int game_frame);   /* RECVX_SPAWN, trace */

#ifdef __cplusplus
}
#endif
#endif
