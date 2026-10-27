#include "../../../ps2/veronica/prog/effsub4.h"
#include "../../../ps2/veronica/prog/effect.h"
#include "../../../ps2/veronica/prog/en01.h"
#include "../../../ps2/veronica/prog/hitchkl.h"
#include "../../../ps2/veronica/prog/njplus.h"
#include "../../../ps2/veronica/prog/main.h"
#include "../../../ps2/veronica/prog/ps2_NaDraw.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaSystem.h"
#include "../../../ps2/veronica/prog/ps2_NaTextureFunction.h"
#include "../../../ps2/veronica/prog/ps2_NinjaCnk.h"
#include "../../../ps2/veronica/prog/ps2_NinjaPtcl.h"
#include "../../../ps2/veronica/prog/sdfunc.h"
#include "../../../ps2/veronica/prog/zonzon.h"

// 100% matching!
void bhEff_SetPtcl(BH_PWORK* epw, NJS_POINT3* pos, int tex_id)
{
    int i;
    int time; 

    time = (rand() % 4) * 4; 

    for (i = 0; i < 3; i++) 
    {
        bhEff_SetPtcl1(epw, pos, tex_id, time);
        
        time += 4;
    }
}

// 100% matching!
void bhEff_SetPtcl1(BH_PWORK* epw, NJS_POINT3* pos, int tex_id, int wcnt)
{
    EFFSUB365_WORK* effect; 
    O_WRK* op;             
    POINT ps;               
    int eff_no;           
    NJS_POINT3* vpos;
	NJS_VECTOR* avec;      
    float* size;           
	static float eff_tbl[2] = 
	{
		0.200000003f, 1.5f
	};

    ps.px = pos->x;
    ps.py = pos->y;
    ps.pz = pos->z;
    
    ps.ox = 0;
    ps.oy = 0;
    ps.oz = 0;
    
    if ((eff_no = bhSetEffect(365, &ps, NULL, 0)) != -1) 
    {
        NJS_POINT3 ps; 
        int i;         
        
        effect = (EFFSUB365_WORK*)bhEff_AllocOwork();
        
        op = &eff[eff_no];
        
        vpos = effect->vtx_pos;
        
        op->txp[0] = epw->mlwP->texP;
        op->tex_id = tex_id;
        
        op->exp0 = (unsigned char*)effect;
        
        op->func = (void*)bhEff_PtclSpriteDraw;
        
        op->ct3 = wcnt;
        
        effect->dtbl.num = 40;
        
        size = effect->size;
        
        effect->dtbl.exp0 = (unsigned char*)vpos;
        
        avec = effect->add_vec;
        
        effect->dtbl.exp1 = (unsigned char*)size;
        
        njUnitMatrix(NULL);
        
        njRotateY(NULL, epw->ay);
        
        for (i = 0; i < 40; i++) 
        {
            vpos->x = pos->x + (-0.5f + (-rand() / -2.1474836E9f));
            vpos->y = pos->y;
            vpos->z = pos->z + (-0.5f + (-rand() / -2.1474836E9f));
            
            ps.x = 0;
            ps.y = -(eff_tbl[0] + (eff_tbl[1] * (-rand() / -2.1474836E9f)));
            ps.z = 0;
            
            size[i] = -0.15f * ps.y;
            
            if (size[i] < 0.1f)
            {
                size[i] = 0.1f;
            }
            
            njCalcPoint(NULL, &ps, avec);
            
            vpos++;
            avec++;
        }
    }
}

// 100% matching!
void bhEff_SetPtcl2(BH_PWORK* epw, NJS_POINT3* pos, NJS_MATRIX* mtx) // first parameter not present on DWARF
{
    POINT ps;              
    O_WRK* op;             
    EFFSUB366_WORK* effect; 
    int eff_no;           
    int i;                
    NJS_POINT3* vtx_p, *vtx_p2;     
    NJS_VECTOR* vec;      
    int ang, ang2;                
    float spd;             
	static float eff_tbl[2] = 
	{
		1.0f, 2.0f
	};

    ps.px = pos->x;
    ps.py = pos->y;
    ps.pz = pos->z;
    
    ps.ox = 0;
    ps.oy = 0;
    ps.oz = 0;
    
    if ((eff_no = bhSetEffect(366, &ps, NULL, 0)) != -1) 
    {
        effect = (EFFSUB366_WORK*)bhEff_AllocOwork();
        
        op = &eff[eff_no];
        
        op->exp0 = (unsigned char*)effect;
        
        op->func = (void*)bhEff_LineDraw;
        
        op->ct3 = 0;
        
        op->type = 0;
        
        vtx_p = effect->vtx_pos;
        
        effect->dtbl.num = 16;
        
        vtx_p2 = effect->vtx_pos2;
        
        effect->dtbl.exp0 = (unsigned char*)vtx_p;
        
        vec = effect->add_vec;
        
        effect->dtbl.exp1 = (unsigned char*)vtx_p2;
        
        njSetMatrix(NULL, mtx);
        
        njUnitTransPortion(NULL);
        njRotateZ(NULL, -16384);
        
        for (i = 0; i < 16; i++) 
        {
            vtx_p->x = vtx_p2->x = pos->x;
            vtx_p->y = vtx_p2->y = pos->y;
            vtx_p->z = vtx_p2->z = pos->z;
            
            ang  = ((rand() % 16) * 4096) & 0xFFFF;
            ang2 = (rand()  % 5461) + 5461;
            
            spd = eff_tbl[0] + (eff_tbl[1] * (-rand() / -2.1474836E9f));
            
            vec->x = -njSin(ang) * (spd * njCos(ang2));
            vec->y = spd * njSin(ang2);
            vec->z = -njCos(ang) * (spd * njCos(ang2));
            
            njCalcPoint(NULL, vec, vec);
            
            vec++;
            vtx_p++;
            vtx_p2++;
        }
        
        return;
    }
}

// 100% matching!
void bhEff_SetPtcl2V(BH_PWORK* epw, NJS_POINT3* pos, NJS_POINT3* dv, int wcnt) // first parameter not present on DWARF
{
    POINT ps;               
    O_WRK* op;             
    EFFSUB366_WORK* effect; 
    int eff_no;             
    int i;                  
    NJS_POINT3* vtx_p, *vtx_p2;      
    NJS_VECTOR* vec;        
    int ang1, ang2;               
    float spd;             
	static float eff_tbl[2] = 
	{
		1.0f, 2.0f
	};

    ps.px = pos->x;
    ps.py = pos->y;
    ps.pz = pos->z;
    
    ps.ox = 0;
    ps.oy = 0;
    ps.oz = 0;
    
    if ((eff_no = bhSetEffect(366, &ps, NULL, 0)) != -1) 
    {
        effect = (EFFSUB366_WORK*)bhEff_AllocOwork();
        
        op = &eff[eff_no];
        
        op->exp0 = (unsigned char*)effect;
        
        op->func = (void*)bhEff_LineDraw;
        
        op->ct3 = wcnt; 
        
        op->type = 1;
        
        vtx_p = effect->vtx_pos;
        
		effect->dtbl.num = 16;
        
        vtx_p2 = effect->vtx_pos2;
        
		effect->dtbl.exp0 = (unsigned char*)vtx_p;
        
        vec = effect->add_vec;

        effect->dtbl.exp1 = (unsigned char*)vtx_p2;
        
        for (i = 0; i < 16; i++) 
        {
            vtx_p->x = vtx_p2->x = pos->x;
            vtx_p->y = vtx_p2->y = pos->y;
            vtx_p->z = vtx_p2->z = pos->z;
            
            ang1 = (rand() % 7281) - 3640;
            ang2 = (rand() % 7281) - 3640;
            
            spd = eff_tbl[0] + (eff_tbl[1] * (-rand() / -2.1474836E9f));
            
            njUnitVector(dv); 
            njUnitMatrix(NULL); 
            
            njRotateXYZ(NULL, 0, ang2, -ang1);
            
            njCalcPoint(NULL, dv, vec); 
            
            vec->x *= spd;
            vec->y *= spd;
            vec->z *= spd;
            
            vec++;
            vtx_p++;
            vtx_p2++;
        }
    }
}

// 100% matching!
static void bhEff_SetBlood5(NJS_POINT3* pos, NJS_VECTOR* vec, float size, int col)
{
    int tmp;

    sys->ef.id = 372;
    
    sys->ef.flg = 1;
    
    sys->ef.sx = size;
    sys->ef.sy = size;
    sys->ef.sz = 0;
    
    sys->ef.px = pos->x;
    sys->ef.py = pos->y;
    sys->ef.pz = pos->z;
    
    sys->ef.ay = 0;
    
    sys->ef.type = 0;
    
    sys->ef.mdlver = col;
    
    tmp = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
    
    if (tmp != -1) 
    {
        eff[tmp].ct3 = 0;
        
        eff[tmp].xn = -vec->x;
        eff[tmp].yn = 0;
        eff[tmp].zn = -vec->z;
        
        eff[tmp].shp_ct = 0.5f;
        
        eff[tmp].aox = 0.98f;
        eff[tmp].aoy = -0.03f;
    }
}

