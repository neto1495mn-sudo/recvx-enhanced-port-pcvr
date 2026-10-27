#ifdef TARGET_PC
#include "ee_asm.h"
#endif
#include "../../../ps2/veronica/prog/objitm.h"
#include "../../../ps2/veronica/prog/MdlPut.h"
#include "../../../ps2/veronica/prog/flag.h"
#include "../../../ps2/veronica/prog/hitchk.h"
#include "../../../ps2/veronica/prog/light.h"
#include "../../../ps2/veronica/prog/main.h"
#include "../../../ps2/veronica/prog/njplus.h"
#include "../../../ps2/veronica/prog/ps2_NaColi.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NinjaCnk.h"
#include "../../../ps2/veronica/prog/pwksub.h"
#include "../../../ps2/veronica/prog/screen.h"
#include "../../../ps2/veronica/prog/sdfunc.h"
#include "../../../ps2/veronica/prog/system.h"
#include "../../../ps2/veronica/prog/weapon.h"

#pragma optimization_level 4

void (*bhJumpObject[101])() = 
{
	bhObjDmy,
	bhObj001,
	bhObj002,
	bhObj003,
	bhObj004,
	bhObj005,
	bhObj006,
	bhObj007,
	bhObj008,
	bhObj009,
	bhObj010,
	bhObj011,
	bhObj012,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjItmBox
};
void (*bhJumpObject2[13])() = 
{
	bhObjClpn,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjDmy,
	bhObjWpn,
	bhObjClpn,
	bhObjWssg
};
unsigned int ulDrawGeneralPurposeWater;

extern void VU0_WAVE_INIT() __attribute__((section(".vutext")));
extern void VU0_WAVE_CALC() __attribute__((section(".vutext")));


#ifdef TARGET_PC
/* third person: objects and items shown only in some cuts (flg 0x40000000, hide[] = the cuts)
   are always drawn, as in the game's own first-person view (gm_flg 0x40) */
#define PC_ALL_CUTS tps_cam_active
#else
#define PC_ALL_CUTS 0
#endif

// 100% matching! 
void bhInitObjItm()
{
    npSetMemory((unsigned char*)sys->obwp, 39936, 0);
    npSetMemory((unsigned char*)sys->itwp, 39936, 0);
}

// 100% matching!
O_WRK* bhSetObject(ETTY_WORK* otp, int no, unsigned char* lkp)
{
    O_WRK* opp;

    opp = &sys->obwp[no];
    
    npSetMemory((unsigned char*)opp, sizeof(O_WRK), 0);
    
    opp->flg = otp->flg;
    
    if ((opp->flg & 0x80000000)) 
    {
        opp->mdflg |= 0x10;
    }
    
    if ((opp->flg & 0x20000000)) 
    {
        opp->mdflg |= 0x40;
    }
    
    opp->id = otp->id;
    
    opp->type = (unsigned char)otp->type;
    
    opp->param = otp->type >> 8;
    
    opp->flr_no = otp->flr_no;
    
    opp->mdlver = otp->mdlver;
    
    opp->draw_tp = otp->prm1;
    
    opp->px = otp->px;
    opp->py = otp->py;
    opp->pz = otp->pz;
    
    opp->ax = otp->ax;
    opp->ay = otp->ay;
    opp->az = otp->az;
    
    opp->aspd = otp->aspd;
    
    opp->sx = opp->sxb = 1.0f;
    opp->sy = opp->syb = 1.0f;
    opp->sz = opp->szb = 1.0f;
    
    opp->hide[0] = otp->hide[0];
    opp->hide[1] = otp->hide[1];
    opp->hide[2] = otp->hide[2];
    opp->hide[3] = otp->hide[3];
    
    opp->lkwkp = lkp;
    
    opp->mtx = (void*)opp->mtxbuf;
    
    opp->clp_jno[0] = 0;
    opp->clp_jno[1] = -1;
    
    opp->idx_ct = no;
    
    return opp;
}

// 100% matching!
O_WRK* bhSetItem(ETTY_WORK* itp, int no, unsigned char* lkp)
{
    O_WRK* opp;

    opp = &sys->itwp[no];
    
    npSetMemory((unsigned char*)opp, sizeof(O_WRK), 0);
    
    opp->flg = itp->flg;
    
    if ((opp->flg & 0x80000000)) 
    {
        opp->mdflg |= 0x10;
    }
    
    if ((opp->flg & 0x20000000)) 
    {
        opp->mdflg |= 0x40;
    }
    
    opp->id = itp->id;
    
    opp->type = (unsigned char)itp->type;
    
    opp->param = itp->type >> 8;
    
    opp->flr_no = itp->flr_no;
    
    opp->mdlver = itp->mdlver;
    
    opp->draw_tp = itp->prm1;
    
    opp->px = itp->px;
    opp->py = itp->py;
    opp->pz = itp->pz;
    
    opp->ax = itp->ax;
    opp->ay = itp->ay;
    opp->az = itp->az;
    
    opp->aspd = itp->aspd;
    
    opp->sx = opp->sxb = 1.0f;
    opp->sy = opp->syb = 1.0f;
    opp->sz = opp->szb = 1.0f;
    
    opp->hide[0] = itp->hide[0];
    opp->hide[1] = itp->hide[1];
    opp->hide[2] = itp->hide[2];
    opp->hide[3] = itp->hide[3];
    
    opp->lkwkp = lkp;
    
    opp->mtx = (void*)opp->mtxbuf;
    
    opp->clp_jno[0] = 0;
    opp->clp_jno[1] = -1;
    
    opp->idx_ct = no;
    
    return opp;
}

// 100% matching!
void bhControlObjItm() 
{
    O_WRK* op;    
    BH_PWORK* pp; 
    int i;     
    int obj_n; 
	
    if ((sys->sp_flg & 0x4)) 
    {
        sys->ob_nlgn = 0;
        sys->ob_hlgn = 0;
        sys->ob_spcn = 0;
        
        op = sys->obwp;
        
        if ((sys->cb_flg & 0x40000000)) 
        {
            obj_n = 4;
        } 
        else 
        {
            obj_n = rom->obj_n;
        }
        
        for (i = 0; i < obj_n; i++, op++)
        {
            sys->onow = i;
            
            if ((op->flg & 0x80)) 
            {
                if ((((O_WRK*)op->lkwkp)->stflg & 0x1000000)) 
                {
                    op->stflg |= 0x1000000;
                } 
                else 
                {
                    op->stflg &= ~0x1000000; 
                }
            }
            
            if ((!(op->stflg & 0x1000000)) || (pl_sleep_cnt != 0))
            {
                if ((op->flg & 0x1))
                {
                    op->pxb = op->px;
                    op->pyb = op->py;
                    op->pzb = op->pz;
                    
                    op->axb = op->ax;
                    op->ayb = op->ay;
                    op->azb = op->az;
                    
                    if ((op->flg & 0x80)) 
                    {
                        pp = (BH_PWORK*)op->lkwkp;
                        
                        njCalcPoint(&pp->mlwP->owP[op->lkono].mtx, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&op->px);
                        
                        if ((!(pp->flg & 0x10)) && ((op->flg & 0x10))) 
                        { 
                            op->flg &= ~0x10000;
                        }
                        else if ((op->flg & 0x10)) 
                        {
                            op->flg |= 0x10000;
                        }
                    }
                    
                    if ((op->flg & 0xC80000)) 
                    {
                        bhActionWeapon((BH_PWORK*)op);
                    }
                    
                    if (op->id >= 1200) 
                    {
                        bhJumpObject2[op->id - 1200](op);
                    } 
                    else
                    {
                        bhJumpObject[op->type]((BH_PWORK*)op);
                    }
                    
                    if ((!(op->mdflg & 0x1)) && (op->mlwP->objP != NULL)) 
                    {
                        if (op->id >= 1000) 
                        {
                            if ((((sys->pt_flg & 0x1)) && (op->id >= 1210)) || (((sys->pt_flg & 0x2)) && (op->id < 1210))) 
                            {
                                sys->ob_hlg[sys->ob_hlgn++] = op;
                            }
                        } 
                        else if ((sys->pt_flg & 0x4)) 
                        {
                            if ((op->type == 5) && (op->aspd != 0)) 
                            {
                                sys->ob_spc[sys->ob_spcn++] = op;
                            } 
                            else 
                            {
                                sys->ob_nlg[sys->ob_nlgn++] = op;
                            }
                        }
                    }
                    
                    if ((op->flg & 0x10000)) 
                    {
                        bhCheckWall((BH_PWORK*)op);
                    }
                    
                    if ((op->flg & 0x200000))
                    {
                        bhControlAlphaFadeObject(op);
                    }
                    
                    bhCalcModel((BH_PWORK*)op);
                }
            }
        }
        
        if (!(sys->cb_flg & 0x40000000))
        {
            op = sys->itwp;
            
            for (i = 0; i < rom->itm_n; i++, op++) 
            {
                if ((!(op->stflg & 0x1000000)) && ((op->flg & 0x1))) 
                {
                    op->pxb = op->px;
                    op->pyb = op->py;
                    op->pzb = op->pz;
                    
                    op->axb = op->ax;
                    op->ayb = op->ay;
                    op->azb = op->az;
                    
                    if (op->aspd != 0)
                    {
                        switch (op->ct0)
                        {        
                        case 1:
                            op->ct1++;
                            
                            if (op->ct1 > op->ct2)
                            {
                                op->mdflg |= 0x80;
                                
                                op->ayp = 0;
                                
                                op->ct1 = 0;
                                op->ct0++;
                            }
                            
                            break;
                        case 2:
                            op->ayp += 8192;
                            
                            op->ct1 = (op->ct1 + 1) & 7;
                            
                            if (op->ct1 == 0) 
                            {
                                op->mdflg &= ~0x80;
                                
                                op->ct0++;
                            }
                            
                            break;
                        default:
                            op->mdflg &= ~0x80;
                            
                            op->ct0 = 1;
                            op->ct1 = 0;
                            op->ct2 = (int)(30.0f * (-rand() / -2.1474836E9f)) + 60;
                            break;
                        }
                    }
                    
                    if ((op->flg & 0x200000)) 
                    {
                        bhControlAlphaFadeObject(op);
                    }
                    
                    bhCalcModel((BH_PWORK*)op);
                }
            }
        }
    }
}

