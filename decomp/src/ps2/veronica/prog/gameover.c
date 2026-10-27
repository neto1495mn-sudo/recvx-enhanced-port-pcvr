#include "../../../ps2/veronica/prog/gameover.h"
#include "../../../ps2/veronica/prog/adv.h"
#include "../../../ps2/veronica/prog/effect.h"
#include "../../../ps2/veronica/prog/flag.h"
#include "../../../ps2/veronica/prog/MdlPut.h"
#include "../../../ps2/veronica/prog/Motion.h"
#include "../../../ps2/veronica/prog/main.h"
#include "../../../ps2/veronica/prog/message.h"
#include "../../../ps2/veronica/prog/player.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaSystem.h"
#include "../../../ps2/veronica/prog/ps2_NaView.h"
#include "../../../ps2/veronica/prog/pwksub.h"
#include "../../../ps2/veronica/prog/room.h"
#include "../../../ps2/veronica/prog/screen.h"
#include "../../../ps2/veronica/prog/sdfunc.h"
#include "../../../ps2/veronica/prog/system.h"
#include "../../../ps2/veronica/prog/weapon.h"

void bhDispFontEx(NJS_POINT2* pos, int code, unsigned int argb, float pri); // see message.h for more info about why this is being included here

typedef void (*bhCtrGov_mode0_proc)();

bhCtrGov_mode0_proc bhCtrGov_mode0[4] = { bhSelectContinue, bhInitGameOver, bhMainGameOver, bhExitGameOver };

const float gov_yn[16] = 
{
    0.005f, 0.010f, 0.005f, 0.008f,
    0.012f, 0.012f, 0.010f, 0.002f,
    0.003f, 0.005f, 0.008f, 0.010f,
    0.002f, 0.008f, 0.005f, 0.001f
};
const struct GV_WRK gov_etb[7] = 
{
    { 40, -25.0f, 10.0f, -80.0f },
    { 54, -14.0f, 10.0f, -60.0f },
    { 68,   0.0f, 10.0f, -40.0f },
    { 82,  -6.0f, 10.0f, -20.0f },
    { 96,   3.0f, 10.0f, -10.0f },
    { 110, -4.0f, 10.0f,  10.0f },
    { 0,    0.0f,  0.0f,   0.0f }
};
const unsigned short cnt_mes[17] =
{
    65281,
    50,
    69,
    84,
    82,
    89,
    31,
    65280,
    65281,
    57,
    69,
    83,
    65281,
    65281,
    46,
    79,
    0xFFFF
};

// 100% matching!
void bhControlGameOver()
{
    bhCtrGov_mode0[sys->gov_md0]();
}

// 100% matching!
void bhSelectContinue()
{
    GV_WORK* gv;

    sys->gov_exp = bhGetFreeMemory(sizeof(GV_WORK), 32);
    
    gv = sys->gov_exp;
    
    gv->cont_x = 0;
    gv->count = 0;
    gv->cur_ct = 0; 
    
    sys->gov_md0 = 1;
    sys->gov_md1 = 0;
}

