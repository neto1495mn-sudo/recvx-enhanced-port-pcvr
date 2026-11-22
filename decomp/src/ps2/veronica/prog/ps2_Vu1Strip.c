#ifdef TARGET_PC
#include "ee_asm.h"
#endif
#include "../../../ps2/veronica/prog/ps2_Vu1Strip.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaView.h"
#include "../../../ps2/veronica/prog/ps2_dummy.h"
#include "../../../ps2/veronica/prog/main.h"

/* rendering rework: with the native 3D path the strips that end in Ps2AddPrim3DEx are clipped
   at draw time (pc/glue/native_glue.c pc_native_prim3d), so the CPU scissor is skipped, and a
   triangle skipped for facing away is marked apart from one skipped for clipping */
#ifdef TARGET_PC
#define PC_SCISSOR (!pc_native_3d())
#define PC_KICK_CULL (pc_native_3d() ? 32769 : 32768)
#define PC_ADDPRIM3DEX(t, d, n) (pc_native_3d() ? Ps2AddPrim3DExNative(t, d, n, 0) : Ps2AddPrim3DEx(t, d, n))
#define PC_ADDPRIM3DEX1P(t, d, n) (pc_native_3d() ? Ps2AddPrim3DExNative(t, d, n, 1) : Ps2AddPrim3DEx1P(t, d, n))
#define PC_ADDPRIM3DEXI(t, d, n) (pc_native_3d() ? Ps2AddPrim3DExNative(t, d, n, 2) : Ps2AddPrim3DExI(t, d, n))
#else
#define PC_SCISSOR 1
#define PC_KICK_CULL 32768
#define PC_ADDPRIM3DEX Ps2AddPrim3DEx
#define PC_ADDPRIM3DEX1P Ps2AddPrim3DEx1P
#define PC_ADDPRIM3DEXI Ps2AddPrim3DExI
#endif

static SCISSOR_PLANE planeset;
static SCISSOR scissorflip;
static NODE node;
float fVu1Projection;
float fVu1NearClip;
float fVu1InvNearClip;
float fVu1FarClip;
float fVu1OffsetX;
float fVu1OffsetY;
float fVu1AspectW;
float fVu1AspectH;
VU1_PRIM_BUF vu1ScessorBuf[16];
void (*pColorCalcFuncTbl[11])(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim) = 
{
	vu1GetVertexColor,
	vu1GetVertexColorCM,
	vu1GetVertexColorIgnore,
	vu1GetVertexColorDif,
	vu1GetVertexColorDifAmb,
	vu1GetVertexColorDifSpe1,
	vu1GetVertexColorDifSpe2,
	vu1GetVertexColorDifSpe3,
	vu1GetVertexColorDifSpe1Amb,
	vu1GetVertexColorDifSpe2Amb,
	vu1GetVertexColorDifSpe3Amb
};
VU1_COLOR vu1Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
VU1_COLOR vu1Specula = { 1.0f, 1.0f, 1.0f, 1.0f };
VU1_COLOR vu1Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
float fVu1AlphaRatio = 128.0f;

extern void VU0_CLIP_VIEW_VOLUME_ALL() __attribute__((section(".vutext")));
extern void VU0_CALC_COLOR() __attribute__((section(".vutext")));
extern void VU0_INIT_CALC_COLOR() __attribute__((section(".vutext")));

// 100% matching!
void vu1SetScreenProjection(float fProjection)
{
    fVu1Projection = fProjection;
    
#ifndef TARGET_PC
    asm volatile 
    {

        mfc1     t0, $f12
        nop
        
        qmtc2    t0, $vf4

        vaddx.z  $vf23, $vf0, $vf4x
        
    }
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        R8.sd[0] = (int32_t)(ee_f2u((float)(fProjection)));
        vu_load_vf(4, &R8);
        vu_upper(U_ADD, 2, 23, 0, 4, BC_X);
    }
#endif
    
    _Make_ClipMatrix(ClipMatrix2, fVu1Projection, _fNaViwClipNear, _fNaViwClipFar);
}

// 100% matching!
void vu1SetNearFarClip(float fNear, float fFar)
{
    fVu1NearClip = fNear;
    fVu1FarClip = fFar;
    
    fVu1InvNearClip = 1.0f / fVu1NearClip;

#ifndef TARGET_PC
    asm volatile 
    {
        
        mfc1     t0, $f12
        mfc1     t1, $f13

        qmtc2    t0, $vf4
        qmtc2    t1, $vf5

        vaddx.x  $vf23x, $vf0x, $vf4x
        vaddx.y  $vf23y, $vf0y, $vf5x
        
    }
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        R8.sd[0] = (int32_t)(ee_f2u((float)(fNear)));
        R9.sd[0] = (int32_t)(ee_f2u((float)(fFar)));
        vu_load_vf(4, &R8);
        vu_load_vf(5, &R9);
        vu_upper(U_ADD, 8, 23, 0, 4, BC_X);
        vu_upper(U_ADD, 4, 23, 0, 5, BC_X);
    }
#endif
    
    _Make_ClipMatrix(ClipMatrix2, fVu1Projection, _fNaViwClipNear, _fNaViwClipFar);
}

// 100% matching!
void vu1SetScreenOffset(float fOffsetX, float fOffsetY)
{
    fVu1OffsetX = fOffsetX;
    fVu1OffsetY = fOffsetY;
    
#ifndef TARGET_PC
    asm volatile 
    {
        
        mfc1     t0, $f12
        mfc1     t1, $f13

        qmtc2    t0, $vf4
        qmtc2    t1, $vf5

        vaddx.x  $vf16, $vf0, $vf4x
        vaddx.y  $vf16, $vf0, $vf5x
        
    }
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        R8.sd[0] = (int32_t)(ee_f2u((float)(fOffsetX)));
        R9.sd[0] = (int32_t)(ee_f2u((float)(fOffsetY)));
        vu_load_vf(4, &R8);
        vu_load_vf(5, &R9);
        vu_upper(U_ADD, 8, 16, 0, 4, BC_X);
        vu_upper(U_ADD, 4, 16, 0, 5, BC_X);
    }
#endif
}

// 100% matching!
void vu1SetScreenAspect(float fAspectW, float fAspectH)
{
    fVu1AspectW = fAspectW;
    fVu1AspectH = fAspectH;

#ifndef TARGET_PC
    asm volatile 
    {
        
        mfc1     t0, $f12
        mfc1     t1, $f13

        qmtc2    t0, $vf4
        qmtc2    t1, $vf5

        vaddx.x  $vf17x, $vf0x, $vf4x
        vaddx.y  $vf17y, $vf0y, $vf5x
        
    }
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        R8.sd[0] = (int32_t)(ee_f2u((float)(fAspectW)));
        R9.sd[0] = (int32_t)(ee_f2u((float)(fAspectH)));
        vu_load_vf(4, &R8);
        vu_load_vf(5, &R9);
        vu_upper(U_ADD, 8, 17, 0, 4, BC_X);
        vu_upper(U_ADD, 4, 17, 0, 5, BC_X);
    }
#endif
}

// 100% matching!
void vu1SetDiffuseMaterial(VU1_COLOR* pDiffuse)
{
    vu1Diffuse.fR = 128.0f * pDiffuse->fR;
    vu1Diffuse.fG = 128.0f * pDiffuse->fG;
    vu1Diffuse.fB = 128.0f * pDiffuse->fB;
    
#ifndef TARGET_PC
    asm volatile 
    ("
        lq         t3, VU1_COLOR.fR(%0)
    
        mfc1       t0, %1
        mfc1       t1, %1
        mfc1       t2, %1
        
        pextlw     t0, t1, t0 
        pcpyld     t0, t2, t0 
    
        qmtc2      t3, $vf4 
        qmtc2      t0, $vf5 
    
        vmul.xyz   $vf4,  $vf4, $vf5 
        vaddx.xyzw $vf20, $vf4, $vf0x 
    " : : "r"(pDiffuse), "f"(128.0f) : "t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pDiffuse);
        op1.f[0] = (float)(128.0f);
        ee_lq(&R11, (op0.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR))));
        R8.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R9.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R10.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        ee_pextlw(&R8, &R9, &R8);
        ee_pcpyld(&R8, &R10, &R8);
        vu_load_vf(4, &R11);
        vu_load_vf(5, &R8);
        vu_upper(U_MUL, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_ADD, 15, 20, 4, 0, BC_X);
    }
#endif
}

// 100% matching!
void vu1SetSpeculaMaterial(VU1_COLOR* pSpecula)
{
    vu1Specula.fR = 31.0f * pSpecula->fR;
    vu1Specula.fG = 31.0f * pSpecula->fG;
    vu1Specula.fB = 31.0f * pSpecula->fB;
    
#ifndef TARGET_PC
    asm volatile 
    ("
        mfc1       t0, %0
        mfc1       t1, %0
        mfc1       t2, %0
        
        pextlw     t0, t1, t0 
        pcpyld     t0, t2, t0 
    
        qmtc2      t3, $vf4 
        qmtc2      t0, $vf5 
    
        vmul.xyz   $vf4,  $vf4, $vf5 
        vaddx.xyzw $vf21, $vf4, $vf0x 
    " : : "f"(31.0f) : "t0", "t1", "t2"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op0 = {{0}};
        op0.f[0] = (float)(31.0f);
        R8.sd[0] = (int32_t)(ee_f2u(op0.f[0]));
        R9.sd[0] = (int32_t)(ee_f2u(op0.f[0]));
        R10.sd[0] = (int32_t)(ee_f2u(op0.f[0]));
        ee_pextlw(&R8, &R9, &R8);
        ee_pcpyld(&R8, &R10, &R8);
        vu_load_vf(4, &R11);
        vu_load_vf(5, &R8);
        vu_upper(U_MUL, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_ADD, 15, 21, 4, 0, BC_X);
    }
#endif
}

// 100% matching!
void vu1SetAmbient(VU1_COLOR* pAmbient)
{
    vu1Ambient.fR = pAmbient->fR;
    vu1Ambient.fG = pAmbient->fG;
    vu1Ambient.fB = pAmbient->fB;
    
#ifndef TARGET_PC
    asm volatile
    ("
        lqc2 vf22, VU1_COLOR.fR(%0)
    " : : "r"(pAmbient) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pAmbient);
        vu_load_vf(22, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
    }
#endif
}

// 100% matching!
void vu1SetAlphaRatio(float fAlpha)
{
    fVu1AlphaRatio = 128.0f * fAlpha;

#ifndef TARGET_PC
    asm volatile 
    ("

        mfc1    t0, %0       
        qmtc2   t0, $vf4 

        vmulx.w $vf23, $vf0, $vf4x

    " : : "f"(fVu1AlphaRatio) : "t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        op0.f[0] = (float)(fVu1AlphaRatio);
        R8.sd[0] = (int32_t)(ee_f2u(op0.f[0]));
        vu_load_vf(4, &R8);
        vu_upper(U_MUL, 1, 23, 0, 4, BC_X);
    }
#endif
}

#include "ps2_Vu1Scissor2.c"

// 100% matching!
void InitNodeArraySet(SCISSOR* scissor)
{
    scissor->rotflag = 0;
    scissor->flipflag = 0;

#ifndef TARGET_PC
    asm volatile 
    ("
        add  t0, zero, %0
        
        addi t2, zero, 3
        
        addi t0, t0, SCISSOR.narray
        addi t1, t0, sizeof(SCISSOR_NODE)
        
        sw   t0, SCISSOR.in(%0)
        sw   t1, SCISSOR.out(%0)
        
        sw   zero, SCISSOR_NODE.nodeNum(t0)
        sw   zero, SCISSOR_NODE.nodeNum(t1)
        
        sw   t2, SCISSOR_NODE.nodeNum(%0)
    " : : "r"(scissor) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(scissor);
        R8.sd[0] = (int32_t)(ee_zero.w[0] + op0.w[0]);
        R10.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(3));
        R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(offsetof(SCISSOR, narray)));
        R9.sd[0] = (int32_t)(R8.w[0] + (uint32_t)((sizeof(SCISSOR_NODE))));
        EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, in))), R8.d[0]);
        EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, out))), R9.d[0]);
        EE_SW((R8.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))), ee_zero.d[0]);
        EE_SW((R9.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))), ee_zero.d[0]);
        EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))), R10.d[0]);
    }
#endif
}