// 100% matching!
void bhDrawGeneralPurposeWater()
{
    int i;
    
    if ((sys->pt_flg & 0x4))
    {
        for (i = 0; i < sys->ob_spcn; i++)
        {
            if ((((sys->stg_no == 1) && (sys->rom_no == 13)) && (i == 0)) 
            || (((sys->stg_no == 3) && (sys->rom_no == 6)) && (i == 0)) 
            || (((sys->stg_no == 7) && (sys->rom_no == 6)) && (i == 0)) 
            || (((sys->stg_no == 7) && (sys->rom_no == 10)) && (i == 0)) 
            || (((sys->stg_no == 9) && (sys->rom_no == 11)) && (i == 0)))
            {
                ulDrawGeneralPurposeWater = 1;
                
                bhDrawSpObject(sys->ob_spc[i]);
                
                ulDrawGeneralPurposeWater = 0;
            }
        }
    }
}

// 100% matching!
void bhDrawObjItm() 
{
    O_WRK* op;   
    int i;     
    NJS_VECTOR vec; 
    float its;    

    if ((sys->pt_flg & 0x4)) 
    {
        if (!(sys->st_flg & 0x20)) 
        {
            bhSetLight();
        }
        
        njCnkSetEasyMultiAmbient(rom->amb_r[rom->amb_obj], rom->amb_g[rom->amb_obj], rom->amb_b[rom->amb_obj]);
        njCnkSetSimpleMultiAmbient(rom->amb_r[rom->amb_obj], rom->amb_g[rom->amb_obj], rom->amb_b[rom->amb_obj]);
        
        for (i = 0; i < sys->ob_nlgn; i++) 
        {
            bhDrawObject(sys->ob_nlg[i]);
        }
        
        if (!(sys->st_flg & 0x20)) 
        {
            bhSetHalfLight();
        }
        
        njCnkSetEasyMultiAmbient(rom->amb_r[rom->amb_chr], rom->amb_g[rom->amb_chr], rom->amb_b[rom->amb_chr]);
        njCnkSetSimpleMultiAmbient(rom->amb_r[rom->amb_chr], rom->amb_g[rom->amb_chr], rom->amb_b[rom->amb_chr]);
        
        for (i = 0; i < sys->ob_hlgn; i++) 
        {
            bhDrawObject(sys->ob_hlg[i]);
        }
        
        for (i = 0; i < sys->ob_spcn; i++) 
        {
            if ((((sys->stg_no != 1) || (sys->rom_no != 13)) || (i != 0)) && (((sys->stg_no != 3) || (sys->rom_no != 6)) || (i != 0)) && (((sys->stg_no != 7) ||(sys->rom_no != 6)) || (i != 0)) && (((sys->stg_no != 7) || (sys->rom_no != 10)) || (i != 0)) && (((sys->stg_no != 9) || (sys->rom_no != 11)) || (i != 0)))
            {
                bhDrawSpObject(sys->ob_spc[i]);
            } 
        }
    }
    
    if (!(sys->st_flg & 0x20)) 
    {
        bhSetLight();
    }
    
    njCnkSetEasyMultiAmbient(rom->amb_r[rom->amb_itm], rom->amb_g[rom->amb_itm], rom->amb_b[rom->amb_itm]);
    njCnkSetSimpleMultiAmbient(rom->amb_r[rom->amb_itm], rom->amb_g[rom->amb_itm], rom->amb_b[rom->amb_itm]);
    
    njControl3D(0x100);
    
    if ((sys->pt_flg & 0x8))
    {
        op = sys->itwp;
        
        for (i = 0; i < rom->itm_n; i++, op++) 
        {
            if (!(op->stflg & 0x1000000)) 
            {
                if ((op->flg & 0x1))
                {
                    if (!(op->mdflg & 0x1)) 
                    {
                        if (((!(op->flg & 0x40000000)) || ((sys->gm_flg & 0x40)) || PC_ALL_CUTS || ((cam.flg & 0x46))) || ((cam.ncut == op->hide[0]) || (cam.ncut == op->hide[1]) || (cam.ncut == op->hide[2]) || (cam.ncut == op->hide[3]))) 
                        {
                            if (((op->mdflg & 0x10)) && ((sys->st_flg & 0x2))) 
                            {
                                njFogDisable();
                            }
                                
                            if ((op->mdflg & 0x80)) 
                            {
                                its = 0.1f * op->aspd;
                                
                                bhGetLightVector(57344, op->ayp, 0, &vec);
                                
                                njSetMatrix(NULL, cam.mtx);
                                
                                njCnkSetEasyMultiLightVector(vec.x, vec.y, vec.z);
                                njCnkSetEasyMultiLightColor(1, its, its, its);
                                njCnkSetEasyMultiLightSwitch(1, 1);
                                njCnkSetEasyMultiLightMatrices();
                                
                                njCnkSetSimpleMultiLightVector(vec.x, vec.y, vec.z);
                                njCnkSetSimpleMultiLightColor(1, its, its, its);
                                njCnkSetSimpleMultiLightSwitch(1, 1);
                                njCnkSetSimpleMultiLightMatrices();
                            } 
                            else
                            {
                                njSetMatrix(NULL, cam.mtx);
                                
                                if (!(sys->st_flg & 0x10000)) 
                                {
                                    njCnkSetEasyMultiLightColor(1, 0, 0, 0);
                                    njCnkSetEasyMultiLightSwitch(1, 0);
                                    
                                    njCnkSetSimpleMultiLightColor(1, 0, 0, 0);
                                    njCnkSetSimpleMultiLightSwitch(1, 0);
                                } 
                                else 
                                {
                                    njCnkSetEasyMultiLightColor(1, sys->lg_r, sys->lg_g, sys->lg_b);
                                    njCnkSetEasyMultiLightVector(sys->lg_vx, sys->lg_vy, sys->lg_vz);
                                    njCnkSetEasyMultiLightSwitch(1, 1);
                                    njCnkSetEasyMultiLightMatrices();
                                    
                                    njCnkSetSimpleMultiLightColor(1, sys->lg_r, sys->lg_g, sys->lg_b);
                                    njCnkSetSimpleMultiLightVector(sys->lg_vx, sys->lg_vy, sys->lg_vz);
                                    njCnkSetSimpleMultiLightSwitch(1, 1);
                                    njCnkSetSimpleMultiLightMatrices();
                                }
                            }
                            
                            if (op->mlwP->objP != NULL) 
                            {
                                bhPutModel((BH_PWORK*)op);
                            }
                            
                            if (((op->mdflg & 0x10)) && ((sys->st_flg & 0x2)))
                            {
                                njFogEnable();
                            }
                        }
                    }
                }
            }
        }
    }
}

