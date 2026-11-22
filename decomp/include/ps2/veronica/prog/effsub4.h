#ifndef _EFFSUB4_H_
#define _EFFSUB4_H_

#include "types.h"

typedef struct EFF370_WORK 
{
    // total size: 0x14
    float spd;   // offset 0x0, size 0x4
    float e;     // offset 0x4, size 0x4
    float g;     // offset 0x8, size 0x4
    short id;    // offset 0xC, size 0x2
    short col;   // offset 0xE, size 0x2
    short wcnt;  // offset 0x10, size 0x2
    short lmode; // offset 0x12, size 0x2
} EFF370_WORK;

typedef struct D_WORK
{
    // total size: 0x14
    unsigned int flg;    // offset 0x0, size 0x4
    unsigned int num;    // offset 0x4, size 0x4
    unsigned char* exp0; // offset 0x8, size 0x4
    unsigned char* exp1; // offset 0xC, size 0x4
    unsigned char* exp2; // offset 0x10, size 0x4
} D_WORK;

typedef struct EFFSUB365_WORK 
{
    // total size: 0x474
    D_WORK dtbl;            // offset 0x0, size 0x14
    float size[40];         // offset 0x14, size 0xA0
    NJS_POINT3 vtx_pos[40]; // offset 0xB4, size 0x1E0
    NJS_VECTOR add_vec[40]; // offset 0x294, size 0x1E0
} EFFSUB365_WORK;

typedef struct EFFSUB366_WORK 
{
    // total size: 0x294
    D_WORK dtbl;             // offset 0x0, size 0x14
    float g[16];             // offset 0x14, size 0x40
    NJS_VECTOR add_vec[16];  // offset 0x54, size 0xC0
    NJS_POINT3 vtx_pos[16];  // offset 0x114, size 0xC0
    NJS_POINT3 vtx_pos2[16]; // offset 0x1D4, size 0xC0
} EFFSUB366_WORK;

typedef struct EFFSUB367_WORK 
{
    // total size: 0x414
    D_WORK dtbl;            // offset 0x0, size 0x14
    float size[32];         // offset 0x14, size 0x80
    float g[32];            // offset 0x94, size 0x80
    float r[32];            // offset 0x114, size 0x80
    int rot[32];            // offset 0x194, size 0x80
    int rot_spd[32];        // offset 0x214, size 0x80
    NJS_POINT3 vtx_pos[32]; // offset 0x294, size 0x180
} EFFSUB367_WORK;

void bhEff_SetPtcl(BH_PWORK* epw, NJS_POINT3* pos, int tex_id);
void bhEff_SetPtcl1(BH_PWORK* epw, NJS_POINT3* pos, int tex_id, int wcnt);
void bhEff_SetPtcl2(BH_PWORK* epw, NJS_POINT3* pos, NJS_MATRIX* mtx);
void bhEff_SetPtcl2V(BH_PWORK* epw, NJS_POINT3* pos, NJS_POINT3* dv, int wcnt);
static void bhEff_SetBlood5(NJS_POINT3* pos, NJS_VECTOR* vec, float size, int col);
void bhEff_Sub350(O_WRK* op);
void bhEff_Sub351(O_WRK* op);
void bhEff_Sub352(O_WRK* op);
void bhEff_Sub353(O_WRK* op);
void bhEff_Sub354(O_WRK* op);
void bhEff_Sub355(O_WRK* op);
void bhEff_Sub356(O_WRK* op);
void bhEff_Sub357();
void bhEff_Sub360(O_WRK* op);
void bhEff_Sub361(O_WRK* op);
void bhEff_Sub362(O_WRK* op);
void bhEff_Sub363(O_WRK* op);
void bhEff_Sub364(O_WRK* op);
void bhEff_Sub365(O_WRK* op);
void bhEff_Sub366(O_WRK* op);
void bhEff_Sub367(O_WRK* op);
void bhEff_Sub368(O_WRK* op);
void bhEff_Sub369(O_WRK* op);
void bhEff_Sub370(O_WRK* op);
void bhEff_Sub371(O_WRK* op);
void bhEff_Sub372(O_WRK* op);
void bhEff_Sub373(O_WRK* op);
void bhEff_Sub398(O_WRK* op);
static O_WRK* bhEff_AllocOwork();
static void bhEff_PtclSpriteDraw(O_WRK* op);
static void bhEff_LineDraw(O_WRK* op);
static void bhEff_SetSprite(O_WRK* op, UV_WORK* uvp, int mode);
static void bhEff_SetSpriteAnime(O_WRK* op, EFF_ANIM* eff_anim, UV_WORK* uvp, int mode);
static void bhEff_3DSpriteDraw(O_WRK* op);
static void bhEff_ModelDraw(O_WRK* op);

#endif
