#ifndef __ANDROID__
#define RECVX_DIAG 1   /* TEMP: PC build with the Quest diagnostics (comparison run) */
#endif
#include "../../../ps2/veronica/prog/MdlPut.h"
#include "../../../ps2/veronica/prog/njplus.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaTextureFunction.h"
#include "../../../ps2/veronica/prog/ps2_NinjaCnk.h"
#include "../../../ps2/veronica/prog/main.h"

void (*bhJumpCnkDraw[4])(BH_PWORK* ewP, NJS_CNK_OBJECT* objP, O_WORK* owP, int obj_num) = 
{ 
	EasyMultiDrawTreeCnk, 
	SimpleMultiDrawTreeCnk, 
	EasyDrawTreeCnk, 
	SimpleDrawTreeCnk 
};

// 100% matching! 
#ifdef TARGET_PC
extern int pose_trace_mark;
void pose_trace(BH_PWORK* p);
#endif
#if defined(__ANDROID__) || defined(RECVX_DIAG)
#include <time.h>
#include <stdio.h>
/* Quest diagnosis: which way a character is put - the conditions that decide whether the skin runs (and which one) */
static void and_put_log(BH_PWORK* ewP, ML_WORK* mlwP, int draw_tp)
{
    static struct { const void* key; long sec; } seen[48];
    static int nseen;
    struct timespec ts;
    int slot = -1;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int i = 0; i < nseen; i++) if (seen[i].key == (const void*)ewP) { slot = i; break; }
    if (slot < 0) { if (nseen == 48) return; slot = nseen++; seen[slot].key = ewP; seen[slot].sec = -100; }
    if (ts.tv_sec - seen[slot].sec < 3) return;
    seen[slot].sec = ts.tv_sec;
    printf("android: put model id %u type %u flg %x: mlwP->flg %x (skin bit %d), skp[%d] %p, mdflg %x (FM %d), obj_num %d, draw_tp %d\n",
           (unsigned)ewP->id, (unsigned)ewP->type, (unsigned)ewP->flg, (unsigned)mlwP->flg, (mlwP->flg & 0x80) != 0, (int)ewP->mdl_no,
           (const void*)ewP->skp[ewP->mdl_no], (unsigned)ewP->mdflg, (ewP->mdflg & 0x100) != 0, (int)mlwP->obj_num, draw_tp);
}
#else
#define and_put_log(ewP, mlwP, draw_tp) ((void)0)
#endif
void bhPutModel(BH_PWORK* ewP) 
{
    // these variables are arranged in an order different from the debugging symbols
    O_WORK* owP;       
    int draw_tp;       
    ML_WORK* mlwP;     
    unsigned int c3df; 
    
#ifdef TARGET_PC
    if (pose_trace_mark)
    {
        pose_trace(ewP);   /* test aid RECVX_POSE_TRACE (pc/glue/pose_trace.c) */
    }
#endif
    mlwP = ewP->mlwP;
    owP = mlwP->owP;
    
    if (mlwP->texP != NULL) 
    {
        njSetTexture(mlwP->texP);
    }
    
    draw_tp = ewP->draw_tp;
    
    if (((sys->gm_flg & 0x80)) && (draw_tp < 2)) 
    {
        draw_tp += 2;
    }
    
    and_put_log(ewP, mlwP, draw_tp);
    if ((mlwP->flg & 0x80))
    {
        if (ewP->skp[ewP->mdl_no] != NULL) 
        {
            npPushMdlstr(mlwP->objP, mlwP->obj_num);
            
            if ((ewP->mdflg & 0x100))
            {
                npCalcSkinFM(ewP, mlwP->obj_num, ewP->skp[ewP->mdl_no]);
            } 
            else 
            {
                npCalcSkin(ewP, mlwP->obj_num, ewP->skp[ewP->mdl_no]);
            }
            
            bhJumpCnkDraw[draw_tp](ewP, mlwP->objP, owP, mlwP->obj_num);
            
            if (((sys->st_flg & 0x100)) && (!(ewP->mdflg & 0x40))) 
            {
                njSetMatrix(cam.mtxb, cam.mtx);
                
                njMirror(cam.mtx, &sys->mr_pl);
                
                njSetMatrix(NULL, cam.mtx);
                
                njCnkSetEasyMultiLightMatrices();
                njCnkSetSimpleMultiLightMatrices();
                
                c3df = _nj_control_3d_flag_;
                
                njControl3D(c3df | 0x10000);
                
                bhJumpCnkDraw[draw_tp](ewP, mlwP->objP, owP, mlwP->obj_num);
                
                njControl3D(c3df);
                
                njSetMatrix(cam.mtx, cam.mtxb);
                njSetMatrix(NULL, cam.mtx);
                
                njCnkSetEasyMultiLightMatrices();
                njCnkSetSimpleMultiLightMatrices();
            }
            
            npPopMdlstr(mlwP->objP, mlwP->obj_num);
        }
        else 
        {
            bhJumpCnkDraw[draw_tp](ewP, mlwP->objP, owP, mlwP->obj_num);
            
            if (((sys->st_flg & 0x100)) && (!(ewP->mdflg & 0x40))) 
            {
                njSetMatrix(cam.mtxb, cam.mtx);
                
                njMirror(cam.mtx, &sys->mr_pl);
                
                njSetMatrix(NULL, cam.mtx);
                
                njCnkSetEasyMultiLightMatrices();
                njCnkSetSimpleMultiLightMatrices();

                c3df = _nj_control_3d_flag_;
                
                njControl3D(c3df | 0x10000);
                
                bhJumpCnkDraw[draw_tp](ewP, mlwP->objP, owP, mlwP->obj_num);
                
                njControl3D(c3df);
                
                njSetMatrix(cam.mtx, cam.mtxb);
                njSetMatrix(NULL, cam.mtx);
                
                njCnkSetEasyMultiLightMatrices();
                njCnkSetSimpleMultiLightMatrices();
            }
        }
    }
    else 
    {
        DrawTreeBsc(mlwP->objP, owP, mlwP->obj_num);
    }
}