// 100% matching! 
void bhDrawObject(O_WRK* op) 
{
    O_WRK* opp; // not from DWARF
    
#ifdef TARGET_PC
    {   /* VR: the camera is in the head - the player's ponytail (1200, 1211) is not drawn; the weapon/hand (1210) is */
        extern int vr_hide_player(void);
        if ((op->id == 1200 || op->id == 1211) && op->lkwkp == (unsigned char*)plp && vr_hide_player()) return;
    }
#endif
    if ((op->flg & 0x80))
    {
        opp = (O_WRK*)op->lkwkp;
        
        if ((opp->stflg & 0x1000000)) 
        {
            op->stflg |= 0x1000000;
        }
        else
        {
            op->stflg &= ~0x1000000;
        }

        if (!((op->id < 1210) || ((sys->pt_flg & 0x1)))) 
        {
            return;
        }
    }

    if ((((op->stflg & 0x1000000)) && (pl_sleep_cnt == 0)) || (((sys->gm_flg & 0x4000)) && ((op->mdflg & 0x40)))) 
    {
        return;
    }
    
    if ((!(op->flg & 0x40000000)) || ((sys->gm_flg & 0x40)) || PC_ALL_CUTS || ((cam.flg & 0x46)) || ((cam.ncut == op->hide[0]) || (cam.ncut == op->hide[1]) || (cam.ncut == op->hide[2]) || (cam.ncut == op->hide[3])))
    {
        if ((op->mdflg & 0x8)) 
        {
            njControl3D(0x2500);
        }
        
        if (((op->mdflg & 0x10)) && ((sys->st_flg & 0x2))) 
        {
            njFogDisable();
        }
        
#ifdef TARGET_PC
        {   /* VR: the player's hand objects (the weapon on node 9, the left hand's on 13) follow the controllers at the headset's rate (gs_gl.c) */
            extern int g_native_tag; extern int vr_hide_player(void);
            int keep = g_native_tag;
            if (op->lkwkp == (unsigned char*)plp && vr_hide_player()) g_native_tag = op->lkono == 13 ? 3 : 2;
            bhPutModel((BH_PWORK*)op);
            g_native_tag = keep;
        }
#else
        bhPutModel((BH_PWORK*)op);
#endif
        
        njControl3D(0x100);
        
        if (((op->mdflg & 0x10)) && ((sys->st_flg & 0x2))) 
        {
            njFogEnable();
        }
    }
}

// 100% matching!
void bhDrawSpObject(O_WRK* op) 
{
    LGT_WORK* lp;
    NJS_VECTOR vec;

    if (!((!(op->flg & 0x40000000)) || ((sys->gm_flg & 0x40)) || PC_ALL_CUTS || ((cam.flg & 0x46)) || ((cam.ncut == op->hide[0]) || (cam.ncut == op->hide[1]) || (cam.ncut == op->hide[2]) || (cam.ncut == op->hide[3]))))
    {
        return;
    }

    switch (op->type) 
    {
    case 5:
        lp = &rom->lgtp[op->aspd];
        
        if (op->draw_tp == 0) 
        {
            njSetMatrix(NULL, cam.mtx);
            
            njCnkSetEasyMultiLightVector(lp->vx, lp->vy, lp->vz);
            njCnkSetEasyMultiLightColor(1, lp->r, lp->g, lp->b);
            njCnkSetEasyMultiLightSwitch(1, 1);
            njCnkSetEasyMultiLightMatrices();
        } 
        else 
        {
            njCalcVector(cam.mtx, (NJS_VECTOR*)&lp->vx, &vec);
            
            njCnkSetEasyLightColor(lp->r, lp->g, lp->b);
            njCnkSetEasyLightIntensity(1.0f, 0.2f);
            njCnkSetEasyLight(vec.x, vec.y, vec.z);
        }
        
        bhPutModel((BH_PWORK*)op);
    }
}

// 100% matching!
void bhSetAlphaFadeObject(O_WRK* op, int jntno, int jnt_n, int alpha, int count)
{
	int* iwk;
	float* fwk;

	op->flg |= 0x200000;

	iwk = (int*)&op->pv[0];

    iwk[0] = jntno;
    iwk[1] = jnt_n;

	fwk = (float*)&op->pv[1];

    fwk[0] = alpha;
    fwk[1] = count;
}

// 91.58% matching
void bhControlAlphaFadeObject(O_WRK* op) 
{ 
    NJS_CNK_OBJECT* objp; 
    int* iwk;            
    float* fwk;        
    int i;              
    int jnt_n;         
    float na;            
    float alpha;         
    float count;     
    
    iwk =   (int*)&op->pv[0];
    fwk = (float*)&op->pv[1];
    
    jnt_n = iwk[1]; 
    
    na    = fwk[0]; 
    count = fwk[1]; 
    
    objp = &op->mlwP->objP[iwk[0]]; 
    
    for (i = 0; i < jnt_n; i++, objp++) 
    { 
        if ((objp->model != NULL) && (!(objp->evalflags & 0x8)))
        { 
            alpha =  ((unsigned char*)&objp->model->plist[2])[3]; 
            alpha += (na - alpha) / count; 
            
            if (alpha < 0)
            {
                alpha = 0; 
            }
            
            if (alpha > 255.0f) 
            {
                alpha = 255.0f; 
            }
            
            npSetAllMatAlphaColor(objp, 1, (unsigned int)alpha); 
        }
    }
    
    count -= 1.0f; 
    
    if (count <= 0) 
    {
        op->flg &= ~0x200000; 
    }
    
    fwk[1] = count; 
} 

// 100% matching!
void bhObjDmy()
{

}

// 100% matching!
void bhObjItmBox(O_WRK* op)
{
    switch (op->mode0) 
    {                            
    case 0:
        op->ct0 = 0;
        
        op->mode0 = 1;
        
        CallSystemSe(0, 0x80000240);
    case 1:
        op->ct0 += 384;
        
        if (op->ct0 > 15360) 
        {
            op->mode0 = 2;
        }
        
        op->ax = -((int)(182.04445f * (80.0f * njSin(op->ct0))) & 0xFFFF);
        break;
    case 2:
        sys->cb_flg |= 0x40000;
        
        op->mode0 = 3;
        break;
    case 3:
        plp->stflg &= ~0x10000;
        
        op->ax = 0;
        
        op->type = 0;
        break;
    }
}