// 100% matching!
void bhEff_Sub350(O_WRK* op)
{
	static UV_WORK uvinfo[11] = 
	{
		{   0.0f,  96.0f,  16.0f,  16.0f },
		{   0.0f, 112.0f,  16.0f,  16.0f },
		{   0.0f, 128.0f,  16.0f,  16.0f },
		{   0.0f, 144.0f,  16.0f,  16.0f },
		{  16.0f,  96.0f,  24.0f,  24.0f },
		{  16.0f, 120.0f,  24.0f,  24.0f },
		{  40.0f,  96.0f,  32.0f,  32.0f },
		{  72.0f,  96.0f,  32.0f,  32.0f },
		{ 104.0f,  96.0f,  32.0f,  32.0f },
		{  40.0f, 128.0f,  32.0f,  32.0f },
		{  72.0f, 128.0f,  32.0f,  32.0f }
	};
	static EFF_ANIM anim00_tbl[9] = 
	{
		{  0,  1 },
		{  1,  1 },
		{  2,  1 },
		{  3,  1 },
		{  4,  1 },
		{  5,  1 },
		{  6,  1 },
		{  7,  1 },
		{ -2,  6 }   
	};
	static EFF_ANIM anim01_tbl[4] = 
	{
		{  8,  1 },
		{  9,  1 },
		{ 10,  1 },
		{ -1,  0 }   
	};
	static EFF_ANIM* anim_tbl[2] = 
	{
		anim00_tbl, anim01_tbl
	};
	NJS_POINT3 p1, p2; 
    float wk0, wk1;     
    int tmp;      
    int i;      
    int eno; // not from DWARF

    switch (op->mode0) 
    {                    
    case 0:
        op->tv[0].col = 0xC0C0C0C0;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->ct0 = 0;
        op->ct1 = 0;
        
        op->spd = 1.0f;
        
        op->mode1 = 0;
        op->mode2 = 0;
        
        wk0 = 0.4f * njCos(-5120);
        wk1 = 0.4f * njSin(-5120);
        
        op->xn = -(wk0 * njSin(op->ay));
        op->zn = -(wk0 * njCos(op->ay));
        
        op->yn = wk1;
        
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0)
        {
            break;
        }

        op->mode2 = 1;
        
        op->mode0++;
    case 2:
        tmp = 0;
        
        op->px += op->xn;
        op->py += op->yn - (0.08f * op->ct2);
        op->pz += op->zn;
        
        op->ct2++;
        
        op->cspr.c.x = op->px;
        op->cspr.c.y = op->py;
        op->cspr.c.z = op->pz;
        
        op->cspr.r = 0.5f * op->sy;
        
        if (npCollisionCheckSC(&op->cspr, &plp->watr) != 0) 
        {
            if ((!(plp->flg & 0x4)) && (!(plp->stflg & 0x80000000)))
            {
                plp->flg |= 0x4;
                
                plp->mode0 = 2;
                plp->mode2 = 1;
                plp->mode3 = 0;
                
                plp->hp -= 10;
                
                p1.x = op->px - plp->px;
                p1.y = 0;
                p1.z = op->pz - plp->pz;
                
                if (bhDGCdirCheck(&p1, plp->ay) == 0) 
                {
                    plp->mode1 = 1;
                }
                else
                {
                    plp->mode1 = 0;
                }
            }
            
            tmp = 2;
        }
        else if (op->ct2 > 150) 
        {
            op->mode1 = 1;
            
            op->ct0 = 0;
            op->ct1 = 0;
            
            op->mode0++;
            break;
        } 
        else 
        {
            p1.x = op->pxb;
            p1.y = op->pyb - 0.5f;
            p1.z = op->pzb;
            
            p2.x = op->px;
            p2.y = op->py - 0.5f;
            p2.z = op->pz;
            
            if (bhCollisionCheckLine2(&p1, &p2, 0x400, -1) != NULL) 
            {
                tmp = 1;
                
                op->px = p2.x;
                op->py = 0.5f + p2.y;
                op->pz = p2.z;
            }
        }
        
        if (tmp != 0) 
        {
            sys->ef.id = 351;
            
            sys->ef.flg = 1;
            
            sys->ef.sx = 4.5f;
            sys->ef.sy = 4.5f;
            sys->ef.sz = 0;
            
            sys->ef.ay = 0;
            
            bhEne01_SePlay((BH_PWORK*)op->lkwkp, (NJS_POINT3*)&op->px, 8961);
            
            if (tmp == 2) 
            {
                sys->ef.px = 0;
                sys->ef.py = 0;
                sys->ef.pz = 0;
                
                bhEne_HitCheckParts(plp, &op->cspr.c);
                
                for (i = 0; i < 3; i++) 
                {
                    p1.x = p1.y = p1.z = 0;
                    
                    eno = bhSetEffectTb(&sys->ef, &p1, (unsigned char*)plp, plp->djnt_no);
                    
                    if (eno != -1)
                    {
                        eff[eno].stflg |= 0x20;
                        
                        eff[eno].txp[0] = op->txp[0];
                        eff[eno].tex_id = op->tex_id;
                        
                        eff[eno].ct3 = i * 3;
                        
                        eff[eno].xn = (0.1f * (-rand()         / -2.1474836E9f)) - 0.05f;
                        eff[eno].yn = 0.5f + ((0.2f * (-rand() / -2.1474836E9f)) - 0.1f);
                        eff[eno].zn = (0.1f * (-rand()         / -2.1474836E9f)) - 0.05f;
                    }
                }
            } 
            else
            {
                sys->ef.px = op->px;
                sys->ef.py = op->py;
                sys->ef.pz = op->pz;
                
                eno = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
                
                if (eno != -1)
                {
                    eff[eno].stflg |= 0x20; 
                    
                    eff[eno].txp[0] = op->txp[0];
                    eff[eno].tex_id = op->tex_id;
                    
                    eff[eno].ct3 = 0;
                    
                    eff[eno].xn = (0.1f * (-rand()         / -2.1474836E9f)) - 0.05f;
                    eff[eno].yn = 0.5f + ((0.2f * (-rand() / -2.1474836E9f)) - 0.1f);
                    eff[eno].zn = (0.1f * (-rand()         / -2.1474836E9f)) - 0.05f;
                }
            }
            
            op->mode1 = 1;
            
            op->ct0 = 0;
            op->ct1 = 0;
            
            op->mode0++;
        } 
        
        break;
    case 3:
        break;
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub351(O_WRK* op) 
{
    static UV_WORK uvinfo[14] =
	{
		{  24.0f,  80.0f,  16.0f,  16.0f },
		{   0.0f,  48.0f,  24.0f,  24.0f },
		{   0.0f,  72.0f,  24.0f,  24.0f },
		{  24.0f,  48.0f,  32.0f,  32.0f },
		{  56.0f,  48.0f,  32.0f,  32.0f },
		{  88.0f,  48.0f,  40.0f,  40.0f },
		{ 128.0f,  48.0f,  40.0f,  40.0f },
		{   0.0f,   0.0f,  48.0f,  48.0f },
		{  48.0f,   0.0f,  48.0f,  48.0f },
		{  96.0f,   0.0f,  48.0f,  48.0f },
		{ 144.0f,   0.0f,  48.0f,  48.0f },
		{ 192.0f,   0.0f,  48.0f,  48.0f },
		{ 168.0f,  48.0f,  40.0f,  40.0f },
		{ 208.0f,  48.0f,  40.0f,  40.0f }
	};
	static EFF_ANIM anim00_tbl[15] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 },
		{ 12,  1 }, { 13,  1 }, { -1,  0 }
	}; 
    
    switch (op->mode0) 
    {                           
    case 0:
        op->tv[0].col = 0x80808080;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->spd = 1.0f;
        
        op->mode2 = 0;
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0)
        {
            break;
        }
        
        op->ct3 = 0;
        
        op->mode2 = 1;
        
        if ((op->flg & 0x80)) 
        {
            op->flg &= ~0x80;
        }
        
        op->mode0++;
    case 2:
        op->px += op->xn;
        op->py += op->yn;
        op->pz += op->zn;
        
        if (++op->ct3 > 8)
        {
            op->tv[0].col += 0xF0000000;
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        }

		break;
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim00_tbl, uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub352(O_WRK* op)
{
	static UV_WORK uvinfo[10] = 
	{
		{   0.0f,   0.0f,  48.0f,  48.0f },
		{  48.0f,   0.0f,  48.0f,  48.0f },
		{  96.0f,   0.0f,  48.0f,  48.0f },
		{ 144.0f,   0.0f,  48.0f,  48.0f },
		{ 192.0f,   0.0f,  48.0f,  48.0f },
		{   0.0f,  48.0f,  48.0f,  48.0f },
		{  48.0f,  48.0f,  48.0f,  48.0f },
		{  96.0f,  48.0f,  48.0f,  48.0f },
		{ 144.0f,  48.0f,  48.0f,  48.0f },
		{ 192.0f,  48.0f,  48.0f,  48.0f }
	};
	static EFF_ANIM anim00_tbl[11] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { -2,  0 }    
	};
	static EFF_ANIM *anim_tbl[1] = 
	{
		anim00_tbl
	};
    NJS_POINT3 p1, p2; 
    O_WORK* owk;  
    
    if ((sys->pt_flg & 0x2)) 
    {
        if ((op->mode0 == 0) || (!(((O_WRK*)op->lkwkp)->stflg & 0x1000000))) 
        {
            switch ((unsigned short)op->mode0) 
            {                   
            case 0:
                op->bl_src = 8;
                op->bl_dst = 10;
                
                op->flg |= 0x4180000;
                
                op->tex_id = 8;
                
                op->ani_ct = 0;
                
                op->spd = 1.0f;
                
                op->tv[0].x = -1.0f;
                op->tv[0].y = -1.5f;
                op->tv[0].z = 0;
                
                op->tv[1].x = 1.0f;
                op->tv[1].y = -1.5f;
                op->tv[1].z = 0;
                
                op->tv[2].x = -1.0f;
                op->tv[2].y = 0.5f;
                op->tv[2].z = 0;
                
                op->tv[3].x = 1.0f;
                op->tv[3].y = 0.5f;
                op->tv[3].z = 0;
                
                if ((op->type == 0) || (op->type == 2)) 
                {
                    op->tv[0].col = 0xC0FFFFFF;
                    
                    op->ct1 = 0;
                } 
                else 
                {
                    op->tv[0].col = 0xB4FFFFFF;
                    
                    op->ct1 = 5;
                    
                    op->sxb *= 1.2f;
                    op->syb *= 1.2f;
                    
                    op->tv[0].z = op->tv[1].z = op->tv[2].z = op->tv[3].z = 0.05f;
                }
                
                op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
                
                op->ct0 = 0;
                op->ct2 = 0;
                op->ct3 = 0;
                
                op->ofx = op->lox;
                op->ofy = op->loy;
                op->ofz = op->loz;
                
                op->xn = op->sxb;
                op->yn = op->sxb / 8.0f;
                
                op->sxb = op->syb = 0;
                
                op->mode1 = 0;
                op->mode2 = 0;
                
                op->mode0++;
            case 1:
                op->ct3--;
                
                if (op->ct3 >= 0) 
                {
                    break;
                }
                
                if (op->type >= 2) 
                {
                    op->ct3 = (rand() % 60) + 30;
                }
                
                op->mode2 = 1;
                
                op->mode0++;
            case 2:
                op->sxb += op->yn; 
                
                if (op->sxb > op->xn) 
                {
                    op->sxb = op->xn;
                }
                
                op->syb = op->sxb;
                
                if ((op->type >= 2) && (--op->ct3 < 0))
                {
                    op->ct3 = 40;
                    
                    op->mode0++;
                }
                
                break;
            case 3:
                op->sxb *= 0.96f;
                op->syb = op->sxb; 
                
                if (--op->ct3 < 0) 
                {
                    op->flg = 0;
                }
                
                break;
            }
            
            if ((op->flg != 0) && ((op->ct2 != 0) && (op->shp_ct > op->mtx[0][13]))) 
            {
                op->flg = 0;
            }
            
            if (op->mode2 != 0)
            {
                bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 0);
                
                p1.x = -cam.vx * op->zn;
                p1.y = -cam.vy * op->zn;
                p1.z = -cam.vz * op->zn;
                
                owk = &((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono];
                
                njSetMatrix(NULL, &owk->mtx);
                
                njCalcPoint(NULL, (NJS_POINT3*)&op->ofx, &p2);
                
                p1.x = (p1.x + p2.x) - owk->mtx[12];
                p1.y = (p1.y + p2.y) - owk->mtx[13];
                p1.z = (p1.z + p2.z) - owk->mtx[14];
                
                njInvertMatrix(NULL);
                
                njCalcVector(NULL, &p1, &p2);
                
                op->lox = p2.x;
                op->loy = p2.y;
                op->loz = p2.z;
                
                op->ct2 = 1;
            }
        }
    }
}

