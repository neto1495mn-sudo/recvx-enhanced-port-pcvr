/* Enemy options (docs/ENEMIES.md "Enemy options"): extra enemies, rearranged placements, randomizer;
   shared with the co-op glue (the host's options and its added enemies go to the guest). */
#ifndef RECVX_ENEMIES_H
#define RECVX_ENEMIES_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct EnemyOpts {
    int extra;                  /* 0 off, 1..3: up to that many extra enemies per room (types the room has) */
    int place;                  /* 1: rearranged placements */
    int rand;                   /* 1: enemy randomizer */
    int seed;                   /* placements and randomizer (0..2^31-1) */
} EnemyOpts;
extern EnemyOpts g_enemy_opts;  /* our settings (F1, recvx.ini) */

/* an enemy that exists on the host but not in the room file: where its data is and where it stands */
typedef struct SpawnNote {
    unsigned short room;        /* source room file RM_<room>.RDX and its record */
    unsigned char rec, id, mdlver, type;
    signed char flr;
    unsigned char pad;          /* SPAWN_NOTE_KEPT: not a new enemy; the host's randomizer kept record `idx` of room
                                   file `room` (stg * 1000 + rom * 10 + case) as it is (type id, model mdlver) */
    float px, py, pz;
    short ay, pad2;
} SpawnNote;
#define SPAWN_NOTE_KEPT 1

/* spawn_at's `mode` (pc/glue/spawn_glue.c) */
#define SPAWN_LOADING 1             /* from the room load (the randomizer): no readiness checks */
#define SPAWN_DICTATED 2            /* the host's (a co-op guest follows it): no texture reserve, only what the upload needs */

/* enemy slots the spawner and the randomizer leave free for what enemies spawn at run time (moth
   eggs, damage dummies; bhSetEnemy has no NULL check in their callers) */
#define SPAWN_SLOTS_FREE 6
int  spawn_slots_needed(int id);                /* a type and the enemies its Init spawns */

/* game thread */
const EnemyOpts* enemy_opts(void);              /* in effect: ours, or the host's on a co-op guest */
int  room_file_now(void);                       /* the room file we are in: RM_<stg * 1000 + rom * 10 + case> */
void enemy_opts_from_host(const EnemyOpts* o);  /* co-op guest (NULL: none yet / session over) */
int  spawn_note_apply(int slot, const SpawnNote* n, int stg, int rom);   /* guest: placed now or once the room runs (1) */
void spawn_pending(int run);                    /* guest, every co-op frame (0: the session ended, forget them) */
void enemy_rand_kept(int room, int rec, int id); /* guest: the host's randomizer kept that record (SPAWN_NOTE_KEPT) */
void coop_rand_kept(int rec, const SpawnNote* n);   /* coop_glue.c, host: this room load's randomizer kept record rec */
void spawn_replay(void);                        /* host: every added enemy of the room again (coop_spawn_note) */
void coop_spawn_note(int slot, const SpawnNote* n);   /* coop_glue.c: host -> guest, if it is in our room */
int  coop_insn_len(const unsigned char* p);     /* coop_glue.c: a room script instruction's length (0: unknown) */

#ifdef __cplusplus
}
#endif
#endif