// 100% matching!
void bhObj001(O_WRK* op) 
{
    NJS_POINT3 pos; 
    ATR_WORK* hp; 
    int pcflg;     
    
    switch (op->mode0) 
    {                              
    case 0:                                         
        if ((op->flg & 0x100000)) 
        {
            op->mode0 = 1;
            break;
        }
        
        op->flg   |= 0x102000;
        op->mdflg |= 0x8;
        
        switch (op->aspd) 
        {                          
        case 0:                                     
        case 6:                                     
            op->aw = 4.5f;
            op->ah = 7.99f;
            op->ad = 4.5f;
            
            op->pn = 1;
            break;
        case 1:                                     
            op->aw = 2.5f;
            op->ah = 19.9f;
            op->ad = 6.0f;
            
            op->pn = 0;
            break;
        case 2:                                     
            op->aw = 3.5f;
            op->ah = 29.9f;
            op->ad = 3.5f;
            
            op->pn = 0;
            break;
        case 3:                                     
            op->aw = 3.0f;
            op->ah = 18.0f;
            op->ad = 7.5f;
            
            op->pn = 0;
            break;
        case 4:                                     
            op->aw = 5.0f;
            op->ah = 10.0f;
            op->ad = 2.5f;
            
            op->pn = 0;
            break;
        case 5:                                     
            op->aw = 2.5f;
            op->ah = 10.0f;
            op->ad = 5.0f;
            
            op->pn = 0;
            break;
        case 7:                                     
            op->aw = 2.25f;
            op->ah = 8.5f;
            op->ad = 5.5f;
            
            op->pn = 0;
            break;
        }
        
        op->ct0 = sys->mwal_n++;
        
        hp = &sys->mwalp[op->ct0];
        
        hp->flg = 0x81;
        hp->type = 0;
        
        hp->flr_no = op->flr_no;
        
        hp->attr = 0x10002;
        
        hp->px = op->px - op->aw;
        hp->py = rom->grand[hp->flr_no + 2];
        hp->pz = op->pz - op->ad;
        
        hp->w = 2.0f * op->aw;
        hp->h = op->ah;
        hp->d = 2.0f * op->ad;
        
        hp->prm0 = hp->prm1 = hp->prm2 = 0;
        hp->prm3 = sys->onow;
        
        if (op->pn != 0) 
        {
            op->ani_ct = sys->mwal_n++;
            
            hp = &sys->mwalp[op->ani_ct];
            
            hp->flg = 0x81;
            hp->type = 7;
            
            hp->flr_no = bhCheckFloorNum(9.0f + op->py);
            
            hp->attr = 1;
            
            hp->px = op->px - op->aw;
            hp->py = rom->grand[hp->flr_no + 2];
            hp->pz = op->pz - op->ad;
            
            hp->w =  2.0f * op->aw;
            hp->h = -1.0f;
            hp->d =  2.0f * op->ad;
            
            hp->prm0 = hp->prm1 = hp->prm2 = 0;
            hp->prm3 = sys->onow;
            
            op->ct1 = sys->metc_n++;
            
            hp = &sys->metcp[op->ct1];
            
            hp->flg = 0x81;
            hp->type = 2;
            
            hp->flr_no = op->flr_no;
            
            hp->attr = 0;
            
            hp->px = op->px - op->aw;
            hp->py = rom->grand[hp->flr_no + 2];
            hp->pz = op->pz - op->ad;
            
            hp->w = 2.0f * op->aw;
            hp->h = 0;
            hp->d = 2.0f * op->ad;
            
            hp->prm0 = hp->prm1 = hp->prm2 = hp->prm3 = 0;
            
            op->ct2 = sys->metc_n++;
            
            hp = &sys->metcp[op->ct2];
            
            hp->flg = 0x81;
            hp->type = 2;
            
            hp->flr_no = bhCheckFloorNum(9.0f + op->py);
            
            hp->attr = 0;
            
            hp->px = op->px - op->aw;
            hp->py = rom->grand[hp->flr_no + 2];
            hp->pz = op->pz - op->ad;
            
            hp->w = 2.0f * op->aw;
            hp->h = 0;
            hp->d = 2.0f * op->ad;
            
            hp->prm0 = 1;
            hp->prm1 = hp->prm2 = hp->prm3 = 0;
            
            op->ct3 = sys->mflr_n++;
            
            hp = &sys->mflrp[op->ct3];
            
            hp->flg = 0x81;
            hp->type = 1;
            
            hp->flr_no = bhCheckFloorNum(9.0f + op->py);
            
            hp->attr = 0;
            
            hp->px = (op->px - op->aw) - 4.0f;
            hp->py = rom->grand[hp->flr_no + 2];
            hp->pz = (op->pz - op->ad) - 4.0f;
            
            hp->w = 8.0f + (2.0f * op->aw);
            hp->h = 0;
            hp->d = 8.0f + (2.0f * op->ad);
            
            hp->prm0 = op->param;
            hp->prm1 = hp->prm2 = hp->prm3 = 0;
        }
        
        op->mode0 = 1;
    case 1:                                         
        hp = &sys->mwalp[op->ct0];
        
        hp->attr &= ~0x3C0000;
        
        pos.x = op->px;
        pos.y = op->py;
        pos.z = (op->pz - op->ad) - 0.2f;
        
        if (bhCheckWallType2(&pos, op->flg, op->aw - 0.1f, 0.1f, op->ah, op->idx_ct) != NULL) 
        {
            hp->attr |= 0x40000;
        }
        
        pos.x = op->px;
        pos.y = op->py;
        pos.z = 0.2f + (op->pz + op->ad);
        
        if (bhCheckWallType2(&pos, op->flg, op->aw - 0.1f, 0.1f, op->ah, op->idx_ct) != NULL) 
        {
            hp->attr |= 0x100000;
        }
        
        pos.x = 0.2f + (op->px + op->aw);
        pos.y = op->py;
        pos.z = op->pz;
        
        if (bhCheckWallType2(&pos, op->flg, 0.1f, op->ad - 0.1f, op->ah, op->idx_ct) != NULL) 
        {
            hp->attr |= 0x80000;
        }
        
        pos.x = (op->px - op->aw) - 0.2f;
        pos.y = op->py;
        pos.z = op->pz;
        
        if (bhCheckWallType2(&pos, op->flg, 0.1f, op->ad - 0.1f, op->ah, op->idx_ct) != NULL) 
        {
            hp->attr |= 0x200000;
        }
        
        if ((hp->attr & 0x20000)) 
        {
            if (plp->psh_idx != (op->ct0 + rom->wal_n)) 
            {
                plp->mode3 = 6;
                
                hp->attr &= ~0x20000;
            }
            else
            {
                switch (op->aspd) 
                {                    
                case 0:                                 
                    sys->psh_snd = 527;
                    break;
                case 6:                                 
                    sys->psh_snd = 528;
                    break;
                case 1:                                 
                case 3:                                 
                    sys->psh_snd = 529;
                    break;
                case 2:                                 
                    sys->psh_snd = 531;
                    break;
                case 4:                                 
                case 5:                                 
                    sys->psh_snd = 532;
                    break;
                default:                                
                    sys->psh_snd = 527;
                    break; 
                }
                
                if (((plp->stflg & 0x80)) && (plp->mode3 == 5)) 
                {
                    op->px -= plp->spd * njSin(plp->ay);
                    op->pz -= plp->spd * njCos(plp->ay);
                    
                    bhCheckWall2Box((BH_PWORK*)op);
                    
                    pcflg = 0;
                    
                    switch (plp->ay & 0xC000) 
                    {                
                    case 0:                           
                        if ((hp->attr & 0x40000))
                        {
                            pcflg = 1;
                        }
                        
                        break;
                    case 0x4000:                        
                        if ((hp->attr & 0x200000)) 
                        {
                            pcflg = 1;
                        }
                        
                        break;
                    case 0x8000:                        
                        if ((hp->attr & 0x100000))
                        {
                            pcflg = 1;
                        }
                        
                        break;
                    case 0xC000:                        
                        if ((hp->attr & 0x80000)) 
                        {
                            pcflg = 1;
                        }
                        
                        break;
                    }
                    
                    if (pcflg != 0) 
                    {
                        plp->mode3 = 6;
                    }
                } 
                else 
                {
                    hp->attr &= ~0x20000;
                }
            }
        }

        hp->px = op->px - op->aw;
        hp->pz = op->pz - op->ad;
        
        if (op->pn != 0) 
        {
            hp = &sys->mwalp[op->ani_ct];
            
            hp->px = op->px - op->aw;
            hp->pz = op->pz - op->ad;
            
            hp = &sys->metcp[op->ct1];
            
            hp->px = op->px - op->aw;
            hp->pz = op->pz - op->ad;
            
            hp = &sys->metcp[op->ct2];
            
            hp->px = op->px - op->aw;
            hp->pz = op->pz - op->ad;
            
            hp = &sys->mflrp[op->ct3];
            
            hp->px = (op->px - op->aw) - 4.0f;
            hp->pz = (op->pz - op->ad) - 4.0f;
        }
        
        break;
    }
}

// 100% matching!
void bhObj002(O_WRK* op)
{
	op->ay += (int)(182.04445f * (0.5f * op->aspd)) & 0xFFFF;
}

// 100% matching!
void bhObj003(O_WRK* op)
{
	op->az = (int)(65536.0f * njSin(op->ct0)) / 32;

    op->ct0 += 1024;
}

// 100% matching!
void bhObj004(O_WRK* op)
{
    switch (op->mode0) 
    {                            
    case 0:
        op->px = 0;
        op->py = 0;
        op->pz = 0;
        
        npChangeMatAlphaColor(op->mlwP->objP, op->mlwP->obj_num, 0);
        
        op->mode0 = 1;
        break;
    case 1:
        npChangeMatAlphaColor(op->mlwP->objP, op->mlwP->obj_num, sys->thunder);
        break;
    }
}