// 100% matching!
void bhEff_Sub353(O_WRK* op)
{
    static UV_WORK uvinfo[1] = 
	{
		0.0f,   0.0f, 128.0f, 128.0f
	};
    
    switch (op->mode0) 
    {                             
    case 0:
        op->tv[0].col = 0xFFFFFF;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg   |= 0x4080000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->ax = 16384;
        op->ay = 0;
        op->az = 0;
        
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct0 = 18;
        
        op->mode2 = 1;
        
        op->mode0++;
    case 2:
        if ((op->tv[0].col & 0xFF000000) >= 0xC0000000) 
        {
            op->tv[0].col = 0xC0FFFFFF;
        }
        else 
        {
            op->tv[0].col += 0x8000000;
        }
        
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        if (--op->ct0 < 0) 
        {
            op->ct0 = 30;
            
            op->mode0++;
        }
        
        break;
    case 3:
        op->sxb += 8.0f;
        op->syb += 8.0f;
        
        if (--op->ct0 < 0)
        {
            op->tv[0].col += 0xF0000000;
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
            
            if ((op->tv[0].col & 0xFF000000) < 0x10000000) 
            {
                op->lkwkp = NULL;
                
                op->flg = 0;
            }
        }
        
        break;
    case 4:
        op->tv[0].col += 0xF0000000;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        if ((op->tv[0].col & 0xFF000000) < 0x10000000)
        {
            op->lkwkp = NULL;
            
            op->flg = 0;
        }
        
        break;
    }
    
    if (op->mode2 != 0) 
    {
        op->sx = op->sxb * (uvinfo[op->ct1].xs / 127.0f);
        op->sy = op->syb * (uvinfo[op->ct1].ys / 127.0f);
        
        op->tv[0].u = uvinfo[op->ct1].u / 127.0f;
        op->tv[0].v = uvinfo[op->ct1].v / 127.0f;
        
        op->tv[3].u = ((uvinfo[op->ct1].u + uvinfo[op->ct1].xs) - 1.0f) / 127.0f;
        op->tv[3].v = ((uvinfo[op->ct1].v + uvinfo[op->ct1].ys) - 1.0f) / 127.0f;
        
        op->tv[1].u = op->tv[3].u;
        op->tv[1].v = op->tv[0].v;
        
        op->tv[2].u = op->tv[0].u;
        op->tv[2].v = op->tv[3].v;
        
        if (op->flg != 0) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }
    }
}

// 100% matching!
void bhEff_Sub354(O_WRK* op) 
{
	static UV_WORK uvinfo[29] = 
	{
		{   0.0f,   0.0f,  40.0f,  40.0f },
		{  40.0f,   0.0f,  40.0f,  40.0f },
		{  80.0f,   0.0f,  40.0f,  40.0f },
		{ 120.0f,   0.0f,  40.0f,  40.0f },
		{ 160.0f,   0.0f,  40.0f,  40.0f },
		{ 200.0f,   0.0f,  40.0f,  40.0f },
		{   0.0f,  40.0f,  40.0f,  40.0f },
		{  40.0f,  40.0f,  40.0f,  40.0f },
		{  80.0f,  40.0f,  40.0f,  40.0f },
		{ 120.0f,  40.0f,  40.0f,  40.0f },
		{ 160.0f,  40.0f,  40.0f,  40.0f },
		{ 200.0f,  40.0f,  40.0f,  40.0f },
		{   0.0f,  96.0f,  40.0f,  24.0f },
		{  40.0f,  96.0f,  40.0f,  24.0f },
		{  80.0f,  96.0f,  40.0f,  24.0f },
		{ 120.0f,  96.0f,  40.0f,  24.0f },
		{ 160.0f,  96.0f,  40.0f,  24.0f },
		{ 200.0f,  96.0f,  40.0f,  24.0f },
		{   0.0f, 128.0f,  40.0f,  40.0f },
		{  40.0f, 128.0f,  40.0f,  40.0f },
		{  80.0f, 128.0f,  40.0f,  40.0f },
		{ 120.0f, 128.0f,  40.0f,  40.0f },
		{ 160.0f, 128.0f,  40.0f,  40.0f },
		{ 200.0f, 128.0f,  40.0f,  40.0f },
		{   0.0f, 176.0f,  48.0f,  48.0f },
		{  48.0f, 176.0f,  48.0f,  48.0f },
		{  96.0f, 176.0f,  48.0f,  48.0f },
		{ 144.0f, 176.0f,  48.0f,  48.0f },
		{ 192.0f, 176.0f,  48.0f,  48.0f }
	};
	static EFF_ANIM anim00_tbl[17] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 },
		{ 11,  1 }, { 11,  1 }, { 11,  1 }, { 11,  1 },
		{ -1,  0 }
	};
	static EFF_ANIM anim01_tbl[7] = 
	{
		{ 12,  1 }, { 13,  1 }, { 14,  1 }, { 15,  1 },
		{ 16,  1 }, { 17,  1 }, { -1,  0 }
	};
	static EFF_ANIM anim02_tbl[7] = 
	{
		{ 18,  1 }, { 19,  1 }, { 20,  1 }, { 21,  1 },
		{ 22,  1 }, { 23,  1 }, { -1,  0 }
	};
	static EFF_ANIM anim03_tbl[6] = 
	{ 
		{ 24,  1 }, { 25,  1 }, { 26,  1 }, { 27,  1 },
		{ 28,  1 }, { -1,  0 }
	};
	static EFF_ANIM *anim_tbl[4] = 
	{
		anim00_tbl, anim01_tbl, anim02_tbl, anim03_tbl
	};
    O_WORK* owk;   
    unsigned int argb_tbl[2] =
	{
		0xC0FFFFFF, 0x20FFFFFF
	};
    NJS_POINT3 ofs; 
    NJS_POINT3 p1, p2;  
    
    switch (op->mode0) 
    {                           
    case 0:
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->tv[0].col = argb_tbl[0];
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->spd = 1.0f;
        
        op->ct0 = 0;
        
        op->ofx = op->lox;
        op->ofy = op->loy;
        op->ofz = op->loz;
        
        op->xn = op->sxb;
        op->yn = op->sxb / 8.0f;
        
        op->mode1 = op->type;
        op->mode2 = 0;
        
        op->func = (void*)bhEff_3DSpriteDraw;
        
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->mode2 = 1;
        
        op->ct3 = 11;
        
        op->mode0++;
    case 2:
        if ((op->mode1 == 0) && (--op->ct3 < 0))
        {
            op->tv[0].col += 0xE0000000;
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        }
        
        break;
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 1);
        
        if (op->lkwkp != NULL) 
        {
            p1.x = -cam.vx * op->zn;
            p1.y = -cam.vy * op->zn;
            p1.z = -cam.vz * op->zn;
            
            owk = &((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono];
            
            njSetMatrix(NULL, &owk->mtx);
            
            ofs.x = op->ofx + ((-rand() / -2147483648.0f) - 0.5);
            ofs.y = op->ofy + ((-rand() / -2147483648.0f) - 0.5);
            ofs.z = op->ofz;
            
            njCalcPoint(NULL, &ofs, &p2);
            
            p1.x = (p1.x + p2.x) - owk->mtx[12];
            p1.y = (p1.y + p2.y) - owk->mtx[13];
            p1.z = (p1.z + p2.z) - owk->mtx[14];
            
            njInvertMatrix(NULL);
            
            njCalcVector(NULL, &p1, &p2);
            
            op->lox = p2.x;
            op->loy = p2.y;
            op->loz = p2.z;
        }
    }
}

// 100% matching!
void bhEff_Sub355(O_WRK* op) 
{
    switch (op->mode0) 
    {                             
    case 0:
        op->tv[0].col = 0xFF0000;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg |= 0x40A0000;
        
        op->ani_ct = 2;
        
        op->tex_id = 4;
        
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->sx = op->sxb;
        op->sy = op->syb;
        
        op->tv[0].u = 0;
        op->tv[0].v = 0;
        
        op->tv[3].u = 1.0f;
        op->tv[3].v = 1.0f;
        
        op->tv[1].u = op->tv[3].u;
        op->tv[1].v = op->tv[0].v;
        
        op->tv[2].u = op->tv[0].u;
        op->tv[2].v = op->tv[3].v;
        
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->mode2 = 1;
        op->mode0++;
    case 2:
        op->tv[0].col += 0xF0000000;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        if ((op->tv[0].col & 0xFF000000) < 0x10000000) 
        {
            op->flg = 0;
        }
        
        break;
    }
    
    if ((((O_WRK*)op->exp0)->stflg & 0x1000000)) 
    {
        op->flg = 0;
    }
    
    if ((op->mode2 != 0) && (op->flg != 0))
    {
        sys->ef_trs[sys->ef_trsn] = op;
        
        sys->ef_trsn++;
    }
}

// 100% matching!
void bhEff_Sub356(O_WRK* op) 
{
    switch (op->mode0) 
    {
    case 0:
        op->mode0++;
        break;
    case 1:
        bhEne01_NikuhenEffect((BH_PWORK*)op->lkwkp, (NJS_POINT3*)&op->px, 4);
        
        op->mode0++;
        break;
    case 2:
        op->mode0++;
        break;
    case 3:
        bhEne01_NeckBloodEffect2((BH_PWORK*)op->lkwkp, 0);
        
        op->ct0 = 24;
        
        op->mode0++;
        break;
    case 4:
        if (--op->ct0 < 0) 
        {
            bhEne01_NeckBloodEffect2((BH_PWORK*)op->lkwkp, 1);
            
            op->ct0 = 12;
            
            op->mode0++;
        }
        
        break;
    case 5:
        if (--op->ct0 < 0) 
        {
            bhEne01_NeckBloodEffect2((BH_PWORK*)op->lkwkp, 2);
            
            op->flg = 0;
        }
        
        break;
    }
}

// 100% matching!
void bhEff_Sub357()
{

}

// 100% matching!
void bhEff_Sub360(O_WRK* op) 
{
    static UV_WORK bl00_uvinfo[17] = 
	{
		{  0.0f,    0.0f,   8.0f,   8.0f },
		{  0.0f,    8.0f,  16.0f,  16.0f },
		{  0.0f,   24.0f,  16.0f,  16.0f },
		{  16.0f,  24.0f,  24.0f,  24.0f },
		{  16.0f,   0.0f,  24.0f,  24.0f },
		{  40.0f,   0.0f,  32.0f,  32.0f },
		{  72.0f,   0.0f,  32.0f,  32.0f },
		{ 104.0f,   0.0f,  40.0f,  40.0f },
		{ 144.0f,   0.0f,  40.0f,  40.0f },
		{ 184.0f,   0.0f,  48.0f,  48.0f },
		{   0.0f,  48.0f,  48.0f,  48.0f },
		{  48.0f,  48.0f,  56.0f,  56.0f },
		{ 104.0f,  48.0f,  56.0f,  56.0f },
		{ 160.0f,  48.0f,  56.0f,  56.0f },
		{   0.0f, 104.0f,  56.0f,  56.0f },
		{  56.0f, 104.0f,  56.0f,  56.0f },
		{ 112.0f, 104.0f,  56.0f,  56.0f }
	};
	static UV_WORK bl01_uvinfo[5] = 
	{
		{  40.0f,  32.0f,   8.0f,   8.0f },
		{  40.0f,  40.0f,  16.0f,  16.0f },
		{  48.0f,  32.0f,  16.0f,  16.0f },
		{  64.0f,  32.0f,  16.0f,  16.0f },
		{  80.0f,  32.0f,  16.0f,  16.0f }
	};
	static EFF_ANIM blanim00_tbl[19] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 },
		{ 12,  1 }, { 13,  1 }, { 14,  1 }, { 15,  1 },
		{ 16,  1 }, { 17,  1 }, { -1,  0 }
	};
	static EFF_ANIM blanim01_tbl[7] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, { -1,  0 }
	};
	UV_WORK* blood_uvinfo[2] = 
	{
		bl00_uvinfo, bl01_uvinfo
	};
	EFF_ANIM* anim_tbl[2] = 
	{ 
		blanim00_tbl, blanim01_tbl 
	};

    switch (op->mode0) 
    {                       
    case 0:
        op->tv[0].col = 0xC0C0C0C0;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->spd = 1.0f;
        
        op->mode1 = 0;
        op->mode2 = 0;
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 < 0) 
        {
            op->mode2 = 1;
            op->mode0++;  
        }
        
        break;
    case 2:
        break;
    }
    
    if (op->mode2 != 0) 
    {
    	bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], blood_uvinfo[op->mode1], 0);
    }
}