// 100% matching!
static void InitScissorPlane(SCISSOR_PLANE* sp)
{
    sp->planeNum = 5;
    
    sp->plane[0].clipmask = 0x820;
    sp->plane[0].xyzflag = 0x12;
    
    sp->plane[1].clipmask = 0x208;
    sp->plane[1].xyzflag = 0x11;
    
    sp->plane[2].clipmask = 0x104;
    sp->plane[2].xyzflag = 0x1;
    
    sp->plane[3].clipmask = 0x82;
    sp->plane[3].xyzflag = 0x10;
    
    sp->plane[4].clipmask = 0x41;
    sp->plane[4].xyzflag = 0;
}

// 100% matching!
void _Init_ScissorSystem()
{
    InitScissorPlane(&planeset);
    InitNodeArraySet(&scissorflip);
}

// 100% matching! 
int _Clip_ViewVolume(float* clip, float local_clip[4], float* vertex)
{
#ifndef TARGET_PC
    asm volatile
    ("
        lqc2         vf4, 0(%1)
            
        vmulax.xyzw  ACC, vf24, vf4x
            
        vmadday.xyzw ACC, vf25, vf4y
        vmaddaz.xyzw ACC, vf26, vf4z
            
        vmaddw.xyzw  vf5, vf27, vf0w
            
        vclipw.xyz   vf5xyz, vf5w
        vnop
        vnop
        vnop
        vnop
        
        sqc2         vf5, 0(%0)
        
        cfc2         v0, vi18
    " : : "r"(clip), "r"(vertex) : 
    );
#else
    { /* asm2c */
        ee_reg R2 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(clip);
        op1.sd[0] = (int32_t)(uintptr_t)(vertex);
        vu_load_vf(4, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_upper(U_MULA, 15, 0, 24, 4, BC_X);
        vu_upper(U_MADDA, 15, 0, 25, 4, BC_Y);
        vu_upper(U_MADDA, 15, 0, 26, 4, BC_Z);
        vu_upper(U_MADD, 15, 5, 27, 0, BC_W);
        vu_clipw(5, 5);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 5);
        R2.sd[0] = (int32_t)(vu_cfc2(18));
        return R2.sw[0]; /* the asm returned in v0 */
    }
#endif
}

// 100% matching!
void PushTriangleNodeArray(SCISSOR* scissor, NODE* nod)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        move   v1, %1
        
        lw     t0, SCISSOR.rotflag(%0)
        
        daddiu at, zero, sizeof(NODE)
    
        mult   t0, at
        nop
        nop
    
        mflo   t1
    
        addi   t0, t0, 1
        add    t1, t1, %0
        
        andi   t0, t0, 0x3
    
        lqc2   vf4, NODE.vertex(v1)
        lqc2   vf5, NODE.color(v1)
        lqc2   vf6, NODE.texUV(v1)
        lqc2   vf7, NODE.clipV(v1)
        
        sqc2   vf4, SCISSOR.triangle.node[0].vertex(t1)
        sqc2   vf5, SCISSOR.triangle.node[0].color(t1)
        sqc2   vf6, SCISSOR.triangle.node[0].texUV(t1)
        sqc2   vf7, SCISSOR.triangle.node[0].clipV(t1)
        
        addi   t2, t0, -3
        
        bltz   t2, l_002D4150
        nop
    
        neg    t0, zero
        
        l_002D4150:
        sw     t0, SCISSOR.rotflag(%0)
    " : : "r"(scissor), "r"(&node) : "v1"
    );
#else
    { /* asm2c */
        ee_reg R1 = {{0}}, R3 = {{0}}, R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
        uint64_t ee_prod;
        ee_reg ee_lo = {{0}}, ee_hi = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(scissor);
        op1.sd[0] = (int32_t)(uintptr_t)(&node);
        R3.d[0] = op1.d[0];
        R8.sd[0] = EE_LW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, rotflag))));
        R1.sd[0] = ee_zero.sd[0] + (int64_t)((sizeof(NODE)));
        ee_prod = (uint64_t)((int64_t)(int32_t)R8.w[0] * (int64_t)(int32_t)R1.w[0]);
        ee_lo.sd[0] = (int32_t)(uint32_t)ee_prod; ee_hi.sd[0] = (int32_t)(uint32_t)(ee_prod >> 32);
        R9.d[0] = ee_lo.d[0];
        R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(1));
        R9.sd[0] = (int32_t)(R9.w[0] + op0.w[0]);
        R8.d[0] = R8.d[0] & (uint64_t)(uint16_t)(3);
        vu_load_vf(4, EE_PTR((R3.w[0] + (uint32_t)(offsetof(NODE, vertex)))));
        vu_load_vf(5, EE_PTR((R3.w[0] + (uint32_t)(offsetof(NODE, color)))));
        vu_load_vf(6, EE_PTR((R3.w[0] + (uint32_t)(offsetof(NODE, texUV)))));
        vu_load_vf(7, EE_PTR((R3.w[0] + (uint32_t)(offsetof(NODE, clipV)))));
        vu_store_vf(EE_PTR((R9.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].vertex)))), 4);
        vu_store_vf(EE_PTR((R9.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].color)))), 5);
        vu_store_vf(EE_PTR((R9.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].texUV)))), 6);
        vu_store_vf(EE_PTR((R9.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].clipV)))), 7);
        R10.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-3));
        if (R10.sd[0] < 0) goto l_002D4150;
        R8.sd[0] = (int32_t)(0u - ee_zero.w[0]);
    l_002D4150:;
        EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, rotflag))), R8.d[0]);
    }
#endif
}

// 100% matching!
void ResetNodeArraySet(SCISSOR* scissor)
{
    scissor->flipflag = 0;

#ifndef TARGET_PC
    asm volatile 
    ("
        add  t0, zero, %0
        
        addi t2, zero, 3
        
        addi t0, t0, SCISSOR.narray
        addi t1, t0, sizeof(SCISSOR_NODE)
        
        sw   t0, SCISSOR.in(%0)
        sw   t1, SCISSOR.out(%0)
        
        sw   zero, SCISSOR_NODE.nodeNum(t0)
        sw   zero, SCISSOR_NODE.nodeNum(t1)
    " : : "r"(scissor) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(scissor);
        R8.sd[0] = (int32_t)(ee_zero.w[0] + op0.w[0]);
        R10.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(3));
        R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(offsetof(SCISSOR, narray)));
        R9.sd[0] = (int32_t)(R8.w[0] + (uint32_t)((sizeof(SCISSOR_NODE))));
        EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, in))), R8.d[0]);
        EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, out))), R9.d[0]);
        EE_SW((R8.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))), ee_zero.d[0]);
        EE_SW((R9.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))), ee_zero.d[0]);
    }
#endif
}

#ifdef TARGET_PC
/* The retail ScissorTriangle (0x2D4190) is one hand-written asm function; the decomp's
   per-block asm has inconsistent operands, so this is a direct C translation of retail.
   Sutherland-Hodgman against each enabled plane: clip flags of curr (bits 6-11) and
   next (bits 0-5) from VCLIPW, intersection parameter along the plane's axis. */
static int ScissorClipFlags(const float* v)
{
    float w = fabsf(v[3]);
    int f = 0;
    for (int c = 0; c < 3; c++) {
        if (v[c] > w) f |= 1 << (c * 2);
        if (v[c] < -w) f |= 2 << (c * 2);
    }
    return f;
}

static void ScissorIntersect(NODE* inter, const NODE* in, const NODE* out, float sgn, int xyz)
{
    float a[4], b[4], t;
    for (int c = 0; c < 4; c++) {
        a[c] = fabsf(in->clipV[c] - in->clipV[3] * sgn);
        b[c] = fabsf(out->clipV[c] - out->clipV[3] * sgn);
    }
    /* PROT3W xyz times: x <- component xyz % 3 */
    t = fabsf(a[xyz % 3] / (b[xyz % 3] + a[xyz % 3]));
    for (int c = 0; c < 4; c++) {
        inter->clipV[c] = in->clipV[c] + (out->clipV[c] - in->clipV[c]) * t;
        inter->vertex[c] = (out->vertex[c] - in->vertex[c]) * t + in->vertex[c];
        inter->color[c] = (out->color[c] - in->color[c]) * t + in->color[c];
        inter->texUV[c] = (out->texUV[c] - in->texUV[c]) * t + in->texUV[c];
    }
    if (inter->vertex[2] < 1.0f) inter->vertex[2] = 1.0f;
}