// 98.68% matching
void bhObj005(O_WRK* op) 
{
    NJS_CNK_MODEL* cmd; 
    NJS_POINT4* ps, *pp;     
    int i, j;            
    int nb;            
    int xp, zp, wt_xp, wt_zp; // wt_xp is not from DWARF         
    float* wty;      
    HDR_PS* pCnk, *pOrg;     

    xp = 0;
    
    cmd = NULL;
    
    switch (op->mode0) 
    {                              
    case 0:                                         
        if ((sys->st_flg & 0x40)) 
        {
            npSetMemoryL((unsigned int*)sys->wt_wvp, sys->wt_nbpt, 0);
            
            op->mode0 = 1;
            break;
        }
        
        cmd = op->mlwP->objP->model;
        
        pCnk = (HDR_PS*)cmd->vlist;
        
        ps = (NJS_POINT4*)&pCnk[1];
        
        op->ct2 = nb = pCnk->usIndexMax;
        
        op->xn = op->zn = -10000.0f;
        
        op->ct0 = op->ct1 = 0;
        
        for (i = 0; i < nb; i++) 
        {
            if (op->xn < (op->px + ps->x)) 
            {
                op->xn = op->px + ps->x;
                
                op->ct0++;
            }
            
            if (op->zn < (op->pz + ps->z)) 
            {
                op->zn = op->pz + ps->z;
                
                op->ct1++;
            }
            
            ps++;
            ps++;
        }
        
        wt_xp = (int)(op->xn - op->px) + 2;
        wt_zp = (int)(op->zn - op->pz) + 2;
        
        sys->wt_nbpt = wt_xp * wt_zp;
        
        sys->wt_px = op->px;
        sys->wt_pz = op->pz;
        
        sys->wt_xp = wt_xp;
        sys->wt_zp = wt_zp;
        
        sys->wt_minx = 0;
        sys->wt_minz = 0;
        
        sys->wt_maxx = wt_xp;
        sys->wt_maxz = wt_zp;
        
        op->exp0 = bhGetFreeMemory((wt_xp * wt_zp) * 4, 64);
        
        sys->wt_wvp = op->exp0;
        
        op->exp1 = (unsigned char*)cmd->vlist;
        op->exp2 = bhGetFreeMemory((nb * 32) + 72, 64);
        
        cmd->vlist = (int*)op->exp2;
        
        npCopyMemory(op->exp2, op->exp1, (nb * 32) + 72);
        
        pOrg = (HDR_PS*)op->exp1;
        
        pp = (NJS_POINT4*)&pOrg[1];
        
        for (i = 0; i < op->ct2; i++)
        {
            wty = (float*)sys->wt_wvp;
            
            xp = pp->x;
            zp = pp->z;
            
            wty += zp + (xp * sys->wt_zp);
            
            *(NJS_POINT4**)pp = (NJS_POINT4*)wty;
            
            pp++;  
            pp++; 
        }
        
        sys->st_flg |= 0x40;
        
        op->mode0 = 1;
        break;
    case 1:                                         
        switch (op->ct3)
        {                          
        case 0:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 1:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 2:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 3:                                     
            npSetOffsetUV2(op->mlwP->objP->model, -768, 256);
            break;
        case 4:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 5:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 6:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 7:                                     
            npSetOffsetUV2(op->mlwP->objP->model, -768, 256);
            break;
        case 8:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 9:                                     
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 10:                                    
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 11:                                    
            npSetOffsetUV2(op->mlwP->objP->model, -768, 256);
            break;
        case 12:                                    
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 13:                                    
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 14:                                    
            npSetOffsetUV2(op->mlwP->objP->model,  256, 0);
            break;
        case 15:                                    
            npSetOffsetUV2(op->mlwP->objP->model, -768, -768);
            break;
        }    
        
        op->ct3 = (op->ct3 + 1) & 0xF;
        
        j = sys->gfrm_ct & 0x1;
        
        pOrg = (HDR_PS*)op->exp2;
        pCnk = (HDR_PS*)op->exp1;
        
        pOrg++;
        pCnk++;
        
        nb = op->ct2;
        
#ifndef TARGET_PC
        asm volatile 
        ("
        .set noreorder
            vcallms    VU0_WAVE_INIT
            
            srl        t0,  %0, 1
            
            muli       $t1, %6, 4
            
            bnez       %5, l_00285C50 
            nop
        
            lwc1       f1,   0x4(%1)
            lw         t2,     0(%1)
            
            addi       t0, t0, -1 
            
            lwc1       f2,     0(t2)
            
            addi       %2, %2, 32 

            add.s      f1, f1, f2

            addi       %1, %1, 32 
            
            swc1       f1, -0x1C(%2)
            
        l_00285C50:
            lw         t2, 0(%1)
            
            addi       %2, %2, 16 
            addi       %1, %1, 16 
            
            addi       t3, t2, -4 

            add        t4, t2, t1 

            addi       t5, t2, 4 
        
            sub        t6, t2, t1 
            
        l_00285C6C:
            lw         t3,    0(t3)
            lw         t4,    0(t4)
            lw         t5,    0(t5)
            lw         t6,    0(t6)
            
            qmtc2      t3, vf6
            qmtc2      t4, vf7
            qmtc2      t5, vf8
            qmtc2      t6, vf9
        
            vcallms    VU0_WAVE_CALC
            
            lw         t7, 0x10(%1)
            lwc1       f1, 0x14(%1)
            lw         t2, 0x30(%1)
            
            lwc1       f2,    0(t7)
            
            addi       t3, t2, -4 
            
            add        t4, t2, t1 
            add.s      f1, f1, f2

            addi       t5, t2, 4 
        
            sub        t6, t2, t1 
            
            vwaitq
            
            vmulq.xyz  vf10, vf10, Q
            
            addi       t0, t0, -1 
            
            swc1       f1, 0x14(%2)
            
            addi       %1, %1, 64 
            
            sqc2       vf10,  0(%2)
            
            bnez       t0, l_00285C6C
        
            addi       %2, %2, 64 
        .set reorder
        " : : "r"(nb), "r"(pCnk), "r"(pOrg), "r"(sys->gfrm_ct), "r"(xp), "r"(j), "r"(sys->wt_zp) : "$s2", "$s5", "memory" 
        );
#else
        { /* asm2c */
            ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R12 = {{0}}, R13 = {{0}}, R14 = {{0}}, R15 = {{0}};
            float F1 = 0.0f, F2 = 0.0f;
            int ee_cond;
            ee_reg op0 = {{0}};
            ee_reg op1 = {{0}};
            ee_reg op2 = {{0}};
            ee_reg op5 = {{0}};
            ee_reg op6 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(nb);
            op1.sd[0] = (int32_t)(uintptr_t)(pCnk);
            op2.sd[0] = (int32_t)(uintptr_t)(pOrg);
            op5.sd[0] = (int32_t)(uintptr_t)(j);
            op6.sd[0] = (int32_t)(uintptr_t)(sys->wt_zp);
            vu0_callms(VU0_ADDR_VU0_WAVE_INIT);
            R8.sd[0] = (int32_t)(op0.w[0] >> 1);
            R9.sd[0] = (int32_t)(op6.w[0] * (uint32_t)(4));
            ee_cond = (op5.sd[0] != 0);
            if (ee_cond) goto l_00285C50;
            F1 = EE_LWF((op1.w[0] + (uint32_t)(4)));
            R10.sd[0] = EE_LW((op1.w[0] + (uint32_t)(0)));
            R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-1));
            F2 = EE_LWF((R10.w[0] + (uint32_t)(0)));
            op2.sd[0] = (int32_t)(op2.w[0] + (uint32_t)(32));
            F1 = vu_clampf(F1 + F2);
            op1.sd[0] = (int32_t)(op1.w[0] + (uint32_t)(32));
            EE_SWF((op2.w[0] + (uint32_t)(-28)), F1);
        l_00285C50:;
            R10.sd[0] = EE_LW((op1.w[0] + (uint32_t)(0)));
            op2.sd[0] = (int32_t)(op2.w[0] + (uint32_t)(16));
            op1.sd[0] = (int32_t)(op1.w[0] + (uint32_t)(16));
            R11.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(-4));
            R12.sd[0] = (int32_t)(R10.w[0] + R9.w[0]);
            R13.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(4));
            R14.sd[0] = (int32_t)(R10.w[0] + -R9.w[0]);
        l_00285C6C:;
            R11.sd[0] = EE_LW((R11.w[0] + (uint32_t)(0)));
            R12.sd[0] = EE_LW((R12.w[0] + (uint32_t)(0)));
            R13.sd[0] = EE_LW((R13.w[0] + (uint32_t)(0)));
            R14.sd[0] = EE_LW((R14.w[0] + (uint32_t)(0)));
            vu_load_vf(6, &R11);
            vu_load_vf(7, &R12);
            vu_load_vf(8, &R13);
            vu_load_vf(9, &R14);
            vu0_callms(VU0_ADDR_VU0_WAVE_CALC);
            R15.sd[0] = EE_LW((op1.w[0] + (uint32_t)(16)));
            F1 = EE_LWF((op1.w[0] + (uint32_t)(20)));
            R10.sd[0] = EE_LW((op1.w[0] + (uint32_t)(48)));
            F2 = EE_LWF((R15.w[0] + (uint32_t)(0)));
            R11.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(-4));
            R12.sd[0] = (int32_t)(R10.w[0] + R9.w[0]);
            F1 = vu_clampf(F1 + F2);
            R13.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(4));
            R14.sd[0] = (int32_t)(R10.w[0] + -R9.w[0]);
            vu_upper(U_MUL, 14, 10, 10, 0, BC_Q);
            R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-1));
            EE_SWF((op2.w[0] + (uint32_t)(20)), F1);
            op1.sd[0] = (int32_t)(op1.w[0] + (uint32_t)(64));
            vu_store_vf(EE_PTR((op2.w[0] + (uint32_t)(0))), 10);
            ee_cond = (R8.sd[0] != 0);
            op2.sd[0] = (int32_t)(op2.w[0] + (uint32_t)(64));
            if (ee_cond) goto l_00285C6C;
        }
#endif 
     
#ifndef TARGET_PC
        asm volatile 
        (" 
            
            srl        t0, %0, 2
            
            mfc1       t1, %2
            
            srl        t2, t0, 2
            
            qmtc2      t1, vf4
            
            lqc2       vf5,    0(%1)
            lqc2       vf6, 0x10(%1)
            
        l_00285D1C:
            lqc2       vf7, 0x20(%1)
            lqc2       vf8, 0x30(%1)
            
            vsubx.xyzw vf9,  vf5, vf4x
            vsubx.xyzw vf10, vf6, vf4x
            vsubx.xyzw vf11, vf7, vf4x
            vsubx.xyzw vf12, vf8, vf4x
            
            vmaxx.xyzw vf9,  vf9,  vf0x
            vmaxx.xyzw vf10, vf10, vf0x
            vmaxx.xyzw vf11, vf11, vf0x
            vmaxx.xyzw vf12, vf12, vf0x
            
            sqc2       vf9,     0(%1)
            sqc2       vf10, 0x10(%1)
            sqc2       vf11, 0x20(%1)
            sqc2       vf12, 0x30(%1)
            
            addi       t2, t2, -1 
            addi       %1, %1, 64 
            
            lqc2       vf5,    0(%1)
            
            bnez       t2, l_00285D1C
        
            lqc2       vf6, 0x10(%1)
            
            andi       t0, t0, 0x3
        
        l_00285D6C:
            bnez       t0, exit
        
            addi       t0, t0, -1 
            
            vsubx.xyzw vf9, vf5, vf4x
            
            addi       %1, %1, 16 
            
            vmaxx.xyzw vf9, vf9, vf0x
            
            lqc2       vf5, 0(%1)
            
            b          l_00285D6C
        
        " : : "r"(sys->wt_nbpt), "r"(sys->wt_wvp), "f"(0.2f) : "$t0", "$t1", "$t2", "memory" 
        );
