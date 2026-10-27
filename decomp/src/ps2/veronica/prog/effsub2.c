#include "../../../ps2/veronica/prog/effsub2.h"
#include "../../../ps2/veronica/prog/effect.h"
#include "../../../ps2/veronica/prog/effsub3.h"
#include "../../../ps2/veronica/prog/effsub6.h"
#include "../../../ps2/veronica/prog/en12.h"
#include "../../../ps2/veronica/prog/en16.h"
#include "../../../ps2/veronica/prog/hitchk.h"
#include "../../../ps2/veronica/prog/hitchkl.h"
#include "../../../ps2/veronica/prog/main.h"
#include "../../../ps2/veronica/prog/njplus.h"
#include "../../../ps2/veronica/prog/ps2_NaColi.h"
#include "../../../ps2/veronica/prog/ps2_NaDraw.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaGraphics3D.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaSystem.h"
#include "../../../ps2/veronica/prog/ps2_NaTextureFunction.h"
#include "../../../ps2/veronica/prog/ps2_NinjaPtcl.h"
#include "../../../ps2/veronica/prog/zonzon1.h"

// 99.93% matching
void bhEff_E00_Mince(O_WRK* op) 
{
    NJS_POINT3 n;
    float spd;
    float size;
    NJS_POINT3 p1, p2;
    static UV_WORK BH_UVTAB0[9] = 
    {
        {  0.875f,  0.0f,    0.125f, 0.125f },
        {  0.75f,   0.0f,    0.125f, 0.125f },
        {  0.625f,  0.0f,    0.125f, 0.125f },
        {  0.0f,    0.0f,    0.125f, 0.125f },
        {  0.125f,  0.0f,    0.125f, 0.125f },
        {  0.25f,   0.0f,    0.125f, 0.125f },
        {  0.375f,  0.0f,    0.125f, 0.125f },
        {  0.5f,    0.0f,    0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK BH_UVTAB1[9] = 
    {
        {  0.875f,  0.125f,  0.125f, 0.125f },
        {  0.75f,   0.125f,  0.125f, 0.125f },
        {  0.625f,  0.125f,  0.125f, 0.125f },
        {  0.0f,    0.125f,  0.125f, 0.125f },
        {  0.125f,  0.125f,  0.125f, 0.125f },
        {  0.25f,   0.125f,  0.125f, 0.125f },
        {  0.375f,  0.125f,  0.125f, 0.125f },
        {  0.5f,    0.125f,  0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK BH_UVTAB2[9] = 
    {
        {  0.875f,  0.25f,   0.125f, 0.125f },
        {  0.75f,   0.25f,   0.125f, 0.125f },
        {  0.625f,  0.25f,   0.125f, 0.125f },
        {  0.0f,    0.25f,   0.125f, 0.125f },
        {  0.125f,  0.25f,   0.125f, 0.125f },
        {  0.25f,   0.25f,   0.125f, 0.125f },
        {  0.375f,  0.25f,   0.125f, 0.125f },
        {  0.5f,    0.25f,   0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK BH_UVTAB3[9] = 
    {
        {  0.875f,  0.375f,  0.125f, 0.125f },
        {  0.75f,   0.375f,  0.125f, 0.125f },
        {  0.625f,  0.375f,  0.125f, 0.125f },
        {  0.0f,    0.375f,  0.125f, 0.125f },
        {  0.125f,  0.375f,  0.125f, 0.125f },
        {  0.25f,   0.375f,  0.125f, 0.125f },
        {  0.375f,  0.375f,  0.125f, 0.125f },
        {  0.5f,    0.375f,  0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK* uvtble[4] = 
    {
        BH_UVTAB0,
        BH_UVTAB1,
        BH_UVTAB2,
        BH_UVTAB3
    };

    switch (op->mode0) 
    {
    case 0:
        op->flg    |= 0x4180000;
        op->stflg  |= 0x20;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src  = 8;
        op->bl_dst  = 3;
        
        op->mtn_no  = op->type & 0x3;
        op->frm_no  = 4.0f * (-rand() / -2.147483648E9f);
        
        op->sxb     = op->sx;
        op->syb     = op->sy;
        
        spd         = 0.5f + (0.6f * (-rand() / -2.147483648E9f));
        
        op->xn      = spd * -njSin(op->ay);
        op->zn      = spd * -njCos(op->ay);
        op->yn      = (1.4f * (-rand() / -2.147483648E9f)) - 0.6f;
        
        op->ct0     = 0;
        op->ct1     = 4.0f * (-rand() / -2.147483648E9f);
        op->ct2     = 1;

        if ((op->type == 0 )|| (op->type == 2)) 
        {
            op->ct3 = 30.0f + (20.0f * (-rand() / -2.147483648E9f));
        } 
        else 
        {
            op->ct3 = 10.0f + (5.0f * (-rand() / -2.147483648E9f));
        }        

        op->mode0 = 1;
    case 1:
        if ((op->ct0 == op->ct3) || (op->ct2 == 0)) 
        {
            op->mode0 = 2;
            
            op->ct0   = 0;
        } 
        else 
        {
            op->frm_no++;
            
            if (op->frm_no > 7) 
            {
                op->frm_no = 3;
            }
            
            op->ct0++;
        }

        if ((!(op->type & 0x100)) && (op->ct0 < 10)) 
        {
            sys->ef.id     = 296;
            
            sys->ef.flg    = 1;
            sys->ef.type   = 1;
            
            sys->ef.px  = op->px;
            sys->ef.py  = op->py;
            sys->ef.pz  = op->pz;
            
            size = 0.1f + (0.3f * (-rand() / -2.147483648E9f));
            
            sys->ef.sx = size;
            sys->ef.sy = size;
            sys->ef.sz = size;
            
            sys->ef.ax     = 0;
            sys->ef.ay     = 0;
            
            sys->ef.mdlver = op->mdlver;
            
            bhSetEffectTb(&sys->ef, NULL, NULL, 0);
            
            op->ct1 = 4.0f * (-rand() / -2.147483648E9f);
        }
        
        break;
    case 2:
        op->ct0++;
        
        if (op->ct0 >= 8) 
        {
            op->flg = 0;
            return;
        }
        
        bhEff_SetBaseColor(op, ((0xFF - (op->ct0 << 5)) << 24) | 0xFFFFFF);
        
        op->sx *= 0.9f;
        op->sy *= 0.9f;
        break;
    }

    if (op->ct2 != 0) 
    {
        op->px += op->xn;
        op->pz += op->zn;
        op->py += op->yn;
        
        op->yn -= 0.06f;

        p1.x = op->pxb;
        p1.y = op->pyb - 0.5f;
        p1.z = op->pzb;
        
        p2.x = op->px;
        p2.y = op->py - 0.5f;
        p2.z = op->pz;

        op->xn *= 0.97f;
        op->zn *= 0.97f;

        if (bhCollisionCheckLine2(&p1, &p2, 0x4400, -1) != NULL) 
        {
            bhGetHitCollisionNormal(&n);
            
            njUnitVector(&n);
            
            if (n.y > 0.9f) 
            {
                op->ct2 = 0;
            } 
            else 
            {
                op->xn = 0;
                op->zn = 0;
                op->yn = 0;
            }
            
            op->px = p2.x;
            op->py = 0.5f + p2.y;
            op->pz = p2.z;
        }
    }

    bhEff_SetUVInfo(op, &uvtble[op->mtn_no][op->frm_no], 0.0625f);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E00_DrawParticleSpr(O_WRK* op) 
{
   	P_WRK* pp;

    pp = (P_WRK*)op->exp0;
    
    if (!(op->stflg & 0x20)) 
    {
        njSetTexture(&sys->ef_tlist);
    }
    else
    {
        njSetTexture(op->txp[0]);
    }
    
    if ((op->flg & 0x20000000)) 
    {
        njTextureFilterMode(0);
    } 
    else 
    {
        njTextureFilterMode(1);
    }
    
    njColorBlendingMode(0, op->bl_src);
    njColorBlendingMode(1, op->bl_dst);
    
    njSetMatrix(NULL, cam.mtx);
    
    njPtclSpriteStart(op->tex_id, pp->col, 1);
    njPtclDrawSprite(pp->pos, pp->num, pp->sx, pp->sy);
    njPtclSpriteEnd();
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 6);
}

// 100% matching!
void bhEff_E00_DrawParticlePly(O_WRK* op)
{
    P_WRK* pp;

    pp = (P_WRK*)op->exp0;
    
    njSetMatrix(NULL, cam.mtx);
    
    njPtclPolygonStart(pp->col);
    njPtclDrawPolygon(pp->pos, pp->num, pp->sx);
    njPtclPolygonEnd();
}

// 100% matching!
void bhEff_E00_DropBlood(O_WRK* op)
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[6] = 
    {
        { 0.84375f,  0.15625f, 0.0625f, 0.0625f },
        { 0.90625f,  0.15625f, 0.0625f, 0.0625f },
        { 0.53125f,  0.21875f, 0.0625f, 0.0625f },
        { 0.59375f,  0.21875f, 0.0625f, 0.0625f },
        { 0.65625f,  0.21875f, 0.0625f, 0.0625f },
        { -1.0f,     0.0f,     0.0f,    0.0f    },
    };
    static UV_WORK BH_UVTAB1[6] = 
    {
        { 0.71875f,  0.21875f, 0.0625f,  0.0625f  },
        { 0.78125f,  0.21875f, 0.09375f, 0.09375f },
        { 0.875f,    0.21875f, 0.09375f, 0.09375f },
        { 0.9375f,   0.3125f,  0.0625f,  0.0625f  },
        { 0.9375f,   0.375f,   0.0625f,  0.0625f  },
        { -1.0f,     0.0f,     0.0f,     0.0f     }
    };
    static UV_WORK* uvtble[2] = 
    {
        BH_UVTAB0,
        BH_UVTAB1
    };

    switch (op->mode0) 
    {
    case 0:
        op->tex_id = 5;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->ct0    = 0;
        op->ct1    = 0;
        
        op->flg   |= 0x180000;
        
        op->sxb    = op->sx;
        op->syb    = op->sy;
        
        op->yn     = bhGetGroundPosition((NJS_POINT3*)&op->px) + 0.2f;
        
        op->ofy    = -0.71f;
        
        op->mode0++;
    case 1:
        uvp = &uvtble[0][op->ct1];
        
        if (uvp->u == -1.0f) 
        {
            op->ct1 = 1;
            
            uvp = &uvtble[0][1];
        }
        
        op->ct1++;

        op->py += op->ofy;
        
        if (op->ofy > -2.0f)
        {
            op->ofy -= 0.15f;
        }
        
        if (op->py < op->yn) 
        {
            op->py = op->yn;
            
            op->mode0++;
        }
        
        break;
    case 2:
        uvp = &uvtble[1][op->ct1];
        
        if (uvp->u == -1.0f) 
        {
            op->flg = 0;
            return;
        }
        
        op->ct1++;
        break;
    }

    bhEff_SetUVInfo(op, uvp, 0.0625f);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E00_Fire(O_WRK* op) 
{
    UV_WORK* uvp;
    BH_PWORK* pp;
    static UV_WORK BH_UVTAB0[11] = 
    {
        {  0.0f,    0.375f,  0.1875f, 0.1875f },
        {  0.1875f, 0.375f,  0.1875f, 0.1875f },
        {  0.375f,  0.375f,  0.1875f, 0.1875f },
        {  0.5625f, 0.375f,  0.1875f, 0.1875f },
        {  0.75f,   0.375f,  0.1875f, 0.1875f },
        {  0.0f,    0.5625f, 0.1875f, 0.1875f },
        {  0.1875f, 0.5625f, 0.1875f, 0.1875f },
        {  0.375f,  0.5625f, 0.1875f, 0.1875f },
        {  0.5625f, 0.5625f, 0.1875f, 0.1875f },
        {  0.75f,   0.5625f, 0.1875f, 0.1875f },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK BH_UVTAB1[11] = 
    {
        {  0.0f,    0.0f,    0.1875f, 0.1875f },
        {  0.1875f, 0.0f,    0.1875f, 0.1875f },
        {  0.375f,  0.0f,    0.1875f, 0.1875f },
        {  0.5625f, 0.0f,    0.1875f, 0.1875f },
        {  0.75f,   0.0f,    0.1875f, 0.1875f },
        {  0.0f,    0.1875f, 0.1875f, 0.1875f },
        {  0.1875f, 0.1875f, 0.1875f, 0.1875f },
        {  0.375f,  0.1875f, 0.1875f, 0.1875f },
        {  0.5625f, 0.1875f, 0.1875f, 0.1875f },
        {  0.75f,   0.1875f, 0.1875f, 0.1875f },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK BH_UVTAB2[11] = 
    {
        {  0.75f,   0.5625f, 0.1875f, 0.1875f },
        {  0.5625f, 0.5625f, 0.1875f, 0.1875f },
        {  0.375f,  0.5625f, 0.1875f, 0.1875f },
        {  0.1875f, 0.5625f, 0.1875f, 0.1875f },
        {  0.0f,    0.5625f, 0.1875f, 0.1875f },
        {  0.75f,   0.375f,  0.1875f, 0.1875f },
        {  0.5625f, 0.375f,  0.1875f, 0.1875f },
        {  0.375f,  0.375f,  0.1875f, 0.1875f },
        {  0.1875f, 0.375f,  0.1875f, 0.1875f },
        {  0.0f,    0.375f,  0.1875f, 0.1875f },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK* uvtble[3] = 
    {
        BH_UVTAB0,
        BH_UVTAB1,
        BH_UVTAB2
    };

    pp = (BH_PWORK*)op->lkwkp;
    
    if ((pp != NULL) && ((pp->stflg & 0x1000000))) 
    {
        return;
    }

    switch (op->mode0) 
    {
    case 0:
        op->flg    |= 0x4180000;
        
        op->tex_id  = 8;
        
        bhEff_SetBaseColor(op, 0x80FFFFFF);
        
        op->bl_src  = 8;
        op->bl_dst  = 10;
        
        op->sxb     = op->sx;
        op->syb     = op->sy;
        
        op->sx      = 0.1f;
        op->sy      = 0.1f;
        
        op->mtn_no  = 0;
        op->frm_no  = 0;
        
        op->mode0++;
        break;
    case 1:
        if (op->frm_no == 0) 
        {
            op->mtn_no++;
            
            op->frm_no = 10.0f * (-rand() / -2.1474836E9f);
            
            op->mode0++;
            
            op->sx      = op->sxb;
            op->sy      = op->syb;
        }

        op->sx += (op->sxb - op->sx) / 32.0f;
        op->sy += (op->syb - op->sy) / 32.0f;
        break;
    case 2:
        if (op->ct0-- == 0) 
        {
            op->mtn_no++;
            
            op->frm_no  = 0;
            
            op->mode0++;
        }
        
        break;
    case 3:
        if (op->frm_no == 0)
        {
            op->flg = 0;
            return;
        }
        
        op->sx += (0.01f - op->sx) / 32.0f;
        op->sy += (0.01f - op->sy) / 32.0f;
        break;
    }

    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }

    bhEff_SetUVInfo(op, uvp, 0.125f);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E00_Acid(O_WRK* op) 
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[8] = 
    {
        {  0.25f,   0.0f,    0.125f,  0.125f  },
        {  0.5f,    0.0f,    0.125f,  0.125f  },
        {  0.75f,   0.0f,    0.125f,  0.125f  },
        {  0.0f,    0.125f,  0.1875f, 0.1875f },
        {  0.1875f, 0.125f,  0.1875f, 0.1875f },
        {  0.375f,  0.125f,  0.1875f, 0.1875f },
        {  0.0f,    0.3125f, 0.1875f, 0.1875f },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK* uvtble[1] = 
    {
        BH_UVTAB0
    };

    switch (op->mode0) 
    {
    case 0:
        op->tex_id  = 2;
        
        op->bl_src  = 8;
        op->bl_dst  = 6;
        
        op->flg    |= 0x180000;
        op->stflg  &= ~0x20;
        
        op->sxb     = op->sx;
        op->syb     = op->sy;
        
        op->yn      = 0.2f + (0.4f * (-rand() / -2.147483648E9f));
        
        op->ct0     = (int)(10.0f * (-rand() / -2.147483648E9f)) + 10;
        op->ct2     = 64;
        
        op->mode0   = 1;
    case 1:
        if (op->ct0-- == 0) 
        {
            op->flg = 0;
            return;
        }

        uvp = &uvtble[0][op->ct1];
        
        if (uvp->u < 0)
        {
            op->ct1--;
            
            uvp      = &uvtble[0][op->ct1];
            
            op->ct2 -= 8;
            
            if (op->ct2 < 5)
            {
                op->ct2 = 0;
            }
        }
        
        op->ct1++;
        
        bhEff_SetBaseColor(op, (op->ct2 << 24) | 0xD2CDC8);
        bhEff_SetUVInfo(op, uvp, 0.09375f);

        op->px += op->xn;
        op->py += op->yn;
        op->pz += op->zn;
        
        op->xn *= 0.9f;
        op->yn *= 0.9f;
        op->zn *= 0.9f;
        
        op->xn += (0.2f * (-rand() / -2.147483648E9f)) - 0.1f;
        op->zn += (0.2f * (-rand() / -2.147483648E9f)) - 0.1f;

        if (sys->ef_trsn < 512) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }

        break;
    }
}

// 100% matching!
void bhEff_E00_AcidGenerator(O_WRK* op) 
{
    UV_WORK* uvp;
    O_WRK* owp; // not from DWARF
    
    owp = (O_WRK*)op->lkwkp;
    
    if ((owp == NULL) || (!(owp->stflg & 0x1000000)))
    {
        switch (op->mode0)
        {                          
        case 0:
            op->ct0 = 5;
            
            op->mode0++;
        case 1:
            if (op->ct0-- == 0)
            {
                op->mode0++;
                return;
            }
            
            break;
        case 2:
            sys->ef.id = 257;
            
            sys->ef.flg = 1;
            sys->ef.type = 0;
            
            sys->ef.px = op->px;
            sys->ef.py = op->py;
            sys->ef.pz = op->pz;
            
            sys->ef.sx = op->sx;
            sys->ef.sy = op->sy;
            sys->ef.sz = op->sz;
            
            sys->ef.ax = 0;
            sys->ef.ay = 0;
            
            bhSetEffectTb(&sys->ef, NULL, NULL, 0);
            
            sys->ef.px = op->px + (0.5f * (op->sx * ((-rand() / -2.1474836E9f) - 0.5f)));
            sys->ef.py = op->py + (0.5f * (op->sy * ((-rand() / -2.1474836E9f) - 0.5f)));
            sys->ef.pz = op->pz + (0.5f * (op->sz * ((-rand() / -2.1474836E9f) - 0.5f)));
            
            sys->ef.sx = 0.4f * op->sx;
            sys->ef.sy = 0.4f * op->sy;
            sys->ef.sz = 0.4f * op->sz;
            
            sys->ef.ax = 0;
            sys->ef.ay = 0;
            
            bhSetEffectTb(&sys->ef, NULL, NULL, 0);
            
            if (op->type > 0)
            {
                op->type--;
            }
            
            if (op->type == 0) 
            {
                op->flg = 0;
            }
            
            break;
        }
    }
}

// 100% matching!
void bhEff_E00_BloodBurst(O_WRK* op)
{
    P_WK* p;
    BH_PWORK* pp;
    NJS_POINT3 v;
    int eno;
    O_WRK* owp; // not from DWARF
    static P_WK type0[2] = 
    {
        { 17,     1, 1.5f, 0.3f, 4      },
        { 0xFFFF, 2, 0.7f, 0.5f, 0xFFFF }
    };
    static P_WK type1[4] = 
    {
        { 17,     1, 1.5f, -0.8f, 0      },
        { 9,      1, 1.5f,  0.3f, 4      },
        { 17,     2, 1.5f,  0.3f, 2      },
        { 0xFFFF, 2, 1.0f,  0.5f, 0xFFFF } 
    };
    static P_WK type2[5] = 
    {
        { 17, 1, 1.5f,  0.1f, 0      },
        { 17, 1, 1.5f, -0.8f, 0      },
        { 9,  1, 1.8f,  0.3f, 4      },
        { 17, 2, 2.3f,  0.3f, 2      },
        { 9,  1, 1.7f,  0.5f, 0xFFFF }
    };
    static P_WK type3[2] = 
    {
        { 9,  2, 1.8f, 0.3f, 3      },
        { 10, 1, 1.7f, 0.5f, 0xFFFF }
    };
    static P_WK type4[5] = 
    {
        { 17,     1, 1.5f, 0.1f, 0      },
        { 0xFFFF, 2, 1.5f, 0.8f, 0      },
        { 10,     2, 1.3f, 0.6f, 6      },
        { 17,     2, 1.5f, 0.4f, 0      },
        { 9,      1, 1.0f, 0.7f, 0xFFFF }
    };
    static P_WK type5[2] = 
    {
        { 17, 1, 1.5f, 0.8f, 0      },
        { 10, 2, 1.3f, 0.6f, 0xFFFF }
    };
    static P_WK type6[1] = 
    {
        { 17, 2, 1.5f, 0.8f, 0xFFFF }
    };
    static P_WK* TypeList[7] = 
    {
        type0,
        type1,
        type2,
        type3,
        type4,
        type5,
        type6
    };

    pp = (BH_PWORK*)op->lkwkp;
    
    if ((pp != NULL) && ((pp->stflg & 0x1000000))) 
    {
        return;
    }

    switch (op->mode0) 
    {
    case 0:
        njUnitVector((NJS_VECTOR*)&op->xn);
        
        op->frm_no = 0;
        
        op->mode0++; 
    case 1:
    	while (TRUE) 
        {
			p = &TypeList[op->type][op->frm_no];

			if (p->type1 >= 0) 
			{
				if (p->spd > 0)
				{
					bhSetEffParticleMk2((BH_PWORK*)op->lkwkp, op->lkono, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&op->xn, BloodCol[op->mdlver][0], BloodCol[op->mdlver][1], p->type1);
				} 
				else
				{
					v.x = -op->xn;
					v.y = -op->yn;
					v.z = -op->zn;
					
					bhSetEffParticleMk2((BH_PWORK*)op->lkwkp, op->lkono, (NJS_POINT3*)&op->lox, &v, BloodCol[op->mdlver][0], BloodCol[op->mdlver][1], p->type1);
				}
			}

			if (p->type2 >= 0) 
			{
				sys->ef.id     = 296;
				
				sys->ef.flg    = 1;
				sys->ef.type   = p->type2;
				
				sys->ef.sx     = op->sx * p->size;
				sys->ef.sy     = op->sy * p->size;
				sys->ef.sz     = 1.0f;
				
				sys->ef.ax     = 0;
				sys->ef.ay     = 0;
				
				sys->ef.mdlver = op->mdlver;

				owp = (O_WRK*)op->lkwkp;
				
				njCalcPoint(&owp->mlwP->owP[op->lkono].mtx, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&sys->ef.px);

				eno = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
				
				if (eno != -1) 
				{
					eff[eno].xn = op->xn * p->spd;
					eff[eno].yn = op->yn * p->spd;
					eff[eno].zn = op->zn * p->spd;
				}
			}

			op->ct0 = p->wait;

			if (p->wait == -1)
			{
				op->flg = 0;
				return;
			}

			if (p->wait != 0) 
			{
				break;
			}

			op->frm_no++;
		}

        op->frm_no++;
        
        op->mode0++;
    case 2:
        if (op->ct0 != 0) 
        {
            op->ct0--;
            return;
        }
        
        op->mode0--;
    }
}

// 100% matching!
void bhEff_E00_Blood(O_WRK* op) 
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[13] =
    {
        {  0.3125f,  0.5f,     0.0625f,  0.0625f  },
        {  0.375f,   0.4375f,  0.125f,   0.125f   },
        {  0.5f,     0.4375f,  0.15625f, 0.15625f },
        {  0.65625f, 0.4375f,  0.15625f, 0.15625f },
        {  0.8125f,  0.4375f,  0.15625f, 0.15625f },
        {  0.0f,     0.5625f,  0.1875f,  0.1875f  },
        {  0.1875f,  0.5625f,  0.1875f,  0.1875f  },
        {  0.375f,   0.59375f, 0.1875f,  0.1875f  },
        {  0.5625f,  0.59375f, 0.1875f,  0.1875f  },
        {  0.0f,     0.78125f, 0.21875f, 0.21875f },
        {  0.21875f, 0.78125f, 0.21875f, 0.21875f },
        {  0.4375f,  0.78125f, 0.21875f, 0.21875f },
        { -1.0f,     0.0f,     0.0f,     0.0f     }
    };
    static UV_WORK BH_UVTAB1[12] = 
    {
        {  0.0f,     0.25f,    0.0625f,  0.0625f  },
        {  0.0f,     0.15625f, 0.09375f, 0.09375f },
        {  0.09375f, 0.15625f, 0.125f,   0.125f   },
        {  0.21875f, 0.15625f, 0.15625f, 0.15625f },
        {  0.375f,   0.15625f, 0.15625f, 0.15625f },
        {  0.0f,     0.0f,     0.15625f, 0.15625f },
        {  0.15625f, 0.0f,     0.15625f, 0.15625f },
        {  0.3125f,  0.0f,     0.15625f, 0.15625f },
        {  0.46875f, 0.0f,     0.15625f, 0.15625f },
        {  0.625f,   0.0f,     0.15625f, 0.15625f },
        {  0.78125f, 0.125f,   0.15625f, 0.15625f },
        { -1.0f,     0.0f,     0.0f,     0.0f     }
    };
    static UV_WORK BH_UVTAB2[14] =
    {
        {  0.0f,    0.3125f, 0.0625f, 0.0625f },
        {  0.0f,    0.5f,    0.0625f, 0.0625f },
        {  0.0f,    0.375f,  0.0625f, 0.0625f },
        {  0.0f,    0.4375f, 0.0625f, 0.0625f },
        {  0.0625f, 0.3125f, 0.125f,  0.125f  },
        {  0.1875f, 0.3125f, 0.125f,  0.125f  },
        {  0.3125f, 0.3125f, 0.125f,  0.125f  },
        {  0.4375f, 0.3125f, 0.125f,  0.125f  },
        {  0.5625f, 0.3125f, 0.125f,  0.125f  },
        {  0.6875f, 0.3125f, 0.125f,  0.125f  },
        {  0.8125f, 0.3125f, 0.125f,  0.125f  },
        {  0.0625f, 0.4375f, 0.125f,  0.125f  },
        {  0.1875f, 0.4375f, 0.125f,  0.125f  },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK* uvtble[3] = 
    {
        BH_UVTAB0,
        BH_UVTAB1,
        BH_UVTAB2
    };

    switch (op->mode0) 
    {
    case 0:
        op->tex_id  = 5;
        
        op->flg    |= 0x4180000;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src  = 8;
        op->bl_dst  = 6;
        
        op->ani_ct  = op->mdlver;
        
        op->sxb     = op->sx;
        op->syb     = op->sy;
        
        op->mtn_no  = op->type;
        op->frm_no  = 0;
        
        op->mode0++;
        break;
    case 1:
        if (op->frm_no == 0) 
        {
            op->flg = 0;
            return;
        }
        
        break;
    }

    op->px += op->xn;
    op->py += op->yn;
    op->pz += op->zn;
    
    op->xn *= 0.97f;
    op->yn *= 0.97f;
    op->zn *= 0.97f;
    
    op->yn -= 0.1f;

    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }

    bhEff_SetUVInfo(op, uvp, 0.125f);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E00_Dust(O_WRK* op)
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[23] = 
    {
        {  0.25f,   0.0f,    0.125f,  0.125f  },
        {  0.5f,    0.0f,    0.125f,  0.125f  },
        {  0.75f,   0.0f,    0.125f,  0.125f  },
        {  0.0f,    0.125f,  0.1875f, 0.1875f },
        {  0.0f,    0.125f,  0.1875f, 0.1875f },
        {  0.375f,  0.125f,  0.1875f, 0.1875f },
        {  0.375f,  0.125f,  0.1875f, 0.1875f },
        {  0.1875f, 0.3125f, 0.1875f, 0.1875f },
        {  0.1875f, 0.3125f, 0.1875f, 0.1875f },
        {  0.1875f, 0.3125f, 0.1875f, 0.1875f },
        {  0.0f,    0.5f,    0.1875f, 0.1875f },
        {  0.0f,    0.5f,    0.1875f, 0.1875f },
        {  0.0f,    0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        {  0.375f,  0.5f,    0.1875f, 0.1875f },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK* uvtble[1] = 
    {
        BH_UVTAB0
    };

    switch (op->mode0)
    {                             
    case 0:
        op->tex_id = 2;
        
        bhEff_SetAlign(op, 2);
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg |= 0x180000;
        
        op->sxb = op->sx;
        op->syb = op->sy;
        
        op->ct0 = 50;
        op->ct1 = 0;
        
        op->mode0 = 1;
    case 1:
        bhEff_SetBaseColor(op, (op->ct0 << 24) | 0x373737);
        
        op->ct0 -= 2;
        
        uvp = &uvtble[0][op->ct1];
        
        if (uvp->u < 0) 
        {
            op->flg = 0;
            return;
        }
        
        op->ct1++;
        
        bhEff_SetUVInfo(op, uvp, 0.09375f);
        
        op->px += op->xn;
        op->py += op->yn;
        op->pz += op->zn;
        
        op->xn *= 0.8f;
        op->yn *= 0.8f;
        op->zn *= 0.8f;
        
        if (sys->ef_trsn < 512) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }
        
        break;
    }
}

// 100% matching!
void bhEff_E02_SandDust(O_WRK* op)
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[12] = 
    {
        {  0.3125f,   0.09375f, 0.03125f,  0.03125f },
        { 0.34375f, 0.0859375f, 0.03125f,  0.03125f },
        {  0.3125f,     0.125f,  0.0625f,   0.0625f },
        {     0.0f,       0.0f, 0.09375f,  0.09375f },
        { 0.09375f,       0.0f, 0.09375f,  0.09375f },
        {  0.1875f,       0.0f, 0.09375f, 0.078125f },
        { 0.28125f,       0.0f, 0.09375f, 0.078125f },
        {   0.375f,       0.0f,   0.125f,    0.125f },
        {     0.5f,  0.015625f, 0.15625f,    0.125f },
        { 0.65625f,  0.015625f, 0.15625f,    0.125f },
        {  0.8125f,  0.015625f, 0.15625f,    0.125f },
        {    -1.0f,       0.0f,     0.0f,      0.0f }
	};
    static UV_WORK BH_UVTAB1[15] = 
    {
        { 0.33984375f,   0.359375f,  0.03125f,  0.03125f },
        {      0.375f,  0.3515625f,  0.03125f,  0.03125f },
        {    0.34375f, 0.38671875f,   0.0625f,   0.0625f },
        { 0.42578125f, 0.37109375f,   0.0625f,   0.0625f },
        {  0.5078125f,   0.359375f, 0.078125f, 0.078125f },
        { 0.58984375f, 0.31640625f,  0.09375f,  0.09375f },
        {   0.703125f,   0.328125f,  0.09375f,  0.09375f },
        {     0.8125f,  0.3203125f, 0.109375f, 0.109375f },
        { 0.01171875f,  0.2109375f,    0.125f,    0.125f },
        {    0.15625f,   0.203125f,  0.15625f,  0.15625f },
        {     0.3125f, 0.19921875f,  0.15625f,  0.15625f },
        {    0.46875f,  0.1484375f,  0.15625f,  0.15625f },
        {      0.625f,  0.1484375f,  0.15625f,  0.15625f },
        {    0.78125f,  0.1484375f,  0.15625f,  0.15625f },
        {       -1.0f,        0.0f,      0.0f,      0.0f }
    };
    static UV_WORK BH_UVTAB2[16] = 
    {
        {    0.0f,  0.4453125f, 0.1875f,     0.1875f },
        { 0.1875f,  0.4453125f, 0.1875f,     0.1875f },
        {  0.375f,     0.4375f, 0.1875f,     0.1875f },
        { 0.5625f,     0.4375f, 0.1875f,     0.1875f },
        {   0.75f,     0.4375f, 0.1875f,     0.1875f },
        {    0.0f,      0.625f, 0.1875f,     0.1875f },
        { 0.1875f, 0.62109375f, 0.1875f,     0.1875f },
        {  0.375f, 0.62109375f, 0.1875f,     0.1875f },
        { 0.5625f,      0.625f, 0.1875f, 0.18359375f },
        {   0.75f,      0.625f, 0.1875f, 0.18359375f },
        {    0.0f,     0.8125f, 0.1875f,     0.1875f },
        { 0.1875f, 0.80859375f, 0.1875f,     0.1875f },
        {  0.375f, 0.80859375f, 0.1875f,     0.1875f },
        { 0.5625f, 0.80859375f, 0.1875f,     0.1875f },
        {   0.75f, 0.80859375f, 0.1875f,     0.1875f },
        {   -1.0f,        0.0f,    0.0f,        0.0f }
    };
    static UV_WORK BH_UVTAB3[15] = 
    {
        {  0.3125f, 0.09375f, 0.03125f, 0.03125f },
        { 0.34375f, 0.09375f, 0.03125f, 0.03125f },
        {  0.3125f,   0.125f,  0.0625f,  0.0625f },
        {     0.0f,     0.0f, 0.09375f, 0.09375f },
        { 0.09375f,     0.0f, 0.09375f, 0.09375f },
        {  0.1875f,     0.0f, 0.09375f, 0.09375f },
        { 0.28125f,     0.0f, 0.09375f, 0.09375f },
        {     0.0f, 0.09375f, 0.09375f, 0.09375f },
        { 0.09375f, 0.09375f, 0.09375f, 0.09375f },
        {  0.1875f, 0.09375f,   0.125f,   0.125f },
        {   0.375f,     0.0f,   0.125f,   0.125f },
        {     0.5f,     0.0f, 0.15625f, 0.15625f },
        { 0.65625f,     0.0f, 0.15625f, 0.15625f },
        {  0.8125f,     0.0f, 0.15625f, 0.15625f },
        {    -1.0f,     0.0f,     0.0f,     0.0f }
    };
    static UV_WORK BH_UVTAB4[15] = 
    {
        {     0.0f,   0.375f, 0.03125f, 0.03125f },
        { 0.03125f,   0.375f, 0.03125f, 0.03125f },
        {     0.0f, 0.40625f,  0.0625f,  0.0625f },
        {  0.0625f,   0.375f, 0.09375f, 0.09375f },
        { 0.15625f,   0.375f, 0.09375f, 0.09375f },
        {    0.25f,   0.375f, 0.09375f, 0.09375f },
        { 0.34375f,   0.375f,   0.125f,   0.125f },
        { 0.46875f,   0.375f,   0.125f,   0.125f },
        {     0.0f, 0.21875f, 0.15625f, 0.15625f },
        { 0.15625f, 0.21875f, 0.15625f, 0.15625f },
        {  0.3125f, 0.21875f, 0.15625f, 0.15625f },
        { 0.46875f, 0.21875f, 0.15625f, 0.15625f },
        {   0.625f, 0.21875f, 0.15625f, 0.15625f },
        { 0.78125f, 0.21875f, 0.15625f, 0.15625f },
        {    -1.0f,     0.0f,     0.0f,     0.0f }
    };
    static UV_WORK BH_UVTAB5[11] =
    {
        {    0.0f, 0.5625f, 0.1875f, 0.1875f },
        { 0.1875f, 0.5625f, 0.1875f, 0.1875f },
        {  0.375f, 0.5625f, 0.1875f, 0.1875f },
        { 0.5625f, 0.5625f, 0.1875f, 0.1875f },
        {   0.75f, 0.5625f, 0.1875f, 0.1875f },
        {    0.0f,   0.75f, 0.1875f, 0.1875f },
        { 0.1875f,   0.75f, 0.1875f, 0.1875f },
        {  0.375f,   0.75f, 0.1875f, 0.1875f },
        { 0.5625f,   0.75f, 0.1875f, 0.1875f },
        {   0.75f,   0.75f, 0.1875f, 0.1875f },
        {   -1.0f,    0.0f,    0.0f,    0.0f }
    };
    static UV_WORK* uvtble[6] = 
    {
        BH_UVTAB0,
        BH_UVTAB1,
        BH_UVTAB2,
        BH_UVTAB3,
        BH_UVTAB4,
        BH_UVTAB5
    };

    switch (op->mode0) 
    {                              
    case 0:
        bhEff_SetAlign(op, 2);
        bhEff_SetBaseColor(op, 0xFF404040);
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->tex_id = op->mdlver + 82;
        
        op->ani_ct = 0;
        
        op->ct0 = 0;
        op->ct1 = 0;
        
        op->flg |= 0x180000;
        op->flg |= 0x4000000;
        
        op->sxb = op->sx;
        op->syb = op->sy;
        
        op->mode0 = 1;
    case 1:
        uvp = &uvtble[op->type + (op->mdlver * 3)][op->ct1];
        
        if (uvp->u < 0) 
        {
            op->flg = 0;
            return;
        }
        
        op->ct1++;
        
        op->px += op->xn;
        op->py += op->yn;
        op->pz += op->zn;
        
        if (op->type != 2) 
        {
            op->xn *= 0.9f;
            op->yn *= 0.9f;
            op->zn *= 0.9f;
        }
        
        op->yn -= 0.1f;
        
        bhEff_SetUVInfo(op, uvp, 0.0625f);
        
        if (sys->ef_trsn < 512) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }
        
        break;
    }
}

// 100% matching!
void bhEff_E02_SandParticle(O_WRK* op) 
{
	P_WRK* pp;
	NJS_POINT3* p, *v;
	int i;
    unsigned int col[3] = 
    {
        0xFF332417,
        0xFF322320,
        0xFF211816
    };
    float px, py, pz; // not from DWARF

    switch (op->mode0) 
    {                             
    case 0:
        pp = (P_WRK*)bhSetExtraEffectWork();
        
        if (pp == NULL) 
        {
            op->flg = 0;
            return;
        }
        
        op->exp0 = (unsigned char*)pp;
        
        p = pp->pos;
        v = pp->vec;
        
        pp->num = 16;
        
        for (i = 0; i < pp->num; i++, p++, v++)  
        {
            px   = op->px + (op->sx * (-rand() / -2.1474836E9f));
            p->x = px - (op->sx * (-rand() / -2.1474836E9f));
            
            py   = op->py + (op->sy * (-rand() / -2.1474836E9f));
            p->y = py - (op->sy * (-rand() / -2.1474836E9f));
            
            pz   = op->pz + (op->sz * (-rand() / -2.1474836E9f));
            p->z = pz - (op->sz * (-rand() / -2.1474836E9f));
            
            px   = op->xn + (op->sxb * (-rand() / -2.1474836E9f));
            v->x = px - (op->sxb * (-rand() / -2.1474836E9f));
            
            py   = op->yn + (op->syb * (-rand() / -2.1474836E9f));
            v->y = py - (op->syb * (-rand() / -2.1474836E9f));
           
            pz   = op->zn + (op->szb * (-rand() / -2.1474836E9f));
            v->z = pz - (op->szb * (-rand() / -2.1474836E9f));
         }
        
        pp->sx = 1.0f;
        pp->sy = 1.0f;
        
        pp->col = col[op->type];
        
        op->ct0 = 10.0f + (10.0f * (-rand() / -2.1474836E9f));
        
        op->flg |= 0x20000000;
        
        op->func = (void*)bhEff_E00_DrawParticlePly;
        
        op->mode0++;
        break;
    case 1:
        pp = (P_WRK*)op->exp0;
        
        p = pp->pos;
        v = pp->vec;
        
        for (i = 0; i < pp->num; i++, p++, v++) 
        {
            p->x += v->x;
            p->y += v->y;
            p->z += v->z;
            
            v->y -= 0.4f;
            v->x *= 0.8f;
            v->z *= 0.8f;
        }
        
        if (op->ct0-- == 0) 
        {
            op->flg = 0;
            pp->flg = 0;
            return;
        }
        
        if (sys->ef_fncn < 128) 
        {
            sys->ef_fnc[sys->ef_fncn] = op;
            
            sys->ef_fncn++;
        }
    }
}

// 99.93% matching
void bhEff_E02_SandParticle2(O_WRK* op) 
{
    O_WRK* opw;
    int eno;
    
    switch (op->mode0) 
    {                              
    case 0:
        op->ct0 = 120.0f + (30.0f * ( -rand() / -2.1474836E9f));
        
        op->mode0++;
        break;
    case 1:
        if (op->ct0-- == 0) 
        {
            op->mode0 = 0;
            
            if ((-rand() / -2.1474836E9f) < 0.3f) 
            {
                sys->ef.id = 254;
                
                sys->ef.flg = 1;
                sys->ef.type = 0;
                
                sys->ef.px = op->px;
                sys->ef.py = op->py;
                sys->ef.pz = op->pz;
                
                sys->ef.ax = 0;
                sys->ef.ay = 0;
                
                eno = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
                
                if (eno >= 0) 
                {
                    opw = &eff[eno];
                    
                    opw->sx = 3.0f;
                    opw->sy = 3.0f;
                    opw->sz = 3.0f;
                    
                    opw->sxb = 0.2f;
                    opw->syb = 0.3f;
                    opw->szb = 0.2f;
                }
            }
        }
        
        break;
    }
}

// 100% matching!
void bhEff_E03_Acid(O_WRK* op)
{
	ATR_WORK* hp; // needs use
	NJS_CAPSULE c;
    ACID_WORK param[7] = 
    {
        {  0, 0.0f,  0       },
        { 10, 0.25f, 0x12308 },
        { 20, 0.4f,  0x12308 },
        { 20, 0.0f,  0x12305 },
        { 20, 0.15f, 0x12304 },
        { 10, 0.25f, 0x12308 },
        { 20, 0.0f,  0x12305 }
    };
	UV_WORK* uvp;
    NJS_POINT3 n;
    static UV_WORK BH_UVTAB0[7] = 
    {
        {  0.0f,   0.4375f, 0.0625f,  0.0625f  },
        {  0.0f,   0.375f,  0.0625f,  0.0625f  },
        {  0.0f,   0.0f,    0.09375f, 0.09375f },
        {  0.0f,   0.09375f,0.09375f, 0.09375f },
        {  0.0f,   0.1875f, 0.125f,   0.125f   },
        {  0.625f, 0.1875f, 0.125f,   0.125f   },
        { -1.0f,   0.0f,    0.0f,     0.0f     }
    };
    static UV_WORK BH_UVTAB1[7] = 
    {
        {  0.15625f, 0.0f,     0.15625f, 0.15625f },
        {  0.3125f,  0.0f,     0.15625f, 0.15625f },
        {  0.46875f, 0.0f,     0.15625f, 0.15625f },
        {  0.15625f, 0.15625f, 0.15625f, 0.15625f },
        {  0.3125f,  0.15625f, 0.15625f, 0.15625f },
        {  0.46875f, 0.15625f, 0.15625f, 0.15625f },
        { -1.0f,     0.0f,     0.0f,     0.0f     }
    };
    static UV_WORK BH_UVTAB2[9] = 
    {
        {  0.625f,  0.0f,    0.1875f, 0.1875f },
        {  0.8125f, 0.0f,    0.1875f, 0.1875f },
        {  0.8125f, 0.1875f, 0.1875f, 0.1875f },
        {  0.0625f, 0.3125f, 0.1875f, 0.1875f },
        {  0.25f,   0.3125f, 0.1875f, 0.1875f },
        {  0.4375f, 0.3125f, 0.1875f, 0.1875f },
        {  0.625f,  0.3125f, 0.1875f, 0.1875f },
        {  0.8125f, 0.375f,  0.1875f, 0.125f  },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK* uvtble[3] = 
    {
        BH_UVTAB0,
        BH_UVTAB1,
        BH_UVTAB2
    };

    switch (op->mode0) 
    {
    case 0:
        bhEff_SetAlign(op, 2);
        
        if (op->type == 6) 
        {
            bhEff_SetBaseColor(op, 0xFFFF3E00);
            
            op->bl_src = 8;
            op->bl_dst = 10;
        } 
        else 
        {
            bhEff_SetBaseColor(op, 0xFF80A880);
            
            op->bl_src = 8;
            op->bl_dst = 6;
        }
        
        op->flg   |= 0x4180000;
        
        op->ct0    = 100;
        op->ct1    = 0; 
        
        op->stflg |= 0x20;
        
        op->sxb    = op->sx;
        op->syb    = op->sy;
        
        if (op->type != 5) 
        {
            op->flg |= 0x10;
        }
        else 
        {
            op->flg &= ~0x10;
        }
        
        op->mtn_no = 0;
        op->frm_no = 0;
        
        op->mode0++;
    case 1:
        if (op->frm_no == 0) 
        {
            op->mtn_no++;
            
            op->frm_no = 6.0f * (-rand() / -2.147483648E9f);
            
            op->mode0++;
        }
        
        break;
    case 2:
        if (op->ct0-- == 0) 
        {
            op->flg = 0;
            return;
        }
        
        break;
    case 3:
        if (op->frm_no == 0) 
        {
            op->flg = 0;
            return;
        }
        
        break;
    default:
        break;
    }

    if (op->mode0 < 3) 
    {
        op->px += op->xn;
        op->py += op->yn;
        op->pz += op->zn;
        
        op->xn *= 0.97f;
        op->yn *= 0.97f;
        op->zn *= 0.97f;
        
        op->yn -= 0.3f;

        if ((op->type != 0) && (op->type != 6)) 
        {
            c.c1.x = op->pxb;
            c.c1.y = op->pyb;
            c.c1.z = op->pzb;
            
            c.c2.x = op->px;
            c.c2.y = op->py;
            c.c2.z = op->pz;
            
            c.r    = 2.0f;

            if (npCollisionCheckCC(&c, &plp->watr) != 0) 
            {
                if ((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000))) 
                {
                    if (bhEne_CheckPlayEffectSE(param[op->type].se) == 0) 
                    {
                        bhEne_CallEffectSE((NJS_POINT3*)&op->px, param[op->type].se);
                    }
                    
                    plp->flg            |= 0x10004;
                    plp->stflg          |= 0x10000;
                    
                    plp->dvx             = op->xn;
                    plp->dvy             = op->yn;
                    plp->dvz             = op->zn;
                    
                    *(int*)&plp->mode0   = 2;
                    
                    plp->hp             -= param[op->type].dam;
                    
                    if ((-rand() / -2.147483648E9f) < param[op->type].poison) 
                    {
                        plp->stflg |= 0x80000;
                    }
                }
                
                sys->ef.id     = 257;
                
                sys->ef.flg    = 1;
                sys->ef.type   = 0;
                
                sys->ef.px     = op->px;
                sys->ef.py     = op->py;
                sys->ef.pz     = op->pz;
                
                sys->ef.sx     = op->sx;
                sys->ef.sy     = op->sy;
                sys->ef.sz     = op->sz;
                
                sys->ef.ax     = 0;
                sys->ef.ay     = 0;
                
                bhSetEffectTb((EF_WORK*)&sys->ef, NULL, NULL, 0);
                
                op->flg = 0;
                return;
            }
        }

        if ((op->flg & 0x10)) 
        {
            if (bhCollisionCheckLine2((NJS_POINT3*)&op->pxb, (NJS_POINT3*)&op->px, 0x4400, -1) != 0) 
            {
                bhGetHitCollisionNormal(&n);
                
                njUnitVector(&n);

                if (n.y > 0.9) // probably mistyped as double
                {
                    op->mtn_no++;
                } 
                else 
                {
                    op->px += n.x;
                    op->py += n.y;
                    op->pz += n.z;
                }

                op->frm_no = 0;
                
                op->mode0  = 3;

                if (op->type == 6)
                {
                    sys->ef.id    = 269;
                    
                    sys->ef.flg   = 1;
                    sys->ef.type  = 1;
                    
                    sys->ef.px    = op->px;
                    sys->ef.py    = op->py;
                    sys->ef.pz    = op->pz;
                    
                    sys->ef.sx    = 1.5f;
                    sys->ef.sy    = 2.0f;
                    sys->ef.sz    = 1.0f;
                    
                    sys->ef.ax    = 0;
                    sys->ef.ay    = 0;
                    
                    if (op->mdlver == 0) 
                    {
                        bhEne_CallEffectSE((NJS_POINT3*)&op->px, param[op->type].se);
                    }
                } 
                else 
                {
                    sys->ef.id    = 257;
                    
                    sys->ef.flg   = 1;
                    sys->ef.type  = 0;
                    
                    sys->ef.px    = op->px;
                    sys->ef.py    = op->py;
                    sys->ef.pz    = op->pz;
                    
                    sys->ef.sx    = op->sx;
                    sys->ef.sy    = op->sy;
                    sys->ef.sz    = op->sz;
                    
                    sys->ef.ax    = 0;
                    sys->ef.ay    = 0;
                    
                    if (bhEne_CheckPlayEffectSE(param[op->type].se) == 0) 
                    {
                        bhEne_CallEffectSE((NJS_POINT3*)&op->px, param[op->type].se);
                    }
                }
                
                bhSetEffectTb((EF_WORK*)&sys->ef, NULL, NULL, 0);
            }
        }
        else if (op->yn < 0) 
        {
            op->mtn_no++;
            
            op->frm_no    = 0;
            
            op->mode0     = 3;
            
            sys->ef.id    = 257;
            
            sys->ef.flg   = 1;
            sys->ef.type  = 0;
            
            sys->ef.px    = op->px;
            sys->ef.py    = op->py;
            sys->ef.pz    = op->pz;
            
            sys->ef.sx    = op->sx;
            sys->ef.sy    = op->sy;
            sys->ef.sz    = op->sz;
            
            sys->ef.ax    = 0;
            sys->ef.ay    = 0;
            
            bhSetEffectTb((EF_WORK*)&sys->ef, NULL, NULL, 0);
            
            if (bhEne_CheckPlayEffectSE(param[op->type].se) == 0) 
            {
                bhEne_CallEffectSE((NJS_POINT3*)&op->px, param[op->type].se);
            }
        }
    }

    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.125f);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E03_Shadow(O_WRK* op) 
{
    BH_PWORK* pp;
    O_WRK* owp; // not from DWARF
    
    owp = (O_WRK*)op->lkwkp;
    
    switch (op->mode0) 
    {                             
    case 0:
        op->flg |= 0x8000080;
        op->flg &= ~0x200000;
        
        op->lkono = 0;
        
        op->mode0 = 1;
        break;
    case 1:
        if (!(owp->flg & 0x1)) 
        {
            op->flg = 0;
            return;
        }
        
        if (((owp->stflg & 0x8)) || ((owp->mdflg & 0x1)) || ((owp->stflg & 0x1000000))) 
        {
            op->flg |= 0x1000000;
            return;
        }
        
        op->flg &= ~0x1000000;
        
        sys->ef_mdf[sys->ef_mdfn++] = op;
        break;
    }
}

// 100% matching!
void bhEff_E05_Blood(O_WRK* op)
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[18] = 
    {
        {     0.0f,     0.0f, 0.03125f, 0.03125f },
        {     0.0f, 0.03125f,  0.0625f,  0.0625f },
        {     0.0f, 0.09375f,  0.0625f,  0.0625f },
        {  0.0625f, 0.09375f, 0.09375f, 0.09375f },
        {  0.0625f,     0.0f, 0.09375f, 0.09375f },
        { 0.15625f,     0.0f,   0.125f,   0.125f },
        { 0.28125f,     0.0f,   0.125f,   0.125f },
        { 0.40625f,     0.0f, 0.15625f, 0.15625f },
        {  0.5625f,     0.0f, 0.15625f, 0.15625f },
        { 0.71875f,     0.0f,  0.1875f,  0.1875f },
        {     0.0f,  0.1875f,  0.1875f,  0.1875f },
        {  0.1875f,  0.1875f, 0.21875f, 0.21875f },
        { 0.40625f,  0.1875f, 0.21875f, 0.21875f },
        {   0.625f,  0.1875f, 0.21875f, 0.21875f },
        {     0.0f, 0.40625f, 0.21875f, 0.21875f },
        { 0.21875f, 0.40625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.40625f, 0.21875f, 0.21875f },
        {    -1.0f,     0.0f,     0.0f,     0.0f }
    };
    static UV_WORK BH_UVTAB1[6] = 
    {
        { 0.15625f,   0.125f, 0.03125f, 0.03125f },
        { 0.15625f, 0.15625f, 0.03125f, 0.03125f },
        {  0.1875f,   0.125f,  0.0625f,  0.0625f },
        {    0.25f,   0.125f,  0.0625f,  0.0625f },
        {  0.3125f,   0.125f,  0.0625f,  0.0625f },
        {    -1.0f,     0.0f,     0.0f,     0.0f }
    };
    static UV_WORK* uvtble[2] = 
    {
        BH_UVTAB0,
        BH_UVTAB1
    };

    switch (op->mode0) 
    {                            
    case 0:
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->ct0 = 0;
        op->ct1 = 0;
        
        op->flg |= 0x4180000;
        op->stflg |= 0x20;
        
        op->sxb = op->sx;
        op->syb = op->sy;
        
        op->yn = op->py;
        
        op->mode0 = 1;
    case 1:
        uvp = &uvtble[op->type][op->ct1];
        
        if (uvp->u < 0) 
        {
            op->flg = 0;
            return;
        }

        op->ct1++;
        
        bhEff_SetUVInfo(op, uvp, 0.0625f);
        
        op->py = op->yn + op->sy;
        
        if (sys->ef_trsn < 512) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }
    }
}

// 100% matching!
void bhEff_E06_Rinpun(O_WRK* op) 
{
    P_WRK* pp;
    NJS_POINT3* p, *v;
    unsigned int col[3] = 
    {
        0x000C180A,
        0x000B1405,
        0x000A1B07
    };
    int i;
    float px, py, pz; // not from DWARF
    
    switch (op->mode0) 
    {                              
    case 0:
        pp = (P_WRK*)bhSetExtraEffectWork();
        
        if (pp == NULL)
        {
            op->flg = 0;
            return;
        }
        
        op->exp0 = (unsigned char*)pp;
        
        p = pp->pos;
        v = pp->vec;
        
        pp->num = 32;
        
        for (i = 0; i < pp->num; i++, p++, v++) 
        {
            p->x = (op->px + (10.0f * (-rand() / -2.1474836E9f))) - 5.0f;
            p->y = (op->py + (3.0f  * (-rand() / -2.1474836E9f))) - 1.5f;
            p->z = (op->pz + (10.0f * (-rand() / -2.1474836E9f))) - 5.0f;
            
            op->xn = 0.1f + (0.2f * (-rand() / -2.1474836E9f));
            
            v->x = op->xn * -njSin(op->ay);
            v->z = op->xn * -njCos(op->ay);
            
            if (op->type == 0) 
            {
                v->y = 0.1f + (-0.3f * (-rand() / -2.1474836E9f));
            }
            else 
            {
                v->y = (0.3f * (-rand() / -2.1474836E9f)) - 0.1f;
            }
        }

        pp->sx = op->sx;
        pp->sy = op->sy;
        
        op->ct0 = 30.0f + (30.0f * (-rand() / -2.1474836E9f));
        op->ct1 = ((plp->flg & 0x4)) ? 0 : 1;
        op->ct2 = 0xFF;
        
        op->tex_id = (int)(4.0f * (-rand() / -2.1474836E9f)) + 1;
        
        op->flg |= 0x20000000;
        op->stflg |= 0x20;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->func = (void*)bhEff_E00_DrawParticleSpr;
        
        op->mode0++;
        break;
    case 1:
        pp = (P_WRK*)op->exp0;
        
        p = pp->pos;
        v = pp->vec;
        
        for (i = 0; i < pp->num; i++, p++, v++) 
        {
            p->x += v->x + ((0.3f * (-rand() / -2.1474836E9f)) - 0.15f);
            p->y += v->y + ((0.3f * (-rand() / -2.1474836E9f)) - 0.15f);
            p->z += v->z + ((0.3f * (-rand() / -2.1474836E9f)) - 0.15f);
            
            v->x *= 0.95f;
            v->z *= 0.95f;
            v->y -= 0.01f;
            
            if (v->y < -0.1f) 
            {
                v->y = -0.1f;
            }

            // the variables were originally unitialized here
            px += p->x;
            py += p->y;
            pz += p->z;
        }

        px /= pp->num;
        py /= pp->num;
        pz /= pp->num;
        
        op->cspr.c.x = px;
        op->cspr.c.y = py;
        op->cspr.c.z = pz;
        
        op->cspr.r = 5.0f;
        
        if ((npCollisionCheckSC(&op->cspr, &plp->watr) != 0) && (op->ct1 != 0) && ((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000)))) 
        {
            int i;
            O_WRK* pp;
            
            *(int*)&plp->mode0 = 2;
            
            plp->hp -= 3;
            
            if (plp->hp < 0) 
            {
                plp->hp = 0;
            }
            
            if ((-rand() / -2.1474836E9f) < 0.2f) 
            {
                plp->stflg |= 0x80000;
            }

            pp = eff;

            for (i = 0; i < 512; i++, pp++)
            {
                if (((pp->flg & 0x1)) && (pp->id == op->id)) 
                {
                    pp->ct1 = 0;
                }
            }
        }

        pp->col = col[(int)(3.0f * (-rand() / -2.1474836E9f))];
        pp->col |= op->ct2 << 24;
        
        op->tex_id += (int)(2.0f * (-rand() / -2.1474836E9f));
        
        if (op->tex_id > 4) 
        {
            op->tex_id = 1;
        }
        
        if (op->ct0 < 30) 
        {
            op->ct2 -= 9;
            
            if (op->ct2 < 4) 
            {
                op->ct2 = 4;
            }
        }
        
        if (op->ct0-- == 0) 
        {
            op->flg = 0;
            pp->flg = 0;
            return;
        }
        
        if (sys->ef_fncn < 128)
        {
            sys->ef_fnc[sys->ef_fncn] = op;
            
            sys->ef_fncn++;
        }
    }
}

// 100% matching!
void bhEff_E11_SearchLight(O_WRK* op)
{
    switch (op->mode0) 
    {
    case 0:
        op->ct0  = 0;
        op->ct1  = 0;
        op->ct2  = 0;
        
        op->ax   = 0;
        
        op->func = (void*)bhEff_E11_SearchLightDraw;
        
        op->mode0 = 0xFF;
        
        if (op->exp0 != NULL) 
        {
            return;
        }
        
        op->exp0 = (unsigned char*)bhSetExtraEffectWork();
        return;
    case 1:
        if (++op->ct0 >= 32) 
        {
            op->mode0++;
        }
        
        op->ax += (short)(op->axp - op->ax) / 16;
        
        op->ct1 += 10;
        break;
    case 2:
        op->ct0 = 32;
        
        op->ax  = op->axp;
        
        if (op->ct2 != 0) 
        {
            op->ct1 += 14;
        } 
        else 
        {
            op->ct1 += 6;
        }
        
        break;
    case 3:
        op->ct0 -= 3;
        
        if (op->ct0 <= 0) 
        {
            op->ct0   = 0;
            
            op->mode0 = 0;
        }
        
        op->ax   = op->ax - (op->ax / 8);
        
        op->ct1 += 2;
        break;
    case 0xFF:
        return;
    }

    op->sx = -rand() / -2.147483648E9f;
    op->sy = -rand() / -2.147483648E9f;
    op->sz = -rand() / -2.147483648E9f;

    if (sys->ef_fncn < 128)
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 99.89% matching
void bhEff_E11_SearchLightDraw(O_WRK* op)
{
    NJS_POINT3 p1, *pt, pos, n;        
    float rate[16];      
    char atr[192];       
    NJS_MATRIX* mtx;    
    int i;               
    NJS_POINT3* p;       
    NJS_COLOR c[3];       
    NJS_POINT3 area[3];  
    NJS_POINT3COL p3c;    

    mtx = &((O_WRK*)op->lkwkp)->mlwP->owP[4].mtx;
    
    p1.x = (*mtx)[12];
    p1.y = (*mtx)[13]; 
    p1.z = (*mtx)[14];
    
    p = pt = (NJS_POINT3*)&op->exp0[4];
    
    for (i = 0; i < 16; i++, p++) 
    {
        pos.x = (100.0f * njCos(182.04445f * ((360 * i) / 16))) * tanf(0.0000958738f * op->ax); 
        pos.y = (100.0f * njSin(182.04445f * ((360 * i) / 16))) * tanf(0.0000958738f * op->ax);
        pos.z = -100.0f;
        
        n.x = pos.x;
        n.y = pos.y;
        n.z = 0; 
        
        njUnitVector(&n);
        
        njCalcVector(mtx, &n, &n);
        njCalcVector(cam.mtx, &n, &n);
        
        if (n.z > 0) 
        {
            n.z -= 0.3f;
            
            if (n.z < 0)
            { 
                n.z = 0;
            }
            
            if (n.z > 0.5f) 
            { 
                n.z = 0.5f;
            }
            
            rate[i] = n.z;
        } 
        else 
        {
            n.z = -n.z - 0.3f;
            
            n.z *= 1.6f;
            
            if (n.z < 0)
            {
                n.z = 0;
            }
            
            if (n.z > 1.0f) 
            {
                n.z = 1.0f;
            }
            
            rate[i] = n.z;
        }
        
        if ((sys->sp_flg & 0x8)) 
        {
            njCalcPoint(mtx, &pos, p);
            
            bhCollisionCheckLine2(&p1, p, 0x4400, -1);
            
            if ((bhEff_E11_CheckCollisionPlayer(&p1, p) != -1) && (op->mode0 == 2))
            {
                op->ct2 = 1;
            }
        }
    } 
    
    njGetSystemAttr((NJS_SYS_ATTR*)atr);
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 10);
    
    njFogDisable();
    
    njSetMatrix(NULL, cam.mtx);
    
    {
    p3c.p   = area;
    p3c.col = c;
    p3c.tex = NULL;
    p3c.num = 3;
        
    if (op->ct2 != 0) 
    {
        c[0].argb.a = 150.0f + (55.0f * op->sx);
        
        c[0].argb.r = op->ct0 * 3; 
        c[0].argb.g = 0;
        c[0].argb.b = 0;
        
        c[1].argb.r = op->ct0 * 2; 
        c[1].argb.g = 0;
        c[1].argb.b = 0;
    }
    else
    {
        c[0].argb.a = 150.0f + (55.0f * op->sx);
        
        c[0].argb.r = 0.8f * op->ct0;
        c[0].argb.g = 0.8f * op->ct0;
        c[0].argb.b = op->ct0; 
        
        c[1].argb.r = 0.4f * op->ct0;
        c[1].argb.g = 0.4f * op->ct0;
        c[1].argb.b = op->ct0; 
    }

    c[2] = c[1];
        
    area[0].x = p1.x;
    area[0].y = p1.y;
    area[0].z = p1.z;
        
    for (i = 0; i < 15; i++) 
    {
        area[1] = pt[i];
        area[2] = pt[i + 1];
       
        c[1].argb.a = (30.0f + (15.0f * op->sx)) * rate[i];
        c[2].argb.a = (30.0f + (15.0f * op->sx)) * rate[i + 1];
        
        njDrawPolygon3D(&p3c, 3, 0x60);
    }
        
    area[1] = pt[i];
    area[2] = pt[0]; 
        
    c[1].argb.a = (30.0f + (15.0f * op->sx)) * rate[i];
    c[2].argb.a = (30.0f + (15.0f * op->sx)) * rate[0]; 
        
    njDrawPolygon3D(&p3c, 3, 0x60);
    }
    
    {
    NJS_POINT3COL p3c;  
    NJS_POINT3 p[2];    
    NJS_COLOR c[2];     
    int id1, id2;           
    float rate;        
    
    p3c.p   = p;
    p3c.col = c;
    p3c.tex = NULL;
    p3c.num = 2;
   
    id1 = (op->ct1 / 16) & 0xF;
    id2 = (id1 + 1)      & 0xF;
        
    rate = (op->ct1 & 0xF) / 16.0f;
        
    p[0].x = p1.x;
    p[0].y = p1.y;
    p[0].z = p1.z;
        
    p[1].x = ((1.0f - rate) * pt[id1].x) + (rate * pt[id2].x);
    p[1].y = ((1.0f - rate) * pt[id1].y) + (rate * pt[id2].y);
    p[1].z = ((1.0f - rate) * pt[id1].z) + (rate * pt[id2].z);
        
    c[0].argb.a = 64.0f * op->sy;
    c[0].argb.r = op->ct2       << 6; 
    c[0].argb.g = (1 - op->ct2) << 6; 
    c[0].argb.b = 0; 
        
    c[1].argb.a = 128.0f * op->sz;
    c[1].argb.r = op->ct2       << 6;
    c[1].argb.g = (1 - op->ct2) << 6;
    c[1].argb.b = 0;
        
    njDrawLine3DEx(&p3c, 1, 0x40);
        
    id2 = id1 = ((op->ct1 / 16) + 8) & 0xF;
        
    p[1].x = ((1.0f - rate) * pt[id1].x) + (rate * pt[(id2 + 1) & 0xF].x);
    p[1].y = ((1.0f - rate) * pt[id1].y) + (rate * pt[(id2 + 1) & 0xF].y);
    p[1].z = ((1.0f - rate) * pt[id1].z) + (rate * pt[(id2 + 1) & 0xF].z); 
        
    njDrawLine3DEx(&p3c, 1, 0x40);
    }
    
    njSetSystemAttr((NJS_SYS_ATTR*)atr);
    
    if ((sys->st_flg & 0x2)) 
    {
        njFogEnable();
    }
}

// 100% matching!
int bhEff_E11_CheckCollisionPlayer(NJS_POINT3* p1, NJS_POINT3* p2)
{
    O_WORK* mtn;
    int num;
    int i, j;
    int ret;
    NJS_CNK_OBJECT* pObj;
    BH_PWORK* ep;

    ret  = -1;
    
    mtn  = plp->mlwP->owP;
    
    num  = plp->mlwP->obj_num;
    
    pObj = plp->mdl[3].objP;

    if (pObj != NULL) 
    {
        for (i = 0; i < num; pObj++, mtn++, i++) 
        {
            if ((pObj->model != NULL) && (bhCollisionCheckL2MDL(p1, p2, pObj->model, &mtn->mtx) != 0))
            {
                ret = i;
            }
        }
    }

    ep = ene;
    
    for (j = 0; j < sys->ewk_n; j++, ep++)
    {
        if (((ep->flg & 0x1)) && (((ep->id >= 41) && (ep->id <= 43)) || ((ep->id >= 91) && (ep->id <= 93)))) 
        { 
            mtn = ep->mlwP->owP;
            
            num = ep->mlwP->obj_num;
            
            pObj = ep->mdl[1].objP;
    
            if (pObj != NULL) 
            { 
                for (i = 0; i < num; pObj++, mtn++, i++) 
                {
                    if ((pObj->model != NULL) && (bhCollisionCheckL2MDL(p1, p2, pObj->model, &mtn->mtx) != 0)) 
                    {
                        ret = i;
                    }
                }
            }
        }
    }

    return ret;
}

// 100% matching!
void bhEff_E16_LaserSight(O_WRK* op) 
{
    O_WRK* opp; // not from DWARF

    opp = (O_WRK*)op->lkwkp;
    
    if ((opp->flg != 0) ^ 1) 
    {
        op->flg = 0;
        return;
    }
    
    switch (op->mode0) 
    {                             
    case 0:
        op->func = (void*)bhEff_E16_LaserSightDraw;

        op->mode0++;
        break;
    default:
        if (!(opp->stflg & 0x1000000)) 
        {
            if (sys->ef_fncn < 128)
            {
                sys->ef_fnc[sys->ef_fncn] = op;
                
                sys->ef_fncn++;
            }
        }
        
        break;
    case 1:
        return;
    }
}

// 99.84% matching
void bhEff_E16_LaserSightDraw(O_WRK* op) 
{
    NJS_POINT4 p1, pt[16], pw, pos, n[16];       
    float rate[16], len;     
    char atr[192];      
    int i;            
    NJS_MATRIX* mtx;    
    int num;            
    NJS_POINT3* effp;   
    BH_PWORK* ep;        
    NJS_POINT4* p;       
    NJS_COLOR c[2];      
 
    mtx = &((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono].mtx;
    
    p1.x = (*mtx)[12];
    p1.y = (*mtx)[13];
    p1.z = (*mtx)[14];
    
    pos.x = 0;
    pos.y = 0;
    pos.z = -1500.0f;
    
    njCalcPoint(mtx, (NJS_POINT3*)&pos, (NJS_POINT3*)&pos);
    
    if ((sys->sp_flg & 0x8)) 
    {
        op->exp0 = (unsigned char*)bhCollisionCheckLine2((NJS_POINT3*)&p1, (NJS_POINT3*)&pos, 0x4400, -1);
        
        op->lox = pos.x;
        op->loy = pos.y;
        op->loz = pos.z;
        
        op->ct0 = bhEff_E11_CheckCollisionPlayer((NJS_POINT3*)&p1, (NJS_POINT3*)&pos);
        
        op->gpx = pos.x;
        op->gpy = pos.y;
        op->gpz = pos.z;
        
        bhEne16_AddEffPos((BH_PWORK*)op->lkwkp, (NJS_POINT3*)&pos);
    }
    else 
    {
        pos.x = op->gpx;
        pos.y = op->gpy;
        pos.z = op->gpz;
    }
    
    len = -njDistanceP2P((NJS_POINT3*)&p1, (NJS_POINT3*)&pos);

    p = pt;
    
    for (i = 0; i < 16; i++, p++)
    {
        pw.x = 0.07f * njCos(182.04445f * ((360 * i) / 16));
        pw.y = 0.07f * njSin(182.04445f * ((360 * i) / 16));
        pw.z = len;
        
        njCalcPoint4(mtx, &pw, p);
        
        n[i].x = pw.x;
        n[i].y = pw.y;
        n[i].z = 0;
        
        njUnitVector((NJS_VECTOR*)&n[i]);
        
        njCalcVector(mtx,     (NJS_VECTOR*)&n[i], (NJS_VECTOR*)&n[i]);
        njCalcVector(cam.mtx, (NJS_VECTOR*)&n[i], (NJS_VECTOR*)&n[i]);
        
        if (n[i].z > 0)
        {
            rate[i] = 0;
        }
        else 
        {
            n[i].z = -n[i].z - 0.3f;
            
            n[i].z *= 1.6f;
            
            if (n[i].z < 0) 
            {
                n[i].z = 0;
            }
            
            if (n[i].z > 1.0f) 
            {
                n[i].z = 1.0f;
            }
            
            rate[i] = n[i].z;
        }
    } 
    
    njGetSystemAttr((NJS_SYS_ATTR*)&atr);
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 10);
    
    njFogDisable();
    
    njSetMatrix(NULL, cam.mtx);
    
    {
    NJS_POINT3COL p3c; 
    NJS_POINT3 p[2];  
        
    p3c.p   = p;
    p3c.col = c;
    p3c.tex = 0;
    p3c.num = 2;
        
    p[0].x = p1.x;
    p[0].y = p1.y;
    p[0].z = p1.z;
        
    p[1].x = pos.x;
    p[1].y = pos.y;
    p[1].z = pos.z; 
        
    c[0].argb.a = 64;
    c[0].argb.r = 64; 
    c[0].argb.g = 0;
    c[0].argb.b = 0;
        
    c[1].argb.a = 128; 
    c[1].argb.r = 64;
    c[1].argb.g = 0;
    c[1].argb.b = 0; 
        
    njDrawLine3DEx(&p3c, 1, 0x40);
    
    ep = (BH_PWORK*)op->lkwkp;
        
    if (ep->type != 0) 
    { 
        num  = bhEne16_GetEffNum(ep);
        effp = bhEne16_GetEffPos(ep);
        
        for (i = 0; i < num; i++, effp++)
        {
            c[0].argb.a = c[1].argb.a = 40 - (i * 4);
            
            p[1].x = effp->x;
            p[1].y = effp->y;
            p[1].z = effp->z;
            
            njDrawLine3DEx(&p3c, 1, 0x40);
        } 
    }
    }
    
    {
    NJS_POINT3 area[3]; 
    NJS_POINT3COL p3c; 
    NJS_COLOR c[3];    
        
    p3c.p   = area;
    p3c.col = c;
    p3c.tex = NULL;
    p3c.num = 3;
        
    c[0].argb.a = 0; 
    c[0].argb.r = 0;
    c[0].argb.g = 0;
    c[0].argb.b = 0;
        
    area[0].x = p1.x;
    area[0].y = p1.y;
    area[0].z = p1.z;
        
    c[1].argb.r = 100;
    c[1].argb.g = 0;
    c[1].argb.b = 0;
        
    c[2] = c[1];
        
    for (i = 0; i < 15; i++) 
    {
        area[1] = *(NJS_POINT3*)&pt[i + 0]; 
        area[2] = *(NJS_POINT3*)&pt[i + 1];
        
        c[1].argb.a = 255.0f * rate[i + 0];
        c[2].argb.a = 255.0f * rate[i + 1]; 
        
        njDrawPolygon3D(&p3c, 3, 0x60);
    } 
        
    area[1] = *(NJS_POINT3*)&pt[i];
    area[2] = *(NJS_POINT3*)&pt[0];
        
    c[1].argb.a = 255.0f * rate[i];
    c[2].argb.a = 255.0f * rate[0];
        
    njDrawPolygon3D(&p3c, 3, 0x60);
        
    njSetSystemAttr((NJS_SYS_ATTR*)&atr);
        
    if ((sys->st_flg & 0x2)) 
    {
        njFogEnable();
    }
    }
}

// 99.62% matching
void bhEff_E12_Fire(O_WRK* op) 
{
    UV_WORK* uvp;            
    LGT_WORK* lp;           
    float dx, dz, ln;                  
    NJS_VECTOR v;            
    O_WRK* fireman, **ent; // ent needs use       
    char* exp0;            // not from DWARF
    O_WRK* temp;           // not from DWARF
    static UV_WORK BH_UVTAB0[10] = 
    {
        { 0.0f,    0.0f,    0.15625f, 0.1875f },
        { 0.1875f, 0.0f,    0.15625f, 0.1875f },
        { 0.375f,  0.0f,    0.15625f, 0.1875f },
        { 0.5625f, 0.0f,    0.15625f, 0.1875f },
        { 0.75f,   0.0f,    0.15625f, 0.1875f },
        { 0.0f,    0.1875f, 0.15625f, 0.1875f },
        { 0.1875f, 0.1875f, 0.15625f, 0.1875f },
        { 0.375f,  0.1875f, 0.15625f, 0.1875f },
        { 0.5625f, 0.1875f, 0.15625f, 0.1875f },
        { -1.0f,   0.0f,    0.0f,     0.0f    }
    };
    static UV_WORK BH_UVTAB1[9] = 
    {
        { 0.03125f, 0.46875f, 0.15625f, 0.21875f },
        { 0.25f,    0.46875f, 0.15625f, 0.21875f },
        { 0.46875f, 0.46875f, 0.15625f, 0.21875f },
        { 0.6875f,  0.46875f, 0.15625f, 0.21875f },
        { 0.03125f, 0.6875f,  0.15625f, 0.21875f },
        { 0.25f,    0.6875f,  0.15625f, 0.21875f },
        { 0.46875f, 0.6875f,  0.15625f, 0.21875f },
        { 0.6875f,  0.6875f,  0.15625f, 0.21875f },
        { -1.0f,    0.0f,     0.0f,     0.0f     }
    };
    static UV_WORK* uvtble[2] = 
    {
        BH_UVTAB0,
        BH_UVTAB1
    };

    switch (op->mode0) 
    {
    case 0:
        op->tex_id = 80;
        
        op->ani_ct = 1;
        
        bhEff_SetBaseColor(op, 0x80FFFFFF);
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        if (op->type == 0) 
        {
            op->flg |= 0x4180000;
            
            bhEff_SetAlign(op, 2);
            
            op->mtn_no = 0;
            op->frm_no = 8.0f * (-rand() / -2.1474836E9f);
        } 
        else
        {
            op->flg |= 0x24080000;
            
            bhEff_SetAlign(op, 3);
            
            op->mtn_no = (int)(2.0f * (-rand() / -2.1474836E9f));
            op->frm_no = 8.0f * (-rand() / -2.1474836E9f);
        }
        
        op->sxb = op->sx;
        op->syb = op->sy;
        
        if (op->type == 1) 
        {
            op->ct0 = (int)(200.0f * (-rand() / -2.1474836E9f)) + 200;
        }
        else if (op->type == 2) 
        {
            op->ct0 = 20;
        }
        
        op->ct1 = 0;
        op->ct3 = 300.0f * (-rand() / -2.1474836E9f);
        
        op->ah = 0.2f;
        op->aw = 1.0f;
        
        if (op->type == 1) 
        {
            fireman = (O_WRK*)rom->lgtp;
            
            lp = (LGT_WORK*)&fireman->mbp;
            
            if (!(lp->flg & 0x1)) 
            {
                lp->ct0 = 0;
                
                lp->lkono = 0; 
                
                lp->vx = 0;
                lp->vy = 0;
                lp->vz = 0;
            }
            
            lp->lkono++;
            
            lp->flg |= 0x3;
            
            lp->type = 13;
            
            lp->aspd = 2;
            
            lp->lkflg = 0;
            
            lp->lsrc = 4; 
            
            lp->nr = 15.0f;
            lp->fr = 20.0f + (lp->lkono / 10.0f);
            
            lp->vx += op->px;
            lp->vy += 5.0f + op->py;
            lp->vz += op->pz;
            
            lp->px = lp->vx / lp->lkono;
            lp->py = lp->vy / lp->lkono;
            lp->pz = lp->vz / lp->lkono;
            
            lp->r = 3.0f;
            lp->g = 1.3f;
            lp->b = 0.3f;
        }
        
        op->mode0++;
        op->mode1 = 0;
        
        op->func = (void*)bhEff_Draw3DSprite;
        
        if (op->type == 0)
        {
            op->mode0 = 6;
            return;
        }
        
        if ((op->type != 0) && (bhEne_CheckPlayEffectSE(74499) == 0))
        {
            bhEne_CallEffectSE((NJS_POINT3*)&op->px, 74499);
        }
    case 1:
        op->ah += 0.2f;
        
        if (op->ah > 1.0f) 
        {
            op->ah = 1.0f;
            
            op->mode0++;
        }
        
        break;
    case 2:
        if (op->type != 0)
        {
            if (op->ct0-- == 0) 
            {
                op->mode0++;
            }
        } 
        else 
        {
            if (op->mode1 == 2) 
            {
                op->mode0++;
            }
        }
        
        if ((op->type == 0) || (op->type == 1)) 
        {
            dx = plp->px - op->px;
            dz = plp->pz - op->pz;
            
            ln = njSqrt((dx * dx) + (dz * dz));
            
            if (ln < (1.5f + plp->ar))
            {
                plp->px = op->px + ((dx * (1.5f + plp->ar)) / ln);
                plp->pz = op->pz + ((dz * (1.5f + plp->ar)) / ln);
                
                PEXP0_F(0x48) = plp->px;
                PEXP0_F(0x50) = plp->pz;
                
                if ((op->ct1 == 0) && (((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000))) && (!(sys->cb_flg & 0x4))))
                {
                    plp->flg   |= 0x10004;
                    plp->stflg |= 0x10000;
                    
                    plp->dvx = dx;
                    plp->dvy = 0;
                    plp->dvz = dz;
                    
                    plp->mode0 = 2;
                    plp->mode1 = bhEne_DGDirCheck(plp);
                    plp->mode2 = 0;
                    plp->mode3 = 0;
                    
                    op->ct1 = 90;
                }
            }
            
            if (op->ct1 != 0) 
            {
                op->ct1--;
            }
        }
        
        if (op->ct3-- == 0) 
        {
            sys->ef.id   = 264;
            sys->ef.type = 0;
            
            sys->ef.flg = 1;
            
            sys->ef.px = op->px;
            sys->ef.py = 5.0f + op->py;
            sys->ef.pz = op->pz;
            
            sys->ef.ax = 0;
            sys->ef.ay = 0;
            
            bhSetEffectTb(&sys->ef, NULL, NULL, 0);
            
            op->ct3 = (int)(200.0f * (-rand() / -2.1474836E9f)) + 200;
        }
        
        break;
    case 3:
        op->ah -= 0.02f;
        op->aw -= 0.01f;
        
        if (op->ah < 0.8f)
        {
            op->ah = 0.8f;
            op->aw = 0.9f;
            
            op->ct3 = 128;
            
            op->mode0++;
        }
        
        break;
    case 4:
        op->ct3 -= 8;
        
        op->ah -= 0.002f;
        op->aw -= 0.001f;
        
        if (op->ct3 < 0) 
        {
            if (op->type != 0) 
            {
                op->flg = 0;
                return;
            }
            else 
            {
                op->mode0++;
                return;
            }
        }
        
        bhEff_SetBaseColor(op, (op->ct3 << 24) | 0xFFFFFF); 
        break;
    case 5:
        return;
    case 6:
        if (op->mode1 == 1) 
        {
            op->mode0 = 1;
        }
        
        return;
    }

    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++; 
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0)
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.09375f);
    
    op->sx *= op->aw;
    op->sy *= op->ah;
    
    if (op->aoy == 0) 
    {
        njCalcVector(cam.mtx, (NJS_VECTOR*)&op->aox, &v);
        
        op->sx *= 0.5f + fabsf(0.5f * v.z);
    }
    
    if (op->type != 0) 
    {
        op->ay = bhArcTan2(cam.mtx[0][8], cam.mtx[0][10]);
    }
    
    if (op->type == 0)
    {
        if (sys->ef_fncn < 128) 
        {
            sys->ef_fnc[sys->ef_fncn] = op;
            
            sys->ef_fncn++;
        } 
    } 
    else 
    {
        temp = (O_WRK*)op->exp1;
        
        exp0 = (char*)temp->exp0;
        
        if (temp->ct0 < 300) 
        {
            ((int*)&exp0[4])[temp->ct0] = (int)op;
            
            temp->ct0++;
        }
    }
}

// 100% matching!
void bhEff_E12_FrameLiquid(O_WRK* op) 
{
    int DGDir;
    int eno;
    int i;
    int hit;
    NJS_POINT3 ofp;

    switch (op->mode0) 
    {                             
    case 0:
        op->tex_id = 80;
        
        op->tv[0].x = -1.0f;
        op->tv[0].y = 0;
        op->tv[0].z = -1.0f;
        
        op->tv[1].x = -1.0f;
        op->tv[1].y = 0;
        op->tv[1].z = 1.0f;
        
        op->tv[2].x = 1.0f;
        op->tv[2].y = 0;
        op->tv[2].z = -1.0f;
        
        op->tv[3].x = 1.0f;
        op->tv[3].y = 0;
        op->tv[3].z = 1.0f;
        
        bhEff_SetBaseColor(op, 0xFF6E0000);
        
        op->flg |= 0x4000000;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->mode0 = 1;
        op->mode1 = 0;
    case 1:
        if (op->type != 0)
        {
            if (op->ct0-- == 0) 
            {
                hit = njSqrt(((plp->px - op->px) * (plp->px - op->px)) + ((plp->pz - op->pz) * (plp->pz - op->pz))) < (1.0f + (1.5f + plp->ar));
                
                if (hit != 0) 
                {
                    if ((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000))) 
                    {
                        plp->flg |= 0x10004;
                        plp->stflg |= 0x10000;
                        
                        plp->dvx = op->px;
                        plp->dvy = op->py;
                        plp->dvz = op->pz;
                        
                        DGDir = bhEne_DGDirCheck(plp);
                        
                        plp->mode0 = 4;
                        plp->mode2 = DGDir;
                        plp->mode3 = 0;
                        
                        plp->hp -= 20;

                        for (i = 0; i < 4; i++) 
                        {
                            ofp.x =  1.0f - (2.0f * (-rand() / -2.1474836E9f));
                            ofp.y = -0.5f + (2.0f * (-rand() / -2.1474836E9f));
                            ofp.z = DGDir ? 1.3f : -1.5f;
                                
                            bhEne_SetFireEffect(plp, 3, &ofp, 0.5f + (-rand() / -2.147483648E9f), (int)(10.0f * (-rand() / -2.147483648e9f)) + 20);
                        }
                    }
                }
                
                sys->ef.id = 263;
                
                sys->ef.type = hit + 1;
                sys->ef.flg = 1;
                
                if (op->aoy > 0) 
                {
                    sys->ef.px = op->px;
                    sys->ef.py = op->py;
                    sys->ef.pz = op->pz;
                } 
                else 
                {
                    sys->ef.px = op->px + (0.5f * op->aox);
                    sys->ef.py = op->py + (0.5f * op->aoy);
                    sys->ef.pz = op->pz + (0.5f * op->aoz);
                }
                
                sys->ef.sx = 2.0f;
                sys->ef.sy = 2.0f + (-rand() / -2.1474836E9f);
                sys->ef.sz = 1.0f;
                
                sys->ef.ax = 0;
                sys->ef.ay = 0;
                
                eno = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
                
                if (eno != -1) 
                {
                    eff[eno].aox  = op->aox;
                    eff[eno].aoy  = op->aoy;
                    eff[eno].aoz  = op->aoz;
                    
                    eff[eno].exp1 = op->exp1;
                }
                
                op->mode0++;
                
                op->ct0 = 110;
            }
        }
        else if (op->mode1 != 0) 
        {
            op->mode0++;
            
            op->ct0 = 110;
        }
        
        break;
    case 2:
        bhEff_SetBaseColor(op, (op->ct0 << 16) | 0xFF000000);
        
        if (op->ct0-- == 0)
        {
            op->ct0 = 0xFF;
            
            op->mode0++;
        }
    case 3:
        op->ct0 -= 5;
        
        if (op->ct0 < 0) 
        {
            if (op->type != 0) 
            {
                op->flg = 0;
                return;
            }
            
            op->mode0 = 4;
            return;
        }
        
        bhEff_SetBaseColor(op, op->ct0 << 24);
        break;
    case 4:
        return;
    }

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E12_FloorBlood2(O_WRK* op) 
{
    int i;
    NJS_POINT3 ofp;
    UV_WORK* uvp;
    float tmp; // not from DWARF
    static UV_WORK BH_UVTAB0[9] = 
    {
        {  0.0f,   0.0f,   0.25f, 0.25f },
        {  0.25f,  0.0f,   0.25f, 0.25f },
        {  0.5f,   0.0f,   0.25f, 0.25f },
        {  0.75f,  0.0f,   0.25f, 0.25f },
        {  0.0f,   0.25f,  0.25f, 0.25f },
        {  0.25f,  0.25f,  0.25f, 0.25f },
        {  0.5f,   0.25f,  0.25f, 0.25f },
        {  0.75f,  0.25f,  0.25f, 0.25f },
        { -1.0f,   0.0f,   0.0f,  0.0f  }
    };
    static UV_WORK* uvtble[1] = 
    {
        BH_UVTAB0
    };
    
    switch (op->mode0) 
    {                             
    case 0:
        op->ct0 = (int)(10.0f * (-rand() / -2.1474836E9f)) + 40;
        
        op->mode0++;
    case 1:
        if (op->ct0-- == 0) 
        {
            if ((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000))) 
            {
                plp->flg |= 0x10004;
                plp->stflg |= 0x10000;
                
                plp->mode0 = 4;
                plp->mode2 = op->mode1;
                plp->mode3 = 0;
                
                plp->hp -= 20;
                
                for (i = 0; i < 4; i++) 
                {
                    ofp.x = 1.0f - (2.0f * (-rand() / -2.1474836E9f));
                    ofp.y = -0.5f + (2.0f * (-rand() / -2.1474836E9f));
                        
                    if (op->mode1 != 0) 
                    {
                        tmp = 1.3f;
                    } 
                    else 
                    {
                        tmp = -1.5f;
                    }
                    
                    ofp.z = tmp;
                    
                    tmp = 0.5f + (-rand() / -2.147483648E9f);
                    
                    bhEne_SetFireEffect(plp, 3, &ofp, tmp, (int)(10.0f * (-rand() / -2.147483648E9f)) + 20);
                }
            }
            
            op->mode0++;
        }
        
        return;
    case 2:
        op->tex_id = 9;
        
        op->stflg &= ~0x20;
        
        op->flg |= 0x4000080;
        op->flg |= 0x180000;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->lox = 1.0f - (2.0f * (-rand() / -2.1474836E9f));
        op->loy = -0.5f + (2.0f * (-rand() / -2.1474836E9f));

        if (op->mode1 != 0) 
        {
            tmp = 1.3f;
        } 
        else 
        {
            tmp = -1.5f;
        }
        
        op->loz = tmp;
        
        op->lkwkp = (unsigned char*)plp;
        op->lkono = 3;
        
        op->sxb = 2.5f;
        op->syb = 2.5f;
        
        op->mtn_no = 0;
        op->frm_no = 0;
        
        op->mode0++;
        break;
    case 3:
        if (op->frm_no == 0) 
        {
            op->flg = 0;
            return;
        }
        
        break;
    }
    
    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;

    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.125f);
    
    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 99.97% matching
void bhEff_E12_FireSpark(O_WRK* op)
{
    P_WRK* pp;
    NJS_POINT3* p, *v;
    int i;
    unsigned int col[3] = 
    {
        0x32230E,
        0x1E1E0A,
        0x50411C
    };
    
    switch (op->mode0) 
    {                            
    case 0:
        pp = (P_WRK*)bhSetExtraEffectWork();
        
        if (pp == NULL) 
        {
            op->flg = 0;
            return;
        }
        
        op->exp0 = (unsigned char*)pp;
        
        p = pp->pos;
        v = pp->vec;
        
        pp->num = 16;
        
        for (i = 0; i < pp->num; i++, p++, v++) 
        {
            p->x = (op->px - (2.0f * (-rand() / -2.1474836E9f))) - 1.0f;
            p->y = op->py + (5.0f * (-rand() / -2.1474836E9f));
            p->z = (op->pz - (2.0f * (-rand() / -2.1474836E9f))) - 1.0f;
            
            op->xn = 0.1f + (0.1f * (-rand() / -2.1474836E9f));
            
            op->ay = 65536.0f * (-rand() / -2.1474836E9f);
            
            v->x = op->xn * -njSin(op->ay);
            v->y = 0.5f + (0.5f * (-rand() / -2.1474836E9f));
            v->z = op->xn * -njCos(op->ay);
        }
        
        pp->sx = 0.1f;
        pp->sy = 0.15f;
        
        op->ct0 = 30.0f + (20.0f * (-rand() / -2.1474836E9f));
        op->ct2 = 0xFF;
        
        op->flg |= 0x20000000;
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->func = (void*)bhEff_E00_DrawParticleSpr;

        op->mode0++;
        break;
    case 1:
        pp = (P_WRK*)op->exp0;
        
        p = pp->pos;
        v = pp->vec;
        
        for (i = 0; i < pp->num; i++, p++, v++) 
        {
            v->x += (0.06f * (-rand() / -2.1474836E9f)) - 0.03f;
            v->y += (0.1f * (-rand() / -2.1474836E9f)) - 0.05f;
            v->z += (0.06f * (-rand() / -2.1474836E9f)) - 0.03f;

            p->x += v->x;
            p->y += v->y;
            p->z += v->z;
            
            v->x *= 0.95f;
            v->y *= 0.95f;
            v->z *= 0.95f;
        }

        pp->col = col[(int)(3.0f * (-rand() / -2.1474836E9f))];
        pp->col |= op->ct2 << 24;
        
        op->tex_id = sys->ef_tn[4] + (int)(4.0f * (-rand() / -2.1474836E9f));
        
        if (op->ct0 < 30) 
        {
            op->ct2 -= 8;
        }
        
        if (op->ct0-- == 0) 
        {
            op->flg = 0;
            pp->flg = 0;
            return;
        }
        
        if (sys->ef_fncn < 128) 
        {
            sys->ef_fnc[sys->ef_fncn] = op;
            
            sys->ef_fncn++;
        }
    }
}

// 100% matching!
void bhEff_E12_FireManager(O_WRK* op) 
{
    switch (op->mode0) 
    {                           
    case 0:
        if (op->exp0 == NULL) 
        {
            op->exp0 = (unsigned char*)bhSetExtraEffectWork();
        }
        
        op->tex_id = 80;
        
        op->ani_ct = 1;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->func = (void*)bhEff_E12_FireBurstDraw;
        
        op->mode0++;
        break;
    case 1:
        op->ct0 = 0;
        break;
    }
    
    if (sys->ef_fncn < 128) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_E12_FireBurstDraw(O_WRK* owp)
{
    O_WRK* op;
    O_WRK** opp;
    int num; // modified original position from DWARF
    char atr[192];

    njSetMatrix(NULL, cam.mtx);
    
    njTextureFilterMode(0);
    
    njSetTexture(&sys->ef_tlist);
    njSetTextureNum(sys->ef_tn[owp->tex_id] + owp->ani_ct);
    
    njGetSystemAttr((NJS_SYS_ATTR*)&atr);
    
    njColorBlendingMode(0, owp->bl_src);
    njColorBlendingMode(1, owp->bl_dst);
    
    njFogDisable();

    opp = (O_WRK**)&owp->exp0[4];
    
    for (num = owp->ct0; num != 0; )
    {
        op = *opp++;
        
        num--;
        
        njPushMatrixEx();
        njPushMatrixEx();
        
        njUnitMatrix(NULL);
        
        njTranslateEx((NJS_VECTOR*)&op->px);
        njRotateEx((Angle*)&op->ax, 0);
        
        njGetMatrix(op->mtx);
        
        njPopMatrixEx();
        
        njMultiMatrix(NULL, op->mtx);

        if ((op->flg & 0x100000)) 
        {
            njUnitRotPortion(NULL);
        }

        njScaleEx((NJS_VECTOR*)&op->sx);
        
        njDrawTexture3DEx(op->tvp, op->pn, 1);
        
        njPopMatrixEx();
    } 
    
    njSetSystemAttr((NJS_SYS_ATTR*)&atr);
     
    if ((sys->st_flg & 0x2)) 
    {
        njFogEnable();
    }
}

// 100% matching!
void bhEff_E12_Fire2(O_WRK* op) 
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[11] = 
    {
        {  0.0f,    0.0f,    0.1875f, 0.1875f },
        {  0.1875f, 0.0f,    0.1875f, 0.1875f },
        {  0.375f,  0.0f,    0.1875f, 0.1875f },
        {  0.5625f, 0.0f,    0.1875f, 0.1875f },
        {  0.75f,   0.0f,    0.1875f, 0.1875f },
        {  0.0f,    0.1875f, 0.1875f, 0.1875f },
        {  0.1875f, 0.1875f, 0.1875f, 0.1875f },
        {  0.375f,  0.1875f, 0.1875f, 0.1875f },
        {  0.5625f, 0.1875f, 0.1875f, 0.1875f },
        {  0.75f,   0.1875f, 0.1875f, 0.1875f },
        { -1.0f,    0.0f,    0.0f,    0.0f    }
    };
    static UV_WORK* uvtble[1] = 
    {
        BH_UVTAB0
    };

    switch (op->mode0) 
    {                          
    case 0:
        op->tex_id = 8;
        
        op->ani_ct = 0;
        
        bhEff_SetBaseColor(op, 0xFF808080);
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg |= 0x4180000;
        
        bhEff_SetAlign(op, 2);
        
        op->mtn_no = 0;
        op->frm_no = 8.0f * (-rand() / -2.1474836E9f);
        
        op->sxb = op->sx;
        op->syb = op->sy;
        
        op->aw = 1.0f;
        op->ah = 1.0f;
        
        op->ct3 = 0xFF;
        
        op->func = (void*)bhEff_Draw3DSprite;
        
        op->mode0++;
    case 1:
        if (op->ct1 != 0) 
        {
            op->ct1--;
            return;
        }
        
        op->mode0++;
        break;
    case 2:
        if (op->ct0 != 0) 
        {
            op->ct0--;
        }
        else 
        {
            op->mode0++;
        }
        
        break;
    case 3:
        op->ct3 -= 8;
        
        if (op->ct3 < 0) 
        {
            op->flg = 0;
            return;
        }
        
        bhEff_SetBaseColor(op, (op->ct3 << 24) | 0xFFFFFF);
        break;
    }
    
    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.09375f);
    
    op->sx *= op->aw;
    op->sy *= op->ah;
    
    if (sys->ef_fncn < 128) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_E12_Fire3(O_WRK* op) 
{
    UV_WORK* uvp;
    static UV_WORK BH_UVTAB0[21] = 
    {
        {     0.0f,     0.0f, 0.21875f, 0.21875f },
        { 0.21875f,     0.0f, 0.21875f, 0.21875f },
        {  0.4375f,     0.0f, 0.21875f, 0.21875f },
        { 0.65625f,     0.0f, 0.21875f, 0.21875f },
        {     0.0f, 0.21875f, 0.21875f, 0.21875f },
        { 0.21875f, 0.21875f, 0.21875f, 0.21875f },
        {  0.4375f, 0.21875f, 0.21875f, 0.21875f },
        { 0.65625f, 0.21875f, 0.21875f, 0.21875f },
        {     0.0f,  0.4375f, 0.21875f, 0.21875f },
        { 0.21875f,  0.4375f, 0.21875f, 0.21875f },
        {  0.4375f,  0.4375f, 0.21875f, 0.21875f },
        { 0.65625f,  0.4375f, 0.21875f, 0.21875f },
        {     0.0f, 0.65625f, 0.21875f, 0.21875f },
        { 0.21875f, 0.65625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.65625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.65625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.65625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.65625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.65625f, 0.21875f, 0.21875f },
        {  0.4375f, 0.65625f, 0.21875f, 0.21875f },
        {    -1.0f,     0.0f,     0.0f,     0.0f }
    };
    static UV_WORK* uvtble[1] = 
    {
        BH_UVTAB0
    };

    switch (op->mode0) 
    {                          
    case 0:
        op->tex_id = 9;
        
        op->ani_ct = 0;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg |= 0x4180000;
        
        bhEff_SetAlign(op, 2);
        
        op->mtn_no = 0;
        op->frm_no = 9.0f * (-rand() / -2.1474836E9f);
        
        op->sxb = op->sx;
        op->syb = op->sy;
        
        op->ct3 = 0xFF;
        
        op->func = (void*)bhEff_Draw3DSprite;
        
        op->mode0++;
        break;
    case 1:
        if (op->frm_no == 0) 
        {
            op->flg = 0;
            return;
        }
        
        if (op->frm_no > 7) 
        {
            op->ct3 -= 16;
            
            if (op->ct3 < 16)
            {
                op->ct3 = 16;
            }
            
            bhEff_SetBaseColor(op, (op->ct3 << 24) | 0xFFFFFF);
        }
        
        break;
    }
    
    uvp = &uvtble[op->mtn_no][op->frm_no]; 
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.1875f);
    
    op->px += op->xn;
    op->py += op->yn;
    op->pz += op->zn;
    
    op->xn *= 0.9f;
    op->yn *= 0.9f;
    op->zn *= 0.9f;
    
    op->xn += (0.04f * (-rand() / -2.1474836E9f)) - 0.02f;
    op->zn += (0.04f * (-rand() / -2.1474836E9f)) - 0.02f;
    op->yn += 0.03f * (-rand() / -2.1474836E9f);
    
    if (sys->ef_fncn < 128) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_E12_BintaEffControl(O_WRK* op) 
{
    BH_PWORK* ep;

    ep = (BH_PWORK*)op->lkwkp;
    
    switch (op->mode0) 
    {                             
    case 0:                                         
        op->flg |= 0x1000000;
        
        op->mode0++;
    case 1:                                         
        if (ep != NULL) 
        {
            switch (op->mode1) 
            { 
            case 0:
                break;
            case 1:                                 
                bhEne12_SetFireBintaEffect(ep, 0);

                op->mode1 = 0;
                break;
            case 2:                                 
                bhEne12_SetFireBintaEffect(ep, 1);

                op->mode1 = 0;
                break;
            case 3:                                 
                bhEne12_SetFireBintaEffect(ep, 2);
                break;
            }
        } 
        
        break;
    }
}

// 99.89% matching
void bhEff_E13_Fluid(O_WRK* op) 
{
    NJS_POINT3 n;
    NJS_POINT3 p1, p2;
    float sz; // not from DWARF
    static UV_WORK BH_UVTAB0[9] = 
    {
        {  0.875f,  0.0f,    0.125f, 0.125f },
        {  0.75f,   0.0f,    0.125f, 0.125f },
        {  0.625f,  0.0f,    0.125f, 0.125f },
        {  0.0f,    0.0f,    0.125f, 0.125f },
        {  0.125f,  0.0f,    0.125f, 0.125f },
        {  0.25f,   0.0f,    0.125f, 0.125f },
        {  0.375f,  0.0f,    0.125f, 0.125f },
        {  0.5f,    0.0f,    0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK BH_UVTAB1[9] =
    {
        {  0.875f,  0.125f,  0.125f, 0.125f },
        {  0.75f,   0.125f,  0.125f, 0.125f },
        {  0.625f,  0.125f,  0.125f, 0.125f },
        {  0.0f,    0.125f,  0.125f, 0.125f },
        {  0.125f,  0.125f,  0.125f, 0.125f },
        {  0.25f,   0.125f,  0.125f, 0.125f },
        {  0.375f,  0.125f,  0.125f, 0.125f },
        {  0.5f,    0.125f,  0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK BH_UVTAB2[9] = 
    {
        {  0.875f,  0.25f,   0.125f, 0.125f },
        {  0.75f,   0.25f,   0.125f, 0.125f },
        {  0.625f,  0.25f,   0.125f, 0.125f },
        {  0.0f,    0.25f,   0.125f, 0.125f },
        {  0.125f,  0.25f,   0.125f, 0.125f },
        {  0.25f,   0.25f,   0.125f, 0.125f },
        {  0.375f,  0.25f,   0.125f, 0.125f },
        {  0.5f,    0.25f,   0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK BH_UVTAB3[9] = 
    {
        {  0.875f,  0.375f,  0.125f, 0.125f },
        {  0.75f,   0.375f,  0.125f, 0.125f },
        {  0.625f,  0.375f,  0.125f, 0.125f },
        {  0.0f,    0.375f,  0.125f, 0.125f },
        {  0.125f,  0.375f,  0.125f, 0.125f },
        {  0.25f,   0.375f,  0.125f, 0.125f },
        {  0.375f,  0.375f,  0.125f, 0.125f },
        {  0.5f,    0.375f,  0.125f, 0.125f },
        { -1.0f,    0.0f,    0.0f,   0.0f   }
    };
    static UV_WORK* uvtble[4] = 
    {
        BH_UVTAB2,
        BH_UVTAB3,
        BH_UVTAB0,
        BH_UVTAB1
    };

    switch (op->mode0) 
    {
    case 0:
        op->flg    |= 0x180000;
        
        op->tex_id  = 81;
        
        op->ani_ct  = 0;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src  = 8;
        op->bl_dst  = 3;
        
        op->mtn_no  = op->type;
        op->frm_no  = 4.0f * (-rand() / -2.147483648E9f);
        
        op->sxb     = op->sx;
        op->syb     = op->sy;
        
        op->ct0     = 0;
        op->ct2     = 1;

        if ((op->type == 0) || (op->type == 2)) 
        {
            op->ct3 = 30.0f + (20.0f * (-rand() / -2.147483648E9f));
        } 
        else 
        {
            op->ct3 = 10.0f + (5.0f * (-rand() / -2.147483648E9f));
        }

        op->mode0 = 1;
    case 1:
        if ((op->ct0 == op->ct3) || (op->ct2 == 0)) 
        {
            op->mode0 = 2;
            
            op->ct0   = 0;
            break;
        }

        op->frm_no++;
        
        if (op->frm_no > 7) 
        {
            op->frm_no = 3;
        }
        
        op->ct0++;
        break;
    case 2:
        op->ct0++;
        
        if (op->ct0 >= 8)
        {
            op->flg = 0;
            return;
        }
        
        bhEff_SetBaseColor(op, ((unsigned int)(0xFF - (op->ct0 * 32)) << 24) | 0xFFFFFF);
        
        op->sx *= 0.9f;
        op->sy *= 0.9f;
        break;
    }

    if (op->ct2 != 0)
    {
        op->px += op->xn;
        op->pz += op->zn;
        op->py += op->yn;
        
        op->yn -= 0.15f;

        p1.x = op->pxb;
        p1.y = op->pyb - 0.5f;
        p1.z = op->pzb;
        
        p2.x = op->px;
        p2.y = op->py - 0.5f;
        p2.z = op->pz;

        op->xn *= 0.97f;
        op->zn *= 0.97f;

        if (bhCollisionCheckLine2(&p1, &p2, 0x4400, -1) != NULL) 
        {
            bhGetHitCollisionNormal(&n);
            
            njUnitVector(&n);

            if (n.y > 0.9f) 
            {
                op->ct2 = 0;
            } 
            else
            {
                op->xn = 0;
                op->zn = 0;
                op->yn = 0;
            }
            
            op->px = p2.x;
            op->py = 0.5f + p2.y;
            op->pz = p2.z;
        }
    }

    sz = 0.0625f;
    
    bhEff_SetUVInfo(op, &uvtble[op->mtn_no][op->frm_no], (float)sz);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E14_Explosion(O_WRK* op) 
{ 
    switch (op->mode0) 
    {                         
    case 0:
        sys->ef.id = 19;
        
        sys->ef.flg = 0x4100001;
        sys->ef.type = 0;
        
        sys->ef.mdlver = 0;
        
        sys->ef.sx = 2.5f;
        sys->ef.sy = 2.5f;
        sys->ef.sz = 2.5f;
        
        sys->ef.px = op->px;
        sys->ef.py = op->py;
        sys->ef.pz = op->pz;
        
        sys->ef.ay = 0;
        sys->ef.ax = 0;
        
        bhSetEffectTb(&sys->ef, NULL, NULL, 0);
        
        sys->ef.id = 19;
        
        sys->ef.flg = 0x4100001;
        sys->ef.type = 2;
        
        sys->ef.mdlver = 0;
        
        sys->ef.sx = 5.0f;
        sys->ef.sy = 5.0f;
        sys->ef.sz = 5.0f;
        
        sys->ef.px = op->px;
        sys->ef.py = op->py;
        sys->ef.pz = op->pz;
        
        sys->ef.ay = 0;
        sys->ef.ax = 0;
        
        bhSetEffectTb(&sys->ef, NULL, NULL, 0);
        
        op->ct0 = 0;
        
        op->mode0++;
        break;
    case 1:
        op->ct0++;
        
        if (op->ct0 > 2) 
        {
            sys->ef.id = 19;
            
            sys->ef.flg = 0x4100001;
            sys->ef.type = 1;
            
            sys->ef.mdlver = 0;
            
            sys->ef.sx = 2.8f;
            sys->ef.sy = 2.8f;
            sys->ef.sz = 2.8f;
            
            sys->ef.px = op->px + cam.vx;
            sys->ef.py = op->py + cam.vy;
            sys->ef.pz = op->pz + cam.vz;
            
            sys->ef.ay = 0;
            sys->ef.ax = 0;
            
            bhSetEffectTb(&sys->ef, NULL, NULL, 0);
            
            op->flg = 0;
        }
        
        break;
    }
}

// 100% matching!
void bhEff_E14_Fire(O_WRK* op) 
{
    UV_WORK* uvp;
    LGT_WORK* lp;
    float ln;
    int i;
    NJS_POINT3 ofp;
    float px, pz; // not from DWARF
    float z;      // not from DWARF
    static UV_WORK BH_UVTAB0[10] = 
    {
        {     0.0f,     0.0f, 0.15625f, 0.1875f },
        {  0.1875f,     0.0f, 0.15625f, 0.1875f },
        {   0.375f,     0.0f, 0.15625f, 0.1875f },
        {  0.5625f,     0.0f, 0.15625f, 0.1875f },
        {    0.75f,     0.0f, 0.15625f, 0.1875f },
        {     0.0f,  0.1875f, 0.15625f, 0.1875f },
        {  0.1875f,  0.1875f, 0.15625f, 0.1875f },
        {   0.375f,  0.1875f, 0.15625f, 0.1875f },
        {  0.5625f,  0.1875f, 0.15625f, 0.1875f },
        {    -1.0f,     0.0f,     0.0f,    0.0f }
    };
    static UV_WORK BH_UVTAB1[9] = 
    {
        { 0.03125f, 0.46875f, 0.15625f, 0.21875f },
        {    0.25f, 0.46875f, 0.15625f, 0.21875f },
        { 0.46875f, 0.46875f, 0.15625f, 0.21875f },
        {  0.6875f, 0.46875f, 0.15625f, 0.21875f },
        { 0.03125f,  0.6875f, 0.15625f, 0.21875f },
        {    0.25f,  0.6875f, 0.15625f, 0.21875f },
        { 0.46875f,  0.6875f, 0.15625f, 0.21875f },
        {  0.6875f,  0.6875f, 0.15625f, 0.21875f },
        {    -1.0f,     0.0f,     0.0f,     0.0f }
    };
    static UV_WORK* uvtble[2] = 
    {
        BH_UVTAB0,
        BH_UVTAB1
    };

    switch (op->mode0) 
    {
    case 0:
        op->tex_id  = 80;
        
        op->ani_ct  = 1;
        
        bhEff_SetBaseColor(op, 0x80FFFFFF);
        
        op->bl_src  = 8;
        op->bl_dst  = 3;
        
        op->flg    |= 0x24080000;
        
        bhEff_SetAlign(op, 3);
        
        op->mtn_no  = (int)(2.0f * (-rand() / -2.147483648E9f));
        op->frm_no  = 8.0f * (-rand() / -2.147483648E9f);
        
        op->sxb     = op->sx;
        op->syb     = op->sy;
        
        op->ct0     = 20;
        op->ct1     = 0;
        op->ct3     = 300.0f * (-rand() / -2.147483648E9f);
        
        op->ah      = 0.2f;
        op->aw      = 1.0f;

        if (op->type == 1) 
        {
            lp = &rom->lgtp[1];

            if (!(lp->flg & 0x1)) 
            {
                lp->ct0    = 0;
                
                lp->lkono  = 0;
                
                lp->vx     = 0;
                lp->vy     = 0;
                lp->vz     = 0;
            }

            lp->lkono++;
            
            lp->flg   |= 0x3;
            lp->type   = 13;
            
            lp->aspd   = 2;
            
            lp->lkflg  = 0;
            lp->lsrc   = 4;
            
            lp->nr     = 15.0f;
            lp->fr     = 20.0f;
            
            lp->vx    += op->px;
            lp->vy    += op->py;
            lp->vz    += op->pz;
            
            lp->px     = lp->vx / lp->lkono;
            lp->py     = lp->vy / lp->lkono;
            lp->pz     = lp->vz / lp->lkono;
            
            lp->r      = 3.0f;
            lp->g      = 1.8f;
            lp->b      = 0.7f;
        }

        op->mode0++;
        
        op->mode1  = 0;
        
        op->func   = (void*)bhEff_Draw3DSprite;

        px = plp->px - op->px;
        pz = plp->pz - op->pz;

        if ((njSqrt((px * px) + (pz * pz)) < (1.5f + plp->ar)) && ((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000)))) 
        {
            plp->flg   |= 0x10004;
            plp->stflg |= 0x10000;
            
            plp->mode0  = 4;
            plp->mode2  = op->mode1;
            plp->mode3  = 0;
            
            plp->hp    -= 20;

            for (i = 0; i < 4; i++) 
            {
                ofp.x = 1.0f - (2.0f * (-rand() / -2.147483648E9f));
                ofp.y = -0.5f + (2.0f * (-rand() / -2.147483648E9f));

                if (op->mode1 != 0) 
                {
                    z = 1.3f;
                } 
                else
                {
                    z = -1.5f;
                }

                ofp.z = z;
            
                ln = 0.5f + (0.3f * (-rand() / -2.147483648E9f));
                
                bhEne_SetFireEffect(plp, 3, &ofp, ln, (int)(10.0f * (-rand() / -2.147483648E9f)) + 20);
            }
        }
    case 1:
        op->ah += 0.2f;
        
        if (op->ah <= 1.0f) 
        {
            break;
        }
        
        op->ah = 1.0f;
        
        op->mode0++;
        break;
    case 2:
        if (op->ct0-- == 0) 
        {
            op->mode0++;
        }
        
        if (op->ct3-- == 0) 
        {
            sys->ef.id    = 264;
            
            sys->ef.type  = 0;
            sys->ef.flg   = 1;
            
            sys->ef.px = op->px;
            sys->ef.py = 5.0f + op->py;
            sys->ef.pz = op->pz;
            
            sys->ef.ax    = 0;
            sys->ef.ay    = 0;
            
            bhSetEffectTb((EF_WORK*)&sys->ef, NULL, NULL, 0);
            
            op->ct3 = (int)(200.0f * (-rand() / -2.147483648E9f)) + 200;
        }
        
        break;
    case 3:
        op->ah -= 0.02f;
        op->aw -= 0.01f;
        
        if (op->ah < 0.6f) 
        {
            op->ah   = 0.6f;
            op->aw   = 0.8f;
            
            op->ct3  = 128;
            
            op->mode0++;
        }
        
        break;
    case 4:
        op->ct3 -= 8;
        
        op->ah  -= 0.002f;
        op->aw  -= 0.001f;
        
        if (op->ct3 < 0)
        {
            if (op->type != 0) 
            {
                op->flg = 0;
                return;
            }
            
            op->mode0++;
            return;
        }
        
        bhEff_SetBaseColor(op, (op->ct3 << 24) | 0xFFFFFF);
        break;
    }

    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.09375f);

    op->sx *= op->aw;
    op->sy *= op->ah;
    
    op->ay = bhArcTan2(cam.mtx[0][8], cam.mtx[0][10]);

    if (sys->ef_fncn < 128)
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_E14_Fire2(O_WRK* op) 
{
    UV_WORK* uvp;
    float loz; // not from DWARF
    static UV_WORK BH_UVTAB0[12] = 
    {
        {  0.5f,  0.5f, 0.25f, 0.25f },
        { 0.25f,  0.5f, 0.25f, 0.25f },
        {  0.0f,  0.5f, 0.25f, 0.25f },
        { 0.75f, 0.25f, 0.25f, 0.25f },
        {  0.5f, 0.25f, 0.25f, 0.25f },
        { 0.25f, 0.25f, 0.25f, 0.25f },
        {  0.0f, 0.25f, 0.25f, 0.25f },
        { 0.75f,  0.0f, 0.25f, 0.25f },
        {  0.5f,  0.0f, 0.25f, 0.25f },
        { 0.25f,  0.0f, 0.25f, 0.25f },
        {  0.0f,  0.0f, 0.25f, 0.25f },
        { -1.0f,  0.0f,  0.0f,  0.0f }
    };
    static UV_WORK* uvtble[1] = 
    {
        BH_UVTAB0
    };
    
    switch (op->mode0) 
    {
    case 0:
        op->flg    |= 0x4180080;
        
        op->bl_src  = 8;
        op->bl_dst  = 10;
        
        op->stflg  &= ~0x20;
        
        op->tex_id  = 9;
        
        bhEff_SetBaseColor(op, -1);
        
        op->lox = 1.0f - (2.0f * (-rand() / -2.147483648E9f));
        op->loy = -0.5f + (2.0f * (-rand() / -2.147483648E9f));

        if (op->mode1 != 0) 
        {
            loz = 1.3f;
        }
        else 
        {
            loz = -1.5f;
        }
        
        op->loz    = loz;
        
        op->lkwkp  = (unsigned char*)plp;
        op->lkono  = 3;
        
        op->sxb    = 2.0f;
        op->syb    = 2.0f;
        
        op->ct0    = 0xFF;
        
        op->mtn_no = 0;
        op->frm_no = 0;
        
        op->mode0++;
        break;
    case 1:
        if (op->frm_no == 0)
        {
            op->flg = 0;
            return;
        }
        
        op->sxb += 0.1f * (0.3f - op->sxb);
        op->syb += 0.1f * (0.3f - op->syb);
        break;
    }

    uvp = &uvtble[op->mtn_no][op->frm_no];
    
    op->frm_no++;
    
    if (uvtble[op->mtn_no][op->frm_no].u < 0) 
    {
        op->frm_no = 0;
    }
    
    bhEff_SetUVInfo(op, uvp, 0.125f);

    if (sys->ef_trsn < 512) 
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_E14_Mucus(O_WRK* op) 
{
    UV_WORK* uvp;
    BH_PWORK* ep;
    int i;
    O_WRK* opp; // not from DWARF
    static UV_WORK BH_UVTAB0[9] = 
    {
        {   0.0f,    0.0f, 0.125f, 0.1875f },
        { 0.125f,    0.0f, 0.125f, 0.1875f },
        {  0.25f,    0.0f, 0.125f, 0.1875f },
        { 0.375f,    0.0f, 0.125f, 0.1875f },
        {   0.5f,    0.0f, 0.125f, 0.1875f },
        { 0.625f,    0.0f, 0.125f, 0.1875f },
        {  0.75f,    0.0f, 0.125f, 0.1875f },
        { 0.875f,    0.0f, 0.125f, 0.1875f },
        {  -1.0f,    0.0f,   0.0f,    0.0f }
    };
    static UV_WORK BH_UVTAB1[13] =
    {
        {   0.0f, 0.1875f, 0.125f, 0.1875f },
        { 0.125f, 0.1875f, 0.125f, 0.1875f },
        {  0.25f, 0.1875f, 0.125f, 0.1875f },
        { 0.375f, 0.1875f, 0.125f, 0.1875f },
        {   0.5f, 0.1875f, 0.125f, 0.1875f },
        { 0.625f, 0.1875f, 0.125f, 0.1875f },
        {  0.75f, 0.1875f, 0.125f, 0.1875f },
        { 0.875f, 0.1875f, 0.125f, 0.1875f },
        {   0.0f,  0.375f, 0.125f, 0.1875f },
        { 0.125f,  0.375f, 0.125f, 0.1875f },
        {  0.25f,  0.375f, 0.125f, 0.1875f },
        { 0.375f,  0.375f, 0.125f, 0.1875f },
        {  -1.0f,    0.0f,   0.0f,    0.0f }
    };
    static UV_WORK BH_UVTAB2[21] = 
    {
        {   0.5f,  0.375f, 0.125f, 0.1875f },
        { 0.625f,  0.375f, 0.125f, 0.1875f },
        {  0.75f,  0.375f, 0.125f, 0.1875f },
        { 0.875f,  0.375f, 0.125f, 0.1875f },
        {   0.0f, 0.5625f, 0.125f, 0.1875f },
        { 0.125f, 0.5625f, 0.125f, 0.1875f },
        {  0.25f, 0.5625f, 0.125f, 0.1875f },
        { 0.375f, 0.5625f, 0.125f, 0.1875f },
        {   0.5f, 0.5625f, 0.125f, 0.1875f },
        { 0.625f, 0.5625f, 0.125f, 0.1875f },
        {  0.75f, 0.5625f, 0.125f, 0.1875f },
        { 0.875f, 0.5625f, 0.125f, 0.1875f },
        {   0.0f,   0.75f, 0.125f, 0.1875f },
        { 0.125f,   0.75f, 0.125f, 0.1875f },
        {  0.25f,   0.75f, 0.125f, 0.1875f },
        { 0.375f,   0.75f, 0.125f, 0.1875f },
        {   0.5f,   0.75f, 0.125f, 0.1875f },
        { 0.625f,   0.75f, 0.125f, 0.1875f },
        {  0.75f,   0.75f, 0.125f, 0.1875f },
        { 0.875f,   0.75f, 0.125f, 0.1875f },
        {  -1.0f,    0.0f,   0.0f,    0.0f }
    };
    static UV_WORK* uvtble[3] = 
    {
        BH_UVTAB0,
        BH_UVTAB1,
        BH_UVTAB2
    };

    switch (op->mode0) 
    {
    case 0:
        op->tex_id  = 81;
        
        op->ani_ct  = 1;
        
        bhEff_SetBaseColor(op, -1);
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg   |= 0x24080000;
        
        op->mtn_no = 0;
        op->frm_no = 0;

        ep = ene;
        
        for (i = 0; i < sys->ewk_n; i++, ep++) 
        {
            if (((ep->flg & 0x1)) && (ep->id == 14))
            {
                op->lkwkp = (unsigned char*)ep;
                break;
            }
        }

        op->func   = (void*)bhEff_E14_MucusDraw;
        
        op->mode0++;
    case 1:
        uvp = &uvtble[0][op->frm_no];
        
        if (op->mode1 != 0) 
        {
            op->mode0++;
        }
        
        break;
    case 2:
        i = op->mode1 - 1;
        
        uvp = &uvtble[i][op->frm_no];
        
        op->frm_no++;
        
        if (uvtble[i][op->frm_no].u < 0) 
        {
            op->mode0++;
        }
        
        break;
    case 3:
        return;
    }

    if (op->lkwkp == NULL) 
    {
        return;
    }

    njUnitMatrix(NULL);
    
    njRotateY(NULL, op->ay);

    {
    NJS_POINT3 v;
    
    v.x = op->sz;
    v.y = 0;
    v.z = 0;
        
    njCalcVector(NULL, &v, &v);

    op->tv[2].x = op->px + v.x;
    op->tv[2].y = op->py + v.y;
    op->tv[2].z = op->pz + v.z;
        
    op->tv[3].x = op->px - v.x;
    op->tv[3].y = op->py - v.y;
    op->tv[3].z = op->pz - v.z;
    }

    opp = (O_WRK*)op->lkwkp;
    
    njSetMatrix(NULL, &opp->mlwP->owP[op->lkono].mtx);

    njTranslate(NULL, op->sx, op->sy, 0);
    njRotateZ(NULL, op->ax);

    {
    NJS_POINT3 v;
        
    v.x = op->sz;
    v.y = 0;
    v.z = 0;
        
    njCalcPoint(NULL, &v, (NJS_POINT3*)&op->tv[0].x);

    v.x = -op->sz;
    v.y = 0;
    v.z = 0;
        
    njCalcPoint(NULL, &v, (NJS_POINT3*)&op->tv[1].x);
    }

    bhEff_SetUVInfo(op, uvp, 0.125f);

    if (sys->ef_fncn < 128) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_E14_MucusDraw(O_WRK* op)
{
    if (((op->flg & 0x1000000)) || ((op->stflg & 0x1000000))) 
    {
        return;
    } 
    else
    {
        njSetMatrix(NULL, cam.mtx);
        
        njTextureFilterMode(1);
        
        njSetTexture(&sys->ef_tlist);
        njSetTextureNum(sys->ef_tn[op->tex_id] + op->ani_ct);
        
        njColorBlendingMode(0, op->bl_src);
        njColorBlendingMode(1, op->bl_dst);
        
        njDrawTexture3DEx(op->tvp, op->pn, 1);
        
        njColorBlendingMode(0, 8);
        njColorBlendingMode(1, 6);
    }
}

// 100% matching!
void bhEff_DamagePoint(O_WRK* op) 
{
    switch (op->mode0) 
    {                            
    case 0:
        op->func = (void*)bhEff_DamagePointDraw;
        
        op->ct0 = 5;
        
        op->ofy = 0.6f;
        
        op->yn = 0.2f + bhGetGroundPosition((NJS_POINT3*)&op->px);
        
        op->mode0++;
        break;
    case 1:
        op->py += op->ofy;
        
        op->ofy -= 0.2f;
        
        if (op->py < op->yn) 
        {
            if (op->ct0-- == 0) 
            {
                op->mode0++;
                
                op->ct0 = 5;
            }
            
            op->ofy *= -0.4f;
            
            op->py = op->yn + (0.6f * (op->yn - op->py));
        }
        
        break;
    case 2:
        if (op->ct0-- == 0) 
        {
            op->flg = 0;
            return;
        }
        
        break;
    }
    
    if (sys->ef_fncn < 128) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_DamagePointDraw(O_WRK* op)
{
    NJS_POINT2 pos;

    njProjectScreen(cam.mtx, (NJS_POINT3*)&op->px, &pos);
}

// 100% matching!
void bhEff_Draw3DSprite(O_WRK* op) 
{
    NJS_POINT3 pa, pb, pc;
    NJS_POINT3 vec;
    O_WRK* opp; // not from DWARF

    njPushMatrixEx();
    
    njSetMatrix(NULL, cam.mtx);
    
    njFogDisable();

    if ((op->flg & 0x20000000)) 
    {
        njTextureFilterMode(0);
    } 
    else
    {
        njTextureFilterMode(1);
    }

    if (!(op->stflg & 0x20)) 
    {
        njSetTexture(&sys->ef_tlist);
        njSetTextureNum(sys->ef_tn[op->tex_id] + op->ani_ct);
    }
    else 
    {
        njSetTexture(op->txp[0]);
        njSetTextureNum(op->tex_id + op->ani_ct);
    }

    njPushMatrixEx();

    if ((!(op->flg & 0x80)) || ((op->flg & 0x200000)))
    {
        njUnitMatrix(NULL);
        
        njTranslateEx((NJS_POINT3*)&op->px);
        njRotateEx((Angle*)&op->ax, 0);
        
        njGetMatrix(op->mtx);
    }
    else
    {
        opp = (O_WRK*)op->lkwkp;
        
        njSetMatrix(op->mtx, &opp->mlwP->owP[op->lkono].mtx);
        
        njTranslate(op->mtx, op->lox, op->loy, op->loz);
        njRotateXYZ(op->mtx, op->ax, op->ay, op->az);
    }

    njPopMatrixEx();
    
    njMultiMatrix(NULL, op->mtx);

    if ((op->flg & 0x100000))
    {
        njUnitRotPortion(NULL);
        njRotateZ(NULL, op->az);
        
        if ((sys->gm_flg & 0x4000)) 
        {
            if (sys->mr_pl.vx)
            {
                njRotateY(NULL, 32768);
            }
            
            if (sys->mr_pl.vy) 
            {
                njRotateX(NULL, 32768);
            }
        }
    }

    njScaleEx((NJS_VECTOR*)&op->sx);

    if ((op->flg & 0x20000)) 
    {
        njCalcPoint(NULL, (NJS_POINT3*)&op->tvp[0].x, &pa);
        njCalcPoint(NULL, (NJS_POINT3*)&op->tvp[1].x, &pb);
        njCalcPoint(NULL, (NJS_POINT3*)&op->tvp[2].x, &pc);
        
        njProjectScreen(NULL, (NJS_POINT3*)&op->tvp[0].x, (NJS_POINT2*)&pa);
        njProjectScreen(NULL, (NJS_POINT3*)&op->tvp[1].x, (NJS_POINT2*)&pb);
        njProjectScreen(NULL, (NJS_POINT3*)&op->tvp[2].x, (NJS_POINT2*)&pc);
        
        njGetPlaneNormal2(&pa, &pb, &pc, &vec);
        
        njUnitVector(&vec);

        if (vec.z < 0) 
        {
            op->stflg |= 0x10;
            
            njPopMatrixEx();
            return;
        }
        
        op->stflg &= ~0x10;
    }

    njColorBlendingMode(0, op->bl_src);
    njColorBlendingMode(1, op->bl_dst);
    
    njDrawTexture3DEx(op->tvp, op->pn, 1);
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 6);
    
    njPopMatrixEx();

    if ((sys->st_flg & 0x2)) 
    {
        njFogEnable();
    }
}

// 100% matching!
void bhEff_SetUVInfo(O_WRK* op, UV_WORK* uvp, float BaseSize)
{
    op->tv[0].u = uvp->u;
    op->tv[0].v = uvp->v;
    
    op->tv[1].u = uvp->u + uvp->xs;
    op->tv[1].v = op->tv[0].v;
    
    op->tv[2].u = op->tv[0].u;
    op->tv[2].v = uvp->v + uvp->ys;
    
    op->tv[3].u = op->tv[1].u;
    op->tv[3].v = op->tv[2].v;
    
    op->sx = (op->sxb * uvp->xs) / BaseSize;
    op->sy = (op->syb * uvp->ys) / BaseSize;
}

// 100% matching!
void bhEff_SetAlign(O_WRK* op, unsigned int flg) 
{
    if ((flg & 0x1))
    {
        if ((flg & 0x2)) 
        {
            op->tv[0].y = 2;
            op->tv[1].y = 2;
            op->tv[2].y = 0;
            op->tv[3].y = 0;
        } 
        else 
        {
            if ((flg & 0x4)) 
            {
                op->tv[0].y = 0;
                op->tv[1].y = 0;
                op->tv[2].y = -2;
                op->tv[3].y = -2;
            } 
            else 
            {
                op->tv[0].y = -1;
                op->tv[1].y = -1;
                op->tv[2].y = 1;
                op->tv[3].y = 1;
            }
        }
    } 
    else 
    {
        if ((flg & 0x4)) 
        {
            op->tv[0].y = 0;
            op->tv[1].y = 0;
            op->tv[2].y = 2;
            op->tv[3].y = 2;
        }
        else if ((flg & 0x2))
        {
            op->tv[0].y = -2;
            op->tv[1].y = -2;
            op->tv[2].y = 0;
            op->tv[3].y = 0;
        } 
        else 
        {
            op->tv[0].y = -1;
            op->tv[1].y = -1;
            op->tv[2].y = 1;
            op->tv[3].y = 1;
        }
    }
    
    op->tv[0].x = -1;
    op->tv[1].x = 1;
    op->tv[2].x = -1;
    op->tv[3].x = 1;
    
    op->tv[0].z = 0;
    op->tv[1].z = 0;
    op->tv[2].z = 0;
    op->tv[3].z = 0;
}

// 100% matching!
void bhEff_SetBaseColor(O_WRK* op, unsigned int col)
{
    op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col = col;
}

// 100% matching!
void bhClrEff_YG()
{
    O_WRK* opp;
    int i;
	
    opp = eff;
    
    for (i = 0; i < 512; i++, opp++) 
    {
        if ((opp->flg & 0x1))
        {
            switch (opp->id) 
            {                      
            case 250:
            case 253:
            case 256:
            case 257:
            case 272:
            case 296:
            case 297:
            case 298:
                opp->flg = 0;
                break;
            case 254:
            case 260:
                *(int*)opp->exp0 = 0;
                
                opp->flg = 0;
                break;
            }
        }
    }
}