void ScissorTriangle(SCISSOR* scissor, SCISSOR_PLANE* plane_set)
{
    SCISSOR_NODE* inarray = &scissor->triangle;
    SCISSOR_NODE* outarray = scissor->out;
    unsigned int i, j;

    for (i = 0; i < plane_set->planeNum; i++) {
        unsigned int flag = plane_set->plane[i].xyzflag;
        int mask = plane_set->plane[i].clipmask;
        int xyz = flag & 0xF;
        float sgn = (flag & 0x10) ? -1.0f : 1.0f;

        for (j = 0; j < inarray->nodeNum; j++) {
            NODE* currN = &inarray->node[j];
            NODE* nextN = &inarray->node[(j + 1 < inarray->nodeNum) ? j + 1 : 0];
            int clip = ((ScissorClipFlags(currN->clipV) << 6) | ScissorClipFlags(nextN->clipV)) & mask;
            NODE interN;

            if (clip == 0) {
                outarray->node[outarray->nodeNum++] = *currN;
            } else if ((clip & 0x3F) && !(clip & 0xFC0)) {
                ScissorIntersect(&interN, currN, nextN, sgn, xyz);
                outarray->node[outarray->nodeNum++] = *currN;
                outarray->node[outarray->nodeNum++] = interN;
            } else if (!(clip & 0x3F) && (clip & 0xFC0)) {
                ScissorIntersect(&interN, nextN, currN, sgn, xyz);
                outarray->node[outarray->nodeNum++] = interN;
            }
        }

        {
            unsigned int old = scissor->flipflag;
            scissor->flipflag = (old + 1) & 1;
            scissor->out = &scissor->narray[old];
            scissor->in = &scissor->narray[scissor->flipflag];
            scissor->out->nodeNum = 0;
            inarray = scissor->in;
            outarray = scissor->out;
        }
    }
}
#else
// 100% matching! 
void ScissorTriangle(SCISSOR* scissor, SCISSOR_PLANE* plane_set)
{
    int i;                 
    int j;                  
    int clip;               
    int mask;               
    int xyz;                
    float sgn;              
    SCISSOR_NODE* inarray;  
    SCISSOR_NODE* outarray; 
    NODE* currN;            
    NODE* nextN;            
    NODE interN;            
    int n;                 

    nextN = NULL;
    currN = NULL;

    sgn = 0;

    clip = 0;
    
    inarray = &scissor->triangle;
    outarray = scissor->out;
    
    for (i = 0; i < plane_set->planeNum; i++) 
    {
        asm volatile
        ("
            lw          t0, 0(%4)
            
            muli        t0, t0, sizeof(PLANE)
        
            add         t0, t0, %1
        
            lw          t1, PLANE.xyzflag(t0)
            
            andi        t4, t1, 16
        
            li          t2, -1 
            li          t3,  1 
            
            movz        t2, t3, t4
            
            qmtc2       t2, vf4
        
            vitof0.xyzw vf4, vf4
            
            lw          t2, PLANE.clipmask(t0)
        
            sw          t2, 0(%2)
            
            andi        t1, t1, 0xF
        
            sw          t1, 0(%3)
            
            qmfc2       v0, vf4
        
            mtc1        v0, %0
        " : "=r"(sgn) : "r"(plane_set), "r"(&mask), "r"(&xyz), "r"(&i) :
        );
        
        for (j = 0; j < inarray->nodeNum; j++) 
        {
            asm volatile
            ("
                lw   t0, 0(%3)
                lw   t1, SCISSOR_NODE.nodeNum(%2)
                
                muli t2, t0, sizeof(NODE)
            
                addi t3, t0, 1 
            
                sub  t4, t3, t1 
                
                bltz t4, l_002D4244
                vnop
            
                neg  t3, zero
                
            l_002D4244:
                muli t3, t3, sizeof(NODE)
                                    
                add  %0, t2, %2
                add  %1, t3, %2 
            " : "=r"(currN), "=r"(nextN) : "r"(inarray), "r"(&j)
            );
                
            asm volatile
            ("
                lqc2       vf4, NODE.clipV(%1)
                lqc2       vf5, NODE.clipV(%2) 
            
                vclipw.xyz vf4, vf4                    
                vclipw.xyz vf5, vf5                   
                vnop
                vnop
                vnop
                vnop
                vnop
            
                cfc2       %0, vi18
            " : "=r"(clip) : "r"(currN), "r"(nextN)
            );
            
            clip &= mask;
            
            if (clip == 0) 
            {
                asm volatile
                ("  
                    lw   t0, SCISSOR_NODE.nodeNum(%2)
                    
                    muli t1, t0, sizeof(NODE)
                
                    add  t1, t1, %2 
                
                    lqc2 vf4, NODE.vertex(%4)
                    lqc2 vf5, NODE.color(%4)
                    lqc2 vf6, NODE.texUV(%4)
                    lqc2 vf7, NODE.clipV(%4)
                    
                    sqc2 vf4, NODE.vertex(t1)
                    sqc2 vf5, NODE.color(t1)
                    sqc2 vf6, NODE.texUV(t1)
                    sqc2 vf7, NODE.clipV(t1)
                    
                    addi t0, t0, 1 
                
                    sw   t0, SCISSOR_NODE.nodeNum(%2)
                " : : "r"(inarray), "r"(&j), "r"(outarray), "r"(mask), "r"(currN)
                );
            } 
            else if (((clip & 0x3F)) && ((clip & 0xFC0)))
            {
                
            } 
            else if (((clip & 0x3F)) && (!(clip & 0xFC0)))
            {
                asm volatile
                ("
                    lqc2       vf8, NODE.clipV(%3)
                    lqc2       vf9, NODE.clipV(%4)
                    
                    mfc1       t0, %0
                
                    qmtc2      t0, vf10
                    
                    vmulx.w    vf11, vf8, vf10
                    vmulx.w    vf12, vf9, vf10
                    
                    vsubw.xyzw vf11, vf8, vf11
                    vsubw.xyzw vf12, vf9, vf12
                    
                    vabs.xyzw  vf11, vf11
                    vabs.xyzw  vf12, vf12
                    
                    qmfc2      t1, vf11
                    qmfc2      t2, vf12
                    nop
                
                l_002D4350:   
                    beqz       %2, l_002D4368
                        
                    prot3w     t1, t1
                    prot3w     t2, t2
                
                    addi       %2, -1
                    j l_002D4350  
                    
                l_002D4368:    
                    qmtc2      t1, vf11
                    qmtc2      t2, vf12
                
                    vsub.xyzw  vf13, vf9, vf8
                    
                    vadd.xyz   vf12, vf12, vf11
                    
                    lqc2       vf6, NODE.vertex(%4)
                    lqc2       vf5, NODE.color(%4)
                    lqc2       vf4, NODE.texUV(%4)
                    
                    vdiv       Q, vf11, vf12
                
                    lqc2       vf9,  NODE.vertex(%3)
                    lqc2       vf10, NODE.color(%3)
                    lqc2       vf11, NODE.texUV(%3)
                    
                    vsub.xyzw  vf6, vf6, vf9
                    vsub.xyzw  vf5, vf5, vf10
                    vsub.xyzw  vf4, vf4, vf11
                    
                    vwaitq
                    
                    vaddq.xyzw vf12, vf0, Q
                    
                    vabs.x     vf12, vf12
                    
                    vmulx.xyzw vf7, vf13, vf12
                    vmulx.xyzw vf6, vf6,  vf12
                    vmulx.xyzw vf5, vf5,  vf12
                    vmulx.xyzw vf4, vf4,  vf12
                    
                    vadd.xyzw  vf7, vf8, vf7
                    vadd.xyzw  vf6, vf6, vf9
                    vadd.xyzw  vf5, vf5, vf10
                    vadd.xyzw  vf4, vf4, vf11
                    
                    vmaxw.z    vf6, vf6, vf0
                    
                    sqc2       vf5, NODE.color(%1)
                    sqc2       vf4, NODE.texUV(%1)
                    sqc2       vf7, NODE.clipV(%1)
                    sqc2       vf6, NODE.vertex(%1)
                " : : "r"(sgn), "r"(&interN), "r"(xyz), "r"(currN), "r"(nextN)
                );
                    
                asm volatile
                ("
                    lw   t0, SCISSOR_NODE.nodeNum(%2)
                
                    muli t1, t0, sizeof(NODE)
                    
                    add  t1, t1, %2 
                
                    lqc2 vf4, NODE.vertex(%3)
                    lqc2 vf5, NODE.color(%3)
                    lqc2 vf6, NODE.texUV(%3)
                    lqc2 vf7, NODE.clipV(%3)
                    
                    sqc2 vf4, NODE.vertex(t1)
                    sqc2 vf5, NODE.color(t1)
                    sqc2 vf6, NODE.texUV(t1)
                    sqc2 vf7, NODE.clipV(t1)
                    
                    addi t0, t0, 1 
                
                    sw   t0, SCISSOR_NODE.nodeNum(%2)
                " : : "r"(&interN), "r"(xyz), "r"(outarray), "r"(currN)
                );
                    
                asm volatile
                ("
                    lw   t0, SCISSOR_NODE.nodeNum(%2)
                    
                    muli t1, t0, sizeof(NODE)
                    
                    add  t1, t1, %2 
                
                    lqc2 vf4, NODE.vertex(%0)
                    lqc2 vf5, NODE.color(%0)
                    lqc2 vf6, NODE.texUV(%0)
                    lqc2 vf7, NODE.clipV(%0)
                    
                    sqc2 vf4, NODE.vertex(t1)
                    sqc2 vf5, NODE.color(t1)
                    sqc2 vf6, NODE.texUV(t1)
                    sqc2 vf7, NODE.clipV(t1)
                    
                    addi t0, t0, 1 
                
                    sw   t0, SCISSOR_NODE.nodeNum(%2)
                " : : "r"(&interN), "r"(xyz), "r"(outarray)
                );
            } 
            else if ((!(clip & 0x3F)) && ((clip & 0xFC0))) 
            {
                asm volatile
                ("
                    lqc2       vf8, NODE.clipV(%3)
                    lqc2       vf9, NODE.clipV(%4)
                    
                    mfc1       t0, %0
                
                    qmtc2      t0, vf10
                    
                    vmulx.w    vf11, vf8, vf10
                    vmulx.w    vf12, vf9, vf10
                    
                    vsubw.xyzw vf11, vf8, vf11
                    vsubw.xyzw vf12, vf9, vf12
                    
                    vabs.xyzw  vf11, vf11
                    vabs.xyzw  vf12, vf12
                    
                    qmfc2      t1, vf11
                    qmfc2      t2, vf12
                    nop
                
                l_002D44C8:
                    beqz       %2, l_002D44E0
                        
                    prot3w     t1, t1
                    prot3w     t2, t2
                
                    addi       %2, %2, -1 
                    j          l_002D44C8
                    
                l_002D44E0:
                    qmtc2      t1, vf11
                    qmtc2      t2, vf12
                
                    vsub.xyzw  vf13, vf9, vf8
                    
                    vadd.xyz   vf12, vf12, vf11
                    
                    lqc2       vf6, NODE.vertex(%4)
                    lqc2       vf5, NODE.color(%4)
                    lqc2       vf4, NODE.texUV(%4)
                    
                    vdiv       Q, vf11, vf12
                
                    lqc2       vf9,  NODE.vertex(%3)
                    lqc2       vf10, NODE.color(%3)
                    lqc2       vf11, NODE.texUV(%3)
                    
                    vsub.xyzw  vf6, vf6, vf9
                    vsub.xyzw  vf5, vf5, vf10
                    vsub.xyzw  vf4, vf4, vf11
                    
                    vwaitq
                    
                    vaddq.xyzw vf12, vf0, Q
                    
                    vabs.x     vf12, vf12
                    
                    vmulx.xyzw vf7, vf13, vf12
                    vmulx.xyzw vf6, vf6, vf12
                    vmulx.xyzw vf5, vf5, vf12
                    vmulx.xyzw vf4, vf4, vf12
                    
                    vadd.xyzw  vf7, vf8, vf7
                    vadd.xyzw  vf6, vf6, vf9
                    vadd.xyzw  vf5, vf5, vf10
                    vadd.xyzw  vf4, vf4, vf11
                    
                    vmaxw.z    vf6, vf6, vf0
                    
                    sqc2       vf5, NODE.color(%1)
                    sqc2       vf4, NODE.texUV(%1)
                    sqc2       vf7, NODE.clipV(%1)
                    sqc2       vf6, NODE.vertex(%1)
                " : : "r"(sgn), "r"(&interN), "r"(xyz), "r"(nextN), "r"(currN)
                );
                
                asm volatile
                ("
                    lw   t0, SCISSOR_NODE.nodeNum(%2)
                    
                    muli t1, t0, sizeof(NODE)
                    
                    add  t1, t1, %2 
                    
                    lqc2 vf4, NODE.vertex(%1)
                    lqc2 vf5, NODE.color(%1)
                    lqc2 vf6, NODE.texUV(%1)
                    lqc2 vf7, NODE.clipV(%1)
                    
                    sqc2 vf4, NODE.vertex(t1)
                    sqc2 vf5, NODE.color(t1)
                    sqc2 vf6, NODE.texUV(t1)
                    sqc2 vf7, NODE.clipV(t1)
                    
                    addi t0, t0, 1 
                    
                    sw   t0, SCISSOR_NODE.nodeNum(%2)
                " : : "r"(xyz), "r"(&interN), "r"(outarray)
                );
            }
        }

        asm volatile
        ("
            lw   t0, SCISSOR.flipflag(%0)
            
            addi t1, t0, 1 
        
            andi t1, t1, 1
            
            sw   t1, SCISSOR.flipflag(%0)
            
            muli t0, t0, sizeof(SCISSOR_NODE)
            muli t1, t1, sizeof(SCISSOR_NODE)
        
            add  t0, t0, %0 
            add  t1, t1, %0 
            
            addi t0, t0, sizeof(SCISSOR_NODE) 
            addi t1, t1, sizeof(SCISSOR_NODE) 
            
            sw   t0,   SCISSOR.out(%0)
            sw   t1,   SCISSOR.in(%0)
            sw   zero, SCISSOR.triangle.nodeNum(t0)
        " : : "r"(scissor)
        );
                
        inarray = scissor->in;   
        outarray = scissor->out;
    }
}
#endif

// 100% matching!
int _Check_DisplayAreaPoint(NJS_VECTOR* vec)
{
    if (((vec->x >= 1.0f) && (vec->x < 4095.0f)) && ((vec->y >= 1.0f) && (vec->y < 4095.0f)) && ((vec->z >= 1.0f) && (vec->z < 65535.0f))) 
    {
        return 0;
    }
    
    return 1;
}

// 100% matching!
void DrawScissorPolygonOpaque(SCISSOR* scissor, unsigned long ulType)
{
    SCISSOR_NODE* in; 
    VU1_PRIM_BUF* pP; // not from the debugging symbols

    in = scissor->in;
    
#ifndef TARGET_PC
    asm volatile
    ("
        addi     t6, %0, SCISSOR.in
    
        lw       t6, SCISSOR.triangle.node[0](t6)
        
        add      t7, zero, %1
    
        lw       t0, SCISSOR_NODE.nodeNum(t6)
        nop
        
        l_002D4720:
        lqc2     vf4, SCISSOR.triangle.node[0].vertex(t6)
        
        vdiv     Q, vf0w, vf4z
    
        lqc2     vf7, SCISSOR.triangle.node[0].color(t6)
        
        sqc2     vf7, VU1_PRIM_BUF.fR(t7)
    
        vaddz.x  vf5, vf0, vf23z
        
        lqc2     vf6, SCISSOR.triangle.node[0].texUV(t6)
    
        addi     t0, t0, -1
        
        vwaitq
        
        vmulq.x  vf5, vf5, Q
        vmulq.xy vf6, vf6, Q
        
        vaddq.z  vf6, vf0, Q
        
        vsub.w   vf6, vf6, vf6
        
        vmulx.xy vf4, vf4, vf5x
        
        sqc2     vf6, VU1_PRIM_BUF.fS(t7)
    
        vmul.xy  vf4, vf4, vf17
        
        vadd.xy  vf4, vf4, vf16
        
        addi     t6, t6, sizeof(NODE)
    
        sqc2     vf4, VU1_PRIM_BUF.fX(t7)
    
        addi     t7, t7, sizeof(VU1_PRIM_BUF)
    
        bnez     t0, l_002D4720
        vnop
    " : : "r"(scissor), "r"(vu1ScessorBuf) : "t7"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R14 = {{0}}, R15 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(scissor);
        op1.sd[0] = (int32_t)(uintptr_t)(vu1ScessorBuf);
        R14.sd[0] = (int32_t)(op0.w[0] + (uint32_t)(offsetof(SCISSOR, in)));
        R14.sd[0] = EE_LW((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0]))));
        R15.sd[0] = (int32_t)(ee_zero.w[0] + op1.w[0]);
        R8.sd[0] = EE_LW((R14.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))));
    l_002D4720:;
        vu_load_vf(4, EE_PTR((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].vertex)))));
        vu_div(0, 3, 4, 2);
        vu_load_vf(7, EE_PTR((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].color)))));
        vu_store_vf(EE_PTR((R15.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 7);
        vu_upper(U_ADD, 8, 5, 0, 23, BC_Z);
        vu_load_vf(6, EE_PTR((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].texUV)))));
        R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-1));
        vu_upper(U_MUL, 8, 5, 5, 0, BC_Q);
        vu_upper(U_MUL, 12, 6, 6, 0, BC_Q);
        vu_upper(U_ADD, 2, 6, 0, 0, BC_Q);
        vu_upper(U_SUB, 1, 6, 6, 6, BC_NONE);
        vu_upper(U_MUL, 12, 4, 4, 5, BC_X);
        vu_store_vf(EE_PTR((R15.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fS)))), 6);
        vu_upper(U_MUL, 12, 4, 4, 17, BC_NONE);
        vu_upper(U_ADD, 12, 4, 4, 16, BC_NONE);
        R14.sd[0] = (int32_t)(R14.w[0] + (uint32_t)((sizeof(NODE))));
        vu_store_vf(EE_PTR((R15.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX)))), 4);
        R15.sd[0] = (int32_t)(R15.w[0] + (uint32_t)((sizeof(VU1_PRIM_BUF))));
        if (R8.sd[0] != 0) goto l_002D4720;
    }