// 100% matching!
static void DrawTreeBsc(NJS_CNK_OBJECT* objP, O_WORK* owP, int obj_num)
{
    for ( ; obj_num > 0; obj_num--, objP++, owP++) 
    {
        njPushMatrix(cam.mtx);
        njMultiMatrix(NULL, &owP->mtx);
        
        if ((objP->model != NULL) && (!(objP->evalflags & 0x8))) 
        {
            njDrawModel((NJS_MODEL*)objP->model);
        }
        
        njPopMatrixEx();
    } 
}

#if defined(__ANDROID__) || defined(RECVX_DIAG)
#include <time.h>
#include <stdio.h>
extern int g_and_trace;
/* Quest diagnosis (a man's arms missing in the first cell): per character, every 3 s, the state of each node the draw loop
   walks - evalflags (bit 8: hidden), and whether the node has a model, a vertex list and a polygon list */
static void and_tree_log(const char* fn, BH_PWORK* ewP, NJS_CNK_OBJECT* objP, int obj_num)
{
    static struct { const void* key; long sec; } seen[48];
    static int nseen;
    struct timespec ts;
    char buf[640];
    int o = 0, slot = -1;
    g_and_trace = 0;
    if (obj_num < 6) return;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int i = 0; i < nseen; i++) if (seen[i].key == (const void*)objP) { slot = i; break; }
    if (slot < 0) { if (nseen == 48) return; slot = nseen++; seen[slot].key = objP; seen[slot].sec = -100; }
    if (ts.tv_sec - seen[slot].sec < 3) return;
    seen[slot].sec = ts.tv_sec;
    g_and_trace = (obj_num >= 11);   /* the characters (the 3 objects of the first cell) */
    for (int i = 0; i < obj_num && i < 40 && o < (int)sizeof(buf) - 24; i++) {
        const NJS_CNK_OBJECT* n = &objP[i];
        o += snprintf(buf + o, sizeof(buf) - (size_t)o, " %d:%x%c%c%c", i, (unsigned)n->evalflags,
                      !n->model ? 'N' : 'm', (n->model && n->model->vlist) ? 'v' : '-', (n->model && n->model->plist) ? 'p' : '-');
    }
    printf("android: tree %s %p id %u type %u mdlver %u flg %x at %.0f %.0f %.0f, %d nodes:%s\n", fn, (const void*)objP, (unsigned)ewP->id, (unsigned)ewP->type,
           (unsigned)ewP->mdlver, (unsigned)ewP->flg, ewP->px, ewP->py, ewP->pz, obj_num, buf);
}
/* RECVX_SHOW_HIDDEN=1 (env.txt): nodes with the hide bit (evalflags & 8) are drawn too */
static int and_showhidden(void)
{
    static int v = -1;
    if (v < 0) { v = getenv("RECVX_SHOW_HIDDEN") != NULL; printf("android: show hidden nodes %s\n", v ? "ON" : "off"); }
    return v;
}
#else
#define and_tree_log(fn, ewP, objP, obj_num) ((void)0)
#define and_showhidden() 0
#endif

// 100% matching!
static void EasyMultiDrawTreeCnk(BH_PWORK* ewP, NJS_CNK_OBJECT* objP, O_WORK* owP, int obj_num)
{
    and_tree_log("EasyMultiDrawTreeCnk", ewP, objP, obj_num);
    for ( ; obj_num > 0; obj_num--, objP++, owP++) 
    {
        njPushMatrix(cam.mtx);
        
        if (!(objP->evalflags & 0x80000000))
        {
            njMultiMatrix(NULL, &owP->mtx);
        } 
        else 
        {
            njTranslate(NULL, ewP->px, ewP->py, ewP->pz);
        }
        
        if ((objP->model != NULL) && (!(objP->evalflags & 0x8) || and_showhidden()))
        {
            njCnkEasyMultiDrawModel(objP->model);
        }

        njPopMatrixEx();
    }
#if defined(__ANDROID__) || defined(RECVX_DIAG)
    g_and_trace = 0;
#endif
}