#else
        { /* asm2c */
            ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
            int ee_cond;
            ee_reg op0 = {{0}};
            ee_reg op1 = {{0}};
            ee_reg op2 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(sys->wt_nbpt);
            op1.sd[0] = (int32_t)(uintptr_t)(sys->wt_wvp);
            op2.f[0] = (float)(0.2f);
            R8.sd[0] = (int32_t)(op0.w[0] >> 2);
            R9.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
            R10.sd[0] = (int32_t)(R8.w[0] >> 2);
            vu_load_vf(4, &R9);
            vu_load_vf(5, EE_PTR((op1.w[0] + (uint32_t)(0))));
            vu_load_vf(6, EE_PTR((op1.w[0] + (uint32_t)(16))));
        l_00285D1C:;
            vu_load_vf(7, EE_PTR((op1.w[0] + (uint32_t)(32))));
            vu_load_vf(8, EE_PTR((op1.w[0] + (uint32_t)(48))));
            vu_upper(U_SUB, 15, 9, 5, 4, BC_X);
            vu_upper(U_SUB, 15, 10, 6, 4, BC_X);
            vu_upper(U_SUB, 15, 11, 7, 4, BC_X);
            vu_upper(U_SUB, 15, 12, 8, 4, BC_X);
            vu_upper(U_MAX, 15, 9, 9, 0, BC_X);
            vu_upper(U_MAX, 15, 10, 10, 0, BC_X);
            vu_upper(U_MAX, 15, 11, 11, 0, BC_X);
            vu_upper(U_MAX, 15, 12, 12, 0, BC_X);
            vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(0))), 9);
            vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(16))), 10);
            vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(32))), 11);
            vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(48))), 12);
            R10.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(-1));
            op1.sd[0] = (int32_t)(op1.w[0] + (uint32_t)(64));
            vu_load_vf(5, EE_PTR((op1.w[0] + (uint32_t)(0))));
            /* reorder mode, as assembled in the retail ELF: the vf6 reload is NOT in the
               delay slot, so iterations 2+ reuse the first vf6 (original behaviour) */
            if (R10.sd[0] != 0) goto l_00285D1C;
            vu_load_vf(6, EE_PTR((op1.w[0] + (uint32_t)(16))));
            R8.d[0] = R8.d[0] & (uint64_t)(uint16_t)(3);
        l_00285D6C:;
            if (R8.sd[0] != 0) goto exit;
            R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-1));
            vu_upper(U_SUB, 15, 9, 5, 4, BC_X);
            op1.sd[0] = (int32_t)(op1.w[0] + (uint32_t)(16));
            vu_upper(U_MAX, 15, 9, 9, 0, BC_X);
            vu_load_vf(5, EE_PTR((op1.w[0] + (uint32_t)(0))));
            goto l_00285D6C;
        }
#endif  
            
    exit:
        break;
    }
}

// 100% matching!
void bhObj006(O_WRK* op) 
{
    NJS_CNK_OBJECT* obp; 
    NJS_POINT3 ps, ps0;       
    NJS_LINE line;      
    float len;           
    LGT_WORK* lp;        

    line.px = op->px;
    line.py = op->py; 
    line.pz = op->pz;
    
    ps.x = 0;
    ps.y = 0; 
    ps.z = -1.0f;
    
    njUnitMatrix(NULL);
    
    njRotateXYZ(NULL, op->ax, op->ay, op->az);
    
    obp = op->mlwP->objP;
    
    njRotateXYZ(NULL, obp[0].ang[0], obp[0].ang[1], obp[0].ang[2]);
    njRotateXYZ(NULL, obp[1].ang[0], obp[1].ang[1], obp[1].ang[2]);
    
    njCalcVector(NULL, &ps, (NJS_VECTOR*)&line.vx);
    
    ps0.x = op->px + line.vx;
    ps0.y = op->py + line.vy;
    ps0.z = op->pz + line.vz;
    
    njGetPlaneNormal2((NJS_VECTOR*)&cam.wpx, (NJS_VECTOR*)&op->px, &ps0, (NJS_POINT3*)&ps);
    
    njUnitVector(&ps);
    njUnitMatrix(NULL);
    
    njTranslate(NULL, op->px, op->py, op->pz);
    njRotateXYZ(NULL, op->ax, op->ay, op->az);
    
    obp = op->mlwP->objP;
    
    njRotateXYZ(NULL, obp[0].ang[0], obp[0].ang[1], obp[0].ang[2]);
    njRotateXYZ(NULL, obp[1].ang[0], obp[1].ang[1], obp[1].ang[2]);
    
    npClrTranslate();
    
    njTransposeMatrix(NULL);
    
    njCalcPoint(NULL, &ps, &ps0);
    
    op->mlwP->objP[2].ang[2] = -(int)(10430.381f * atan2f(-ps0.x, -ps0.y)) - 16384;
    
    if (op->aspd > 0) 
    {
        lp = &rom->evlp[op->aspd];
        
        line.vx *= 200.0f;
        line.vy *= 200.0f;
        line.vz *= 200.0f;
        
        ps.x = ps.y = 0;
        
        if (bhCheckL2Wall(&line, 1024, &len) != 0)
        {
            ps.z = -(len - 30.0f);
            
            if (ps.z > 0)
            {
                ps.z = 0;
            }
            
            lp->nr = 0.1f * len;
            
            if (lp->nr < 10.0f) 
            {
                lp->nr = 10.0f;
            }
            
            lp->fr = 10.0f + lp->nr;
        } 
        else 
        {
            ps.z = -170.0f;
            
            lp->nr = 20.0f;
            lp->fr = 30.0f;
        }
        
        njUnitMatrix(NULL);
        
        njRotateXYZ(NULL, op->ax, op->ay, op->az);
        
        obp = op->mlwP->objP;
        
        njRotateXYZ(NULL, obp[0].ang[0], obp[0].ang[1], obp[0].ang[2]);
        njRotateXYZ(NULL, obp[1].ang[0], obp[1].ang[1], obp[1].ang[2]);
        
        njCalcPoint(NULL, &ps, &ps0);
        
        lp->px = op->px + ps0.x;
        lp->py = op->py + ps0.y;
        lp->pz = op->pz + ps0.z;
    }
}