#endif

    ulType = (ulType & 0xFFFDFFFFFFFFFFFF) | 0x6800000000000;
    
    pP = vu1ScessorBuf;
    
    Ps2AddPrim3DEx(ulType, pP, in->nodeNum);
}

// 100% matching! 
void vu1DrawTriangleStripOpaqueSingle(unsigned long ulType, VU1_STRIP_BUF* pStripTop, unsigned short usStripMax, unsigned short usMode)
{
    VU1_PRIM_BUF* pP;                            
    VU1_STRIP_BUF* pS0, *pS1, *pS2;                         
    float fIz;                                   
    unsigned short usStripCnt;                  
    void (*pFunc)(VU1_STRIP_BUF*, VU1_PRIM_BUF*); 
    unsigned int clipflag;                     
    int flg;                                    
    int count;                                

    pS0 = pStripTop; 
    pS1 = &pS0[1];
    pS2 = &pS1[1];
    
    InitNodeArraySet2();
    
    pP = (VU1_PRIM_BUF*)Ps2_DRAW_TMP;
    
    pFunc = pColorCalcFuncTbl[(usMode & 0xFFFF) & 0xF];
    
    pFunc(pS0, pP);
    
    _Clip_ViewVolume2((NJS_POINT4*)&pS0->fVx);
    
    fIz = pS0->fIz;
   
    pP->fS = pS0->fU * fIz;
    pP->fT = pS0->fV * fIz;
    pP->fQ = fIz;
    
    pP->ulKick = 0;
    
    pP->fX = fVu1OffsetX + pS0->fSx;
    pP->fY = fVu1OffsetY + pS0->fSy;
    pP->fZ = pS0->fVz;
    
    pP->fF = pS0->fFog;
    
    flg = _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
    
    flg <<= 1;
    
    pP++;
    
    pFunc(pS1, pP);
    
    _Clip_ViewVolume2((NJS_POINT4*)&pS1->fVx);
    
    fIz = pS1->fIz;
    
    pP->fS = pS1->fU * fIz;
    pP->fT = pS1->fV * fIz;
    pP->fQ = fIz;
    
    pP->ulKick = 0;
    
    pP->fX = fVu1OffsetX + pS1->fSx;
    pP->fY = fVu1OffsetY + pS1->fSy;
    pP->fZ = pS1->fVz;
    
    pP->fF = pS1->fFog;
    
    flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
    
    flg <<= 1;
    
    pP++; 
    
    for (usStripCnt = 2; usStripCnt < usStripMax; usStripCnt++, pS0++, pS1++, pS2++, pP++) 
    {
        pFunc(pS2, pP);
        
        _Check_ClipViewAll((NJS_POINT4*)&pS2->fVx);
        
        fIz = pS2->fIz;
        
        pP->fS = pS2->fU * fIz;
        pP->fT = pS2->fV * fIz;
        pP->fQ = fIz;
        
        pP->ulKick = 0;
        
        pP->fX = fVu1OffsetX + pS2->fSx;
        pP->fY = fVu1OffsetY + pS2->fSy;
        pP->fZ = pS2->fVz;
        
        pP->fF = pS2->fFog;
        
        flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
        
        clipflag = _Get_ClipViewVolume2();
        
        if ((0 < pS0->fNz) && (0 < pS1->fNz) && (0 < pS2->fNz))  
        {
            pP->ulKick = PC_KICK_CULL;
        }
        else if (PC_SCISSOR && ((flg & 0x7)) && ((clipflag & 0x3FFFF)))
        {
            pP->ulKick = 32768;
            
            if (_Get_ClipVolumePlane() != 0) 
            {
                _Set_NodeArray(pS2, pP);
                
                count = _Check_ScissorPlane();
                
                if (count != 0)
                {
                    DrawScissorPolygonOpaque2(count, ulType);
                }
            }
        }
        
        flg <<= 1;
    }
    
    PC_ADDPRIM3DEX(ulType, Ps2_DRAW_TMP, usStripMax);
}

// 100% matching! 
void vu1DrawTriangleStripOpaqueDouble(unsigned long ulType, VU1_STRIP_BUF* pS, unsigned short usStripMax, unsigned short usMode)
{
    VU1_PRIM_BUF* pP;                            
    float fIz;                                   
    unsigned short usStripCnt;                  
    void (*pFunc)(VU1_STRIP_BUF*, VU1_PRIM_BUF*); 
    unsigned int clipflag;                     
    unsigned int flg;                                    
    int count;   

    InitNodeArraySet2();
    
    pP = (VU1_PRIM_BUF*)Ps2_DRAW_TMP;
    
    pFunc = pColorCalcFuncTbl[(usMode & 0xFFFF) & 0xF];
    
    pFunc(pS, pP);
    
    fIz = pS->fIz;
   
    pP->fS = pS->fU * fIz;
    pP->fT = pS->fV * fIz;
    pP->fQ = fIz;
    
    pP->ulKick = 0;

    _Clip_ViewVolume2((NJS_POINT4*)&pS->fVx);
    
    pP->fX = fVu1OffsetX + pS->fSx;
    pP->fY = fVu1OffsetY + pS->fSy;
    pP->fZ = pS->fVz;
    
    pP->fF = pS->fFog;
    
    flg = _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
    
    flg <<= 1;
    
    pP++;
    pS++;
    
    pFunc(pS, pP);
    
    fIz = pS->fIz;
    
    pP->fS = pS->fU * fIz;
    pP->fT = pS->fV * fIz;
    pP->fQ = fIz;
    
    pP->ulKick = 0;

    _Clip_ViewVolume2((NJS_POINT4*)&pS->fVx);
    
    pP->fX = fVu1OffsetX + pS->fSx;
    pP->fY = fVu1OffsetY + pS->fSy;
    pP->fZ = pS->fVz;
    
    pP->fF = pS->fFog;
    
    flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
    
    flg <<= 1;
    
    pP++; 
    pS++;
    
    for (usStripCnt = 2; usStripCnt < usStripMax; usStripCnt++, pS++, pP++) 
    {
        pFunc(pS, pP);
        
        fIz = pS->fIz;
        
        pP->fS = pS->fU * fIz;
        pP->fT = pS->fV * fIz;
        pP->fQ = fIz;
        
        pP->ulKick = 0;

        _Check_ClipViewAll((NJS_POINT4*)&pS->fVx);
        
        pP->fX = fVu1OffsetX + pS->fSx;
        pP->fY = fVu1OffsetY + pS->fSy;
        pP->fZ = pS->fVz;
        
        pP->fF = pS->fFog;
        
        flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
        
        clipflag = _Get_ClipViewVolume2();
        
        if (PC_SCISSOR && ((flg & 0x7)) && ((clipflag & 0x3FFFF)))
        {
            pP->ulKick = 32768;
            
            if (_Get_ClipVolumePlane() != 0) 
            {
                _Set_NodeArray(pS, pP);
                
                count = _Check_ScissorPlane();
                
                if (count != 0)
                {
                    DrawScissorPolygonOpaque2(count, ulType);
                }
            }
        }
        
        flg <<= 1;
    }
    
    PC_ADDPRIM3DEX(ulType, Ps2_DRAW_TMP, usStripMax);
}

// 100% matching! 
void vu1DrawTriangleStripTransSingle(unsigned long ulType, VU1_STRIP_BUF* pStripTop, unsigned short usStripMax, unsigned short usMode)
{
    VU1_PRIM_BUF* pP;                            
    VU1_STRIP_BUF* pS0, *pS1, *pS2;                         
    float fIz;                                   
    unsigned short usStripCnt;                  
    void (*pFunc)(VU1_STRIP_BUF*, VU1_PRIM_BUF*); 
    unsigned int clipflag;                     
    int flg;                                    
    int count;                                

    pS0 = pStripTop; 
    pS1 = &pS0[1];
    pS2 = &pS1[1];
    
    InitNodeArraySet2();
    
    pP = (VU1_PRIM_BUF*)Ps2_DRAW_TMP;
    
    pFunc = pColorCalcFuncTbl[(usMode & 0xFFFF) & 0xF];
    
    pFunc(pS0, pP);
    
    _Clip_ViewVolume2((NJS_POINT4*)&pS0->fVx);
    
    fIz = pS0->fIz;
   
    pP->fS = pS0->fU * fIz;
    pP->fT = pS0->fV * fIz;
    pP->fQ = fIz;
    
    pP->ulKick = 0;
    
    pP->fX = fVu1OffsetX + pS0->fSx;
    pP->fY = fVu1OffsetY + pS0->fSy;
    pP->fZ = pS0->fVz;
    
    pP->fF = pS0->fFog;
    
    flg = _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
    
    flg <<= 1;
    
    pP++;
    
    pFunc(pS1, pP);
    
    _Clip_ViewVolume2((NJS_POINT4*)&pS1->fVx);
    
    fIz = pS1->fIz;
    
    pP->fS = pS1->fU * fIz;
    pP->fT = pS1->fV * fIz;
    pP->fQ = fIz;
    
    pP->ulKick = 0;
    
    pP->fX = fVu1OffsetX + pS1->fSx;
    pP->fY = fVu1OffsetY + pS1->fSy;
    pP->fZ = pS1->fVz;
    
    pP->fF = pS1->fFog;
    
    flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
    
    flg <<= 1;
    
    pP++; 
    
    for (usStripCnt = 2; usStripCnt < usStripMax; usStripCnt++, pS0++, pS1++, pS2++, pP++) 
    {
        pFunc(pS2, pP);
        
        _Check_ClipViewAll((NJS_POINT4*)&pS2->fVx);
        
        fIz = pS2->fIz;
        
        pP->fS = pS2->fU * fIz;
        pP->fT = pS2->fV * fIz;
        pP->fQ = fIz;
        
        pP->ulKick = 0;
        
        pP->fX = fVu1OffsetX + pS2->fSx;
        pP->fY = fVu1OffsetY + pS2->fSy;
        pP->fZ = pS2->fVz;
        
        pP->fF = pS2->fFog;
        
        flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX);
        
        clipflag = _Get_ClipViewVolume2();
        
        if ((0 < pS0->fNz) && (0 < pS1->fNz) && (0 < pS2->fNz))  
        {
            pP->ulKick = PC_KICK_CULL;
        }
        else if (PC_SCISSOR && ((flg & 0x7)) && ((clipflag & 0x3FFFF)))
        {
            pP->ulKick = 32768;
            
            if (_Get_ClipVolumePlane() != 0) 
            {
                _Set_NodeArray(pS2, pP);
                
                count = _Check_ScissorPlane();
                
                if (count != 0)
                {
                    DrawScissorPolygonOpaque2(count, ulType);
                }
            }
        }
        
        flg <<= 1;
    }
    
    PC_ADDPRIM3DEX(ulType, Ps2_DRAW_TMP, usStripMax);
}