// 100% matching! 
static void SimpleMultiDrawTreeCnk(BH_PWORK* ewP, NJS_CNK_OBJECT* objP, O_WORK* owP, int obj_num)
{
    and_tree_log("SimpleMultiDrawTreeCnk", ewP, objP, obj_num);
    for ( ; obj_num > 0; obj_num--, objP++, owP++) 
    {
        njPushMatrix(cam.mtx);
        
        if (!(objP->evalflags & 0x80000000))
        {
            njMultiMatrix(NULL, &owP->mtx);
        } 
        else 
        {
            njTranslate(NULL, ewP->px, ewP->py, ewP->pz);
        }
        
        if ((objP->model != NULL) && (!(objP->evalflags & 0x8)))
        {
            njCnkSimpleMultiDrawModel(objP->model);
        }
        
        njPopMatrixEx();
    } 
}

// 100% matching! 
static void EasyDrawTreeCnk(BH_PWORK* ewP, NJS_CNK_OBJECT* objP, O_WORK* owP, int obj_num)
{
    and_tree_log("EasyDrawTreeCnk", ewP, objP, obj_num);
    for ( ; obj_num > 0; obj_num--, objP++, owP++) 
    {
        njPushMatrix(cam.mtx);
        
        if (!(objP->evalflags & 0x80000000))
        {
            njMultiMatrix(NULL, &owP->mtx);
        } 
        else 
        {
            njTranslate(NULL, ewP->px, ewP->py, ewP->pz);
        }
        
        if ((objP->model != NULL) && (!(objP->evalflags & 0x8)))
        {
            njCnkEasyDrawModel(objP->model);
        }
        
        njPopMatrixEx();
    } 
}

// 100% matching! 
static void SimpleDrawTreeCnk(BH_PWORK* ewP, NJS_CNK_OBJECT* objP, O_WORK* owP, int obj_num)
{
    and_tree_log("SimpleDrawTreeCnk", ewP, objP, obj_num);
    for ( ; obj_num > 0; obj_num--, objP++, owP++) 
    {
        njPushMatrix(cam.mtx);
        
        if (!(objP->evalflags & 0x80000000))
        {
            njMultiMatrix(NULL, &owP->mtx);
        } 
        else 
        {
            njTranslate(NULL, ewP->px, ewP->py, ewP->pz);
        }
        
        if ((objP->model != NULL) && (!(objP->evalflags & 0x8)))
        {
            njCnkSimpleDrawModel(objP->model);
        }
        
        njPopMatrixEx();
    } 
}

// 100% matching! 
void bhCalcModel(BH_PWORK* ewP) 
{
    if ((ewP->flg & 0x80))
    {
        njSetMatrix(ewP->mtx, &((BH_PWORK*)ewP->lkwkp)->mlwP->owP[ewP->lkono].mtx);
        
        njTranslate(ewP->mtx, ewP->lox, ewP->loy, ewP->loz);
        
        if ((ewP->flg & 0x1000)) 
        {
            njUnitRotPortion(ewP->mtx);
        }
        
        njRotateXYZ(ewP->mtx, ewP->ax, ewP->ay, ewP->az);
    }
    else if (!(ewP->flg & 0x1000))
    {
        njUnitMatrix(ewP->mtx);
        
        njTranslate(ewP->mtx, ewP->px, ewP->py, ewP->pz);
        
        njRotateXYZ(ewP->mtx, ewP->ax, ewP->ay, ewP->az);
    }
    
    bhCalcTree(ewP->mtx, ewP->mlwP);
}

// 100% matching!
void bhCalcTree(NJS_MATRIX* basP, ML_WORK* mlwP) 
{
    O_WORK* owP;
    NJS_CNK_OBJECT* objP;
    int obj_num;

    owP = mlwP->owP;
    objP = mlwP->objP;

    njPushMatrix(basP);

    if (basP == NULL)
    {
        njUnitMatrix(NULL);
    }

    obj_num = mlwP->obj_num;

    for ( ; obj_num > 0; obj_num--, objP++, owP++)
    {
        if (objP->sibling != NULL)
        {
            njPushMatrixEx();
        }

        njTranslate(NULL, objP->pos[0], objP->pos[1], objP->pos[2]);
        njRotateXYZ(NULL, objP->ang[0], objP->ang[1], objP->ang[2]);

        if (!(owP->flg & 0x1)) 
        {
            njGetMatrix(&owP->mtx);
        }
        else 
        {
            njSetMatrix(NULL, &owP->mtx);
        }

        if (objP->child == NULL)
        {
            njPopMatrixEx();
        }
    }

    njPopMatrixEx();
}