// 100% matching!
void bhObj007(O_WRK* op) 
{
    NJS_LINE lin;   
    NJS_POINT3 ps0, ps1, ps2; 
    float len0, len1, len2;  
    float inn;   

    if ((op->py >= 1.0f) && (op->mode0 != 4)) 
    {
        switch (op->mode0) 
        {                     
        case 0:
            op->ct0 = 0;
            
            op->mode0 = 1;
            break;
        case 1:
            lin.px = op->px;
            lin.py = 0;
            lin.pz = op->pz;
            
            lin.vx = -njSin((op->ay + 32767) + 1);
            lin.vy = 0;
            lin.vz = -njCos((op->ay + 32767) + 1);
            
            njDistanceP2L((NJS_POINT3*)&plp->px, &lin, &ps0);
            
            ps2 = ps0;
            
            ps0.x -= op->px;
            ps0.y = 0;
            ps0.z -= op->pz;
             
            len0 = njScalor(&ps0);
            
            njUnitVector(&ps0);
            
            if (len0 > 10.0f) 
            {
                len0 = 10.0f;
                
                ps2.x = op->px + (10.0f * lin.vx);
                ps2.z = op->pz + (10.0f * lin.vz);
            }
            
            ps1.x = plp->px - op->px;
            ps1.y = 0;
            ps1.z = plp->pz - op->pz;
            
            len1 = njScalor(&ps1);
            
            njUnitVector(&ps1);
            
            inn = njInnerProduct((NJS_VECTOR*)&lin.vx, &ps1);
            
            ps0.x = plp->px - ps2.x;
            ps0.y = 0;
            ps0.z = plp->pz - ps2.z;
            
            len2 = njScalor(&ps0);
            
            njUnitVector(&ps0);
            
            if ((inn > 0) && (((len0 + plp->ar) >= len1) && (len2 < plp->ar))) 
            {
                plp->px = PEXP0_F(72) = ps2.x + (ps0.x * plp->ar);
                plp->pz = PEXP0_F(80) = ps2.z + (ps0.z * plp->ar);
                
                ps1.x = -njSin(plp->ay);
                ps1.y = 0;
                ps1.z = -njCos(plp->ay); 
                
                inn = njInnerProduct(&ps0, &ps1);
                
                if ((len0 < 8.4f) && (inn < -0.7f) && ((sys->pad_on & 0x1)) && (!(plp->stflg & 0x80))) 
                {
                    if ((plp->mode2 == 3) || (plp->mode2 == 4) || (plp->mode2 == 5))
                    {
                        op->ct0++;
                        
                        if (op->ct0 > 5) 
                        {
                            ps0.x = -njSin(op->ay + 16384);
                            ps0.y = 0;
                            ps0.z = -njCos(op->ay + 16384);
                            
                            if (njInnerProduct(&ps0, &ps1) > 0) 
                            {
                                plp->azp = op->ay + 16384;
                                
                                op->mode0 = 2;
                            } 
                            else
                            {
                                plp->azp = op->ay - 16384;
                                
                                op->mode0 = 3;
                            }
                            
                            plp->stflg |= 0x80;
                            
                            plp->mode2 = 0x16;
                            plp->mode3 = 0;
                            
                            op->spd = len0;
                        }
                    }
                } 
                else 
                {
                    op->ct0 = 0;
                }
            }
            
            break;
        case 2:
            if (plp->mode3 == 5)
            {
                if (!(sys->pad_on & 0x1))
                {
                    op->mode0 = 1;
                    
                    op->ct0 = 0;
                    break;
                }
                
                op->ay -= (int)(182.04445f * njSin(((plp->frm_no / 65536) * 819) & 0x7FFF)) & 0xFFFF;
                
                plp->ay = op->ay + 16384;
                
                ps0.x = op->px - (op->spd * njSin((op->ay + 32767) + 1));
                ps0.z = op->pz - (op->spd * njCos((op->ay + 32767) + 1));
                
                plp->px = PEXP0_F(72) = ps0.x - (plp->ar * njSin(op->ay - 16384));
                plp->pz = PEXP0_F(80) = ps0.z - (plp->ar * njCos(op->ay - 16384));
                
                if (ABS((short)op->ay) < 182)
                {
                    op->ay = 0;
                    
                    plp->mode3 = 6;
                    op->mode0 = 4;
                    
                    bhStFlg(sys->ev_flg, 2);
                }
            } 
            else if (!(plp->stflg & 0x80)) 
            {
                op->mode0 = 1;
                
                op->ct0 = 0;
            }
            
            break;
        case 3:
            if (plp->mode3 == 5) 
            {
                if (!(sys->pad_on & 0x1)) 
                {
                    op->mode0 = 1;
                    
                    op->ct0 = 0;
                    break;
                }
                
                op->ay += (int)(182.04445f * njSin(((plp->frm_no / 65536) * 819) & 0x7FFF)) & 0xFFFF;
                
                plp->ay = op->ay - 16384;
                
                ps0.x = op->px - (op->spd * njSin((op->ay + 32767) + 1));
                ps0.z = op->pz - (op->spd * njCos((op->ay + 32767) + 1));
                
                plp->px = PEXP0_F(72) = ps0.x - (plp->ar * njSin(op->ay + 16384));
                plp->pz = PEXP0_F(80) = ps0.z - (plp->ar * njCos(op->ay + 16384));
                
                if (ABS((short)op->ay) < 182) 
                {
                    op->ay = 0;
                    
                    plp->mode3 = 6;
                    op->mode0 = 4;
                    
                    bhStFlg(sys->ev_flg, 2);
                }
            } 
            else if (!(plp->stflg & 0x80))
            {
                op->mode0 = 1;
                
                op->ct0 = 0;
            }
            
            break;
        }
    }
}

// 100% matching!
void bhObj008(O_WRK* op) 
{
    ATR_WORK* hp;
    
    if (op->mode0 == 0) 
    {
        if ((op->flg & 0x100000)) 
        {
            op->mode0 = 1;
            return;
        }
        
        op->flg |= 0x100000;
        
        op->gpx = op->px;
        op->gpy = op->py;
        op->gpz = op->pz;
        
        op->ct0 = sys->mwal_n++;
        
        hp = &sys->mwalp[op->ct0];
        
        hp->flg = 129;
        hp->type = 0;
        
        hp->flr_no = op->flr_no;
        
        hp->attr = 0;
        
        hp->px = op->px - 6.0f;
        hp->py = rom->grand[hp->flr_no + 2];
        hp->pz = op->pz - 11.0f;
        
        hp->w = 12.0f;
        hp->h = 20.0f;
        hp->d = 22.0f;
        
        hp->prm3 = 0;
        hp->prm2 = 0;
        hp->prm1 = 0;
        hp->prm0 = 0;
        
        op->mode0 = 1;
        return;
    }
    
    hp = &sys->mwalp[op->ct0];
    
    hp->px = op->mlwP->owP->mtx[12] - 6.0f;
    hp->py = op->mlwP->owP->mtx[13]; 
    hp->pz = op->mlwP->owP->mtx[14] - 11.0f;
}

// 100% matching!
void bhObj009(O_WRK* op)
{
    POS* ptp;    
    NJS_VECTOR vec; 
    float ln;     

    if (bhCkFlg(&sys->rm_flg, 31) != 0) 
    {
        switch (op->mode0) 
        {
        case 1:
            ptp = rom->posp;
            
            op->px = op->gpx;
            op->pz = op->gpz;
            
            op->ay += (op->ayp - op->ay) / 16;
            
            vec.x = ptp[op->ct0 + 1].px - ptp[op->ct0].px;
            vec.y = 0;
            vec.z = ptp[op->ct0 + 1].pz - ptp[op->ct0].pz;
            
            ln = njSqrt((vec.x * vec.x) + (vec.z * vec.z));
            
            njUnitVector(&vec);
            
            op->px += 0.25f * vec.x;
            op->pz += 0.25f * vec.z;
            
            op->gpx = op->px;
            op->gpz = op->pz; 
            
            op->spd += 0.25f;
            
            if (op->spd >= ln) 
            {
                op->ct0++;
                
                op->spd = 0;
                
                op->ayp = ptp[op->ct0 + 1].ay - 16384;
                
                if ((op->ayp & 0xFF00) != (op->ay & 0xFF00)) 
                {
                    op->xn = 0.8f;
                }
                
                if (op->ct0 >= 14) 
                {
                    op->ct0 = 7;
                    
                    op->px = ptp[op->ct0].px;
                    op->pz = ptp[op->ct0].pz;
                    
                    op->gpx = op->px;
                    op->gpz = op->pz;
                    
                    op->ay = op->ayp = ptp[op->ct0].ay - 16384;
                    
                    if (bhCkFlg(sys->ev_flg, 60) != 0) 
                    {
                        bhCrFlg(&sys->rm_flg, 31);
                        
                        op->ct1 = 0;
                        
                        op->mode0 = 2;
                        break;
                    }
                    
                    op->mode1 = 1;
                    
                    bhSetScreenFade(sys->fade_pbk, 30.0f);
                }
            }
            
            op->px = op->gpx + ((op->xn - (0.5f * op->xn)) * (-rand() / -2.1474836E9f));
            op->pz = op->gpz + ((op->xn - (0.5f * op->xn)) * (-rand() / -2.1474836E9f));
            
            if (op->xn > 0)
            {
                op->xn *= 0.9f;
                
                if (op->xn < 0.1f) 
                {
                    op->xn = 0;
                }
            }
            
            if ((op->ct0 == 13) && (op->spd >= (0.5f * ln)) && (op->ct2 == 0)) 
            {
                op->ct2++;
                
                bhSetScreenFade(0xFF000000, 30.0f);
            }
            
            if ((op->mode1 != 0) && (op->ct0 == 11) && (op->spd >= (0.75f * ln))) 
            {
                bhCrFlg(&sys->rm_flg, 31);
                
                op->mode0 = 0;
            }
        }
    } 
    else 
    {
        switch (op->mode0) 
        {                          
        case 0:
            ptp = rom->posp;
            
            vec.x = ptp[12].px - ptp[11].px;
            vec.z = ptp[12].pz - ptp[11].pz;
            
            op->spd = 0.75f * njSqrt((vec.x * vec.x) + (vec.z * vec.z));
            
            op->px = ptp[11].px + (0.75f * vec.x);
            op->pz = ptp[11].pz + (0.75f * vec.z);
            
            op->gpx = op->px;
            op->gpz = op->pz;
            
            op->ay  = 0;
            op->ayp = ptp[12].ay - 16384;
            
            op->zn = 0;
            op->xn = 0;
            
            op->ct0 = 11;
            op->ct2 = 0;
            op->ct1 = 0;
            
            op->mode0 = 1;
            op->mode1 = 0;
            break;
        case 2:
            op->ct1++;
            
            if (op->ct1 >= 4) 
            {
                bhSetScreenFade(sys->fade_pbk, 20.0f);
                
                *(int*)&op->mode0 = 0;
            }
            
            break;
        }
    }
}

// 100% matching!
void bhObj010(O_WRK* op)
{
	if (bhCkFlg(&sys->rm_flg, 31) != 0)
    {
        npSetOffsetUV(op->mlwP->objP->model, 32, 0);

        op->ct0 = (op->ct0 + 32) & 0x3FF;

        if (op->ct0 == 0)
        {
            npSetOffsetUV(op->mlwP->objP->model, -1024, 0);
        }
    }
}