// 99.38% matching
int _Clip_Screen(float* clip)
{
    int ret;         
    static float para[2][4] = { 2048.0f, 2048.0f, 0, 240.0f, 0.75f, 1.0f, 0, 0 }; 

    ret = 0;
#ifdef TARGET_PC
    para[1][0] = 0.75f / pc_hor_scale();   /* hor+: effects (njDrawTexture3DEx) are culled at the widened view */
#endif

#ifndef TARGET_PC
    asm volatile
    ("
        lqc2       vf4, 0(%1)
        lqc2       vf5, 0(%2)
        lqc2       vf6, 16(%2)
    
        vsub.xy    vf4, vf4, vf5
        
        vmul.xyzw  vf4, vf4, vf6
        
        vclipw.xyz vf4, vf5
        vnop
        vnop
        vnop
        vnop
        vnop
    
        cfc2       %0, vi18
    " : "=r"(ret) : "f"(clip), "r"(&para) :
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(ret);
        op1.sd[0] = (int32_t)(uintptr_t)(clip); /* decomp declares this operand "f" but it is the clip-vector pointer */
        op2.sd[0] = (int32_t)(uintptr_t)(&para);
        vu_load_vf(4, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(0))));
        vu_load_vf(6, EE_PTR((op2.w[0] + (uint32_t)(16))));
        vu_upper(U_SUB, 12, 4, 4, 5, BC_NONE);
        vu_upper(U_MUL, 15, 4, 4, 6, BC_NONE);
        vu_clipw(4, 5);
        op0.sd[0] = (int32_t)(vu_cfc2(18));
        ret = (__typeof__(ret))(uintptr_t)op0.w[0];
    }
#endif

    return ret;
}

// 97.44% matching
void vu1DrawTriangleStripTransDouble(unsigned long ulType, VU1_STRIP_BUF* pS, unsigned short usStripMax, unsigned short usMode)
{ 
    VU1_PRIM_BUF* pP;                            
    float fIz;                                   
    unsigned short usStripCnt;                  
    unsigned int clipflag;                     
    int flg;                                    
    int count;   
    int clipswitch;

    clipswitch = 0; 
    
    clipflag = 0;
    
#ifndef TARGET_PC
    asm volatile 
    ("
    .set noreorder
        andi    v0, %0, 0xF    

        add     t0, zero, v0 
        
        ctc2    t0, vi1      
    
        vcallms VU0_INIT_CALC_COLOR 
        nop
    .set reorder
    " : "r"(usMode) : 
    );
#else
    { /* asm2c */
        ee_reg R2 = {{0}}, R8 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(usMode);
        R2.d[0] = op0.d[0] & (uint64_t)(uint16_t)(15);
        R8.sd[0] = (int32_t)(ee_zero.w[0] + R2.w[0]);
        vu_ctc2(1, R8.w[0]);
        vu0_callms(VU0_ADDR_VU0_INIT_CALC_COLOR);
        usMode = (__typeof__(usMode))(uintptr_t)op0.w[0];
    }
#endif
   
    pP = (VU1_PRIM_BUF*)Ps2_DRAW_TMP; 
    
    if (clipswitch == 0) 
    { 
#ifndef TARGET_PC
        asm volatile
        ("
        .set noreorder
            lqc2    vf4, VU1_STRIP_BUF.fIr(%2) 
            lqc2    vf14, VU1_STRIP_BUF.fVx(%2)
            
            vcallms VU0_CALC_COLOR  
        
            lwc1    f1, VU1_STRIP_BUF.fU(%2)
            lwc1    f2, VU1_STRIP_BUF.fV(%2) 
            lwc1    f3, VU1_STRIP_BUF.fIz(%2) 
            
            lq      t0, VU1_STRIP_BUF.fVx(%2) 
            
            lwc1    f4, VU1_STRIP_BUF.fSx(%2) 
            lwc1    f5, VU1_STRIP_BUF.fSy(%2) 
            
            mul.s   f1, f1, f3 
            mul.s   f2, f2, f3 
            
            add.s   f4, f4, %0 
            add.s   f5, f5, %1 
            
            sq      t0, VU1_PRIM_BUF.fX(%3) 
            
            swc1    f1, VU1_PRIM_BUF.fS(%3) 
            swc1    f2, VU1_PRIM_BUF.fT(%3) 
            swc1    f3, VU1_PRIM_BUF.fQ(%3) 
            
            sw      zero, VU1_PRIM_BUF.ulKick(%3) 
            
            swc1    f4, VU1_PRIM_BUF.fX(%3) 
            swc1    f5, VU1_PRIM_BUF.fY(%3) 
        .set reorder
        " : : "f"(fVu1OffsetX), "f"(fVu1OffsetY), "r"(pS), "r"(pP) : "$f1", "$f2", "$f3", "$f4", "$f5", "$t0", "memory"
        );
#else
        { /* asm2c */
            ee_reg R8 = {{0}};
            float F1 = 0.0f, F2 = 0.0f, F3 = 0.0f, F4 = 0.0f, F5 = 0.0f;
            const ee_reg ee_zero = {{0}};
            ee_reg op0 = {{0}};
            ee_reg op1 = {{0}};
            ee_reg op2 = {{0}};
            ee_reg op3 = {{0}};
            op0.f[0] = (float)(fVu1OffsetX);
            op1.f[0] = (float)(fVu1OffsetY);
            op2.sd[0] = (int32_t)(uintptr_t)(pS);
            op3.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_load_vf(4, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
            vu_load_vf(14, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx)))));
            vu0_callms(VU0_ADDR_VU0_CALC_COLOR);
            F1 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fU))));
            F2 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fV))));
            F3 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIz))));
            ee_lq(&R8, (op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx))));
            F4 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSx))));
            F5 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSy))));
            F1 = vu_clampf(F1 * F3);
            F2 = vu_clampf(F2 * F3);
            F4 = vu_clampf(F4 + op0.f[0]);
            F5 = vu_clampf(F5 + op1.f[0]);
            ee_sq((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX))), &R8);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fS))), F1);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fT))), F2);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fQ))), F3);
            EE_SW((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, ulKick))), ee_zero.d[0]);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX))), F4);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fY))), F5);
        }
#endif  
            
        flg = _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX) << 2;
        
#ifndef TARGET_PC
        asm volatile 
        ("
            qmfc2.i t0, vf4    
            
            sq      t0, VU1_PRIM_BUF.fR(%0) 
        " : : "r"(pP) : "$t0", "memory" 
        );
#else
        { /* asm2c */
            ee_reg R8 = {{0}};
            ee_reg op0 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_store_vf(&R8, 4);
            ee_sq((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR))), &R8);
        }
#endif
        
        pP++; 
        pS++; 
        
#ifndef TARGET_PC
        asm volatile 
        (" 
        .set noreorder
            lqc2    vf4, VU1_STRIP_BUF.fIr(%2) 
            lqc2    vf14, VU1_STRIP_BUF.fVx(%2)
            
            vcallms VU0_CALC_COLOR  
        
            lwc1    f1, VU1_STRIP_BUF.fU(%2)
            lwc1    f2, VU1_STRIP_BUF.fV(%2) 
            lwc1    f3, VU1_STRIP_BUF.fIz(%2) 
            
            lq      t0, VU1_STRIP_BUF.fVx(%2) 
            
            lwc1    f4, VU1_STRIP_BUF.fSx(%2) 
            lwc1    f5, VU1_STRIP_BUF.fSy(%2) 
            
            mul.s   f1, f1, f3 
            mul.s   f2, f2, f3 
            
            add.s   f4, f4, %0 
            add.s   f5, f5, %1 
            
            sq      t0, VU1_PRIM_BUF.fX(%3) 
            
            swc1    f1, VU1_PRIM_BUF.fS(%3) 
            swc1    f2, VU1_PRIM_BUF.fT(%3) 
            swc1    f3, VU1_PRIM_BUF.fQ(%3) 
            
            sw      zero, VU1_PRIM_BUF.ulKick(%3) 
            
            swc1    f4, VU1_PRIM_BUF.fX(%3) 
            swc1    f5, VU1_PRIM_BUF.fY(%3) 
        .set reorder
        " : : "f"(fVu1OffsetX), "f"(fVu1OffsetY), "r"(pS), "r"(pP) : "$f1", "$f2", "$f3", "$f4", "$f5", "$t0", "memory"
        );
#else
        { /* asm2c */
            ee_reg R8 = {{0}};
            float F1 = 0.0f, F2 = 0.0f, F3 = 0.0f, F4 = 0.0f, F5 = 0.0f;
            const ee_reg ee_zero = {{0}};
            ee_reg op0 = {{0}};
            ee_reg op1 = {{0}};
            ee_reg op2 = {{0}};
            ee_reg op3 = {{0}};
            op0.f[0] = (float)(fVu1OffsetX);
            op1.f[0] = (float)(fVu1OffsetY);
            op2.sd[0] = (int32_t)(uintptr_t)(pS);
            op3.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_load_vf(4, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
            vu_load_vf(14, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx)))));
            vu0_callms(VU0_ADDR_VU0_CALC_COLOR);
            F1 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fU))));
            F2 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fV))));
            F3 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIz))));
            ee_lq(&R8, (op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx))));
            F4 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSx))));
            F5 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSy))));
            F1 = vu_clampf(F1 * F3);
            F2 = vu_clampf(F2 * F3);
            F4 = vu_clampf(F4 + op0.f[0]);
            F5 = vu_clampf(F5 + op1.f[0]);
            ee_sq((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX))), &R8);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fS))), F1);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fT))), F2);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fQ))), F3);
            EE_SW((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, ulKick))), ee_zero.d[0]);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX))), F4);
            EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fY))), F5);
        }
#endif
        
        flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX) << 1; 
        
#ifndef TARGET_PC
        asm volatile 
        ("
            qmfc2.i t0, vf4    
            
            sq      t0, VU1_PRIM_BUF.fR(%0) 
        " : : "r"(pP) : "$t0", "memory" 
        );
#else
        { /* asm2c */
            ee_reg R8 = {{0}};
            ee_reg op0 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_store_vf(&R8, 4);
            ee_sq((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR))), &R8);
        }
#endif
        
        pP++; 
        pS++;
        
        for (usStripCnt = 2; usStripCnt < usStripMax; usStripCnt++, pS++, pP++) 
        {
#ifndef TARGET_PC
            asm volatile 
            (" 
            .set noreorder
                lqc2    vf4, VU1_STRIP_BUF.fIr(%2) 
                lqc2    vf14, VU1_STRIP_BUF.fVx(%2)
                
                vcallms VU0_CLIP_VIEW_VOLUME_ALL  
            
                lwc1    f1, VU1_STRIP_BUF.fU(%2)
                lwc1    f2, VU1_STRIP_BUF.fV(%2) 
                lwc1    f3, VU1_STRIP_BUF.fIz(%2) 
                
                lq      t0, VU1_STRIP_BUF.fVx(%2) 
                
                lwc1    f4, VU1_STRIP_BUF.fSx(%2) 
                lwc1    f5, VU1_STRIP_BUF.fSy(%2) 
                
                mul.s   f1, f1, f3 
                mul.s   f2, f2, f3 
                
                add.s   f4, f4, %0 
                add.s   f5, f5, %1 
                
                sq      t0, VU1_PRIM_BUF.fX(%3) 
                
                swc1    f1, VU1_PRIM_BUF.fS(%3) 
                swc1    f2, VU1_PRIM_BUF.fT(%3) 
                swc1    f3, VU1_PRIM_BUF.fQ(%3) 
                
                sw      zero, VU1_PRIM_BUF.ulKick(%3) 
                
                swc1    f4, VU1_PRIM_BUF.fX(%3) 
                swc1    f5, VU1_PRIM_BUF.fY(%3) 
            .set reorder
            " : : "f"(fVu1OffsetX), "f"(fVu1OffsetY), "r"(pS), "r"(pP) : "$f1", "$f2", "$f3", "$f4", "$f5", "$t0", "memory"
            );
#else
            { /* asm2c */
                ee_reg R8 = {{0}};
                float F1 = 0.0f, F2 = 0.0f, F3 = 0.0f, F4 = 0.0f, F5 = 0.0f;
                const ee_reg ee_zero = {{0}};
                ee_reg op0 = {{0}};
                ee_reg op1 = {{0}};
                ee_reg op2 = {{0}};
                ee_reg op3 = {{0}};
                op0.f[0] = (float)(fVu1OffsetX);
                op1.f[0] = (float)(fVu1OffsetY);
                op2.sd[0] = (int32_t)(uintptr_t)(pS);
                op3.sd[0] = (int32_t)(uintptr_t)(pP);
                vu_load_vf(4, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
                vu_load_vf(14, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx)))));
                vu0_callms(VU0_ADDR_VU0_CLIP_VIEW_VOLUME_ALL);
                F1 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fU))));
                F2 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fV))));
                F3 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIz))));
                ee_lq(&R8, (op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx))));
                F4 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSx))));
                F5 = EE_LWF((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSy))));
                F1 = vu_clampf(F1 * F3);
                F2 = vu_clampf(F2 * F3);
                F4 = vu_clampf(F4 + op0.f[0]);
                F5 = vu_clampf(F5 + op1.f[0]);
                ee_sq((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX))), &R8);
                EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fS))), F1);
                EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fT))), F2);
                EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fQ))), F3);
                EE_SW((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, ulKick))), ee_zero.d[0]);
                EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX))), F4);
                EE_SWF((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fY))), F5);
            }
