/* The catalogue of pc/src/host/actors.h: where each enemy / NPC model is on the disc. Room and
   record were picked by a census over all room files (the smallest room holding the model and,
   for a model with its own motions, its type's motion set; docs/ACTORS.md "You appear as"). */
#include "actors.h"

#define MA ANIM_MALE    /* a man's (Chris's while Claire is played) */
#define FE ANIM_FEMALE  /* a woman's (Claire's while a man is played) */
#define I ANIM_IDLE     /* its own idle motion */

int g_appear_anim = 1;
int g_appear_face = 0;

const ActorModel actor_models[] = {
    /* the story cast, moved by the player's motions (a man's or a woman's, section 8) */
    { "Claire (cutscene model)", 3150, 0, 91, 0, FE },
    { "Chris (cutscene model)", 9180, 2, 92, 0, MA },
    { "Chris (event model)", 9320, 1, 42, 0, MA },
    { "Steve (cutscene model)", 40, 0, 93, 0, MA },
    { "Steve (event model)", 1140, 0, 43, 0, MA },
    { "Wesker (cutscene model)", 9360, 2, 97, 0, MA },
    { "Wesker (event model)", 1002, 2, 49, 0, MA },
    { "Wesker (event model 2)", 8030, 0, 47, 0, MA },
    { "Alfred Ashford", 1020, 1, 94, 0, MA },
    { "Alfred Ashford 2", 3240, 2, 96, 0, MA },
    { "Alfred Ashford (wounded)", 9180, 1, 96, 1, MA },
    { "Alfred Ashford (event model)", 7030, 2, 44, 0, MA },
    { "Alexia Ashford", 9150, 2, 95, 0, FE },
    { "Rodrigo Juan Raval", 0, 1, 98, 0, MA },
    { "Man in a hazmat suit", 3130, 2, 99, 0, MA },
    { "Alexia (first form)", 5820, 1, 12, 0, FE },
    /* creatures: humanoids moved by the player's motions (a man's), the others playing their own idle */
    { "Nosferatu", 5800, 0, 15, 0, MA },
    { "Tyrant", 5840, 0, 19, 0, MA },
    { "Tyrant 2", 5010, 0, 19, 1, MA },
    { "Monster Steve", 9280, 0, 17, 0, MA },
    { "Hunter", 8010, 0, 5, 0, MA },
    { "Hunter (Sweeper)", 9130, 0, 5, 1, MA },
    { "Bandersnatch", 2000, 0, 9, 0, MA },
    { "Bandersnatch 2", 3130, 0, 9, 1, MA },
    { "Cerberus", 3020, 2, 4, 0, I },
    { "Anatomist zombie", 110, 4, 26, 0, MA },
    { "Albinoid (adult)", 7100, 0, 22, 0, I },
    { "Albinoid (infant)", 3120, 0, 21, 0, I },
    { "Black Widow", 7080, 0, 3, 0, I },
    { "Giant Black Widow", 9050, 0, 23, 0, I },
    { "Moth", 6030, 3, 6, 0, I },
    { "Bat", 130, 0, 7, 0, I },
    { "Fish", 5510, 0, 8, 0, I },
    { "Spotter", 7081, 0, 11, 0, I },
    { "Ant", 9170, 12, 20, 0, I },
    { "Zombie: prisoner", 5520, 0, 1, 0, MA },
    { "Zombie: prisoner 2", 150, 0, 1, 1, MA },
    { "Zombie: prisoner 3", 5520, 1, 1, 31, MA },
    { "Zombie: soldier", 1070, 0, 1, 2, MA },
    { "Zombie: soldier 2", 1070, 1, 1, 3, MA },
    { "Zombie: soldier 3", 9120, 1, 1, 4, MA },
    { "Zombie: soldier 4", 9310, 1, 1, 21, MA },
    { "Zombie: soldier 5", 5680, 3, 1, 32, MA },
    { "Zombie: soldier 6", 5680, 0, 1, 33, MA },
    { "Zombie: soldier 7", 9310, 0, 1, 34, MA },
    { "Zombie: lab coat", 3200, 0, 1, 5, MA },
    { "Zombie: lab coat 2", 5520, 3, 1, 6, MA },
    { "Zombie: worker", 3200, 2, 1, 7, MA },
    { "Zombie: worker 2", 1100, 2, 1, 8, MA },
    { "Zombie: frozen", 3060, 1, 1, 9, MA },
    { "Zombie: frozen 2", 3060, 0, 1, 10, MA },
    { "Zombie: base staff", 6020, 1, 1, 35, MA },
    { "Zombie: base staff 2", 9020, 0, 1, 36, MA },
    { "Zombie: base staff 3", 5620, 1, 1, 42, MA },
    { "Zombie: civilian", 9190, 1, 1, 37, MA },
    { "Zombie: civilian 2", 9120, 0, 1, 38, MA },
    { "Zombie: hazmat suit", 3140, 7, 1, 39, MA },
    { "Zombie: dark uniform", 5700, 0, 1, 40, MA },
    { "Zombie: dark uniform 2", 8000, 3, 1, 41, MA },
    /* scripted zombies without motions of their own */
    { "Zombie: prisoner 4", 3000, 2, 51, 0, MA },
    { "Zombie: bandaged", 6040, 4, 52, 0, MA },
    { "Zombie: bandaged 2", 9050, 1, 52, 1, MA },
};
_Static_assert(sizeof(actor_models) / sizeof(actor_models[0]) == ACTOR_MODELS, "ACTOR_MODELS");

