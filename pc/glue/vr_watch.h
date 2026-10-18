/* VR watch (vr_watch.c): the rules the VR code's changes to the game must keep */
#pragma once

typedef struct VwFrame {
    float x, z;            /* her place (metres) */
    int ay;                /* her facing (game units, 65536 a turn) */
    int m0, m1, m2;        /* the game's player modes */
    int room, chr;         /* RM_ number, character */
    int event, message;    /* a scene running, a message on screen */
    int gun_rounds;        /* the gun in hand's rounds (-1 none) */
    int mag_pending;       /* a magazine out (B) not back yet */
    int mag_inv_ok;        /* ... and the inventory it came from is the one now */
    int fire_vr;           /* the VR sends the fire button */
    int fire_finger;       /* a trigger is pulled */
} VwFrame;

void vw_frame(const VwFrame* f);
void vw_undo(float metres, int m2);
void vw_pull_dropped(int side, int rounds);   /* rounds: the gun in hand's (0: empty, not counted) */
void vw_inv(int slot, unsigned item0, unsigned item1, unsigned n0, unsigned n1, unsigned type1, const char* who, int play);