// 100% matching!
void bhEff_Sub361(O_WRK* op) 
{
    static UV_WORK bl00_uvinfo[8] = 
	{
		{   0.0f,  88.0f,  48.0f,  48.0f },
		{  48.0f,  88.0f,  48.0f,  48.0f },
		{  96.0f,  88.0f,  48.0f,  48.0f },
		{ 144.0f,  88.0f,  48.0f,  48.0f },
		{ 192.0f,  88.0f,  48.0f,  48.0f },
		{   0.0f, 136.0f,  48.0f,  48.0f },
		{  48.0f, 136.0f,  48.0f,  48.0f },
		{  96.0f, 136.0f,  48.0f,  48.0f }
	};
	static UV_WORK bl01_uvinfo[9] = 
	{
		{  24.0f,  32.0f,  16.0f,  16.0f },
		{  24.0f,  48.0f,  16.0f,  16.0f },
		{  40.0f,  48.0f,  24.0f,  24.0f },
		{  40.0f,  72.0f,  32.0f,  32.0f },
		{   0.0f, 104.0f,  40.0f,  40.0f },
		{  40.0f, 104.0f,  40.0f,  40.0f },
		{  80.0f, 104.0f,  40.0f,  40.0f },
		{   0.0f, 144.0f,  40.0f,  40.0f },
		{  40.0f, 144.0f,  40.0f,  40.0f }
	};
	static EFF_ANIM blanim00_tbl[9] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{ -1,  0 }  
	};
	static EFF_ANIM blanim01_tbl[11] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { -1,  0 }   
	};
	UV_WORK* blood_uvinfo[2] = 
	{ 
		bl00_uvinfo, bl01_uvinfo
	};
	EFF_ANIM* anim_tbl[2] = 
	{ 
		blanim00_tbl, blanim01_tbl 
	};

    switch (op->mode0) 
    {                       
    case 0:
        if (op->type == 0) 
        {
            op->tv[0].col = 0xC0808080;
        }
        else 
        {
            op->tv[0].col = 0xFFC0C0C0;
        }
        
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->spd = 1.0f;
        
        op->mode1 = op->type;
        op->mode2 = 0;
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 < 0) 
        {
            op->mode2 = 1;
            op->mode0++;  
        }
        
        break;
    case 2:
        break;
    }
    
    if (op->mode2 != 0) 
    {
    	bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], blood_uvinfo[op->mode1], 0);
    }
}

// 100% matching!
void bhEff_Sub362(O_WRK* op)
{
	static UV_WORK bl00_uvinfo[48] = 
	{
		{   0.0f,   0.0f,  32.0f,  32.0f },
		{  32.0f,   0.0f,  32.0f,  32.0f },
		{  64.0f,   0.0f,  32.0f,  32.0f },
		{  96.0f,   0.0f,  32.0f,  32.0f },
		{ 128.0f,   0.0f,  32.0f,  32.0f },
		{ 160.0f,   0.0f,  32.0f,  32.0f },
		{ 192.0f,   0.0f,  32.0f,  32.0f },
		{ 224.0f,   0.0f,  32.0f,  32.0f },
		{   0.0f,  96.0f,  48.0f,  48.0f },
		{  96.0f,  96.0f,  48.0f,  48.0f },
		{   0.0f,  32.0f,  32.0f,  32.0f },
		{  32.0f,  32.0f,  32.0f,  32.0f },
		{  64.0f,  32.0f,  32.0f,  32.0f },
		{  96.0f,  32.0f,  32.0f,  32.0f },
		{ 128.0f,  32.0f,  32.0f,  32.0f },
		{ 160.0f,  32.0f,  32.0f,  32.0f },
		{ 192.0f,  32.0f,  32.0f,  32.0f },
		{ 224.0f,  32.0f,  32.0f,  32.0f },
		{  48.0f,  96.0f,  48.0f,  48.0f },
		{ 144.0f,  96.0f,  48.0f,  48.0f },
		{   0.0f,  64.0f,  32.0f,  32.0f },
		{  32.0f,  64.0f,  32.0f,  32.0f },
		{  64.0f,  64.0f,  32.0f,  32.0f },
		{  96.0f,  64.0f,  32.0f,  32.0f },
		{ 128.0f,  64.0f,  32.0f,  32.0f },
		{ 160.0f,  64.0f,  32.0f,  32.0f },
		{ 192.0f,  64.0f,  32.0f,  32.0f },
		{ 224.0f,  64.0f,  32.0f,  32.0f },
		{   0.0f, 144.0f,  48.0f,  48.0f },
		{  96.0f, 144.0f,  48.0f,  48.0f },
		{   0.0f,  96.0f,  32.0f,  32.0f },
		{  32.0f,  96.0f,  32.0f,  32.0f },
		{  64.0f,  96.0f,  32.0f,  32.0f },
		{  96.0f,  96.0f,  32.0f,  32.0f },
		{ 128.0f,  96.0f,  32.0f,  32.0f },
		{ 160.0f,  96.0f,  32.0f,  32.0f },
		{ 192.0f,  96.0f,  32.0f,  32.0f },
		{ 224.0f,  96.0f,  32.0f,  32.0f },
		{  48.0f, 144.0f,  48.0f,  48.0f },
		{ 144.0f, 144.0f,  48.0f,  48.0f },
		{   0.0f, 224.0f,  32.0f,  32.0f },
		{  32.0f, 224.0f,  32.0f,  32.0f },
		{  64.0f, 224.0f,  32.0f,  32.0f },
		{  96.0f, 224.0f,  32.0f,  32.0f },
		{ 128.0f, 224.0f,  32.0f,  32.0f },
		{ 160.0f, 224.0f,  32.0f,  32.0f },
		{ 192.0f, 224.0f,  32.0f,  32.0f },
		{ 224.0f, 224.0f,  32.0f,  32.0f }
	};
	static EFF_ANIM blanim00_tbl[6] = 
	{
		{ 20,  1 }, { 21,  1 }, { 22,  1 }, { 23,  1 },
		{ 24,  1 }, { -2,  0 }   
	};
	static EFF_ANIM blanim01_tbl[6] =
	{
		{ 30,  1 }, { 31,  1 }, { 32,  1 }, { 33,  1 },
		{ 34,  1 }, { -2,  0 }  
	};
	static EFF_ANIM blanim02_tbl[6] = 
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, { -2,  0 }   
	};
	static EFF_ANIM blanim03_tbl[6] = 
	{
		{ 10,  1 }, { 11,  1 }, { 12,  1 }, { 13,  1 },
		{ 14,  1 }, { -2,  0 }   
	};
	static EFF_ANIM blanim04_tbl[6] = 
	{
		{ 40,  1 }, { 41,  1 }, { 42,  1 }, { 43,  1 },
		{ 44,  1 }, { -2,  0 }   
	};
    EFF_ANIM* anim_tbl[5] =    
	{
		blanim00_tbl, blanim01_tbl, blanim02_tbl, blanim03_tbl, blanim04_tbl
	};
	float eff_sub362_tbl[1][3] = 
	{
		{  2.0f, 0.05f, 0.97f }
	};
    NJS_POINT3 p1, p2;             
    NJS_POINT3 n;            
  
    switch (op->mode0)
    {                                     
    case 0:
        op->tv[0].col = 0xFF808080;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        op->ct3 = rand() % 3;
        
        op->frm_no = 0;
        
        op->spd = 1.0f;
        
        op->mode1 = op->type;
        op->mode2 = 0;
        
        op->spd = eff_sub362_tbl[0][0] + (-rand() / -2.1474836E9f);
        
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct3 = 0;
        
        op->mode2 = 1;
        
        op->mode0++;
    case 2:
        op->spd *= eff_sub362_tbl[0][2];
        
        op->px += op->xn  * op->spd;
        op->py += (op->yn * op->spd) - (eff_sub362_tbl[0][1] * op->ct2);
        op->pz += op->zn  * op->spd;
        
        op->ct2++;
        
        if (op->ct2 > 150)
        {
            op->mode0++;
        } 
        else
        {
            p1.x = op->pxb;
            p1.y = op->pyb - 0.5f;
            p1.z = op->pzb;
            
            p2.x = op->px;
            p2.y = op->py - 0.5f;
            p2.z = op->pz;
            
            if (bhCollisionCheckLine2(&p1, &p2, 0x4400, -1) != NULL)
            {
                op->px = p2.x;
                op->py = 0.5f + p2.y;
                op->pz = p2.z;
                
                bhGetHitCollisionNormal(&n);
                
                njUnitVector(&n);
                
                if ((n.y > 0.9f) || (op->ct3 > 3)) 
                {
                    op->xn = 0;
                    op->yn = 0;
                    op->zn = 0;
                    
                    op->frm_no = 0;
                    
                    op->mode0++;
                    break;
                } 
                
                op->ct3++;
                
                op->xn = 0;
                op->zn = 0;
            }
            
            if ((op->frm_no % 3) == 0) 
            {
                if (op->mode1 < 4) 
                {
                    bhEff_SetBlood5((NJS_POINT3*)&op->px, (NJS_POINT3*)&op->xn, 1.0f + op->sxb, 0);
                } 
                else 
                {
                    bhEff_SetBlood5((NJS_POINT3*)&op->px, (NJS_POINT3*)&op->xn, 1.0f + op->sxb, 1);
                }
            }
            
            op->frm_no++;
        }
        
        break;
    case 3:
        op->tv[0].col += 0xE0000000;
        
        if ((op->tv[0].col & 0xFF000000) < 0x20000000) 
        {
            op->flg = 0;
        }
        else 
        {
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
            
            op->sxb = op->sxb * 0.9f;
            op->syb = op->syb * 0.9f;
            
            if ((op->frm_no % 3) == 0) 
            {
                if (op->mode1 < 4) 
                {
                    bhEff_SetBlood5((NJS_POINT3*)&op->px, (NJS_POINT3*)&op->xn, 1.0f + op->sxb, 0);
                } 
                else
                {
                    bhEff_SetBlood5((NJS_POINT3*)&op->px, (NJS_POINT3*)&op->xn, 1.0f + op->sxb, 1);
                }
            }
            
            op->frm_no++;
        }
        
        break;
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], bl00_uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub363(O_WRK* op)
{
    static UV_WORK eff_uvinfo[12] =
	{
		{   0.0f, 160.0f,  32.0f,  32.0f }, {  32.0f, 160.0f,  32.0f,  32.0f }, {  64.0f, 160.0f,  32.0f,  32.0f }, 
		{  96.0f, 160.0f,  32.0f,  32.0f }, {   0.0f, 192.0f,  32.0f,  32.0f }, {  32.0f, 192.0f,  32.0f,  32.0f }, 
		{  64.0f, 192.0f,  32.0f,  32.0f }, {  96.0f, 192.0f,  32.0f,  32.0f }, {   0.0f, 224.0f,  32.0f,  32.0f }, 
		{  32.0f, 224.0f,  32.0f,  32.0f }, {  64.0f, 224.0f,  32.0f,  32.0f }, {  96.0f, 224.0f,  32.0f,  32.0f }
	};
    
    switch (op->mode0)
    {                              
    case 0:                                         
        op->tv[0].col = 0xFF808080;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4080000;
        op->stflg |= 0x20;
        
        if ((op->type == 0) || (op->type == 2) || (op->type == 4)) 
        {                        
            op->ct0 = 16;
        }
        else
        {
            op->ct0 = 8;
        }
        
        if ((op->type == 0) || (op->type == 1)) 
        {
            op->ct1 = rand() % 4;
        } 
        else if ((op->type == 2) || (op->type == 3)) 
        {
            op->ct1 = (rand() % 4) + 4;
        } 
        else 
        {
            op->ct1 = (rand() % 4) + 8;
        }

        op->ar = 1.0f;
        op->ah = 1.0f;
        
        op->ofx = 0.8f;
        op->ofy = 0.05f;
        
        op->mode0++;
    case 1:                                         
        if (op->ct3 <= 0) 
        {
            op->spd *= op->ofx;
            
            if (op->spd < 0) 
            {
                op->spd = 0;
            }
            
            if ((op->type == 0) || (op->type == 2) || (op->type == 4))
            {
                op->py += (op->yn * op->spd) - (op->ofy * op->ct2);

                op->ax += 4096;
            } 
            else
            {
                op->py += op->yn * op->spd;
            }
            
            op->px += op->xn * op->spd;
            op->pz += op->zn * op->spd;
            
            op->ct2++;
        }
        else 
        {
            op->ct3--;
        }
        
        break;
    case 2:                                         
        break;
    }
    
    if (op->ct0 <= 0) 
    {
        op->tv[0].col += 0xF0000000;
        
        if ((op->tv[0].col & 0xFF000000) < 0x10000000) 
        {
            op->flg = 0;
        }
        
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
    } 
    else 
    {
        op->ct0--;
    }
    
    bhEff_SetSprite(op, &eff_uvinfo[op->ct1], 0);
}