#endif
                
            flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX); 
            
#ifndef TARGET_PC
            asm volatile 
            ("
                qmfc2.i t0, vf4     
                
                sq      t0, VU1_PRIM_BUF.fR(%1) 
                
                cfc2    %0, vi18    
                
                addi    %0, %0, 0
            " : "=r"(clipflag) : "r"(pP) : "$t0" 
            );
#else
            { /* asm2c */
                ee_reg R8 = {{0}};
                ee_reg op0 = {{0}};
                ee_reg op1 = {{0}};
                op0.sd[0] = (int32_t)(uintptr_t)(clipflag);
                op1.sd[0] = (int32_t)(uintptr_t)(pP);
                vu_store_vf(&R8, 4);
                ee_sq((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR))), &R8);
                op0.sd[0] = (int32_t)(vu_cfc2(18));
                op0.sd[0] = (int32_t)(op0.w[0] + (uint32_t)(0));
                clipflag = (__typeof__(clipflag))(uintptr_t)op0.w[0];
            }
#endif

            if (PC_SCISSOR && (flg != 0) && ((clipflag & 0x3FFFF)))
            { 
                pP->ulKick = 32768; 
                
                if (_Get_ClipVolumePlane() != 0) 
                { 
                    _Set_NodeArray(pS, pP); 
                    
                    count = _Check_ScissorPlane(); 
                    
                    if (count != 0)
                    { 
                        DrawScissorPolygonOpaque2(count, ulType); 
                    }
                }
            }
            
            flg = (flg << 1) & 0x7; 
        } 
    }
    else
    {
#ifndef TARGET_PC
        asm volatile 
        ("
        .set noreorder
            lqc2    vf4, VU1_STRIP_BUF.fIr(%0) 
            
            vcallms VU0_CALC_COLOR  
            nop
        .set reorder
        " : : "r"(pS) : 
        );
#else
        { /* asm2c */
            ee_reg op0 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(pS);
            vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
            vu0_callms(VU0_ADDR_VU0_CALC_COLOR);
        }
#endif   
        
        fIz = pS->fIz; 
        
        pP->fS = pS->fU * fIz; 
        pP->fT = pS->fV * fIz; 
        pP->fQ = fIz; 
        
        pP->ulKick = 0; 
        
        pP->fX = fVu1OffsetX + pS->fSx; 
        pP->fY = fVu1OffsetY + pS->fSy; 
        pP->fZ = pS->fVz; 
        
        pP->fF = pS->fFog; 
        
#ifndef TARGET_PC
        asm volatile 
        ("
            sqc2 vf4, VU1_PRIM_BUF.fR(%0) 
            nop                         
        " : : "r"(pP) : 
        );
#else
        { /* asm2c */
            ee_reg op0 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
        }
#endif

        _Clip_ViewVolume2((NJS_POINT4*)&pS->fVx); 
        
        pP++; 
        pS++; 

#ifndef TARGET_PC
        asm volatile 
        ("
        .set noreorder
            lqc2    vf4, VU1_STRIP_BUF.fIr(%0) 
            
            vcallms VU0_CALC_COLOR  
            nop
        .set reorder
        " : : "r"(pS) : 
        );
#else
        { /* asm2c */
            ee_reg op0 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(pS);
            vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
            vu0_callms(VU0_ADDR_VU0_CALC_COLOR);
        }
#endif
        
        fIz = pS->fIz; 
        
        pP->fS = pS->fU * fIz;  
        pP->fT = pS->fV * fIz;
        pP->fQ = fIz; 
        
        pP->ulKick = 0; 
        
        pP->fX = fVu1OffsetX + pS->fSx; 
        pP->fY = fVu1OffsetY + pS->fSy; 
        pP->fZ = pS->fVz; 
        
        pP->fF = pS->fFog; 
        
#ifndef TARGET_PC
        asm volatile 
        ("
            sqc2 vf4, VU1_PRIM_BUF.fR(%0) 
            nop                         
        " : : "r"(pP) : 
        );
#else
        { /* asm2c */
            ee_reg op0 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
        }
#endif
    
        _Clip_ViewVolume2((NJS_POINT4*)&pS->fVx); 
        
        pP++; 
        pS++; 
        
        for (usStripCnt = 2; usStripCnt < usStripMax; usStripCnt++, pS++, pP++) 
        {
#ifndef TARGET_PC
            asm volatile 
            ("
            .set noreorder
                lqc2    vf4, VU1_STRIP_BUF.fIr(%0) 
                
                vcallms VU0_CALC_COLOR  
                nop
            .set reorder
            " : : "r"(pS) : 
            );
#else
            { /* asm2c */
                ee_reg op0 = {{0}};
                op0.sd[0] = (int32_t)(uintptr_t)(pS);
                vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
                vu0_callms(VU0_ADDR_VU0_CALC_COLOR);
            }
#endif
            
            fIz = pS->fIz; 
            
            pP->fS = pS->fU * fIz;
            pP->fT = pS->fV * fIz; 
            pP->fQ = fIz; 
            
            pP->ulKick = 0; 
            
            pP->fX = fVu1OffsetX + pS->fSx; 
            pP->fY = fVu1OffsetY + pS->fSy; 
            pP->fZ = pS->fVz; 
            
            pP->fF = pS->fFog; 
            
#ifndef TARGET_PC
            asm volatile 
            ("
                sqc2 vf4, VU1_PRIM_BUF.fR(%0) 
                nop                         
            " : : "r"(pP) : 
            );
#else
            { /* asm2c */
                ee_reg op0 = {{0}};
                op0.sd[0] = (int32_t)(uintptr_t)(pP);
                vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
            }
#endif
            
            _Clip_ViewVolume2((NJS_POINT4*)&pS->fVx); 
            
            clipflag = _Get_ClipViewVolume2(); 
            
            if ((clipflag & 0x3FFFF))
            { 
                pP->ulKick = 32768; 
            } 
        } 
    } 
    
    PC_ADDPRIM3DEX(ulType, Ps2_DRAW_TMP, usStripMax); 
} 