// 100% matching!
void bhObj011(O_WRK* op)
{
	op->mdflg |= 0x8;
}

// 100% matching!
void bhObj012(O_WRK* op) 
{
    ATR_WORK* hp;
    
    if (op->mode0 == 0) 
    {
        if ((op->flg & 0x100000)) 
        {
            op->mode0 = 1;
            return;
        }
        
        op->flg |= 0x100000;
        
        op->ct0 = sys->mwal_n++;
        
        hp = &sys->mwalp[op->ct0];
        
        hp->flg = 129;
        hp->type = 2;
        
        hp->flr_no = op->flr_no;
        
        hp->attr = 0x1000000;
        
        hp->px = op->px;
        hp->py = rom->grand[hp->flr_no + 2];
        hp->pz = op->pz;
        
        hp->w = 4.0f;
        hp->h = 15.0f;
        hp->d = 0;
        
        hp->prm2 = 0;
        hp->prm1 = 0;
        hp->prm0 = 0;
        hp->prm3 = sys->onow;
        
        op->mode0 = 1;
        return;
    }
    
    hp = &sys->mwalp[op->ct0];
    
    hp->px = op->mlwP->owP->mtx[12]; 
    hp->py = op->mlwP->owP->mtx[13];
    hp->pz = op->mlwP->owP->mtx[14];
}

// 99.96% matching
void bhObjClpn(O_WRK* op)
{
    BH_PWORK* pp;   
    HAIR_WORK* hair; 
    int i;      
    short ax, ay;   
    short rx, ry;        
    NJS_POINT3* p3p, *g3p, *psp; 
    NJS_POINT3 ps1, ps2, ps3, ps4, ps5;  
    float px, py, pz; // px is not from DWARF
    float ln;       
    float tmp;        // not from DWARF
    
    if (((op->flg & 0x80)) && (op->mode0 != 2))
    {
        switch (op->mode0) 
        {                          
        case 0:
            op->flg |= 0x1000;
            
            if (!(op->flg & 0x100000))
            {
                op->flg |= 0x100000;
                
                if (op->lkwkp == (unsigned char*)plp) 
                {
                    op->exp0 = &sys->pletcp[4096];
                    op->exp3 = sys->pletcp;
                }
                else 
                {
                    op->exp0 = bhGetFreeMemory(0x4000, 32);
                    op->exp3 = bhGetFreeMemory(0x1000, 32);
                }
            }
            
            op->ct0 = 0;
            op->ct1 = 0;
            op->ct2 = 0;
            op->ct3 = 0;
            
            p3p = (NJS_POINT3*)op->exp0;
            
            for (i = 128; i-- != 0; p3p++) 
            {
                p3p->x = 0;
                p3p->y = 0; 
                p3p->z = 0;
            } 
            
            op->xn  = op->yn  = op->zn  = 0;
            op->gpx = op->gpy = op->gpz = 0;
            
            op->spd = 0;
            
            op->mode0 = 1;
            break;
        case 1:
            pp = (BH_PWORK*)op->lkwkp;
            
            hair = (HAIR_WORK*)op->exp3;
            
            if ((pp->flg2 & 0x2))
            {
                pp->flg2 &= ~0x2;
                
                npSetMemoryL((unsigned int*)op->exp0, 0x1000, 0);
                npSetMemory(op->exp3, sizeof(HAIR_WORK), 0);
                
                p3p = (NJS_POINT3*)op->exp0;
                
                for (i = 128; i-- != 0; p3p++) 
                {
                    p3p->x = 0;
                    p3p->y = 0;
                    p3p->z = 0;
                } 
                
                op->xn  = op->yn  = op->zn  = 0;
                op->gpx = op->gpy = op->gpz = 0;
                
                op->spd = 0;
            }
            
            bhCalcHair(op, pp);

            p3p = (NJS_POINT3*)op->exp0 + op->ct0;
            
            tmp = -hair->spx * 0.25f;
            py  = -hair->spy * 0.25f;
            pz  = -hair->spz * 0.25f;
            
            op->xn += 0.333f * (tmp - op->xn);
            op->yn += 0.333f * (py  - op->yn);
            op->zn += 0.333f * (pz  - op->zn);  
            
            px = op->mlwP->owP->mtx[12];
            py = op->mlwP->owP->mtx[13];
            pz = op->mlwP->owP->mtx[14];
            
            p3p->x = op->xn;
            p3p->y = op->yn;
            p3p->z = op->zn;
            
            g3p = (NJS_POINT3*)&op->exp0[4096]; 
            psp = (NJS_POINT3*)&op->exp0[8192];

            rx = 0;
            ry = 0;
            
            for (i = 0; i < 4; psp++, g3p++, i++) 
            {
                p3p = (NJS_POINT3*)op->exp0 + ((op->ct0 - (3 * i)) & 0x7F);
                
                njUnitMatrix(NULL);
                
                njRotateY(NULL, hair->ay);
                
                njCalcPoint(NULL, p3p, &ps1);
                
                if ((pp->flg2 & 0x100))
                {
                    g3p->x = px;
                    g3p->y = py - 1.0f;
                    g3p->z = pz;
                } 
                else
                {
                    g3p->x = px + ps1.x;
                    
                    op->spd += p3p->y;
                    op->spd -= 0.333f;
                    
                    if (op->spd > 0.5f)
                    {
                        op->spd = 0.5f;
                    }
                    
                    if (op->spd < -0.5f)
                    {
                        op->spd = -0.5f;
                    }
                    
                    g3p->y = op->spd + (py + ps1.y);
                    g3p->z = pz + ps1.z;
                }
                
                ps2.x = 0;
                ps2.y = 1.0f;
                ps2.z = 0; 
                
                njCalcPoint(&pp->mlwP->owP[5].mtx, &ps2, &ps3);
                
                ps2.x = g3p->x - ps3.x;
                ps2.y = g3p->y - ps3.y;
                ps2.z = g3p->z - ps3.z;
                
                ln = njSqrt((ps2.x * ps2.x) + (ps2.z * ps2.z));
                
                if (ln < 1.0f) 
                {
                    ps2.y = 0;
                        
                    njUnitVector(&ps2);
                    
                    g3p->x = ps3.x + ps2.x;
                    g3p->z = ps3.z + ps2.z; 
                }
                
                psp->x += 0.333f * (g3p->x - psp->x);
                psp->y += 0.333f * (g3p->y - psp->y);
                psp->z += 0.333f * (g3p->z - psp->z);
                
                ps1.x = psp->x - px;
                ps1.y = psp->y - py;
                ps1.z = psp->z - pz;
                
                if (njScalor(&ps1) > 0.5f)
                {
                    njUnitVector(&ps1);
                    
                    ps1.x *= 0.5f;
                    ps1.y *= 0.5f;
                    ps1.z *= 0.5f;
                    
                    psp->x = px + ps1.x;
                    psp->y = py + ps1.y;
                    psp->z = pz + ps1.z;
                }
                
                ps4.x =  0;
                ps4.y =  1.8f;
                ps4.z = -0.3f;
                
                njCalcPoint(&pp->mlwP->owP[5].mtx, &ps4, &ps5);
                
                ps5.y = ps3.y;
                
                ps2.x = psp->x - ps5.x;
                ps2.y = psp->y - ps5.y;
                ps2.z = psp->z - ps5.z;
                
                ln = njSqrt((ps2.x * ps2.x) + (ps2.z * ps2.z));
                
                if (ln < 1.3f) 
                {
                    ps2.y = 0;
                        
                    njUnitVector(&ps2);
                    
                    psp->x = ps5.x + (1.3f * ps2.x);
                    psp->z = ps5.z + (1.3f * ps2.z);
                }
                
                ps1.x = psp->x - px;
                ps1.y = psp->y - py;
                ps1.z = psp->z - pz;
                
                if (njScalor(&ps1) > 0.5f) 
                {
                    njUnitVector(&ps1);
                    
                    ps1.x *= 0.5f;
                    ps1.y *= 0.5f;
                    ps1.z *= 0.5f;
                    
                    psp->x = px + ps1.x;
                    psp->y = py + ps1.y;
                    psp->z = pz + ps1.z;
                }
                
                njUnitMatrix(NULL);
                
                njRotateX(NULL, rx);
                njRotateY(NULL, ry);
                
                njCalcPoint(NULL, &ps1, &ps3);
                
                ay = 10430.381f * atan2f(ps3.x, 0.1f + ps3.z);
                ry -= ay;
                
                op->mlwP->objP[i].ang[1] = ay;
                
                ax = 182.04445f * (150.0f * -ps3.y);
                rx -= ax;
                
                op->mlwP->objP[i].ang[0] = ax;
                
                px = psp->x;
                py = psp->y;
                pz = psp->z;
                
                op->pv[i].x = px;
                op->pv[i].y = py;  
                op->pv[i].z = pz;
            }
            
            op->ct0 = (op->ct0 + 1) & 0x7F;
            break;
        }
    }
}

// 100% matching!
void bhObjWssg()
{

}
