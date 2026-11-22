#ifndef _GAMEOVER_H_
#define _GAMEOVER_H_

#include "types.h"

typedef struct GV_WORK 
{
    // total size: 0x2C30
    NJS_POINT3COL p3c;  // offset 0x0, size 0x10
    NJS_POINT3 pt[256]; // offset 0x10, size 0xC00
    NJS_COLOR cl[256];  // offset 0xC10, size 0x400
    NJS_COLOR uv[256];  // offset 0x1010, size 0x400
    NJS_POINT3 cp[256]; // offset 0x1410, size 0xC00
    NJS_POINT3 op[256]; // offset 0x2010, size 0xC00
    float pz;           // offset 0x2C10, size 0x4
    float scl;          // offset 0x2C14, size 0x4
    float tfn;          // offset 0x2C18, size 0x4
    float tnn;          // offset 0x2C1C, size 0x4
    int cont_x;         // offset 0x2C20, size 0x4
    int count;          // offset 0x2C24, size 0x4
    int cur_ct;         // offset 0x2C28, size 0x4
    int exp_ct;         // offset 0x2C2C, size 0x4
} GV_WORK;

typedef struct GV_WRK 
{
    // total size: 0x10
    int frm;  // offset 0x0, size 0x4
    float px; // offset 0x4, size 0x4
    float py; // offset 0x8, size 0x4
    float pz; // offset 0xC, size 0x4
} GV_WRK;

void bhControlGameOver();
void bhSelectContinue();
void bhInitGameOver();
void bhMainGameOver();
void bhExitGameOver();
void bhDrawGameOver();

#endif