// 100% matching!
void vu1GetVertexColor(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2      vf4, VU1_STRIP_BUF.fIr(%0)
        
        mfc1      t0, %2
        mfc1      t1, %2
        mfc1      t2, %2
        mfc1      t3, %3
        
        pextlw    t0, t1, t0
        pextlw    t2, t3, t2
        pcpyld    t0, t2, t0
    
        qmtc2     t0, vf5
    
        vmul.xyzw vf4, vf4, vf5 
        
        sqc2      vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "f"(128.0f), "f"(256.0f) : "$t0", "$t1", "$t2", "$t3"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.f[0] = (float)(128.0f);
        op3.f[0] = (float)(256.0f);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        R8.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R9.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R10.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R11.sd[0] = (int32_t)(ee_f2u(op3.f[0]));
        ee_pextlw(&R8, &R9, &R8);
        ee_pextlw(&R10, &R11, &R10);
        ee_pcpyld(&R8, &R10, &R8);
        vu_load_vf(5, &R8);
        vu_upper(U_MUL, 15, 4, 4, 5, BC_NONE);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorCM(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim) // first parameter is not present on the debugging symbols
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2    vf4, VU1_COLOR.fR(%1)

        lw      t0, fVu1AlphaRatio

        qmtc2   t0, vf5 
    
        vmulx.w vf4, vf0, vf5x 

        sqc2    vf4, VU1_PRIM_BUF.fR(%0)
    " : : "r"(pPrim), "r"(&vu1Diffuse), "f"(fVu1AlphaRatio) : "$v1", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op1.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        vu_load_vf(4, EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(5, &R8);
        vu_upper(U_MUL, 1, 4, 0, 5, BC_X);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorIgnore(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim) // first parameter is not present on the debugging symbols
{
#ifndef TARGET_PC
    asm volatile 
    ("
        addi        t0, zero, 128
        
        qmtc2       t0, vf5
        
        vitof0.xyzw vf5, vf5
        
        lw          t1, fVu1AlphaRatio
    
        qmtc2       t1, vf6
    
        vaddx.xyz   vf4, vf0, vf5x
        vmulx.w     vf4, vf0, vf6x 

        sqc2        vf4, VU1_PRIM_BUF.fR(%0)
    " : : "r"(pPrim), "f"(fVu1AlphaRatio) : "$t1"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pPrim);
        R8.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(128));
        vu_load_vf(5, &R8);
        vu_upper(U_ITOF0, 15, 5, 5, 0, BC_NONE);
        R9.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(6, &R9);
        vu_upper(U_ADD, 14, 4, 0, 5, BC_X);
        vu_upper(U_MUL, 1, 4, 0, 6, BC_X);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDif(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)
        lqc2       vf5, VU1_COLOR.fR(%2)

        lw         t0, fVu1AlphaRatio

        vminiw.xyz vf4, vf4, vf0w
        vmaxx.xyz  vf4, vf4, vf0x 
        
        qmtc2      t0, vf6 

        vmul.xyz   vf4, vf4, vf5
        vmulx.w    vf4, vf0, vf6x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "r"(&vu1Diffuse), "f"(fVu1AlphaRatio) : "$v1", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_load_vf(6, &R8);
        vu_upper(U_MUL, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 6, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDifAmb(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim) 
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)

        lw         t0, fVu1AlphaRatio
        
        qmtc2      t0, vf8

        vadd.xyz   vf7, vf4, vf22
        
        vmaxx.xyz  vf7, vf7, vf0x
        vminiw.xyz vf7, vf7, vf0w
        
        vmul.xyz   vf4, vf7, vf20
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "f"(fVu1AlphaRatio) : "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(8, &R8);
        vu_upper(U_ADD, 14, 7, 4, 22, BC_NONE);
        vu_upper(U_MAX, 14, 7, 7, 0, BC_X);
        vu_upper(U_MINI, 14, 7, 7, 0, BC_W);
        vu_upper(U_MUL, 14, 4, 7, 20, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 96.47% matching
void vu1GetVertexColorDifSpe1(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)

        lw         t0, fVu1AlphaRatio
    
        vmove.xyzw vf5, vf4
        vmove.xyzw vf6, vf4
        
        qmtc2      t0, vf8
    
        vsubw.xyz  vf7, vf4, vf0w

        vmaxx.xyz  vf4, vf4, vf0x
        vminiw.xyz vf4, vf4, vf0w
        
        vmaxx.xyz  vf7, vf7, vf0x
        vminiw.xyz vf7, vf7, vf0w
        
        vmul.xyz   vf4, vf4, vf5
        vmul.xyz   vf7, vf7, vf6
        
        vadd.xyz   vf4, vf4, vf7
        
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "f"(fVu1AlphaRatio) : "$t0", "memory"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_move(15, 5, 4);
        vu_move(15, 6, 4);
        vu_load_vf(8, &R8);
        vu_upper(U_SUB, 14, 7, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 7, 7, 0, BC_X);
        vu_upper(U_MINI, 14, 7, 7, 0, BC_W);
        vu_upper(U_MUL, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_MUL, 14, 7, 7, 6, BC_NONE);
        vu_upper(U_ADD, 14, 4, 4, 7, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDifSpe2(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)
        lqc2       vf5, VU1_COLOR.fR(%2)
        lqc2       vf6, VU1_COLOR.fR(%3)
        lqc2       vf7, VU1_COLOR.fR(%4)

        lw         t0, fVu1AlphaRatio
        
        qmtc2      t0, vf8
    
        vadd.xyz   vf5, vf4, vf5
        vsubw.xyz  vf5, vf5, vf0w

        vmaxx.xyz  vf4, vf4, vf0x
        vminiw.xyz vf4, vf4, vf0w
        
        vmaxx.xyz  vf5, vf5, vf0x
        vminiw.xyz vf5, vf5, vf0w
        
        vmul.xyz   vf4, vf4, vf6
        vmul.xyz   vf5, vf5, vf7
        
        vadd.xyz   vf4, vf4, vf5
        
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "r"(&vu1Ambient), "r"(&vu1Diffuse), "r"(&vu1Specula), "f"(fVu1AlphaRatio) : "$v1", "$a2", "$a3", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        ee_reg op4 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.sd[0] = (int32_t)(uintptr_t)(&vu1Ambient);
        op3.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        op4.sd[0] = (int32_t)(uintptr_t)(&vu1Specula);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(6, EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(7, EE_PTR((op4.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(8, &R8);
        vu_upper(U_ADD, 14, 5, 4, 5, BC_NONE);
        vu_upper(U_SUB, 14, 5, 5, 0, BC_W);
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 5, 5, 0, BC_X);
        vu_upper(U_MINI, 14, 5, 5, 0, BC_W);
        vu_upper(U_MUL, 14, 4, 4, 6, BC_NONE);
        vu_upper(U_MUL, 14, 5, 5, 7, BC_NONE);
        vu_upper(U_ADD, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDifSpe3(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)
        lqc2       vf5, VU1_COLOR.fR(%2)
        lqc2       vf6, VU1_COLOR.fR(%3)

        lw         t0, fVu1AlphaRatio
        
        qmtc2      t0, vf8

        addi       t1, zero, 17

        vaddw.xyz  vf7, vf0, vf0w

        l_002D5770:
        vmul.xyz   vf7, vf7, vf4

        addi       t1, t1, -1
        nop
        nop
        nop

        bnez       t1, l_002D5770
        nop

        vmaxx.xyz  vf4, vf4, vf0x
        vminiw.xyz vf4, vf4, vf0w
        
        vmaxx.xyz  vf7, vf7, vf0x
        vminiw.xyz vf7, vf7, vf0w
        
        vmul.xyz   vf4, vf4, vf5
        vmul.xyz   vf7, vf7, vf6
        
        vadd.xyz   vf4, vf4, vf7
        
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "r"(&vu1Diffuse), "r"(&vu1Specula), "f"(fVu1AlphaRatio) : "$v1", "$a2", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        op3.sd[0] = (int32_t)(uintptr_t)(&vu1Specula);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(6, EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(8, &R8);
        R9.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(17));
        vu_upper(U_ADD, 14, 7, 0, 0, BC_W);
    l_002D5770:;
        vu_upper(U_MUL, 14, 7, 7, 4, BC_NONE);
        R9.sd[0] = (int32_t)(R9.w[0] + (uint32_t)(-1));
        if (R9.sd[0] != 0) goto l_002D5770;
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 7, 7, 0, BC_X);
        vu_upper(U_MINI, 14, 7, 7, 0, BC_W);
        vu_upper(U_MUL, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_MUL, 14, 7, 7, 6, BC_NONE);
        vu_upper(U_ADD, 14, 4, 4, 7, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDifSpe1Amb(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)
        lqc2       vf5, VU1_COLOR.fR(%2)
        lqc2       vf6, VU1_COLOR.fR(%3)
        lqc2       vf7, VU1_COLOR.fR(%4)

        lw         t0, fVu1AlphaRatio
        
        qmtc2      t0, vf8
    
        vsubw.xyz  vf9, vf4, vf0w

        vmaxx.xyz  vf9, vf9, vf0x
        vminiw.xyz vf9, vf9, vf0w

        vadd.xyz   vf4, vf4, vf5
        
        vmaxx.xyz  vf4, vf4, vf0x
        vminiw.xyz vf4, vf4, vf0w
        
        vmul.xyz   vf9, vf9, vf7
        vmul.xyz   vf4, vf4, vf6
        
        vadd.xyz   vf4, vf4, vf9
        
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "r"(&vu1Ambient), "r"(&vu1Diffuse), "r"(&vu1Specula), "f"(fVu1AlphaRatio) : "$v1", "$a2", "$a3", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        ee_reg op4 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.sd[0] = (int32_t)(uintptr_t)(&vu1Ambient);
        op3.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        op4.sd[0] = (int32_t)(uintptr_t)(&vu1Specula);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(6, EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(7, EE_PTR((op4.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(8, &R8);
        vu_upper(U_SUB, 14, 9, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 9, 9, 0, BC_X);
        vu_upper(U_MINI, 14, 9, 9, 0, BC_W);
        vu_upper(U_ADD, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MUL, 14, 9, 9, 7, BC_NONE);
        vu_upper(U_MUL, 14, 4, 4, 6, BC_NONE);
        vu_upper(U_ADD, 14, 4, 4, 9, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDifSpe2Amb(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)
        lqc2       vf5, VU1_COLOR.fR(%2)
        lqc2       vf6, VU1_COLOR.fR(%3)
        lqc2       vf7, VU1_COLOR.fR(%4)

        lw         t0, fVu1AlphaRatio
        
        qmtc2      t0, vf8

        vadd.xyz   vf4, vf4, vf5
        vsubw.xyz  vf9, vf4, vf0w

        vmaxx.xyz  vf9, vf9, vf0x
        vminiw.xyz vf9, vf9, vf0w
        
        vmaxx.xyz  vf4, vf4, vf0x
        vminiw.xyz vf4, vf4, vf0w
        
        vmul.xyz   vf9, vf9, vf7
        vmul.xyz   vf4, vf4, vf6
        
        vadd.xyz   vf4, vf4, vf9
        
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "r"(&vu1Ambient), "r"(&vu1Diffuse), "r"(&vu1Specula), "f"(fVu1AlphaRatio) : "$v1", "$a2", "$a3", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        ee_reg op4 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.sd[0] = (int32_t)(uintptr_t)(&vu1Ambient);
        op3.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        op4.sd[0] = (int32_t)(uintptr_t)(&vu1Specula);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(6, EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(7, EE_PTR((op4.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(8, &R8);
        vu_upper(U_ADD, 14, 4, 4, 5, BC_NONE);
        vu_upper(U_SUB, 14, 9, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 9, 9, 0, BC_X);
        vu_upper(U_MINI, 14, 9, 9, 0, BC_W);
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MUL, 14, 9, 9, 7, BC_NONE);
        vu_upper(U_MUL, 14, 4, 4, 6, BC_NONE);
        vu_upper(U_ADD, 14, 4, 4, 9, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching!
void vu1GetVertexColorDifSpe3Amb(VU1_STRIP_BUF* pStrip, VU1_PRIM_BUF* pPrim)
{
#ifndef TARGET_PC
    asm volatile 
    ("
        lqc2       vf4, VU1_STRIP_BUF.fIr(%0)
        lqc2       vf5, VU1_COLOR.fR(%2)
        lqc2       vf6, VU1_COLOR.fR(%3)
        lqc2       vf7, VU1_COLOR.fR(%4)

        lw         t0, fVu1AlphaRatio
        
        qmtc2      t0, vf8

        vadd.xyz   vf4, vf4, vf5

        addi       t1, zero, 17

        vaddw.xyz  vf9, vf0, vf0w

        l_002D58E0:
        vmul.xyz   vf9, vf9, vf4

        addi       t1, t1, -1
        nop
        nop
        nop

        bnez       t1, l_002D58E0
        nop

        vmaxx.xyz  vf4, vf4, vf0x
        vminiw.xyz vf4, vf4, vf0w
        
        vmaxx.xyz  vf9, vf9, vf0x
        vminiw.xyz vf9, vf9, vf0w
        
        vmul.xyz   vf4, vf4, vf6
        vmul.xyz   vf9, vf9, vf7
        
        vadd.xyz   vf4, vf4, vf9
        
        vmulx.w    vf4, vf0, vf8x 

        sqc2       vf4, VU1_PRIM_BUF.fR(%1)
    " : : "r"(pStrip), "r"(pPrim), "r"(&vu1Ambient), "r"(&vu1Diffuse), "r"(&vu1Specula), "f"(fVu1AlphaRatio) : "$v1", "$a2", "$a3", "$t0"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        ee_reg op4 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pStrip);
        op1.sd[0] = (int32_t)(uintptr_t)(pPrim);
        op2.sd[0] = (int32_t)(uintptr_t)(&vu1Ambient);
        op3.sd[0] = (int32_t)(uintptr_t)(&vu1Diffuse);
        op4.sd[0] = (int32_t)(uintptr_t)(&vu1Specula);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fIr)))));
        vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(6, EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        vu_load_vf(7, EE_PTR((op4.w[0] + (uint32_t)(offsetof(VU1_COLOR, fR)))));
        R8.sd[0] = EE_LW((uint32_t)(uintptr_t)&fVu1AlphaRatio);
        vu_load_vf(8, &R8);
        vu_upper(U_ADD, 14, 4, 4, 5, BC_NONE);
        R9.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(17));
        vu_upper(U_ADD, 14, 9, 0, 0, BC_W);
    l_002D58E0:;
        vu_upper(U_MUL, 14, 9, 9, 4, BC_NONE);
        R9.sd[0] = (int32_t)(R9.w[0] + (uint32_t)(-1));
        if (R9.sd[0] != 0) goto l_002D58E0;
        vu_upper(U_MAX, 14, 4, 4, 0, BC_X);
        vu_upper(U_MINI, 14, 4, 4, 0, BC_W);
        vu_upper(U_MAX, 14, 9, 9, 0, BC_X);
        vu_upper(U_MINI, 14, 9, 9, 0, BC_W);
        vu_upper(U_MUL, 14, 4, 4, 6, BC_NONE);
        vu_upper(U_MUL, 14, 9, 9, 7, BC_NONE);
        vu_upper(U_ADD, 14, 4, 4, 9, BC_NONE);
        vu_upper(U_MUL, 1, 4, 0, 8, BC_X);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 4);
    }
#endif
}

// 100% matching! 
void vu1RotTransStripBuf(NJS_MATRIX* pMatrix, NJS_VECTOR* pVector, VU1_STRIP_BUF* pBuf)
{
    float fZ;
    float fIz;
    
    njCalcPoint(pMatrix, pVector, (NJS_POINT3*)&pBuf->fVx);
    
    fIz = 1.0f / pBuf->fVz;
    
    fZ = fVu1Projection * fIz;
    
    pBuf->fSx = fVu1AspectW * (pBuf->fVx * fZ);
    pBuf->fSy = fVu1AspectH * (pBuf->fVy * fZ);
    
    pBuf->fIz = fIz;
    
    pBuf->fFog = njCalcFogPowerEx(fIz);
}

// 100% matching! 
void vu1DrawTriangleStripTransDoubleI(unsigned long ulType, VU1_STRIP_BUF* pS, unsigned short usStripMax, unsigned short usMode)
{ 
    VU1_PRIM_BUF* pP;                              
    unsigned short usStripCnt;                      
    void (*pFunc)(VU1_STRIP_BUF*, VU1_PRIM_BUF*); 
    unsigned int clipflag;                                     
    int flg;                                              

    clipflag = 0; 
    flg = 0;

    InitNodeArraySet(&scissorflip); 
    
    pP = (VU1_PRIM_BUF*)Ps2_DRAW_TMP; 
    
    pFunc = pColorCalcFuncTbl[usMode & 0xF]; 
    
    for (usStripCnt = 0; usStripCnt < usStripMax; usStripCnt++, pS++, pP++) 
    {
        pFunc(pS, pP);
        
#ifndef TARGET_PC
        asm volatile
        ("
            lqc2         vf4, VU1_STRIP_BUF.fSx(%2)   
            lqc2         vf5, VU1_STRIP_BUF.fU(%2)    
            lqc2         vf6, VU1_STRIP_BUF.fVx(%2)    
              
            vmulz.xy     vf5, vf5, vf4z  
              
            vadd.z       vf5, vf0, vf4   
            vsub.w       vf5, vf5, vf5   
              
            sqc2         vf5, VU1_PRIM_BUF.fS(%3)      
          
            lqc2         vf8, VU1_STRIP_BUF.fVx(%2)    
            lqc2         vf10, VU1_STRIP_BUF.fU(%2)   
            lqc2         vf9, VU1_PRIM_BUF.fR(%3)    
        
            vmulax.xyzw  ACC, vf24, vf8x
            
            vmadday.xyzw ACC, vf25, vf8y 
            vmaddaz.xyzw ACC, vf26, vf8z 
            vmaddw.xyzw  vf11, vf27, vf0w 
            
            vclipw.xyz   vf11, vf11w       
            
            vmove.xyzw   vf7, vf6         
            
            vadd.xy      vf7, vf4, vf16  
              
            sqc2         vf7, VU1_PRIM_BUF.fX(%3)   
              
            lw           t1, 0(%1)     
              
            cfc2.ni      t0, vi18        
              
            andi         t0, t0, 0x3F   
            or           t1, t0, t1     
              
            sw           t1, 0(%1)     
              
            lw           t2, SCISSOR.rotflag(%0)   
              
            muli         t3, t2, 64    
          
            addi         t2, t2, 1     
            add          t4, t3, %0      
              
            andi         t2, t2, 0x3   
              
            sqc2         vf8, SCISSOR.triangle.node[0].vertex(t4)    
            sqc2         vf9, SCISSOR.triangle.node[0].color(t4)    
            sqc2         vf10, SCISSOR.triangle.node[0].texUV(t4)   
            sqc2         vf11, SCISSOR.triangle.node[0].clipV(t4) 
              
            addi         t5, t2, -3    
              
            bltz         t5, l_002D5AE0        
            vnop                      
          
            neg          t2, zero   
            
            l_002D5AE0:
            sw           t2, SCISSOR.rotflag(%0)    
        " : : "r"(&scissorflip), "r"(&clipflag), "r"(pS), "r"(pP) : "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "memory");
#else
        { /* asm2c */
            ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R12 = {{0}}, R13 = {{0}};
            const ee_reg ee_zero = {{0}};
            ee_reg op0 = {{0}};
            ee_reg op1 = {{0}};
            ee_reg op2 = {{0}};
            ee_reg op3 = {{0}};
            op0.sd[0] = (int32_t)(uintptr_t)(&scissorflip);
            op1.sd[0] = (int32_t)(uintptr_t)(&clipflag);
            op2.sd[0] = (int32_t)(uintptr_t)(pS);
            op3.sd[0] = (int32_t)(uintptr_t)(pP);
            vu_load_vf(4, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fSx)))));
            vu_load_vf(5, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fU)))));
            vu_load_vf(6, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx)))));
            vu_upper(U_MUL, 12, 5, 5, 4, BC_Z);
            vu_upper(U_ADD, 2, 5, 0, 4, BC_NONE);
            vu_upper(U_SUB, 1, 5, 5, 5, BC_NONE);
            vu_store_vf(EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fS)))), 5);
            vu_load_vf(8, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fVx)))));
            vu_load_vf(10, EE_PTR((op2.w[0] + (uint32_t)(offsetof(VU1_STRIP_BUF, fU)))));
            vu_load_vf(9, EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))));
            vu_upper(U_MULA, 15, 0, 24, 8, BC_X);
            vu_upper(U_MADDA, 15, 0, 25, 8, BC_Y);
            vu_upper(U_MADDA, 15, 0, 26, 8, BC_Z);
            vu_upper(U_MADD, 15, 11, 27, 0, BC_W);
            vu_clipw(11, 11);
            vu_move(15, 7, 6);
            vu_upper(U_ADD, 12, 7, 4, 16, BC_NONE);
            vu_store_vf(EE_PTR((op3.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX)))), 7);
            R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(0)));
            R8.sd[0] = (int32_t)(vu_cfc2(18));
            R8.d[0] = R8.d[0] & (uint64_t)(uint16_t)(63);
            R9.d[0] = R8.d[0] | R9.d[0];
            EE_SW((op1.w[0] + (uint32_t)(0)), R9.d[0]);
            R10.sd[0] = EE_LW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, rotflag))));
            R11.sd[0] = (int32_t)(R10.w[0] * (uint32_t)(64));
            R10.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(1));
            R12.sd[0] = (int32_t)(R11.w[0] + op0.w[0]);
            R10.d[0] = R10.d[0] & (uint64_t)(uint16_t)(3);
            vu_store_vf(EE_PTR((R12.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].vertex)))), 8);
            vu_store_vf(EE_PTR((R12.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].color)))), 9);
            vu_store_vf(EE_PTR((R12.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].texUV)))), 10);
            vu_store_vf(EE_PTR((R12.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].clipV)))), 11);
            R13.sd[0] = (int32_t)(R10.w[0] + (uint32_t)(-3));
            if (R13.sd[0] < 0) goto l_002D5AE0;
            R10.sd[0] = (int32_t)(0u - ee_zero.w[0]);
        l_002D5AE0:;
            EE_SW((op0.w[0] + (uint32_t)(offsetof(SCISSOR, rotflag))), R10.d[0]);
        }
