#ifndef _SUB1_H_
#define _SUB1_H_

#include "types.h"

typedef struct PULSE_WORK 
{
    // total size: 0x120
    NJS_POINT2 pos_00[5];         // offset 0x0, size 0x28
    NJS_POINT2 pos_01[5];         // offset 0x28, size 0x28
    NJS_POINT2 pos_02[5];         // offset 0x50, size 0x28
    NJS_POINT2 heal_pos00;        // offset 0x78, size 0x8
    NJS_POINT2 heal_pos01;        // offset 0x80, size 0x8
    NJS_COLOR uv_00[5];           // offset 0x88, size 0x14
    NJS_COLOR uv_01[5];           // offset 0x9C, size 0x14
    NJS_COLOR col00[5];           // offset 0xB0, size 0x14
    NJS_COLOR col01[5];           // offset 0xC4, size 0x14
    unsigned int ocol00[5];       // offset 0xD8, size 0x14
    unsigned int ocol01[5];       // offset 0xEC, size 0x14
    unsigned short fadework00[5]; // offset 0x100, size 0xA
    unsigned short fadework01[5]; // offset 0x10A, size 0xA
    unsigned int pulseflg;        // offset 0x114, size 0x4
    unsigned int fadeflg;         // offset 0x118, size 0x4
    unsigned int count;           // offset 0x11C, size 0x4
} PULSE_WORK;

void Ps2ZbuffOffI();
void Ps2DrawOTagI();
void StatusInit(S_WORK* st);
void ItemBoxInit(S_WORK* st);
void CursorMove(S_WORK* st);
void CursorSet(S_WORK* st);
void SpriteSet2D(S_WORK* st, PARTS* cbjim, NJS_TEXANIM* anim1, NJS_TEXLIST* tlist);
void ItemSort();
void ItemSet(S_WORK* st, unsigned char flg);
void KazuSet(S_WORK* st, unsigned char flg);
unsigned char ItemUse(S_WORK* st);
void SpriteH(S_WORK* st);
void SpriteUV(S_WORK* st);
void BGFadeIn();
void BGFadeOut();
void ItemTaskCheck();
void StatusMain();
void GetItem(S_WORK* st);
void MainCommand(S_WORK* st);
void StatusCancel(S_WORK* st);
void WeaponSet(unsigned int wpncsr, unsigned int lt_c);
void ItemCommand(S_WORK* st);
void ArmsSet(S_WORK* st, unsigned char flg);
void WindowJyoutai(S_WORK* st);
void ItemCommandErase(S_WORK* st, int param);
void ItemCombination(S_WORK* st);
void SidePackSet();
void TrigerSet(S_WORK* st);
PARTS* BulletSet(S_WORK* st, PARTS* pb1, PARTS* pb2, unsigned int itemid, unsigned short mode);
unsigned char Use_00(S_WORK* st);
unsigned char Use_01(S_WORK* st);
unsigned char Use_02();
unsigned char Use_04();
unsigned char Use_05(S_WORK* st);
unsigned char Combi_00(short ps, unsigned int* moto, unsigned int* aite);
unsigned char Combi_01(short ps, unsigned int* moto, unsigned int* aite);
unsigned char Combi_02(short ps, unsigned int* moto, unsigned int* aite);
unsigned char Combi_03(short ps, unsigned int* moto, unsigned int* aite);
unsigned char Combi_04(short ps, unsigned int* moto, unsigned int* aite);
unsigned char Combi_05(short ps, unsigned int* moto, unsigned int* aite);
unsigned char Combi_99(short ps, unsigned short moto, unsigned short aite);
void ItemBox(S_WORK* st);
void ItemBoxChange(S_WORK* st, unsigned short ibcsr);
void BorderLineSet(S_WORK* st, unsigned char border, float ypos);
void SpriteOnOff(S_WORK* st);
void NameChangeSet(S_WORK* st);
void AllItemInit();
void ExtraGameItemInit();
void ItemBoxIconSet(S_WORK* st);
int bhSearchBullet();
void CenterPositionInit();
void BgColorInit();
void CursorInit(NJS_TEXANIM* disp_a00, PARTS* disp_b00, S_WORK* st);
unsigned int* ItemSearch(unsigned short itemid);
void EraseItem(unsigned int* wp);
void PushPalletBuffer();
void PopPalletBuffer();
void SbsTextureInit();
void PulseInit();
void PulseMain();
void PulseAnim();
void PulseFadeAnim();
void PulseHealAnim(NJS_POINT2* pos, NJS_COLOR* texuv);
void PulsePoisonHealAnim(NJS_POINT2* pos, NJS_COLOR* texuv);
void Pulse00(NJS_POINT2* pos, NJS_COLOR* texuv);
void MultiWindowBack();
void KazariAnim();
void DrawPoly2D(NJS_POINT2* pos00, NJS_COLOR* col00, NJS_COLOR* uv00, float pri, unsigned int atr, int texnum);
void DispFadeInit();
void FadePolyDisp();
void StatusMapFlagInit();

extern S_WORK swork;
extern char ViewCnt;
extern unsigned char menuanim[3][4];
extern PARTS parts_00b[24];
extern PARTS parts_10b[24];
extern PARTS parts_22b[6]; 
extern float cen_pos99[12][6];
extern float cen_pos[12][6];
extern STCAM_WRK st_cam;
extern FADE fade;
extern SITEM sitem;

#endif
