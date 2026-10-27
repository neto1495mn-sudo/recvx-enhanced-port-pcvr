/* "You appear as" list (pc/src/host/actors.c): event doubles left out, every chr still valid. */
#include "../src/host/actors.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

int main(void)
{
    int n = appear_count();
    CHECK(n == COOP_CHR_MAX + 1 - 12);
    for (int i = 0; i < n; i++) {
        CHECK(appear_pos(appear_at(i)) == i && appear_canon(appear_at(i)) == appear_at(i));
        if (i) CHECK(appear_at(i) > appear_at(i - 1));   /* catalogue order: player files first */
    }
    for (int c = 0; c <= COOP_CHR_MAX; c++) CHECK(appear_canon(appear_canon(c)) == appear_canon(c));
    CHECK(appear_at(0) == 0 && appear_at(5) == 5);                       /* As played, the player files */
    CHECK(appear_canon(COOP_CHR_ACTOR + 0) == 1);                        /* Claire's cutscene double: Claire */
    CHECK(appear_canon(COOP_CHR_ACTOR + 7) == 4);                        /* a Wesker double: Wesker */
    CHECK(appear_canon(COOP_CHR_ACTOR + 18) == COOP_CHR_ACTOR + 17);     /* Tyrant 2: Tyrant */
    CHECK(appear_canon(COOP_CHR_MAX + 1) == 0 && appear_canon(-1) == 0);
    CHECK(appear_step(5, 1) == COOP_CHR_ACTOR + 8);                      /* after the player files: Alfred */
    CHECK(appear_step(0, -1) == COOP_CHR_MAX && appear_step(COOP_CHR_MAX, 1) == 0);
    CHECK(appear_step(COOP_CHR_ACTOR + 5, 1) == 5);                      /* from a hidden one (Wesker double): on from Wesker */
    CHECK(!strcmp(appear_name(1), "Claire"));
    /* gender-matched animations: Chris's for a man while Claire is played, Claire's for a woman while a man is */
    CHECK(actor_models[8].anim == ANIM_MALE && actor_models[12].anim == ANIM_FEMALE && actor_models[24].anim == ANIM_IDLE);   /* Alfred, Alexia, Cerberus */
    CHECK(appear_motion_chr(ANIM_MALE, 0) == 1 && appear_motion_chr(ANIM_MALE, 1) < 0 && appear_motion_chr(ANIM_MALE, 3) < 0);
    CHECK(appear_motion_chr(ANIM_FEMALE, 0) < 0 && appear_motion_chr(ANIM_FEMALE, 2) == 0);
    CHECK(appear_motion_chr(ANIM_IDLE, 0) < 0 && appear_motion_chr(ANIM_PLAYED, 0) < 0);
    CHECK(appear_part_anim(0) == ANIM_FEMALE && appear_part_anim(4) == ANIM_FEMALE && appear_part_anim(3) == ANIM_MALE);
    g_appear_anim = 0;
    CHECK(appear_motion_chr(ANIM_MALE, 0) < 0);
    g_appear_anim = 1;
    CHECK(appear_grab_motion(1, 406, 0, 1) == 431 && appear_grab_motion(1, 431, 1, 0) == 406);   /* zombie */
    CHECK(appear_grab_motion(3, 60, 0, 1) == 74 && appear_grab_motion(7, 31, 0, 1) == 34);       /* black widow, bat */
    CHECK(appear_grab_motion(1, 10, 0, 1) < 0 && appear_grab_motion(5, 406, 0, 1) < 0);          /* not a grab motion, no table */
    /* voices: own bank for the four player characters and their doubles, men Chris's, women Claire's */
    CHECK(appear_voice(0) < 0 && appear_voice(1) == 0 && appear_voice(2) == 1 && appear_voice(3) == 2 && appear_voice(4) == 3 && appear_voice(5) == 0);
    CHECK(appear_voice(COOP_CHR_ACTOR + 4) == 2 && appear_voice(COOP_CHR_ACTOR + 7) == 3 && appear_voice(COOP_CHR_ACTOR + 19) == 2);   /* Steve, Wesker, Monster Steve */
    CHECK(appear_voice(COOP_CHR_ACTOR + 8) == 1 && appear_voice(COOP_CHR_ACTOR + 13) == 1 && appear_voice(COOP_CHR_ACTOR + 17) == 1);  /* Alfred, Rodrigo, Tyrant */
    CHECK(appear_voice(COOP_CHR_ACTOR + 12) == 0 && appear_voice(COOP_CHR_ACTOR + 24) < 0 && appear_voice(COOP_CHR_MAX + 1) < 0);    /* Alexia, Cerberus */
    printf("appear: ok, %d of %d listed\n", n, COOP_CHR_MAX + 1);
    return 0;
}
