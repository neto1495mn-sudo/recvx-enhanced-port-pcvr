/* Enemy and NPC models a player can appear as ("You appear as", docs/ACTORS.md "You appear as").
   The catalogue names where each model is found on the disc (room file and enemy record); the
   game thread loads it from there (pc/glue/actor_glue.c). No disc data lives here. */
#ifndef RECVX_ACTORS_H
#define RECVX_ACTORS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ActorModel {
    const char* name;
    unsigned short room;       /* RM_<room>.RDX, e.g. 1140 */
    unsigned char rec;         /* enemy record of the room that carries the model */
    unsigned char id, mdlver;  /* expected record type and model version (checked at load) */
    unsigned char anim;        /* driven by the player's skeleton (if a humanoid node map is found at
                                  load) with the motions of: ANIM_PLAYED the character played,
                                  ANIM_MALE / ANIM_FEMALE a man's (Chris's) / a woman's (Claire's)
                                  when "gender-matched animations" is on; ANIM_IDLE (0): plays its
                                  own motion `idle`, looped */
    unsigned char idle;
} ActorModel;

enum { ANIM_IDLE, ANIM_PLAYED, ANIM_MALE, ANIM_FEMALE };

#define ACTOR_MODELS 62
extern const ActorModel actor_models[];

/* "Use gender-matched animations" (ini appear_gender_anim, default 1): a model of the other sex
   than the character played moves with the motions of a player character of its own sex */
extern int g_appear_anim;
/* "Talking faces in cutscenes" (ini appear_cutscene_face, default 0, experimental): the cutscene
   double shown as a cast model moves its own face with the double's lip sync; a player file shown
   there is replaced by the same character's cutscene model (which has a face) */
extern int g_appear_face;
/* the character (ply_id) whose motion set a model with `anim` (ANIM_*) moves with while character
   `played` (0 Claire, 1 Chris, 2 Steve, 3 Wesker) is played: Chris for a man while Claire is
   played, Claire for a woman while a man is played, else -1 (the played character's own) */
int appear_motion_chr(int anim, int played);
int appear_part_anim(int file);       /* ANIM_* of player part file 10 + file (0..4) */
/* the player character (0..3) whose voice bank (CORE_<n>.SPQ) "You appear as" chr speaks with,
   or -1: the played character's own */
int appear_voice(int chr);
/* a grab motion `mtn` of character `from` in the set of enemy type `id` (the player plays the
   enemy's motions while grabbed): the same motion of character `to`, or -1 if not known */
int appear_grab_motion(int id, int mtn, int from, int to);

/* "You appear as" (g_coop.chr): 0 the character played, 1..5 the player files, then these */
#define COOP_CHR_ACTOR 6
#define COOP_CHR_MAX (COOP_CHR_ACTOR + ACTOR_MODELS - 1)

/* The list the menus show (F3, the lobby, ENHANCED SETTINGS): one entry per distinct look, the player
   files first. Event and cutscene doubles of a listed look are left out of the list only: their
   chr values stay valid (co-op, recvx.ini) and appear_canon maps them to the listed one. */
int appear_count(void);
int appear_at(int i);                 /* the chr listed at position i */
int appear_pos(int chr);              /* the position of chr (a hidden one: its listed twin's) */
int appear_canon(int chr);            /* chr, or the listed twin of a hidden one */
int appear_step(int chr, int dir);    /* the next (dir 1) or previous (-1) listed chr, wrapping */
const char* appear_name(int chr);
/* game thread (pc/glue/actor_glue.c). who: display slot 0 our player, 1 the co-op partner,
   ACTOR_DOUBLE the cutscene double of the character played. idx: catalogue entry, ACTOR_PART + n
   for player part file 10 + n, or -1. */
#define ACTOR_SLOTS 3
#define ACTOR_DOUBLE 2
#define ACTOR_PART 1000
int  actor_set(int who, int idx);       /* 1 = shown (a failed entry is not retried) */
int  actor_shown(int who);
struct BH_PWORK;
void actor_pose(int who, struct BH_PWORK* src);   /* after src's bhSetMotion and bhCalcModel */
void actor_draw(int who);               /* bhPutModel: the caller sets camera matrix, lights, textures state */
struct BH_PWORK* actor_work(int who);   /* its work (model, node matrices), NULL when none is shown */
int  actor_hand_free(int who, int node);   /* 1: the source's hand node (9, 13) is at the model's hand, which has no mesh of its own */
int  actor_head(int who, float* out);   /* world position of its head node (else its highest node); 0 when none is shown */
void actor_doubles(int idx);            /* at the hand-off: the appearance for the cutscene doubles */
void actor_double_tick(void);           /* every tick before the objects follow their links: the double's pose, hands, face */
/* before actor_pose(who, src) (slots 0, 1): src shows the mesh of player character `mesh` (0..3, a
   part-file mesh on the real skeleton; -1: a catalogue model is shown) and plays the motions of
   character `motion`. Gender-matched animations need it; without it src's own motions are used. */
void actor_source(int who, struct BH_PWORK* src, int mesh, int motion);
int  actor_put_body(int who, struct BH_PWORK* src);   /* draw src (bhPutModel), in its gender-matched pose if it has one */
void actor_textures_released(void);     /* njReleaseTextureAll */

/* game thread: where a catalogue-format model is in its room file (read and expanded into a buffer
   that stays valid until another room file is requested); used by the enemy spawner too */
typedef struct ActorData {
    const void* rec;                    /* its ETTY_WORK record */
    const unsigned char *mdl, *mtn;     /* its model chunk list, its type's motion list (NULL: none) */
    int mlen, nlen, ntex;
    const unsigned char* tex[16];       /* texture data per textured sub-model in chunk order (NULL: not found) */
} ActorData;
int actor_find(const ActorModel* am, ActorData* out);
const unsigned char* actor_fx_tex(int room, int fx);   /* room file `room`'s texture data for room effect fx, NULL: none */
/* game thread: player part files, for co-op (the partner, our own mesh), the lobby's previews and here.
   afs_read: file `file` of AFS partition `part` into buf (cap bytes); 1 = read (more than 4096 bytes:
   unused costumes are 11-byte placeholders). player_model_read: bhReadPlayerData for `who` from part
   file data `file` into the given buffers (plp, sys->memp, the player buffers and freemem swapped for
   it and put back). player_hair: Claire's hair or Wesker's piece objs[2] on `who`'s head node for
   character chr (as bhSetPlayer sets up sys->obwp[2]; none for the others); exp0/exp3: the hair
   simulation's work (NULL: bhObjClpn takes the room heap, for our own player only).
   player_weapon_read: bhReadWeaponData for `who` (wpnr_no = wpn) from weapon file data `file`: the hand
   objects objs[0..1] (as sys->obwp[0..1]; their mlwP must point at a model, whose textures it
   releases), models into wr/wl (32 KB each), motions into wmt (64 KB) and who->mnwP[100..]. */
struct O_WRK;
int  afs_read(int part, int file, void* buf, int cap);
void player_model_read(struct BH_PWORK* who, unsigned char* file, unsigned char* mdl, unsigned char* bmt, unsigned char* mth, unsigned char* zmt);
void player_weapon_read(struct BH_PWORK* who, struct O_WRK* objs, int wpn, unsigned char* file, unsigned char* wr, unsigned char* wl, unsigned char* wmt);
void player_hair(struct O_WRK* objs, struct BH_PWORK* who, int chr, unsigned char* exp0, unsigned char* exp3);
void actor_tex_free(int* entries, int* heap);      /* free texture entries and heap bytes (after compacting) */

#ifdef __cplusplus
}
#endif
#endif