/* ---------------------------------------------------------------- gender-matched animations */
int appear_motion_chr(int anim, int played)
{
    if (!g_appear_anim) return -1;
    if (anim == ANIM_MALE && played == 0) return 1;      /* Chris's for a man while Claire is played */
    if (anim == ANIM_FEMALE && played != 0) return 0;    /* Claire's for a woman while a man is played */
    return -1;
}

int appear_part_anim(int file) { return file == 0 || file == 4 ? ANIM_FEMALE : ANIM_MALE; }

/* ---------------------------------------------------------------- voices
   The player's vocal sounds (SE bank 4: hit 1024/1026/1027, death 1025, grabs) come from
   CORE_<ply_id>.SPQ; the disc has one per player character (Claire, Chris, Steve, Wesker, same 4
   programs each). The story cast speaks with its own, other men with Chris's, women with Claire's;
   creatures that play their own idle motion keep the played character's (docs/ACTORS.md "Voices"). */
int appear_voice(int chr)
{
    static const signed char files[COOP_CHR_ACTOR] = { -1, 0, 1, 2, 3, 0 };
    int m = chr - COOP_CHR_ACTOR;
    if (chr < 0 || chr > COOP_CHR_MAX) return -1;
    if (m < 0) return files[chr];
    if (m >= 3 && m <= 4) return 2;   /* Steve's doubles */
    if (m >= 5 && m <= 7) return 3;   /* Wesker's */
    if (m == 19) return 2;            /* Monster Steve */
    return actor_models[m].anim == ANIM_MALE ? 1 : actor_models[m].anim == ANIM_FEMALE ? 0 : -1;
}

/* Enemies that grab the player play the player's grab motions from their own set, at a first
   motion per character (ply_id 0..3, from the enemy code's tables: en01.c En01_PlyMtn_OffsetTbl,
   en02.c / en13.c / en23.c mtn[3][8], en03.c 14 * ply_id, en04.c / en09.c *_PlyMtn_OffsetTbl,
   en07.c ply_motion; Wesker's row where the code has none: Chris's). */
static const struct { unsigned char id; short first[4]; } grabs[] = {
    { 1, { 400, 425, 450, 425 } },   /* zombie */
    { 2, { 21, 29, 29, 29 } },       /* gulp worm */
    { 3, { 0, 14, 28, 42 } },        /* black widow */
    { 4, { 100, 105, 100, 105 } },   /* cerberus */
    { 7, { 30, 33, 36, 33 } },       /* bat */
    { 9, { 100, 110, 120, 110 } },   /* bandersnatch */
    { 13, { 10, 16, 16, 16 } },      /* alexia (second form) */
    { 23, { 60, 66, 66, 66 } },      /* giant black widow */
};

int appear_grab_motion(int id, int mtn, int from, int to)
{
    for (int i = 0; i < (int)(sizeof(grabs) / sizeof(grabs[0])); i++)
        if (grabs[i].id == id && from >= 0 && from < 4 && to >= 0 && to < 4) {
            int k = mtn + grabs[i].first[to] - grabs[i].first[from];
            return mtn >= grabs[i].first[from] && k >= 0 && k < 512 ? k : -1;
        }
    return -1;
}

/* ---------------------------------------------------------------- "You appear as" list
   Left out of the menus (docs/ACTORS.md "The list"): event and cutscene doubles that
   look like an entry that stays. Their chr values keep working; each maps to the one kept. */
#define M(n) (COOP_CHR_ACTOR + (n))   /* chr of catalogue entry n */
static const struct { short hidden, kept; } twins[] = {
    { M(0), 1 },                         /* Claire's cutscene double: Claire's player file */
    { M(1), 2 }, { M(2), 2 },            /* Chris */
    { M(3), 3 }, { M(4), 3 },            /* Steve */
    { M(5), 4 }, { M(6), 4 }, { M(7), 4 },   /* Wesker (the doubles wear the glasses as a separate prop) */
    { M(9), M(8) }, { M(11), M(8) },     /* Alfred's other uniforms: the same look */
    { M(18), M(17) },                    /* Tyrant 2: the same with a blood stain */
    { M(23), M(22) },                    /* Bandersnatch 2: only its long arm (an event piece) */
};

int appear_canon(int chr)
{
    if (chr < 0 || chr > COOP_CHR_MAX) return 0;
    for (int i = 0; i < (int)(sizeof(twins) / sizeof(twins[0])); i++)
        if (twins[i].hidden && chr == twins[i].hidden) return twins[i].kept;
    return chr;
}

int appear_count(void)
{
    int n = 0;
    for (int c = 0; c <= COOP_CHR_MAX; c++) n += appear_canon(c) == c;
    return n;
}

int appear_at(int i)
{
    for (int c = 0; c <= COOP_CHR_MAX; c++)
        if (appear_canon(c) == c && i-- == 0) return c;
    return 0;
}

int appear_pos(int chr)
{
    int n = 0;
    chr = appear_canon(chr);
    for (int c = 0; c < chr; c++) n += appear_canon(c) == c;
    return n;
}

int appear_step(int chr, int dir)
{
    int n = appear_count();
    return appear_at((appear_pos(chr) + dir + n) % n);
}

const char* appear_name(int chr)
{
    static const char* files[COOP_CHR_ACTOR] = { "As played", "Claire", "Chris", "Steve", "Wesker", "Claire (2nd costume)" };
    return chr >= 0 && chr < COOP_CHR_ACTOR ? files[chr] : chr <= COOP_CHR_MAX ? actor_models[chr - COOP_CHR_ACTOR].name : "?";
}