#endif
    
        flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX); 
        
        if (PC_SCISSOR && (((flg & 0x7)) && ((clipflag & 0x3FFFF))) && (usStripCnt >= 2)) 
        { 
            pP->ulKick = 32768;
            
            ScissorTriangle(&scissorflip, &planeset); 
            
            if (scissorflip.in->nodeNum != 0) 
            { 
                DrawScissorPolygonOpaque(&scissorflip, ulType); 
       
                ResetNodeArraySet(&scissorflip); 
            } 
        }
        
        clipflag <<= 6; 
        flg <<= 1; 
    } 
    
    PC_ADDPRIM3DEXI(ulType, Ps2_DRAW_TMP, usStripMax); 
} 

// 100% matching!
void DrawScissorPolygonTrans1P(SCISSOR* scissor, unsigned long ulType)
{
    SCISSOR_NODE* in; 
    VU1_PRIM_BUF* pP; // not from the debugging symbols

    in = scissor->in;
    
#ifndef TARGET_PC
    asm volatile
    ("
        addi     t6, %0, SCISSOR.in
    
        lw       t6, SCISSOR.triangle.node[0](t6)
        
        add      t7, zero, %1
    
        lw       t0, SCISSOR_NODE.nodeNum(t6)
        nop
        
        l_002D4720:
        lqc2     vf4, SCISSOR.triangle.node[0].vertex(t6)
        
        vdiv     Q, vf0w, vf4z
    
        lqc2     vf7, SCISSOR.triangle.node[0].color(t6)
        
        sqc2     vf7, VU1_PRIM_BUF.fR(t7)
    
        vaddz.x  vf5, vf0, vf23z
        
        lqc2     vf6, SCISSOR.triangle.node[0].texUV(t6)
    
        addi     t0, t0, -1
        
        vwaitq
        
        vmulq.x  vf5, vf5, Q
        vmulq.xy vf6, vf6, Q
        
        vaddq.z  vf6, vf0, Q
        
        vsub.w   vf6, vf6, vf6
        
        vmulx.xy vf4, vf4, vf5x
        
        sqc2     vf6, VU1_PRIM_BUF.fS(t7)
    
        vmul.xy  vf4, vf4, vf17
        
        vadd.xy  vf4, vf4, vf16
        
        addi     t6, t6, sizeof(NODE)
    
        sqc2     vf4, VU1_PRIM_BUF.fX(t7)
    
        addi     t7, t7, sizeof(VU1_PRIM_BUF)
    
        bnez     t0, l_002D4720
        vnop
    " : : "r"(scissor), "r"(vu1ScessorBuf) : "t7"
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R14 = {{0}}, R15 = {{0}};
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(scissor);
        op1.sd[0] = (int32_t)(uintptr_t)(vu1ScessorBuf);
        R14.sd[0] = (int32_t)(op0.w[0] + (uint32_t)(offsetof(SCISSOR, in)));
        R14.sd[0] = EE_LW((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0]))));
        R15.sd[0] = (int32_t)(ee_zero.w[0] + op1.w[0]);
        R8.sd[0] = EE_LW((R14.w[0] + (uint32_t)(offsetof(SCISSOR_NODE, nodeNum))));
    l_002D4720:;
        vu_load_vf(4, EE_PTR((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].vertex)))));
        vu_div(0, 3, 4, 2);
        vu_load_vf(7, EE_PTR((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].color)))));
        vu_store_vf(EE_PTR((R15.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fR)))), 7);
        vu_upper(U_ADD, 8, 5, 0, 23, BC_Z);
        vu_load_vf(6, EE_PTR((R14.w[0] + (uint32_t)(offsetof(SCISSOR, triangle.node[0].texUV)))));
        R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-1));
        vu_upper(U_MUL, 8, 5, 5, 0, BC_Q);
        vu_upper(U_MUL, 12, 6, 6, 0, BC_Q);
        vu_upper(U_ADD, 2, 6, 0, 0, BC_Q);
        vu_upper(U_SUB, 1, 6, 6, 6, BC_NONE);
        vu_upper(U_MUL, 12, 4, 4, 5, BC_X);
        vu_store_vf(EE_PTR((R15.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fS)))), 6);
        vu_upper(U_MUL, 12, 4, 4, 17, BC_NONE);
        vu_upper(U_ADD, 12, 4, 4, 16, BC_NONE);
        R14.sd[0] = (int32_t)(R14.w[0] + (uint32_t)((sizeof(NODE))));
        vu_store_vf(EE_PTR((R15.w[0] + (uint32_t)(offsetof(VU1_PRIM_BUF, fX)))), 4);
        R15.sd[0] = (int32_t)(R15.w[0] + (uint32_t)((sizeof(VU1_PRIM_BUF))));
        if (R8.sd[0] != 0) goto l_002D4720;
    }
#endif

    ulType = (ulType & 0xFFFDFFFFFFFFFFFF) | 0x6800000000000;
    
    pP = vu1ScessorBuf;
    
    Ps2AddPrim3DEx1P(ulType, pP, in->nodeNum);
}

// 100% matching!
void vu1DrawTriangleStripTransDouble1P(unsigned long ulType, VU1_STRIP_BUF* pS, unsigned short usStripMax, unsigned short usMode)
{ 
    VU1_PRIM_BUF* pP;                                      
    float fIz;                                    
    float sam;                                    
    unsigned short usStripCnt;                                   
    void (*pFunc)(VU1_STRIP_BUF*, VU1_PRIM_BUF*); 
    unsigned int clipflag;                                                           
    int flg;                                                             

    clipflag = 0; 
    flg = 0;

    InitNodeArraySet(&scissorflip); 
    
    pP = (VU1_PRIM_BUF*)Ps2_DRAW_TMP; 

    sam = 0;
    
    pFunc = pColorCalcFuncTbl[usMode & 0xF]; 
    
    for (usStripCnt = 0; usStripCnt < usStripMax; usStripCnt++)
    {
        sam += pS[usStripCnt].fVz;
    }
    
    Ps2AddPrimPrio = sam / usStripMax;
    
    for (usStripCnt = 0; usStripCnt < usStripMax; usStripCnt++, pS++, pP++) 
    {
        pFunc(pS, pP);

        fIz = pS->fIz;
       
        pP->fS = pS->fU * fIz;
        pP->fT = pS->fV * fIz;
        pP->fQ = fIz;
        
        pP->fX = fVu1OffsetX + pS->fSx;
        pP->fY = fVu1OffsetY + pS->fSy;
        pP->fZ = pS->fVz;
        
        pP->fF = pS->fFog;

        pP->ulKick = 0;

        clipflag |= _Clip_ViewVolume(node.clipV, ClipMatrix2[0], &pS->fVx) & 0x3F;
    
        flg |= _Check_DisplayAreaPoint((NJS_VECTOR*)&pP->fX); 

        *(u_long128*)&node.texUV = *(u_long128*)&pS->fU;
        *(u_long128*)&node.vertex = *(u_long128*)&pS->fVx;
        *(u_long128*)&node.color = *(u_long128*)&pP->fR;
        
        PushTriangleNodeArray(&scissorflip, &node);

        if (PC_SCISSOR && (((flg & 0x7)) && ((clipflag & 0x3FFFF))) && (usStripCnt >= 2)) 
        { 
            pP->ulKick = 32768;
            
            ScissorTriangle(&scissorflip, &planeset); 
            
            if (scissorflip.in->nodeNum != 0) 
            { 
                DrawScissorPolygonTrans1P(&scissorflip, ulType); 
       
                ResetNodeArraySet(&scissorflip); 
            } 
        }
        
        clipflag <<= 6; 
        flg <<= 1; 
    } 
    
    PC_ADDPRIM3DEX1P(ulType, Ps2_DRAW_TMP, usStripMax); 
} 