// 100% matching!
void bhEff_Sub364(O_WRK* op)
{
    switch (op->mode0) 
    {                          
    case 0:
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg |= 0x240000;
        
        op->ani_ct = 0;
        
        op->ct0 = 1;
        op->ct1 = 0;
        op->ct2 = 0;
        op->ct3 = 0;
        
        op->func = (void*)bhEff_ModelDraw;
        
        op->mode0++;
    case 1:
        if (--op->ct0 < 0) 
        {
            op->flg = 0;
        }
        
        break;
    }
    
    if (op->flg != 0) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_Sub365(O_WRK* op) 
{
    EFFSUB365_WORK* effect; 
    NJS_POINT3* vpos; 
    NJS_VECTOR* avec; 
    int i; 
	
    effect = (EFFSUB365_WORK*)op->exp0;
    
    vpos = effect->vtx_pos;
    avec = effect->add_vec;
    
    switch (op->mode0) 
    {                           
    case 0:
        op->tv->col = 0xFF404040;
        
        op->sx = 1.0f;
        op->sy = 1.0f;
        
        op->ct0 = 3;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->mode2 = 0;
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct0 = 0;
        
        op->mode2 = 1;
        op->mode0++;
    case 2:
        for (i = 0; i < effect->dtbl.num; i++) 
        {
            vpos->x += avec->x;
            vpos->y += avec->y + (-0.02f * op->ct0);
            vpos->z += avec->z;

            vpos++;
            avec++;
        }
        
        op->ct0++;
        
        if (op->ct0 > 8) 
        {
            op->tv->col += 0xF0000000;
            
            if ((op->tv->col & 0xFF000000) < 0x10000000) 
            {
                op->flg          = 0;
                effect->dtbl.flg = 0;
            }
        }
    }
    
    if ((op->mode2 != 0) && (op->flg != 0)) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_Sub366(O_WRK* op) 
{
    EFFSUB366_WORK* effect; 
    NJS_POINT3* vtx_p, *vtx_p2;   
    NJS_VECTOR* vec;        
    float* g;             
    int i;                  
	
    effect = (EFFSUB366_WORK*)op->exp0;
    
    vtx_p  = effect->vtx_pos;
    vtx_p2 = effect->vtx_pos2;
    
    vec = effect->add_vec;
    
    g = effect->g;
    
    switch (op->mode0) 
    {                            
    case 0:
        if (op->type == 0) 
        {
            op->tv[0].col = -0xC0;
            op->tv[1].col = -0xC0;
            
            op->bl_src = 8;
            op->bl_dst = 10;
        }
        else
        {
            op->tv[0].col = 0xFF600000;
            op->tv[1].col = 0xFF100000;
            
            op->bl_src = 8;
            op->bl_dst = 3;
        }
        
        op->sx = 1.0f;
        op->sy = 1.0f;
        
        op->ct0 = 3;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->mode2 = 0;
        
        op->mode0++;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct0 = 0;
        
        op->mode2 = 1;
        
        op->mode0++;
    case 2:
        op->ct0++;
        op->ct1++;
        
        for (i = 0; i < effect->dtbl.num; i++) 
        {
            *g += -0.2f;
            
            vtx_p2->x = vtx_p->x;
            vtx_p2->y = vtx_p->y;
            vtx_p2->z = vtx_p->z;
            
            vtx_p->x += vec->x;
            vtx_p->y += vec->y + *g;
            vtx_p->z += vec->z;
            
            if (vtx_p->y < 0) 
            {
                vtx_p->y = 0;
                
                *g = 0;
                
                vec->x *= 0.9f;
                vec->y *= 0.9f;
                vec->z *= 0.9f;
            }
            
            g++;
            vtx_p++;
            vtx_p2++;
            vec++;
        }
        
        if ((op->type == 0) && (op->ct1 < 5)) 
        {
            op->tv[0].col -= 0x800;
            op->tv[1].col -= 0x2000;
        }
        
        if (op->ct1 > 4) 
        {
            op->tv[0].col += 0xE0000000;
            op->tv[1].col += 0xE0000000;
            
            if (op->ct1 >= 12) 
            {
                op->flg          = 0;
                effect->dtbl.flg = 0;
            }
        }
        
        break;
    }
    
    if ((op->mode2 != 0) && (op->flg != 0)) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_Sub367(O_WRK* op) 
{
    EFFSUB367_WORK* effect; 
    NJS_POINT3* vtx_p;     
    float* g, *r;            
    int* rot, *rot_spd;          
    int i;               
  
    effect = (EFFSUB367_WORK*)op->exp0;
    
    vtx_p = effect->vtx_pos;
    
    g = effect->g;
    r = effect->r;
    
    rot     = effect->rot;
    rot_spd = effect->rot_spd;
    
    switch (op->mode0) 
    {                           
    case 0:
        op->tv[0].col = 0x80FFFFFF;
        
        op->sx = 1.0f;
        op->sy = 1.0f;
        
        op->ct0 = 3;
        op->ct1 = 0;
        op->ct2 = 0;
        op->ct3 = 0;
        
        op->mode0++;
    case 1:
        op->ct0++;
        op->ct1++;
        
        for (i = 0; i < effect->dtbl.num; i++)
        {
            *rot += *rot_spd;
            
            vtx_p->x = op->px - (*r   * njSin(*rot));
            vtx_p->z = op->pz - (*r++ * njCos(*rot));
            
            vtx_p->y += *g++ * op->ct0;
            
            rot_spd++;
            rot++;
            vtx_p++;
        }
        
        if (op->ct1 > 120) 
        {
            op->flg          = 0;
            effect->dtbl.flg = 0;
        }
        
        break;
    }
    
    if (op->flg != 0) 
    {
        sys->ef_fnc[sys->ef_fncn] = op;
        
        sys->ef_fncn++;
    }
}

// 100% matching!
void bhEff_Sub368(O_WRK* op)
{
    static UV_WORK uvinfo[1] =
	{
		0.0f, 160.0f,  48.0f,  56.0f    
	};
    
    switch (op->mode0) 
    {                             
    case 0:
        op->tv[0].col = 0x40808080;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->tv[0].x = -1.0f;
        op->tv[0].y = 0;
        op->tv[0].z = 0;
        
        op->tv[1].x = 1.0f;
        op->tv[1].y = 0;
        op->tv[1].z = 0;
        
        op->tv[2].x = -1.0f;
        op->tv[2].y = -2.0f;
        op->tv[2].z = 0;
        
        op->tv[3].x = 1.0f;
        op->tv[3].y = -2.0f;
        op->tv[3].z = 0;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4080000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 120;
        
        op->mode2 = 0;
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->mode2 = 1;
        op->mode0++;
    case 2:
        if (--op->ct2 < 0) 
        {
            op->mode0++;
        } 
        else 
        {
            op->syb *= 1.01f;
        }
    case 3:
        break;
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSprite(op, uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub369(O_WRK* op) 
{
    static UV_WORK uvinfo[15] = 
	{
		{   0.0f,   0.0f,  16.0f,  16.0f },
		{  16.0f,   0.0f,  24.0f,  24.0f },
		{  40.0f,   0.0f,  24.0f,  24.0f },
		{  64.0f,   0.0f,  32.0f,  32.0f },
		{  96.0f,   0.0f,  48.0f,  48.0f },
		{ 144.0f,   0.0f,  48.0f,  48.0f },
		{ 192.0f,   0.0f,  64.0f,  64.0f },
		{   0.0f,  64.0f,  64.0f,  64.0f },
		{  64.0f,  64.0f,  64.0f,  64.0f },
		{ 128.0f,  64.0f,  64.0f,  64.0f },
		{ 192.0f,  64.0f,  64.0f,  64.0f },
		{   0.0f, 128.0f,  64.0f,  64.0f },
		{  64.0f, 128.0f,  64.0f,  64.0f },
		{ 128.0f, 128.0f,  64.0f,  64.0f },
		{ 192.0f, 128.0f,  64.0f,  64.0f }
	};
	static EFF_ANIM anim00_tbl[12] =
	{
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 },
		{ 12,  1 }, { 13,  1 }, { 14,  1 }, { -1,  0 }
	};
	static EFF_ANIM anim01_tbl[16] =
	{
		{  0,  1 }, {  1,  1 }, {  2, 1  }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6, 1  }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10, 1  }, { 11,  1 },
		{ 12,  1 }, { 13,  1 }, { 14, 1  }, { -1,  0 }
	};
	static EFF_ANIM* anim_tbl[2] =
	{
		anim00_tbl, anim01_tbl
	};
    
    switch (op->mode0)
    {                              
    case 0:
        op->tv[0].col = 0x40808080;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 10;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->spd = 1.0f;
        
        op->mode1 = op->type;
        op->mode2 = 0;
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->mode2 = 1;
        op->mode0++;
    case 2:
        op->xn *= op->zn;
        
        op->px += op->xn * -njSin(op->ay);
        op->pz += op->xn * -njCos(op->ay);
        op->py += op->yn;
        break;
    }
    
    if (op->mode2 != 0)
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub370(O_WRK* op)
{
	static EFF370_WORK eff370_tbl[11] = 
	{
		{   0.5f,  0.98f, -0.02f, 371,   0,   4,   0 },
		{   0.3f,  0.98f, -0.02f, 371,   0,   4,   0 },
		{   0.2f,  0.98f, -0.04f, 371,   0,   4,   0 },
		{   0.7f,  0.98f, -0.04f, 371,   0,   4,   1 },
		{  0.25f,  0.98f, -0.03f, 371,   0,   2,   0 },
		{   0.5f,  0.98f, -0.03f, 371,   0,   3,   1 },
		{   0.7f,  0.98f, -0.04f, 373,   1,   4,   1 },
		{   0.5f,  0.98f, -0.03f, 373,   1,   3,   1 },
		{   0.5f,  0.97f, -0.05f, 373,   3,   2,   0 },
		{   0.0f,   0.0f,   0.0f,   0,   3,   3,   2 },
		{   0.5f,  0.98f, -0.03f, 373,   0,   3,   2 }
	};
    unsigned int bl_col[4] = 
	{
		0xFF5F0000, 0xFF400000, 0xFF400000, 0xFF400000
	};
    EFF370_WORK* eff_p;      
    NJS_VECTOR v1, v2;          
    int eno;                
	
    eff_p = eff370_tbl;
    
    eff_p += op->type;
    
    switch (op->mode0)
    {                             
    case 0:
        op->ani_ct = 0;
        op->ct2    = 0;
        
        op->mode1 = 0;
        
        op->mode0++;
    case 1:
        if (--op->ct0 < 0) 
        {
            sys->ef.id = eff_p->id;
            
            sys->ef.mdlver = eff_p->col;
            
            sys->ef.flg = 1;
            
            sys->ef.type = 0;
            
            sys->ef.sx = op->sx;
            sys->ef.sy = op->sy;
            sys->ef.sz = op->sz;
            
            if (op->mode1 == 5)
            {
                sys->ef.type = 3;
            }
            
            if (eff_p->id == 0) 
            {
                if ((op->flg & 0x80)) 
                {
                    bhSetEffParticle((BH_PWORK*)op->lkwkp, op->lkono, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&op->xn, bl_col[eff_p->col], 3);
                } 
                else
                {
                    bhSetEffParticle(NULL, -1, (NJS_POINT3*)&op->px, (NJS_POINT3*)&op->xn, bl_col[eff_p->col], 3);
                }
            } 
            else 
            {
                if ((op->flg & 0x80))
                {
                    if (eff_p->lmode != 0) 
                    {
                        njCalcPoint(&((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono].mtx, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&sys->ef.px);
                        
                        eno = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
                    } 
                    else 
                    {
                        sys->ef.px = 0;
                        sys->ef.py = 0;
                        sys->ef.pz = 0;
                        
                        eno = bhSetEffectTb(&sys->ef, (NJS_VECTOR* ) &op->lox, op->lkwkp, op->lkono);
                    }
                }
                else
                {
                    sys->ef.px = op->px;
                    sys->ef.py = op->py;
                    sys->ef.pz = op->pz;
                    
                    eno = bhSetEffectTb(&sys->ef, NULL, NULL, 0);
                }
                
                if (eno != -1) 
                {
                    if (sys->ef.id == 371) 
                    {
                        eff[eno].stflg |= 0x20; 
                        
                        eff[eno].txp[0] = op->txp[0];
                        eff[eno].tex_id = op->tex_id;
                    }
                    
                    if (eff_p->lmode == 1) 
                    {
                        v1.x = op->xn;
                        v1.y = op->yn;
                        v1.z = op->zn;
                        
                        njSetMatrix(NULL, &((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono].mtx);
                        
                        njCalcVector(NULL, &v1, &v2);
                    } 
                    else 
                    {
                        v2.x = op->xn;
                        v2.y = op->yn;
                        v2.z = op->zn;
                    }
                    
                    eff[eno].ct3 = 0;
                    
                    eff[eno].xn = v2.x;
                    eff[eno].yn = v2.y;
                    eff[eno].zn = v2.z;
                    
                    eff[eno].shp_ct = eff_p->spd + (0.3f * (-rand() / -2.1474836E9f));
                    
                    eff[eno].aox = eff_p->e;
                    eff[eno].aoy = eff_p->g;
                }
            }
            
            op->ct0 = eff_p->wcnt;
            
            op->ct2++;
            
            if (op->ct1 <= op->ct2)
            {
                op->flg = 0;
            }
        }
        
        break;
    }
}

// 100% matching!
void bhEff_Sub371(O_WRK* op)
{
    static UV_WORK uvinfo[32] = 
	{
		{  48.0f,  40.0f,   8.0f,   8.0f },
		{  48.0f,  48.0f,   8.0f,   8.0f },
		{  64.0f,  72.0f,  16.0f,  16.0f },
		{   0.0f,  64.0f,  24.0f,  24.0f },
		{   0.0f,  40.0f,  24.0f,  24.0f },
		{  24.0f,  40.0f,  24.0f,  24.0f },
		{  56.0f,  40.0f,  32.0f,  32.0f },
		{  88.0f,  40.0f,  32.0f,  32.0f },
		{   0.0f,   0.0f,  40.0f,  40.0f },
		{  40.0f,   0.0f,  40.0f,  40.0f },
		{  80.0f,   0.0f,  40.0f,  40.0f },
		{ 120.0f,   0.0f,  40.0f,  40.0f },
		{ 160.0f,   0.0f,  40.0f,  40.0f },
		{ 200.0f,   0.0f,  40.0f,  40.0f },
		{  48.0f,  56.0f,   8.0f,   8.0f },
		{  48.0f,  64.0f,   8.0f,   8.0f },
		{  48.0f,  72.0f,  16.0f,  16.0f },
		{  24.0f,  64.0f,  24.0f,  24.0f },
		{ 120.0f,  40.0f,  32.0f,  32.0f },
		{ 152.0f,  40.0f,  32.0f,  32.0f },
		{ 184.0f,  40.0f,  32.0f,  32.0f },
		{ 216.0f,  40.0f,  32.0f,  32.0f },
		{  88.0f,  72.0f,  16.0f,  16.0f },
		{ 104.0f,  72.0f,  16.0f,  16.0f },
		{ 120.0f,  72.0f,  16.0f,  16.0f },
		{ 136.0f,  72.0f,  16.0f,  16.0f },
		{ 152.0f,  72.0f,  16.0f,  16.0f },
		{ 168.0f,  72.0f,  16.0f,  16.0f },
		{ 184.0f,  72.0f,  16.0f,  16.0f },
		{ 200.0f,  72.0f,  16.0f,  16.0f },
		{ 216.0f,  72.0f,  16.0f,  16.0f },
		{ 232.0f,  72.0f,  16.0f,  16.0f }
	};
	static EFF_ANIM anim00_tbl[15] =
	{
		{  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 },
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 },
		{ 12,  1 }, { 13,  1 }, { -1,  0 }
	};
	static EFF_ANIM anim01_tbl[9] =
	{
		{ 14,  1 }, { 15,  1 }, { 16,  1 }, { 17,  1 },
		{ 18,  1 }, { 19,  1 }, { 20,  1 }, { 21,  1 },
		{ -1,  0 }
	};
	static EFF_ANIM anim02_tbl[11] =
	{
		{ 22,  1 }, { 23,  1 }, { 24,  1 }, { 25,  1 },
		{ 26,  1 }, { 27,  1 }, { 28,  1 }, { 29,  1 },
		{ 30,  1 }, { 31,  1 }, { -1,  0 }
	};
	static EFF_ANIM anim03_tbl[11] =
	{
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 },
		{ 12,  1 }, { 13,  1 }, { -1,  0}
	};
	static EFF_ANIM* anim_tbl[4] =
	{
		anim00_tbl, anim01_tbl, anim02_tbl, anim03_tbl
	};
    
    switch (op->mode0) 
    {                          
    case 0:
        op->tv[0].col = 0xA0C0C0C0;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg   |= 0x4180000;
        op->stflg |= 0x20;
        
        op->ani_ct = 0;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0; 
        
        op->spd = 1.0f;
        
        op->mode1 = op->type;
        op->mode2 = 0;
        
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct3 = 0;
        
        op->mode2 = 1;
        
        op->mode0++;
    case 2:
        op->shp_ct *= op->aox;
        
        if ((op->flg & 0x80))
        {
            op->lox += op->xn  * op->shp_ct;
            op->loy += (op->yn * op->shp_ct) + (op->aoy * op->ct3);
            op->loz += op->zn  * op->shp_ct;
        }
        else 
        {
            op->px += op->xn  * op->shp_ct;
            op->py += (op->yn * op->shp_ct) + (op->aoy * op->ct3);
            op->pz += op->zn  * op->shp_ct;
        }
        
        op->ct3++;
        
        if (op->type == 0) 
        {
            if (op->ct3 > 7) 
            {
                op->tv[0].col += 0xF8000000;
                op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
            }
        } 
        else if (op->ct3 > 6)
        {
            op->tv[0].col += 0xE0000000;
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        }
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub372(O_WRK* op)
{
    static UV_WORK uvinfo[13] = 
	{ 
		{   0.0f,  80.0f,  16.0f,  16.0f },
		{   0.0f, 128.0f,  16.0f,  16.0f },
		{   0.0f,  96.0f,  16.0f,  16.0f },
		{   0.0f, 112.0f,  16.0f,  16.0f },
		{  16.0f,  80.0f,  32.0f,  32.0f },
		{  48.0f,  80.0f,  32.0f,  32.0f },
		{  80.0f,  80.0f,  32.0f,  32.0f },
		{ 112.0f,  80.0f,  32.0f,  32.0f },
		{ 144.0f,  80.0f,  32.0f,  32.0f },
		{ 176.0f,  80.0f,  32.0f,  32.0f },
		{ 208.0f,  80.0f,  32.0f,  32.0f },
		{  16.0f, 112.0f,  32.0f,  32.0f },
		{  48.0f, 112.0f,  32.0f,  32.0f }
	};
	static EFF_ANIM anim00_tbl[9] =
	{
		{  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 },
		{  8,  1 }, {  9,  1 }, { 10,  1 }, { 12,  1 },
		{ -1,  0 }
	};
	static EFF_ANIM* anim_tbl[1] =
	{
		anim00_tbl
	};
    
    switch (op->mode0) 
    {                           
    case 0:
        op->tv[0].col = -1;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->tex_id = 5;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg |= 0x4180000;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        
        op->spd = 1.0f;
        
        op->ani_ct = op->mdlver;
        
        op->mode1 = 0;
        op->mode2 = 0;
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct3 = 0;
        
        op->mode2 = 1;
        op->mode0++;
    case 2:
        op->px += op->xn  * op->shp_ct;
        op->py += (op->yn * op->shp_ct) + (op->aoy * op->ct3);
        op->pz += op->zn  * op->shp_ct;
        
        op->ct3++;
        
        if (op->ct3 >= 5) 
        {
            op->tv[0].col += 0xD0000000;
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        }
        
        break;
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub373(O_WRK* op)
{
    static UV_WORK uvinfo[13] = 
	{
		{   0.0f,  80.0f,  16.0f,  16.0f },
		{   0.0f, 128.0f,  16.0f,  16.0f },
		{   0.0f,  96.0f,  16.0f,  16.0f },
		{   0.0f, 112.0f,  16.0f,  16.0f },
		{  16.0f,  80.0f,  32.0f,  32.0f },
		{  48.0f,  80.0f,  32.0f,  32.0f },
		{  80.0f,  80.0f,  32.0f,  32.0f },
		{ 112.0f,  80.0f,  32.0f,  32.0f },
		{ 144.0f,  80.0f,  32.0f,  32.0f },
		{ 176.0f,  80.0f,  32.0f,  32.0f },
		{ 208.0f,  80.0f,  32.0f,  32.0f },
		{  16.0f, 112.0f,  32.0f,  32.0f },
		{  48.0f, 112.0f,  32.0f,  32.0f }
	};
	static EFF_ANIM anim00_tbl[11] =
	{
		{  2,  1 }, {  3,  1 }, {  4,  1}, {  5,  1},
		{  6,  1 }, {  7,  1 }, {  8,  1}, {  9,  1},
		{ 10,  1 }, { 12,  1 }, { -1,  0}
	};
	static EFF_ANIM* anim_tbl[1] =
	{
		anim00_tbl
	};
    
    switch (op->mode0) 
    {                          
    case 0:
        op->tv[0].col = -1;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->tex_id = 5;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg |= 0x4180000;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0; 
        
        op->spd = 1.0f; 
        
        op->ani_ct = op->mdlver;
        
        op->mode1 = op->type;
        op->mode2 = 0;
        
        op->mode0++;
        break;
    case 1:
        op->ct3--;
        
        if (op->ct3 >= 0) 
        {
            break;
        }
        
        op->ct3 = 0;
        
        op->mode2 = 1;
        
        op->mode0++;
    case 2:
        op->shp_ct *= op->aox;
        
        if ((op->flg & 0x80)) 
        {
            op->lox += op->xn  * op->shp_ct;
            op->loy += (op->yn * op->shp_ct) + (op->aoy * op->ct3);
            op->loz += op->zn  * op->shp_ct;
        } 
        else 
        {
            op->px += op->xn  * op->shp_ct;
            op->py += (op->yn * op->shp_ct) + (op->aoy * op->ct3);
            op->pz += op->zn  * op->shp_ct;
        }
        
        op->ct3++;
        
        if (op->ct3 >= 5) 
        {
            op->tv[0].col += 0xE0000000;
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        }
    }
    
    if (op->mode2 != 0) 
    {
        bhEff_SetSpriteAnime(op, anim_tbl[op->mode1], uvinfo, 0);
    }
}

// 100% matching!
void bhEff_Sub398(O_WRK* op)
{
	static UV_WORK bl00_uvinfo[16] = 
	{
		{   0.0f,   0.0f,  24.0f,  24.0f },
		{  24.0f,   0.0f,  32.0f,  32.0f },
		{  56.0f,   0.0f,  40.0f,  40.0f },
		{  56.0f,  40.0f,  40.0f,  40.0f },
		{   8.0f,  32.0f,  48.0f,  48.0f },
		{  96.0f,   0.0f,  48.0f,  48.0f },
		{ 144.0f,   0.0f,  48.0f,  48.0f },
		{ 192.0f,   0.0f,  48.0f,  48.0f },
		{  96.0f,  48.0f,  48.0f,  48.0f },
		{ 144.0f,  48.0f,  48.0f,  48.0f },
		{ 192.0f,  48.0f,  48.0f,  48.0f },
		{   0.0f,  96.0f,  48.0f,  64.0f },
		{  48.0f,  96.0f,  48.0f,  64.0f },
		{  96.0f,  96.0f,  48.0f,  64.0f },
		{ 144.0f,  96.0f,  48.0f,  64.0f },
		{ 192.0f,  96.0f,  48.0f,  64.0f }
	};
	static UV_WORK bl01_uvinfo[7] = 
	{
		{   0.0f, 160.0f,  32.0f,  24.0f },
		{  32.0f, 160.0f,  32.0f,  24.0f },
		{  64.0f, 160.0f,  32.0f,  24.0f },
		{  96.0f, 160.0f,  32.0f,  24.0f },
		{ 128.0f, 160.0f,  32.0f,  24.0f },
		{ 160.0f, 160.0f,  32.0f,  24.0f },
		{ 192.0f, 160.0f,  32.0f,  24.0f }
	};
	static UV_WORK bl02_uvinfo[7] = 
	{
		{  0.0f,  184.0f,  24.0f,  24.0f },
		{  24.0f, 184.0f,  24.0f,  24.0f },
		{  48.0f, 184.0f,  24.0f,  24.0f },
		{  72.0f, 184.0f,  24.0f,  24.0f },
		{  96.0f, 184.0f,  24.0f,  24.0f },
		{ 120.0f, 184.0f,  24.0f,  24.0f },
		{ 144.0f, 184.0f,  24.0f,  24.0f }
	};
	static UV_WORK bl03_uvinfo[6] = 
	{
		{   0.0f, 208.0f,  16.0f,  16.0f },
		{  16.0f, 208.0f,  16.0f,  16.0f },
		{  32.0f, 208.0f,  16.0f,  16.0f },
		{  48.0f, 208.0f,  16.0f,  16.0f },
		{  64.0f, 208.0f,  16.0f,  16.0f },
		{  80.0f, 208.0f,  16.0f,  16.0f }
	};
	static UV_WORK bl04_uvinfo[6] = 
	{
		{ 224.0f, 160.0f,   8.0f,   8.0f },
		{ 224.0f, 168.0f,   8.0f,   8.0f },
		{ 224.0f, 176.0f,   8.0f,   8.0f },
		{ 232.0f, 160.0f,   8.0f,   8.0f },
		{ 232.0f, 168.0f,   8.0f,   8.0f },
		{ 232.0f, 176.0f,   8.0f,   8.0f }
	};
	static UV_WORK bl05_uvinfo[6] = 
	{
		{ 240.0f,  0.0f,   16.0f,  24.0f },
		{ 240.0f,  24.0f,  16.0f,  24.0f },
		{ 240.0f,  48.0f,  16.0f,  24.0f },
		{ 240.0f,  72.0f,  16.0f,  24.0f },
		{ 240.0f,  96.0f,  16.0f,  24.0f },
		{ 240.0f, 120.0f,  16.0f,  24.0f }
	};
	static UV_WORK bl06_uvinfo[6] = 
	{
		{   0.0f, 224.0f,  16.0f,  24.0f },
		{  16.0f, 224.0f,  16.0f,  24.0f },
		{  32.0f, 224.0f,  16.0f,  24.0f },
		{  48.0f, 224.0f,  16.0f,  24.0f },
		{  64.0f, 224.0f,  16.0f,  24.0f },
		{  80.0f, 224.0f,  16.0f,  24.0f }
	};
	static UV_WORK bl07_uvinfo[5] = 
	{
		{  96.0f, 208.0f,   8.0f,  16.0f },
		{ 104.0f, 208.0f,   8.0f,  16.0f },
		{ 112.0f, 208.0f,   8.0f,  16.0f },
		{ 120.0f, 208.0f,   8.0f,  16.0f },
		{ 128.0f, 208.0f,   8.0f,  16.0f }
	};
	static UV_WORK bl08_uvinfo[5] = 
	{
		{  96.0f, 224.0f,   8.0f,  16.0f },
		{ 104.0f, 224.0f,   8.0f,  16.0f },
		{ 112.0f, 224.0f,   8.0f,  16.0f },
		{ 120.0f, 224.0f,   8.0f,  16.0f },
		{ 128.0f, 224.0f,   8.0f,  16.0f }
	};
	static UV_WORK bl09_uvinfo[5] = 
	{
		{ 168.0f, 184.0f,  16.0f,  24.0f },
		{ 184.0f, 184.0f,  16.0f,  24.0f },
		{ 200.0f, 184.0f,  16.0f,  24.0f },
		{ 216.0f, 184.0f,  16.0f,  24.0f },
		{ 232.0f, 184.0f,  16.0f,  24.0f }
	};
	static UV_WORK bl10_uvinfo[5] =
	 {
		{  96.0f, 240.0f,   8.0f,  16.0f },
		{ 104.0f, 240.0f,   8.0f,  16.0f },
		{ 112.0f, 240.0f,   8.0f,  16.0f },
		{ 120.0f, 240.0f,   8.0f,  16.0f },
		{ 128.0f, 240.0f,   8.0f,  16.0f }
	};
	static UV_WORK* blood_uvinfo[11] = 
	{
		bl00_uvinfo, bl01_uvinfo, bl02_uvinfo, bl03_uvinfo,
		bl04_uvinfo, bl05_uvinfo, bl06_uvinfo, bl07_uvinfo,
		bl08_uvinfo, bl09_uvinfo, bl10_uvinfo
	};
	static EFF_ANIM blanim00_tbl[13] = 
	{
		{   1, 1 }, {   3, 1 }, {   5, 1 }, {   7, 1 },
		{   8, 2 }, {   9, 2 }, {  10, 2 }, {  11, 2 },
		{  12, 2 }, {  13, 2 }, {  14, 2 }, {  15, 2 },
		{  -1, 0 }
	};
	static EFF_ANIM blanim01_tbl[9] = 
	{
		{ 100, 3 }, {   0, 2 }, {   1, 2 }, {   2, 2 },
		{   3, 2 }, {   4, 2 }, {   5, 2 }, {   6, 2 },
		{  -1, 0 }
	};
	static EFF_ANIM blanim03_tbl[8] = 
	{
		{ 100, 4 }, {   0, 2 }, {   1, 2 }, {   2, 2 },
		{   3, 2 }, {   4, 2 }, {   5, 2 }, {  -1, 0 }
	};
	static EFF_ANIM blanim07_tbl[7] = 
	{
		{ 100, 8 }, {   0, 2 }, {   1, 2 }, {   2, 2 },
		{   3, 2 }, {   4, 2 }, {  -1, 0 }
	};
	EFF_ANIM* blanim_tbl[11] = 
	{  
		blanim00_tbl, blanim01_tbl, blanim01_tbl, blanim03_tbl,
		blanim03_tbl, blanim03_tbl, blanim03_tbl, blanim07_tbl,
		blanim07_tbl, blanim07_tbl, blanim07_tbl
	};
	float pos_tbl[11][2] = 
	{
		{   0.0f,   0.0f }, { -68.0f, -48.0f }, {  64.0f, -72.0f }, { -40.0f, -92.0f },
		{  40.0f,   8.0f }, { -48.0f,  12.0f }, {  60.0f, -12.0f }, {  16.0f,  36.0f },
		{ -32.0f,  12.0f }, { -64.0f, -24.0f }, {  -8.0f, -88.0f }
	};
    UV_WORK* uvp;   
    EFF_ANIM* bani; 
    NJS_POINT3 ps;  
    int i;          
    
    switch (op->mode0) 
    {                           
    case 0:
        op->tv[0].col = 0xF0FFFFFF;
        op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
        
        op->tv[0].x = -1.0f;
        op->tv[0].y = 0;
        op->tv[0].z = 0; 
        
        op->tv[1].x = 1.0f;
        op->tv[1].y = 0;
        op->tv[1].z = 0;
        
        op->tv[2].x = -1.0f;
        op->tv[2].y = -2.0f;
        op->tv[2].z = 0;
        
        op->tv[3].x = 1.0f;
        op->tv[3].y = -2.0f;
        op->tv[3].z = 0;
        
        op->bl_src = 8;
        op->bl_dst = 3;
        
        op->flg |= 0x4080000;
        
        op->tex_id = 30;
        
        op->xn = op->px;
        op->yn = op->py;
        op->zn = op->pz;
        
        op->ct0 = 0;
        op->ct1 = 0;
        op->ct2 = 0;
        op->ct3 = 0;
        
        op->sxb = 5.0f;
        op->syb = 5.0f;
        op->szb = 5.0f;
        
        op->mode0++;
        
        if (op->type == 0) 
        {
            for (i = 1; i < 11; i++) 
            {
                sys->ef.id   = 398;
                sys->ef.type = i;
                
                sys->ef.flg = 1;
                
                sys->ef.px = op->px - ((0.01f * i) * njSin(op->ay));
                sys->ef.py = op->py;
                sys->ef.pz = op->pz - ((0.01f * i) * njCos(op->ay));
                
                sys->ef.sx = op->sxb;
                sys->ef.sy = op->syb;
                sys->ef.sz = op->szb;
                
                sys->ef.ax = op->ax;
                sys->ef.ay = op->ay;
                
                bhSetEffectTb(&sys->ef, NULL, NULL, 0);
            } 
        }
        
        break;
    case 1:
        break;
    }
    
    bani = blanim_tbl[op->type];
    bani += op->ct1;
    
    if (bani->ptrn == -1) 
    {
        op->tv[0].col += 0xFF000000;
        
        if ((op->tv[0].col & 0xFF000000) < 0x1000000) 
        {
            op->flg = 0;
        } 
        else
        {
            op->sy *= 1.005f;
            
            op->tv[3].col = op->tv[2].col = op->tv[1].col = op->tv[0].col;
            
            if (op->flg != 0) 
            {
                sys->ef_trs[sys->ef_trsn] = op;
                
                sys->ef_trsn++;
            }
        }
    }
    else if (bani->ptrn < 100) 
    {
        uvp = blood_uvinfo[op->type];
        uvp += bani->ptrn;
        
        op->sx = op->sxb * (uvp->xs / 64.0f);
        op->sy = op->syb * (uvp->ys / 64.0f);
        
        njUnitMatrix(NULL);
        
        njTranslate(NULL, op->xn, op->yn, op->zn);
        njRotateXYZ(NULL, op->ax, op->ay, op->az);
        
        ps.x = op->sxb * (pos_tbl[op->type][0] / 64.0f);
        ps.y = op->sxb * (pos_tbl[op->type][1] / 64.0f);
        ps.z = 0;
        
        njCalcPoint(NULL, &ps, (NJS_POINT3*)&op->px);
        
        op->tv[0].u = uvp->u / 255.0f;
        op->tv[0].v = uvp->v / 255.0f;
        
        op->tv[3].u = ((uvp->u + uvp->xs) - 1.0f) / 255.0f;
        op->tv[3].v = ((uvp->v + uvp->ys) - 1.0f) / 255.0f;
        
        op->tv[1].u = op->tv[3].u;
        op->tv[1].v = op->tv[0].v;
        
        op->tv[2].u = op->tv[0].u;
        op->tv[2].v = op->tv[3].v;
        
        if (op->flg != 0) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }
    }
    
    if (bani->ptrn != -1) 
    {
        op->ct0++;
        
        if (bani->timer == op->ct0)
        {
            op->ct0 = 0;
            
            op->ct1++;
        }
    }
}

// 100% matching!
static O_WRK* bhEff_AllocOwork()
{
    O_WRK* op;
    int i;

    op = eff;
    
    for (i = 0; i < 512; i++, op++) 
    {
        if (!(op->flg & 0x3))
        {
            npSetMemory((unsigned char*)op, sizeof(*op), 0);
            
            op->flg = 0x2;
            
            return op;
        }
    }
    
    return NULL;
}

// 100% matching!
static void bhEff_PtclSpriteDraw(O_WRK* op)
{
    D_WORK* dtbl;      
    NJS_POINT3* vtx_p; 
    float* size;      
    char atr[192];    
    int i;             
    
    dtbl = (D_WORK*)op->exp0;
    
    njGetSystemAttr((NJS_SYS_ATTR*)&atr);
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 10);
    
    njTextureFilterMode(0);
    
    njSetMatrix(NULL, cam.mtx);
    njSetTexture(op->txp[0]);
    
    vtx_p = (NJS_POINT3*)dtbl->exp0;
    size  = (float*)dtbl->exp1;
    
    njPtclSpriteStart(op->tex_id, op->tv->col, 1);
    
    for (i = 0; i < dtbl->num; i++) 
    {
        njPtclDrawSprite(vtx_p, 1, *size, *size);
        
        vtx_p++; 
        size++;
    }
    
    njPtclSpriteEnd();
    
    njSetSystemAttr((NJS_SYS_ATTR*)&atr);
}

// 100% matching!
static void bhEff_LineDraw(O_WRK* op) 
{
    D_WORK* dtbl;           
    NJS_POINT3* vtx_p, *vtx_p2;       
    char atr[192];          
    int i;                 
    NJS_POLYGON_VTX poly[3]; 

    dtbl = (D_WORK*)op->exp0;
    
    njGetSystemAttr((NJS_SYS_ATTR*)&atr);
    
    njColorBlendingMode(0, op->bl_src);
    njColorBlendingMode(1, op->bl_dst);
    
    njFogDisable();
    
    njSetMatrix(NULL, cam.mtx);
    
    vtx_p  = (NJS_POINT3*)dtbl->exp0;
    vtx_p2 = (NJS_POINT3*)dtbl->exp1;
    
    poly[0].col = op->tv[0].col;
    poly[1].col = op->tv[0].col;
    poly[2].col = op->tv[1].col;
    
    for (i = 0; i < dtbl->num; i++) 
    {
        poly[0].x = vtx_p->x;
        poly[0].y = vtx_p->y;
        poly[0].z = vtx_p->z;
        
        poly[2].x = vtx_p2->x;
        poly[2].y = vtx_p2->y;
        poly[2].z = vtx_p2->z;
        
        poly[1].x = 0.2f + vtx_p->x;
        poly[1].y = 0.3f + vtx_p->y;
        poly[1].z = 0.2f + vtx_p->z;
        
        njDrawPolygon3DEx(poly, 3, 1);
        
        vtx_p++; 
        vtx_p2++; 
    }
    
    njFogEnable();
    
    njSetSystemAttr((NJS_SYS_ATTR*)&atr);
}

// 100% matching!
static void bhEff_SetSprite(O_WRK* op, UV_WORK* uvp, int mode)
{
    op->sx = op->sxb * (uvp->xs / 100.0f);
    op->sy = op->syb * (uvp->ys / 100.0f);
    
    op->tv[0].u = uvp->u / 255.0f;
    op->tv[0].v = uvp->v / 255.0f;
    
    op->tv[3].u = ((uvp->u + uvp->xs) - 1.0f) / 255.0f;
    op->tv[3].v = ((uvp->v + uvp->ys) - 1.0f) / 255.0f;
    
    op->tv[1].u = op->tv[3].u;
    op->tv[1].v = op->tv[0].v;
    
    op->tv[2].u = op->tv[0].u;
    op->tv[2].v = op->tv[3].v;
    
    if (op->flg != 0) 
    {
        if (mode == 0) 
        {
            sys->ef_trs[sys->ef_trsn] = op;
            
            sys->ef_trsn++;
        }
        else 
        {
            sys->ef_fnc[sys->ef_fncn] = op;
            
            sys->ef_fncn++;
        }
    }
}

// 99.79% matching (matches on NGC)
static void bhEff_SetSpriteAnime(O_WRK* op, EFF_ANIM* eff_anim, UV_WORK* uvp, int mode)
{
    EFF_ANIM* anim_p;
    
    anim_p = &eff_anim[op->ct1];
    
    if (anim_p->ptrn == -1)
    {
        op->flg = 0;
        return;
    }
    
    if (anim_p->ptrn == -2)
    {
        op->ct1 = anim_p->timer;
        
        anim_p = &eff_anim[op->ct1];
    }
    
    op->ct0++;
    
    if (anim_p->timer == op->ct0) 
    {
        op->ct0 = 0;
        
        if (op->spd != 0) 
        {
            op->ct1++;
        }
    }
    
    if (anim_p->ptrn < 100)
    {
        bhEff_SetSprite(op, &uvp[anim_p->ptrn], mode);
    }
}

// 100% matching!
static void bhEff_3DSpriteDraw(O_WRK* op)
{
    unsigned int tnm;
    
    njPushMatrixEx();
    
    if (((op->flg == 0) || ((op->flg & 0x1000000)) || ((op->stflg & 0x1000000))) || ((sys->gm_flg & 0x4000)) && ((op->mdflg & 0x40)))
    {
        njPopMatrixEx();
        return;
    }
    
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
        
        tnm = sys->ef_tn[op->tex_id] + op->ani_ct;
        
        njSetTextureNum(tnm);
    } 
    else 
    {
        njSetTexture(op->txp[0]);
        
        tnm = op->tex_id + op->ani_ct;
        
        njSetTextureNum(tnm);
    }
    
    if ((op->flg & 0x10000000)) 
    {
        njSetPaletteBankNum(tnm, op->bank);
    }
    
    njPushMatrixEx();
    
    if ((!(op->flg & 0x80)) || ((op->flg & 0x200000)))
    {
        njUnitMatrix(NULL);
        
        njTranslateEx((NJS_VECTOR*)&op->px);
        njRotateEx(&op->ax, 0);
        
        njGetMatrix(op->mtx);
    }
    else 
    {
        njSetMatrix(op->mtx, &((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono].mtx);
        
        njTranslate(op->mtx, op->lox, op->loy, op->loz);
        njRotateXYZ(op->mtx, op->ax,  op->ay,  op->az);
    }
    
    njPopMatrixEx();
    
    njMultiMatrix(NULL, op->mtx);
    
    if ((op->flg & 0x100000)) 
    {
        njUnitRotPortion(NULL);
        
        njRotateZ(NULL, op->az);
    }
    
    njScaleEx((NJS_VECTOR*)&op->sx);
    
    njColorBlendingMode(0, op->bl_src);
    njColorBlendingMode(1, op->bl_dst);
    
    njFogDisable();
    
    njDrawTexture3DEx(op->tvp, op->pn, 1);
    
    njFogEnable();
    
    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 6);
    
    njPopMatrixEx();
}

// 100% matching!
static void bhEff_ModelDraw(O_WRK* op)
{
    NJS_TEXNAME* tnp;
    ML_WORK* mlp;
    
    njCnkSetSimpleLightColor(1.0f, 1.0f, 1.0f);
    njCnkSetSimpleLightIntensity(1.0f, 0.8f);
    njCnkSetSimpleLight(0, 0.3f, 0.7f);
    
    njPushMatrixEx();
    
    op->flg = op->flg;
    
    if ((((op->flg & 0x1000000)) || ((op->stflg & 0x1000000))) || (((sys->gm_flg & 0x4000)) && ((op->mdflg & 0x40)))) 
    {
        njPopMatrixEx();
        return;
    }
    
    if ((op->flg & 0x20000000)) 
    {
        njTextureFilterMode(0);
    }
    else 
    {
        njTextureFilterMode(1);
    }
    
    mlp = op->mlwP;
    
    if (!(op->stflg & 0x20)) 
    {
        tnp = sys->ef_tlist.textures;
        
        sys->ef_tlist.textures = &tnp[op->tex_id + op->ani_ct];
        
        njSetTexture(&sys->ef_tlist);
        njSetTextureNum(0);
    } 
    else 
    {
        njSetTexture(op->txp[0]);
        njSetTextureNum(0);
    }
    
    njPushMatrixEx();
    
    if (((op->flg & 0x200000)) || (!(op->flg & 0x80))) 
    {
        njUnitMatrix(NULL);
        
        njTranslateEx((NJS_VECTOR*)&op->px);
        njRotateEx(&op->ax, 0);
        
        njGetMatrix(op->mtx);
    } 
    else 
    {
        njSetMatrix(op->mtx, &((O_WRK*)op->lkwkp)->mlwP->owP[op->lkono].mtx);
        
        njTranslate(op->mtx, op->lox, op->loy, op->loz);
        njRotateXYZ(op->mtx, op->ax,  op->ay,  op->az);
    }
    
    njPopMatrixEx();
    
    njMultiMatrix(NULL, op->mtx);
    
    njScaleEx((NJS_VECTOR*)&op->sx);
    
    if (op->flr_no != 0) 
    {
        npSetAllMatColor(&mlp->objP[op->mdlver], 1, op->tv->col);
    }
    
    njCnkSimpleMultiDrawModel(mlp->objP[op->mdlver].model);
    
    if (!(op->stflg & 0x20)) 
    {
        sys->ef_tlist.textures = tnp;
    }
            
    njPopMatrixEx();
}