// 99.35% matching
void bhInitGameOver()
{
    GV_WORK* gv;       
    float ar;         
    float scl;       
    int i, j;             
    int x, y;             
    int ct;           
    int rn;           
    volatile int stack __attribute__((aligned(16))); // not from DWARF

    sys->ts_flg |= 0x100;
    
    sys->cb_flg &= ~0x4;
    
    sys->gov_ct = 0;
    
    sys->gov_md0 = 2;
    sys->gov_md1 = 0;
    
    bhCrFlg(sys->ev_flg, 69);
    bhCrFlg(sys->ev_flg, 67);
    bhCrFlg(sys->ev_flg, 70);
    
    sys->evt_tmd = 0;
    sys->evt_tim = 0;
    
    if (sys->gm_mode == 3) 
    {
        sys->gm_flg &= ~0x80000000;
    }
    
    bhSetScreenFade(0xFFE0E0E0, 60.0f);
    
    gv = sys->gov_exp;
    
    gv->p3c.p   = gv->pt;
    gv->p3c.col = gv->cl;
    gv->p3c.tex = gv->uv;
    gv->p3c.num = 256;
    
    gv->pz = -40.0f;
    
    gv->scl = 0.01f;
    
    gv->tfn = 50.0f;
    gv->tnn = -gv->tfn / 30.0f;
    
    for (i = 0; i < 256; i++)
    {
        gv->cl[i].color = -1;
    }
    
    for (y = 0; y < 16; y++)
    {
        for (x = 0; x < 16; x++)
        {
            gv->pt[x + (y * 16)].x = x   - 7.5f;
            gv->pt[x + (y * 16)].y = -(y - 7.5f);
            gv->pt[x + (y * 16)].z = 0;
            
            gv->cp[x + (y * 16)] = gv->pt[x + (y * 16)];
            
            gv->op[x + (y * 16)].x = 0;
            gv->op[x + (y * 16)].y = 0;
            gv->op[x + (y * 16)].z = 0;
            
            njUnitVector(&gv->cp[x + (y * 16)]);
        }
    }
    
    scl = 126.0f;

    /* this var seems like it belongs to some debug code that the compiler wasn't able to properly optimize away, and it 
       doesn't really affect the code so it is being used here mostly for matching purposes. */
    stack = 8;
    stack;
    
    ct = 15;
    
    //stack -= 1; // the assembly shows this operation happens, but matches worse
    
    scl -= 14.0f;
    
    x = 0;
    y = 0;
    
    while (scl) 
    {
        rn = 0;
        
        ar = 360.0f / (4.0f * ct);
        
        for (j = 0; j < ct; j++, rn++) 
        {
            gv->uv[(x + j) + (y * 16)].tex.u = (int)(scl   * njSin((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))  + 128;
            gv->uv[(x + j) + (y * 16)].tex.v = (int)(-(scl * njCos((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))) + 128;
        } 
        
        for (j = 0; j < ct; j++, rn++) 
        {
            gv->uv[(y + ct) + ((x + j) * 16)].tex.u = (int)(scl   * njSin((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))  + 128;
            gv->uv[(y + ct) + ((x + j) * 16)].tex.v = (int)(-(scl * njCos((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))) + 128;
        } 
        
        for (j = ct; j > 0; j--, rn++)  
        {
            gv->uv[(y + j) + ((x + ct) * 16)].tex.u = (int)(scl   * njSin((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))  + 128;
            gv->uv[(y + j) + ((x + ct) * 16)].tex.v = (int)(-(scl * njCos((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))) + 128;
        } 
        
        for (j = ct; j > 0; j--, rn++) 
        {
            gv->uv[y + ((x + j) * 16)].tex.u = (int)(scl   * njSin((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))  + 128;
            gv->uv[y + ((x + j) * 16)].tex.v = (int)(-(scl * njCos((int)(182.04445f * ((ar * rn) - 45.0f)) & 0xFFFF))) + 128;
        } 
        
        x++;
        y++;
        
        stack -= 1;
        ct    -= 2;
        scl   -= 14.0f;
    }
}

// 99.26% matching
void bhMainGameOver() 
{
    GV_WORK* gv;     
    NJS_POINT2 cpos;   
    float scl, scn;        
    float ar;         
    int i, j;            
    int x, y;            
    int ct;           
    int rn;           
    unsigned int argb; 
    float* yn; // not from DWARF
    int temp;  // not from DWARF

    gv = sys->gov_exp;
    
    switch (sys->gov_md1) 
    {      
    case 0:
        sys->gov_ct++;
        
        if (sys->gov_ct > 70) 
        {
            sys->gov_ct = 0;
            
            njFogDisable();
            
            njClipZ(-2.0f, -20000.0f);
            
            if (((sys->gm_flg & 0x40)) && ((sys->st_flg & 0x800000))) 
            {
                if (!(sys->gm_flg & 0x1000000))
                {
                    sys->gm_flg &= ~0x40;
                    sys->gm_flg &= ~0x80;
                }
                
                sys->st_flg &= ~0x800000;
                sys->gm_flg &= ~0x80000;
            }
            
            if ((sys->gm_flg & 0x400))
            {
                sys->gov_md1 = 10;
                
                bhSetScreenFade(0, 30.0f);
                
                RequestAllStopSoundEx(1, 1, 30);
                
                njSetBackColor(0x00000000, 0x00000000, 0x00000000); 
                
                sys->sp_flg = 0;
                sys->sp_flg |= 0x6C;
                
                sys->pt_flg = 0;
                sys->pt_flg |= 0x15;
                
                sys->gm_flg &= ~0x100100;
                
                sys->cb_flg |= 0x40000000;
                
                bhClearEffect();
                
                plp->stflg &= ~0x1000000;
                
                plp->px = 0;
                plp->py = 0;
                plp->pz = 0;
                
                plp->ay = 35499;
                
                plp->mnwP = plp->mnwPb;
                
                plp->mtn_no = PlMtnAct[((EXP_WORK*)plp->exp0)->wpntp][((EXP_WORK*)plp->exp0)->dmlvl][3];
                
                plp->hokan_rate  = 0;
                plp->hokan_count = 0;
                
                plp->mtn_add = 65536;
                
                plp->mlwP->texP = plp->txp[0];
                
                plp->mlwP->owP[7].flg  &= ~0x2;
                plp->mlwP->owP[11].flg &= ~0x2;
                plp->mlwP->owP[5].flg  &= ~0x2;
                plp->mlwP->owP[4].flg  &= ~0x2;
                
                sys->obwp[0].mdflg &= ~0x1; 
                sys->obwp[1].mdflg &= ~0x1;
                
                sys->st_flg |= 0x1;
                
                cam.flg = 0x40;
                
                cam.pers = 11832;
                
                cam.ln = 0;
                
                cam.ax = cam.axp = 0;
                cam.ay = cam.ayp = 0;
                cam.az = cam.azp = 0;
                
                cam.px = cam.pxp = 0;
                cam.py = cam.pyp = 10.0f;
                cam.pz = cam.pzp = 35.0f;
                
                sys->gm_flg |= 0x200080;
                sys->st_flg |= 0x4;
                return;
            }
            
            sys->gov_md1++;
            
            bhSetScreenFade(0, 30.0f);
            
            RequestAllStopSoundEx(1, 1, 60);
            
            njSetBackColor(0x00000000, 0x00000000, 0x00000000); 
            
            sys->sp_flg = 0;
            sys->sp_flg |= 0x20;
            
            sys->pt_flg = 0;
            
            sys->gm_flg &= ~0x100100;
        }
        
        return;
    case 1:
        scn = gv->tfn;
        
        ct = 15;
        
        x = 0;
        y = 0;
        
        temp = 0;
    
        for (scl = 9.0f, i = 8; i--, scl -= 1.0f; x++, temp += 16, y++, ct -= 2)
        {
            rn = 0;
            
            ar = 360.0f / (4.0f * ct);
            
            for (j = 0; j < ct; j++, rn++) 
            {
                gv->cp[(x + j) + temp].x = scl * njSin((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
                gv->cp[(x + j) + temp].y = scl * njCos((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
            }
            
            for (j = 0; j < ct; j++, rn++) 
            {
                gv->cp[(x + ct) + ((y + j) * 16)].x = scl * njSin((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
                gv->cp[(x + ct) + ((y + j) * 16)].y = scl * njCos((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
            }
            
            for (j = ct; j > 0; j--, rn++) 
            {
                gv->cp[(x + j) + ((y + ct) * 16)].x = scl * njSin((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
                gv->cp[(x + j) + ((y + ct) * 16)].y = scl * njCos((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
            }
            
            for (j = ct; j > 0; j--, rn++) 
            {
                gv->cp[x + ((y + j) * 16)].x = scl * njSin((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
                gv->cp[x + ((y + j) * 16)].y = scl * njCos((int)(182.04445f * (((ar * rn) - 45.0f) + (scl * scn))) & 0xFFFF);
            }
        }
    
        for (y = 0; y < 16; y++) 
        {
            yn = (float*)&gov_yn;
            
            for (x = 0; x < 16; x++) 
            {
                gv->pt[x + (y * 16)].x = gv->cp[x + (y * 16)].x + gv->op[x + (y * 16)].x;
                gv->pt[x + (y * 16)].y = gv->cp[x + (y * 16)].y + gv->op[x + (y * 16)].y;
                gv->pt[x + (y * 16)].z = gv->pz                 + gv->op[x + (y * 16)].z;
                
                gv->op[x + (y * 16)].y = gv->op[x + (y * 16)].y + (gv->scl * ((gv->cp[x + (y * 16)].y - 9.0f) * yn[x]));
            }
        }
    
        if (sys->gov_ct == 50) 
        {
            if (sys->gm_mode < 3) 
            {
                sys->gov_md1 = 2;
            } 
            else 
            {
                CallPlayerDeadVoice(sys->ply_id);
            }
        }
        
        if (gv->tfn > 0) 
        {
            gv->tfn += gv->tnn;
        }
        
        gv->pz  += 0.1f * (-14.0f - gv->pz);
        gv->scl += 0.005f;
        
        sys->gov_ct++;
    
        if ((sys->gov_ct > 120) && (!(sys->cb_flg & 0x2))) 
        {
            bhSetScreenFade(0xFF000000, 30.0f);
            
            RequestAllStopSoundEx(1, 1, 30);
        } 
        else if (sys->gov_ct > 170) 
        {
            sys->gov_md0 = 3;
        }
        
        break;
    case 2:
        sys->mes_tp = cnt_mes;
        
        bhDispMessageEx(264.0f, 211.0f, -0.9f, 2, 0, (gv->count << 24) | 0xE0E0E0, 0);
        
        if (gv->count < 0xFF) 
        {
            gv->count += 16;
            
            if (gv->count > 0xFF) 
            {
                gv->count = 0xFF;
            }
        } 
        else 
        {
            argb = (((int)(128.0f * njSin(gv->cur_ct)) + 120) << 24) | 0xE0E0E0;
            
            gv->cur_ct = (gv->cur_ct + 2048) & 0x7FFF;
            
            if (gv->cont_x == 0) 
            {
                cpos.x = 248.0f;
            } 
            else 
            {
                cpos.x = 328.0f;
            }
            
            cpos.y = 241.0f;
            
            bhDispFontEx(&cpos, 28, argb, -0.9f);
            
            if (((sys->pad_ps & 0x4)) && (gv->cont_x > 0)) 
            {
                gv->cont_x--;
                
                CallSystemSe(0, 2);
            }
            
            if (((sys->pad_ps & 0x8)) && (gv->cont_x <= 0))
            {
                gv->cont_x++;
                
                CallSystemSe(0, 2);
            }
            
            if ((sys->pad_ps & 0x800)) 
            {
                if (gv->cont_x == 0) 
                {
                    bhSetScreenFade(0xFF000000, 10.0f);
                    
                    RequestAllStopSoundEx(1, 1, 10);
                    
                    sys->gov_md1 = 3;
                    
                    CallSystemSe(0, 3);
                } 
                else 
                {
                    sys->gov_md1 = 1;
                    
                    if (!(sys->gm_flg & 0x400)) 
                    {
                        if (bhCkFlg(sys->ev_flg, 73) != 0)
                        {
                            CallPlayerDeadVoice(0);
                        } 
                        else 
                        {
                            CallPlayerDeadVoice(sys->ply_id);
                        }
                    }
                }
            }
        }
        
        break; 
    case 3:
        if (!(sys->cb_flg & 0x2)) 
        {
            sys->ts_flg |= 0x4000;
            
            bhPopGameData();
            return;
        }
        
        break;
    case 10:
        sys->st_flg |= 0x4; 
        
        bhSetMotion(plp, plp->mtn_add, 0, NULL);
        
        if ((plp->mtn_no == 25) && ((plp->frm_no / 65536) == 0))
        {
            plp->mtn_no = 26;
        }
        
        bhCalcModel(plp);
        
        if ((gov_etb[gv->exp_ct].frm != 0) && (gov_etb[gv->exp_ct].frm <= sys->gov_ct)) 
        {
            CallPlayerActionSe(534, 1);
            
            bhSetExplosionEffect((NJS_POINT3*)&gov_etb[gv->exp_ct].px);
            
            gv->exp_ct++;
            
            if (gv->exp_ct == 5) 
            {
                plp->mtn_no = 25;
                
                plp->hokan_rate  = 49152;
                plp->hokan_count = 8;
                
                plp->mtn_add = 65536;
                
                plp->frm_no = 0;
                
                CallPlayerDeadVoice(sys->ply_id);
                
                bhSetScreenFade(-1, 30.0f);
            }
        }
        
        sys->gov_ct++;
        
        if (sys->gov_ct > 150) 
        {
            bhSetScreenFade(0, 30.0f);
            
            sys->gov_ct  = 0;
            sys->gov_md1 = 1;
            
            sys->sp_flg = 0;
            sys->sp_flg |= 0x20;
            
            sys->pt_flg = 0;
        }
        
        break;
    }

    njSetPerspective(11832);
    njSetMatrix(NULL, cmat);
    
    bhDrawGameOver();
}

// 100% matching!
void bhExitGameOver() 
{ 
    if (sys->gm_mode < 3)
    { 
        bhReturnTitle(); 
        return;
    }
    
    sys->ts_flg |= 0x4000; 
    
    bhPopGameData();
} 

// 100% matching!
void bhDrawGameOver()
{
    GV_WORK* gv;     
    NJS_POINT3COL p3c; 
    NJS_POINT3 pt[32]; 
    NJS_COLOR cl[32], uv[32]; 
    int x, y;             

    gv = sys->gov_exp;
    
    njTextureFilterMode(1);
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 6);
    
    njSetTexture(&sys->ef_tlist);
    njSetTextureNum(4);
    
    p3c.p   = pt;
    p3c.col = cl;
    p3c.tex = uv;
    p3c.num = 32; 
    
    for (y = 0; y < 15; y++) 
    {
        for (x = 0; x < 16; x++) 
        {
            pt[x * 2].x = gv->pt[x + (y * 16)].x;
            pt[x * 2].y = gv->pt[x + (y * 16)].y;
            pt[x * 2].z = gv->pt[x + (y * 16)].z;
            
            uv[x * 2].tex.u = gv->uv[x + (y * 16)].tex.u;
            uv[x * 2].tex.v = gv->uv[x + (y * 16)].tex.v;  
            cl[x * 2].color = gv->cl[x + (y * 16)].color;
            
            pt[(x * 2) + 1].x = gv->pt[x + ((y + 1) * 16)].x;
            pt[(x * 2) + 1].y = gv->pt[x + ((y + 1) * 16)].y; 
            pt[(x * 2) + 1].z = gv->pt[x + ((y + 1) * 16)].z; 
            
            uv[(x * 2) + 1].tex.u = gv->uv[x + ((y + 1) * 16)].tex.u;
            uv[(x * 2) + 1].tex.v = gv->uv[x + ((y + 1) * 16)].tex.v;
            cl[(x * 2) + 1].color = gv->cl[x + ((y + 1) * 16)].color;
        }
        
        njDrawTriangle3D(&p3c, 30, 0x80000062);
    } 
}
