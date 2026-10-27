#ifdef TARGET_PC
#include "ee_asm.h"
#endif
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaView.h"
#include "../../../ps2/veronica/prog/main.h"

int lNaMatIsUnitMatrix;
int lNaMatMatrixStuckMax;
int lNaMatMatrixStuckCnt;
NJS_MATRIX* pNaMatMatrixStuckPtr;
NJS_MATRIX* pNaMatMatrixStuckTop;
/* unused below */
/*NJS_MATRIX TempMatrix0;
NJS_MATRIX TempMatrix1;*/

extern void VU0_INIT_CALC_PROCESS() __attribute__((section(".vutext")));

// 98.85% matching
void	njInitMatrix(NJS_MATRIX *m, Sint32 n, Int flag)
{
    register float pi;

    pNaMatMatrixStuckTop = m;
    pNaMatMatrixStuckPtr = m; 
    
    lNaMatMatrixStuckCnt = 0; 
    lNaMatMatrixStuckMax = n; 
    
    lNaMatIsUnitMatrix = flag; 
    
    pi = 3.141592f;
    
#ifndef TARGET_PC
    asm volatile 
    {
    .set noreorder
    
        vaddw.xyz  vf1, vf0, vf0w   
        
        vmul.w     vf1, vf0, vf0                     

        sub        t5, t5, t5     
        
        addi       t0, t5, 0x80                       
        addi       t1, t5, 0x100          
        
        mult       zero, t1, t1        
        
        mflo       t2               
        
        mfc1       t3, pi           
        
        qmtc2      t0, vf3                          
        qmtc2      t1, vf9                               
        qmtc2      t2, vf10                             
        qmtc2      t3, vf11                              

        vitof0.xyzw vf3, vf3                        
        vitof0.xyzw vf9, vf9                 
        vitof0.xyzw vf10, vf10         
        
        vaddx.y    vf3, vf0, vf9x                     
        vaddx.z    vf3, vf0, vf10x       
        
        vmulx.w    vf3, vf0, vf11x        
        
        vaddw.xyzw vf2, vf0, vf0w        
        
        vdiv       Q, vf0w, vf2w        
        
        vaddw.xyzw vf2, vf2, vf0w                    
        vaddw.xyzw vf2, vf2, vf0w                     
        vaddw.yzw  vf2, vf2, vf0w                     
        vaddw.zw   vf2, vf2, vf0w   
        
        vwaitq                     
        
        vmulq.w    vf2, vf0, Q                        
        
        addi       t0, zero, 0xFFF      
        
        qmtc2      t0, vf4                               
        
        vitof0.xyz vf4, vf4      
        
        vaddx.z    vf16, vf0, vf4x          
        
        vcallms    VU0_INIT_CALC_PROCESS                          
        
        nop       
    
    .set reorder
    }
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R13 = {{0}};
        uint64_t ee_prod;
        ee_reg ee_lo = {{0}}, ee_hi = {{0}};
        ee_reg ee_dummy;
        const ee_reg ee_zero = {{0}};
        ee_reg cv_pi = {{0}};
        cv_pi.f[0] = pi;
        vu_upper(U_ADD, 14, 1, 0, 0, BC_W);
        vu_upper(U_MUL, 1, 1, 0, 0, BC_NONE);
        R13.sd[0] = (int32_t)(R13.w[0] + -R13.w[0]);
        R8.sd[0] = (int32_t)(R13.w[0] + (uint32_t)(128));
        R9.sd[0] = (int32_t)(R13.w[0] + (uint32_t)(256));
        ee_prod = (uint64_t)((int64_t)(int32_t)R9.w[0] * (int64_t)(int32_t)R9.w[0]);
        ee_lo.sd[0] = (int32_t)(uint32_t)ee_prod; ee_hi.sd[0] = (int32_t)(uint32_t)(ee_prod >> 32);
        ee_dummy.sd[0] = ee_lo.sd[0];
        R10.d[0] = ee_lo.d[0];
        R11.sd[0] = (int32_t)(ee_f2u(cv_pi.f[0]));
        vu_load_vf(3, &R8);
        vu_load_vf(9, &R9);
        vu_load_vf(10, &R10);
        vu_load_vf(11, &R11);
        vu_upper(U_ITOF0, 15, 3, 3, 0, BC_NONE);
        vu_upper(U_ITOF0, 15, 9, 9, 0, BC_NONE);
        vu_upper(U_ITOF0, 15, 10, 10, 0, BC_NONE);
        vu_upper(U_ADD, 4, 3, 0, 9, BC_X);
        vu_upper(U_ADD, 2, 3, 0, 10, BC_X);
        vu_upper(U_MUL, 1, 3, 0, 11, BC_X);
        vu_upper(U_ADD, 15, 2, 0, 0, BC_W);
        vu_div(0, 3, 2, 3);
        vu_upper(U_ADD, 15, 2, 2, 0, BC_W);
        vu_upper(U_ADD, 15, 2, 2, 0, BC_W);
        vu_upper(U_ADD, 7, 2, 2, 0, BC_W);
        vu_upper(U_ADD, 3, 2, 2, 0, BC_W);
        vu_upper(U_MUL, 1, 2, 0, 0, BC_Q);
        R8.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(4095));
        vu_load_vf(4, &R8);
        vu_upper(U_ITOF0, 14, 4, 4, 0, BC_NONE);
        vu_upper(U_ADD, 2, 16, 0, 4, BC_X);
        vu0_callms(VU0_ADDR_VU0_INIT_CALC_PROCESS);
    }
#endif
}

// 100% matching
void	njCalcPoints(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd, Int num)
{
	if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lw          a4,   0(%3)
        
        ldl         a6, 0x7(%1)
        ldr         a6,   0(%1)

        lw          a7, NJS_VECTOR.z(%1) 

        pcpyld      a6, a7, a6

        qmtc2.ni    a6, vf4

        lqc2        vf5,    0(%0)
        lqc2        vf6, 0x10(%0)
        lqc2        vf7, 0x20(%0)
        lqc2        vf8, 0x30(%0)

        l_002D67C4:
        addi        %1, %1, 12

        vmulax.xyz  ACC,  vf5, vf4
        
        vmadday.xyz ACC,  vf6, vf4
        vmaddaz.xyz ACC,  vf7, vf4
        vmaddw.xyz  vf18, vf8, vf0

        addi        a4, a4, -1

        ldl         a6, 0x7(%1)
        ldr         a6,   0(%1)

        lw          a7, NJS_VECTOR.z(%1) 

        pcpyld      a6, a7, a6

        qmtc2.ni    a6, vf4
        qmfc2.ni    t5, vf18

        pcpyud      t6, t5, t5

        sdl         t5, 0x7(%2)
        sdr         t5,   0(%2)

        sw          t6, NJS_VECTOR.z(%2) 

        addi        %2, %2, 12

        bgtz        a4, l_002D67C4
        vnop
    .set reorder
    " : : "r"(m), "r"(ps), "r"(pd), "r"(&num) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R10 = {{0}}, R11 = {{0}}, R13 = {{0}}, R14 = {{0}};
        int ee_cond;
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(ps);
        op2.sd[0] = (int32_t)(uintptr_t)(pd);
        op3.sd[0] = (int32_t)(uintptr_t)(&num);
        R8.sd[0] = EE_LW((op3.w[0] + (uint32_t)(0)));
        R10.d[0] = ee_ldl(R10.d[0], (op1.w[0] + (uint32_t)(7)));
        R10.d[0] = ee_ldr(R10.d[0], (op1.w[0] + (uint32_t)(0)));
        R11.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R10, &R11, &R10);
        vu_load_vf(4, &R10);
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(48))));
    l_002D67C4:;
        op1.sd[0] = (int32_t)(op1.w[0] + (uint32_t)(12));
        vu_upper(U_MULA, 14, 0, 5, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 6, 4, BC_Y);
        vu_upper(U_MADDA, 14, 0, 7, 4, BC_Z);
        vu_upper(U_MADD, 14, 18, 8, 0, BC_W);
        R8.sd[0] = (int32_t)(R8.w[0] + (uint32_t)(-1));
        R10.d[0] = ee_ldl(R10.d[0], (op1.w[0] + (uint32_t)(7)));
        R10.d[0] = ee_ldr(R10.d[0], (op1.w[0] + (uint32_t)(0)));
        R11.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R10, &R11, &R10);
        vu_load_vf(4, &R10);
        vu_store_vf(&R13, 18);
        ee_pcpyud(&R14, &R13, &R13);
        ee_sdl((op2.w[0] + (uint32_t)(7)), R13.d[0]);
        ee_sdr((op2.w[0] + (uint32_t)(0)), R13.d[0]);
        EE_SW((op2.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))), R14.d[0]);
        op2.sd[0] = (int32_t)(op2.w[0] + (uint32_t)(12));
        ee_cond = (R8.sd[0] > 0);
        if (ee_cond) goto l_002D67C4;
    }
#endif
}

// 100% matching! 
void    njGetTranslation(NJS_MATRIX *m, NJS_POINT3 *p)
{
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile 
    ("
    .set noreorder
        lw t0, 0x30(%0)
        lw t1, 0x34(%0)
        lw t2, 0x38(%0)
        
        sw t0, NJS_POINT3.x(%1)
        sw t1, NJS_POINT3.y(%1)
        sw t2, NJS_POINT3.z(%1)
    .set reorder
    " : : "r"(m), "r"(p) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(p);
        R8.sd[0] = EE_LW((op0.w[0] + (uint32_t)(48)));
        R9.sd[0] = EE_LW((op0.w[0] + (uint32_t)(52)));
        R10.sd[0] = EE_LW((op0.w[0] + (uint32_t)(56)));
        EE_SW((op1.w[0] + (uint32_t)(offsetof(NJS_POINT3, x))), R8.d[0]);
        EE_SW((op1.w[0] + (uint32_t)(offsetof(NJS_POINT3, y))), R9.d[0]);
        EE_SW((op1.w[0] + (uint32_t)(offsetof(NJS_POINT3, z))), R10.d[0]);
    }
#endif
}

// 100% matching!
void    njUnitTransPortion(NJS_MATRIX *m)
{
	if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile 
    ("
    .set noreorder
        sqc2 vf0, 0x30(%0)
    .set reorder
    " : : "r"(m) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(48))), 0);
    }
#endif
}

// 100% matching! 
void    njUnitRotPortion(NJS_MATRIX *m)
{
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile 
    ("
    .set noreorder
        vmulw.xyzw vf4, vf0, vf0w
        
        vmr32.xyzw vf5, vf4
        vmr32.xyzw vf6, vf5
        vmr32.xyzw vf7, vf6
        
        sqc2       vf5, 0x20(%0)
        sqc2       vf6, 0x10(%0)
        sqc2       vf7,  0x0(%0)
    .set reorder
    " : : "r"(m) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        vu_upper(U_MUL, 15, 4, 0, 0, BC_W);
        vu_mr32(15, 5, 4);
        vu_mr32(15, 6, 5);
        vu_mr32(15, 7, 6);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 5);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 6);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 7);
    }
#endif
}

// 100% matching!
void    njClearMatrix()
{
    lNaMatMatrixStuckCnt = 0;

    pNaMatMatrixStuckPtr = pNaMatMatrixStuckTop;
    
    njSetMatrix(NULL, &NaViwViewMatrix);
}

// 100% matching!
Bool	njPushMatrix(NJS_MATRIX *m)
{
	NJS_MATRIX* fpSrc;
    NJS_MATRIX* fpDst;

    if (lNaMatMatrixStuckMax <= lNaMatMatrixStuckCnt) 
    {
        return FALSE;
    }
    
    lNaMatMatrixStuckCnt++;

    if (m == NULL) 
    {
        fpSrc = pNaMatMatrixStuckPtr;
    }
    else 
    {
        fpSrc = m;
    }

    pNaMatMatrixStuckPtr++;
    
    fpDst = pNaMatMatrixStuckPtr;
    
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2 vf4, 0(%0) 
        lqc2 vf5, 0x10(%0) 
        lqc2 vf6, 0x20(%0) 
        lqc2 vf7, 0x30(%0) 
    
        sqc2 vf4, 0(%1) 
        sqc2 vf5, 0x10(%1) 
        sqc2 vf6, 0x20(%1) 
        sqc2 vf7, 0x30(%1) 
    .set reorder
    " : : "r"(fpSrc), "r"(fpDst) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(fpSrc);
        op1.sd[0] = (int32_t)(uintptr_t)(fpDst);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(0))), 4);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(16))), 5);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(32))), 6);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(48))), 7);
    }
#endif

    return TRUE;
}

// 100% matching!
Bool	njPopMatrix(Uint32 n)
{
    int lNumber;

    lNumber = lNaMatMatrixStuckCnt - n;
    
    if (lNumber < 0) 
    {
        return FALSE;
    }
    
    lNaMatMatrixStuckCnt = lNumber;
    
    pNaMatMatrixStuckPtr -= n;
    
    return TRUE;
}

// 100% matching!
void	njUnitMatrix(register NJS_MATRIX *m)
{
    if (m == NULL) 
    {
        m = pNaMatMatrixStuckPtr;
    }
    
#ifndef TARGET_PC
    asm volatile 
    {
    .set noreorder

        vmulw.xyzw $vf4xyzw, $vf0xyzw, $vf0w

        vmr32.xyzw $vf5xyzw, $vf4xyzw
        vmr32.xyzw $vf6xyzw, $vf5xyzw
        vmr32.xyzw $vf7xyzw, $vf6xyzw
    
        sqc2       $vf4, 0x30(m)
        sqc2       $vf5, 0x20(m)
        sqc2       $vf6, 0x10(m) 
        sqc2       $vf7,  0x0(m)
        
    .set reorder
    }
#else
    { /* asm2c */
        ee_reg cv_m = {{0}};
        cv_m.sd[0] = (int32_t)(uintptr_t)(m);
        vu_upper(U_MUL, 15, 4, 0, 0, BC_W);
        vu_mr32(15, 5, 4);
        vu_mr32(15, 6, 5);
        vu_mr32(15, 7, 6);
        vu_store_vf(EE_PTR((cv_m.w[0] + (uint32_t)(48))), 4);
        vu_store_vf(EE_PTR((cv_m.w[0] + (uint32_t)(32))), 5);
        vu_store_vf(EE_PTR((cv_m.w[0] + (uint32_t)(16))), 6);
        vu_store_vf(EE_PTR((cv_m.w[0] + (uint32_t)(0))), 7);
    }
#endif
}

// 100% matching!
void	njSetMatrix(NJS_MATRIX *md, NJS_MATRIX *ms)
{
    register NJS_MATRIX* fpSrc; 
    register NJS_MATRIX* fpDst; 

    fpSrc = ms;
    fpDst = md ? md : pNaMatMatrixStuckPtr;

#ifndef TARGET_PC
    asm volatile 
    {
    .set noreorder
        
        lqc2       $vf4, 0x0(fpSrc) 
        lqc2       $vf5, 0x10(fpSrc) 
        lqc2       $vf6, 0x20(fpSrc) 
        lqc2       $vf7, 0x30(fpSrc) 
    
        sqc2       $vf4, 0x0(fpDst) 
        sqc2       $vf5, 0x10(fpDst) 
        sqc2       $vf6, 0x20(fpDst) 
        sqc2       $vf7, 0x30(fpDst) 
        
    .set reorder
    }
#else
    { /* asm2c */
        ee_reg cv_fpDst = {{0}};
        ee_reg cv_fpSrc = {{0}};
        cv_fpDst.sd[0] = (int32_t)(uintptr_t)(fpDst);
        cv_fpSrc.sd[0] = (int32_t)(uintptr_t)(fpSrc);
        vu_load_vf(4, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(32))));
        vu_load_vf(7, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(48))));
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(0))), 4);
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(16))), 5);
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(32))), 6);
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(48))), 7);
    }
#endif
}

// 100% matching!
void njSetMatrixCN(NJS_MATRIX* pMat)
{
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2 vf28,  0x0(%0) 
        lqc2 vf29, 0x10(%0) 
        lqc2 vf30, 0x20(%0) 
        lqc2 vf31, 0x30(%0) 
    .set reorder 
    " : : "r"(pMat) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pMat);
        vu_load_vf(28, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(29, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(30, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(31, EE_PTR((op0.w[0] + (uint32_t)(48))));
    }
#endif
}

// 100% matching!
void	njGetMatrix(NJS_MATRIX *m)
{
    register NJS_MATRIX* fpSrc; 
    register NJS_MATRIX* fpDst; 

    fpSrc = pNaMatMatrixStuckPtr;
    fpDst = m; 
    
#ifndef TARGET_PC
    asm volatile 
    {
    .set noreorder
        
        lqc2       $vf4, 0x0(fpSrc)
        lqc2       $vf5, 0x10(fpSrc)
        lqc2       $vf6, 0x20(fpSrc)
        lqc2       $vf7, 0x30(fpSrc)
    
        sqc2       $vf4, 0x0(fpDst)
        sqc2       $vf5, 0x10(fpDst)
        sqc2       $vf6, 0x20(fpDst)
        sqc2       $vf7, 0x30(fpDst)
        
    .set reorder
    }
#else
    { /* asm2c */
        ee_reg cv_fpDst = {{0}};
        ee_reg cv_fpSrc = {{0}};
        cv_fpDst.sd[0] = (int32_t)(uintptr_t)(fpDst);
        cv_fpSrc.sd[0] = (int32_t)(uintptr_t)(fpSrc);
        vu_load_vf(4, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(32))));
        vu_load_vf(7, EE_PTR((cv_fpSrc.w[0] + (uint32_t)(48))));
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(0))), 4);
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(16))), 5);
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(32))), 6);
        vu_store_vf(EE_PTR((cv_fpDst.w[0] + (uint32_t)(48))), 7);
    }
#endif
}

// 100% matching!
void	njMultiMatrix(NJS_MATRIX *md, NJS_MATRIX *ms)
{
    if (md == NULL)
    {
        md = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2        vf4,  0(%0)
        lqc2        vf5,  0x10(%0)
        lqc2        vf6,  0x20(%0)
        lqc2        vf7,  0x30(%0)
        lqc2        vf8,  0(%1)
        lqc2        vf9,  0x10(%1)
        lqc2        vf10, 0x20(%1)
        lqc2        vf11, 0x30(%1)

        vmulax.xyz  ACC,  vf4, vf8
        
        vmadday.xyz ACC,  vf5, vf8
        vmaddz.xyz  vf8,  vf6, vf8
        
        vmulax.xyz  ACC,  vf4, vf9

        vmadday.xyz ACC,  vf5, vf9
        vmaddz.xyz  vf9,  vf6, vf9
        
        vmulax.xyz  ACC,  vf4, vf10

        vmadday.xyz ACC,  vf5, vf10
        vmaddz.xyz  vf10, vf6, vf10
        
        vmulax.xyz  ACC,  vf4, vf11

        vmadday.xyz ACC,  vf5, vf11
        vmaddaz.xyz ACC,  vf6, vf11
        vmaddw.xyz  vf11, vf7, vf0

        sqc2        vf8,  0x0(%0)
        sqc2        vf9,  0x10(%0)
        sqc2        vf10, 0x20(%0)
        sqc2        vf11, 0x30(%0)
    .set reorder
    " : : "r"(md), "r"(ms) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(md);
        op1.sd[0] = (int32_t)(uintptr_t)(ms);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_load_vf(8, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(9, EE_PTR((op1.w[0] + (uint32_t)(16))));
        vu_load_vf(10, EE_PTR((op1.w[0] + (uint32_t)(32))));
        vu_load_vf(11, EE_PTR((op1.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 14, 0, 4, 8, BC_X);
        vu_upper(U_MADDA, 14, 0, 5, 8, BC_Y);
        vu_upper(U_MADD, 14, 8, 6, 8, BC_Z);
        vu_upper(U_MULA, 14, 0, 4, 9, BC_X);
        vu_upper(U_MADDA, 14, 0, 5, 9, BC_Y);
        vu_upper(U_MADD, 14, 9, 6, 9, BC_Z);
        vu_upper(U_MULA, 14, 0, 4, 10, BC_X);
        vu_upper(U_MADDA, 14, 0, 5, 10, BC_Y);
        vu_upper(U_MADD, 14, 10, 6, 10, BC_Z);
        vu_upper(U_MULA, 14, 0, 4, 11, BC_X);
        vu_upper(U_MADDA, 14, 0, 5, 11, BC_Y);
        vu_upper(U_MADDA, 14, 0, 6, 11, BC_Z);
        vu_upper(U_MADD, 14, 11, 7, 0, BC_W);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 9);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 10);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(48))), 11);
    }
#endif
}

// 100% matching!
void	njTranslate(NJS_MATRIX *m, Float x, Float y, Float z)
{
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        mfc1        t0, %1 
        mfc1        t1, %2
        mfc1        t2, %3
        
        qmtc2       t0, vf4
        qmtc2       t1, vf5
        qmtc2       t2, vf6
    
        lqc2        vf7,  0(%0)
        lqc2        vf8,  0x10(%0)
        lqc2        vf9,  0x20(%0)
        lqc2        vf10, 0x30(%0)

        vmulax.xyz  ACC,  vf7,  vf4
        
        vmaddax.xyz ACC,  vf8,  vf5
        vmaddax.xyz ACC,  vf9,  vf6
        vmaddw.xyz  vf11, vf10, vf0
    
        sqc2        vf11, 0x30(%0)
    .set reorder
    " : : "r"(m), "f"(x), "f"(y), "f"(z) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.f[0] = (float)(x);
        op2.f[0] = (float)(y);
        op3.f[0] = (float)(z);
        R8.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R9.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R10.sd[0] = (int32_t)(ee_f2u(op3.f[0]));
        vu_load_vf(4, &R8);
        vu_load_vf(5, &R9);
        vu_load_vf(6, &R10);
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(9, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(10, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 14, 0, 7, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 8, 5, BC_X);
        vu_upper(U_MADDA, 14, 0, 9, 6, BC_X);
        vu_upper(U_MADD, 14, 11, 10, 0, BC_W);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(48))), 11);
    }
#endif
}

// 100% matching!
void	njTranslateV(NJS_MATRIX *m, NJS_VECTOR *v)
{
	if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl         a4, 0x7(%1)
        ldr         a4,   0(%1)

        lw          a5, NJS_VECTOR.z(%1) 

        pcpyld      a4, a5, a4

        qmtc2.ni    a4, vf4

        lqc2        vf5, 0(%0)
        lqc2        vf6, 0x10(%0)
        lqc2        vf7, 0x20(%0)
        lqc2        vf8, 0x30(%0)

        vmulax.xyz  ACC,  vf5, vf4x
        
        vmadday.xyz ACC,  vf6, vf4y
        vmaddaz.xyz ACC,  vf7, vf4z
        vmaddw.xyz  vf9,  vf8, vf0w
    
        sqc2        vf9,  0x30(%0)
    .set reorder
    " : : "r"(m), "r"(v) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(v);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 14, 0, 5, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 6, 4, BC_Y);
        vu_upper(U_MADDA, 14, 0, 7, 4, BC_Z);
        vu_upper(U_MADD, 14, 9, 8, 0, BC_W);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(48))), 9);
    }
#endif
}

// 100% matching!
void	njRotateX(NJS_MATRIX *m, Angle ang)
{
    float fSin;
    float fCos;

    ang &= 0xFFFF;
    
    njSinCos(ang, &fSin, &fCos);
    
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        mfc1      t6, %1 
        mfc1      t7, %2
        
        qmtc2     t6, vf11
        qmtc2     t7, vf12
    
        lqc2      vf4, 0x10(%0)
        lqc2      vf5, 0x20(%0)

        vsub.x    vf10, vf0, vf11
        
        vmulx.xyz vf6, vf4, vf12
        vmulx.xyz vf7, vf5, vf11
        vmulx.xyz vf4, vf4, vf10
        vmulx.xyz vf5, vf5, vf12

        vadd.xyz  vf8, vf6, vf7
        vadd.xyz  vf9, vf4, vf5

        sqc2      vf8, 0x10(%0)
        sqc2      vf9, 0x20(%0)
    .set reorder
    " : : "r"(m), "f"(fSin), "f"(fCos) : 
    );
#else
    { /* asm2c */
        ee_reg R14 = {{0}}, R15 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.f[0] = (float)(fSin);
        op2.f[0] = (float)(fCos);
        R14.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R15.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        vu_load_vf(11, &R14);
        vu_load_vf(12, &R15);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_SUB, 8, 10, 0, 11, BC_NONE);
        vu_upper(U_MUL, 14, 6, 4, 12, BC_X);
        vu_upper(U_MUL, 14, 7, 5, 11, BC_X);
        vu_upper(U_MUL, 14, 4, 4, 10, BC_X);
        vu_upper(U_MUL, 14, 5, 5, 12, BC_X);
        vu_upper(U_ADD, 14, 8, 6, 7, BC_NONE);
        vu_upper(U_ADD, 14, 9, 4, 5, BC_NONE);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 9);
    }
#endif
}

// 100% matching!
void	njRotateY(NJS_MATRIX *m, Angle ang)
{
    float fSin;
    float fCos;

    ang &= 0xFFFF;
    
    njSinCos(ang, &fSin, &fCos);
    
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        mfc1      t6, %1 
        mfc1      t7, %2
        
        qmtc2     t6, vf11
        qmtc2     t7, vf12
    
        lqc2      vf4, 0x0(%0)
        lqc2      vf5, 0x20(%0)

        vsub.x    vf10, vf0, vf11
        
        vmulx.xyz vf6, vf4, vf12
        vmulx.xyz vf7, vf5, vf10
        vmulx.xyz vf4, vf4, vf11
        vmulx.xyz vf5, vf5, vf12

        vadd.xyz  vf8, vf6, vf7
        vadd.xyz  vf9, vf4, vf5

        sqc2      vf8, 0x0(%0)
        sqc2      vf9, 0x20(%0)
    .set reorder
    " : : "r"(m), "f"(fSin), "f"(fCos) : 
    );
#else
    { /* asm2c */
        ee_reg R14 = {{0}}, R15 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.f[0] = (float)(fSin);
        op2.f[0] = (float)(fCos);
        R14.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R15.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        vu_load_vf(11, &R14);
        vu_load_vf(12, &R15);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_SUB, 8, 10, 0, 11, BC_NONE);
        vu_upper(U_MUL, 14, 6, 4, 12, BC_X);
        vu_upper(U_MUL, 14, 7, 5, 10, BC_X);
        vu_upper(U_MUL, 14, 4, 4, 11, BC_X);
        vu_upper(U_MUL, 14, 5, 5, 12, BC_X);
        vu_upper(U_ADD, 14, 8, 6, 7, BC_NONE);
        vu_upper(U_ADD, 14, 9, 4, 5, BC_NONE);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 9);
    }
#endif
}

// 100% matching!
void	njRotateZ(NJS_MATRIX *m, Angle ang)
{
    float fSin;
    float fCos;

    ang &= 0xFFFF;
    
    njSinCos(ang, &fSin, &fCos);
    
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        mfc1      t6, %1 
        mfc1      t7, %2
        
        qmtc2     t6, vf11
        qmtc2     t7, vf12
    
        lqc2      vf4, 0x0(%0)
        lqc2      vf5, 0x10(%0)

        vsub.x    vf10, vf0, vf11
        
        vmulx.xyz vf6, vf4, vf12
        vmulx.xyz vf7, vf5, vf11
        vmulx.xyz vf4, vf4, vf10
        vmulx.xyz vf5, vf5, vf12

        vadd.xyz  vf8, vf6, vf7
        vadd.xyz  vf9, vf4, vf5

        sqc2      vf8, 0x0(%0)
        sqc2      vf9, 0x10(%0)
    .set reorder
    " : : "r"(m), "f"(fSin), "f"(fCos) : 
    );
#else
    { /* asm2c */
        ee_reg R14 = {{0}}, R15 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.f[0] = (float)(fSin);
        op2.f[0] = (float)(fCos);
        R14.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R15.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        vu_load_vf(11, &R14);
        vu_load_vf(12, &R15);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_upper(U_SUB, 8, 10, 0, 11, BC_NONE);
        vu_upper(U_MUL, 14, 6, 4, 12, BC_X);
        vu_upper(U_MUL, 14, 7, 5, 11, BC_X);
        vu_upper(U_MUL, 14, 4, 4, 10, BC_X);
        vu_upper(U_MUL, 14, 5, 5, 12, BC_X);
        vu_upper(U_ADD, 14, 8, 6, 7, BC_NONE);
        vu_upper(U_ADD, 14, 9, 4, 5, BC_NONE);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 9);
    }
#endif
}

// 100% matching!
void	njRotateXYZ(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz)
{
    if (m == NULL) 
    {
        m = pNaMatMatrixStuckPtr;
    }
    
    njRotateZ(m, angz);
    njRotateY(m, angy);
    njRotateX(m, angx);
}

// 100% matching!
void njRotXYZ(NJS_MATRIX* pMatrix, int lAngleX, int lAngleY, int lAngleZ)
{
    float fSin;
    float fCos;

    lAngleX &= 0xFFFF;
    
    njSinCos(lAngleX, &fSin, &fCos);
    
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2      vf4,  0x0(%0)
        lqc2      vf5, 0x10(%0)
        lqc2      vf6, 0x20(%0)

        vsub.x    vf10, vf0, vf11
        
        vmulx.xyz vf7, vf5, vf12
        vmulx.xyz vf8, vf6, vf11
        vmulx.xyz vf5, vf5, vf10
        vmulx.xyz vf6, vf6, vf12

        vadd.xyz  vf6, vf5, vf6
        vadd.xyz  vf5, vf7, vf8
    .set reorder
    " : : "r"(pMatrix) :  
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pMatrix);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_SUB, 8, 10, 0, 11, BC_NONE);
        vu_upper(U_MUL, 14, 7, 5, 12, BC_X);
        vu_upper(U_MUL, 14, 8, 6, 11, BC_X);
        vu_upper(U_MUL, 14, 5, 5, 10, BC_X);
        vu_upper(U_MUL, 14, 6, 6, 12, BC_X);
        vu_upper(U_ADD, 14, 6, 5, 6, BC_NONE);
        vu_upper(U_ADD, 14, 5, 7, 8, BC_NONE);
    }
#endif

    lAngleY &= 0xFFFF;
    
    njSinCos(lAngleY, &fSin, &fCos);
    
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        vsub.x    vf10, vf0, vf11
        
        vmulx.xyz vf7, vf4, vf12
        vmulx.xyz vf8, vf6, vf10
        vmulx.xyz vf4, vf4, vf11
        vmulx.xyz vf6, vf6, vf12

        vadd.xyz  vf6, vf4, vf6
        vadd.xyz  vf4, vf7, vf8
    .set reorder
    " : : "r"(pMatrix) :  
    );
#else
    { /* asm2c */
        vu_upper(U_SUB, 8, 10, 0, 11, BC_NONE);
        vu_upper(U_MUL, 14, 7, 4, 12, BC_X);
        vu_upper(U_MUL, 14, 8, 6, 10, BC_X);
        vu_upper(U_MUL, 14, 4, 4, 11, BC_X);
        vu_upper(U_MUL, 14, 6, 6, 12, BC_X);
        vu_upper(U_ADD, 14, 6, 4, 6, BC_NONE);
        vu_upper(U_ADD, 14, 4, 7, 8, BC_NONE);
    }
#endif

    lAngleZ &= 0xFFFF;
    
    njSinCos(lAngleZ, &fSin, &fCos);
    
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        vsub.x    vf10, vf0, vf11
        
        vmulx.xyz vf7, vf4, vf12
        vmulx.xyz vf8, vf5, vf11
        vmulx.xyz vf4, vf4, vf10
        vmulx.xyz vf5, vf5, vf12

        vadd.xyz  vf8, vf7, vf8
        vadd.xyz  vf9, vf4, vf5

        sqc2      vf8,  0x0(%0)
        sqc2      vf9, 0x10(%0)
        sqc2      vf6, 0x20(%0)
    .set reorder
    " : : "r"(pMatrix) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pMatrix);
        vu_upper(U_SUB, 8, 10, 0, 11, BC_NONE);
        vu_upper(U_MUL, 14, 7, 4, 12, BC_X);
        vu_upper(U_MUL, 14, 8, 5, 11, BC_X);
        vu_upper(U_MUL, 14, 4, 4, 10, BC_X);
        vu_upper(U_MUL, 14, 5, 5, 12, BC_X);
        vu_upper(U_ADD, 14, 8, 7, 8, BC_NONE);
        vu_upper(U_ADD, 14, 9, 4, 5, BC_NONE);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 9);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 6);
    }
#endif
}

// 100% matching!
void	njRotate(NJS_MATRIX *m, NJS_VECTOR *v, Angle ang)
{
    float fSin;
	float fCos;

    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

    ang &= 0xFFFF;
	ang /= 2;
    
    njSinCos(ang, &fSin, &fCos);

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl       a4, 0x7(%1)
        ldr       a4,   0(%1)

        lw        a5, NJS_VECTOR.z(%1)

        pcpyld    a4, a5, a4

        qmtc2.ni  a4, vf4

        vmul.xyz  vf5, vf4, vf4

        vaddy.x   vf5, vf5, vf5
        vaddz.x   vf5, vf5, vf5

        vrsqrt    Q, vf0w, vf5

        mfc1      t4, %2 
        mfc1      t5, %3
    
        qmtc2     t4, vf11
        qmtc2     t5, vf12

        vwaitq

        vmulq.x   vf11, vf11, Q
        vmulx.xyz vf4,   vf4, vf11
        vmul.x    vf5,  vf12, vf12

        vaddx.yz  vf5,  vf0,  vf5

        vmulx.xyz vf6,  vf4,  vf4
        vmuly.xyz vf7,  vf4,  vf4
        vmulz.xyz vf8,  vf4,  vf4
        vmulx.xyz vf9,  vf4,  vf12

        vadd.x    vf11, vf5,  vf6
        vsuby.x   vf11, vf11, vf7
        vsubz.x   vf11, vf11, vf8
        vsubz.x   vf12, vf7,  vf9
        vadd.x    vf12, vf12, vf12
        vaddy.x   vf13, vf8,  vf9
        vadd.x    vf13, vf13, vf13

        vaddz.y   vf11, vf6,  vf9
        vadd.y    vf11, vf11, vf11
        vsubx.y   vf12, vf5,  vf6
        vadd.y    vf12, vf12, vf7
        vsubz.y   vf12, vf12, vf8
        vsubx.y   vf13, vf8,  vf9
        vadd.y    vf13, vf13, vf13

        vsuby.z   vf11, vf6,  vf9
        vadd.z    vf11, vf11, vf11
        vaddx.z   vf12, vf7,  vf9
        vadd.z    vf12, vf12, vf12
        vsubx.z   vf13, vf5,  vf6
        vsuby.z   vf13, vf13, vf7
        vadd.z    vf13, vf13, vf8

        lqc2      vf4,    0(%0)
        lqc2      vf5, 0x10(%0)
        lqc2      vf6, 0x20(%0)

        vmulx.xyz vf7,  vf4, vf11
        vmuly.xyz vf10, vf5, vf11
        vmulz.xyz vf11, vf6, vf11

        vadd.xyz  vf7,  vf7, vf10
        vadd.xyz  vf7,  vf7, vf11

        vmulx.xyz vf8,  vf4, vf12
        vmuly.xyz vf10, vf5, vf12
        vmulz.xyz vf12, vf6, vf12

        vadd.xyz  vf8,  vf8, vf10
        vadd.xyz  vf8,  vf8, vf12
    
        vmulx.xyz vf9,  vf4, vf13
        vmuly.xyz vf10, vf5, vf13
        vmulz.xyz vf13, vf6, vf13

        vadd.xyz  vf9,  vf9, vf10
        vadd.xyz  vf9,  vf9, vf13
        
        sqc2      vf7,    0(%0)
        sqc2      vf8, 0x10(%0)
        sqc2      vf9, 0x20(%0)
    .set reorder
    " : : "r"(m), "r"(v), "f"(fSin), "f"(fCos) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R12 = {{0}}, R13 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(v);
        op2.f[0] = (float)(fSin);
        op3.f[0] = (float)(fCos);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_upper(U_MUL, 14, 5, 4, 4, BC_NONE);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Y);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Z);
        vu_rsqrt(0, 3, 5, 0);
        R12.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R13.sd[0] = (int32_t)(ee_f2u(op3.f[0]));
        vu_load_vf(11, &R12);
        vu_load_vf(12, &R13);
        vu_upper(U_MUL, 8, 11, 11, 0, BC_Q);
        vu_upper(U_MUL, 14, 4, 4, 11, BC_X);
        vu_upper(U_MUL, 8, 5, 12, 12, BC_NONE);
        vu_upper(U_ADD, 6, 5, 0, 5, BC_X);
        vu_upper(U_MUL, 14, 6, 4, 4, BC_X);
        vu_upper(U_MUL, 14, 7, 4, 4, BC_Y);
        vu_upper(U_MUL, 14, 8, 4, 4, BC_Z);
        vu_upper(U_MUL, 14, 9, 4, 12, BC_X);
        vu_upper(U_ADD, 8, 11, 5, 6, BC_NONE);
        vu_upper(U_SUB, 8, 11, 11, 7, BC_Y);
        vu_upper(U_SUB, 8, 11, 11, 8, BC_Z);
        vu_upper(U_SUB, 8, 12, 7, 9, BC_Z);
        vu_upper(U_ADD, 8, 12, 12, 12, BC_NONE);
        vu_upper(U_ADD, 8, 13, 8, 9, BC_Y);
        vu_upper(U_ADD, 8, 13, 13, 13, BC_NONE);
        vu_upper(U_ADD, 4, 11, 6, 9, BC_Z);
        vu_upper(U_ADD, 4, 11, 11, 11, BC_NONE);
        vu_upper(U_SUB, 4, 12, 5, 6, BC_X);
        vu_upper(U_ADD, 4, 12, 12, 7, BC_NONE);
        vu_upper(U_SUB, 4, 12, 12, 8, BC_Z);
        vu_upper(U_SUB, 4, 13, 8, 9, BC_X);
        vu_upper(U_ADD, 4, 13, 13, 13, BC_NONE);
        vu_upper(U_SUB, 2, 11, 6, 9, BC_Y);
        vu_upper(U_ADD, 2, 11, 11, 11, BC_NONE);
        vu_upper(U_ADD, 2, 12, 7, 9, BC_X);
        vu_upper(U_ADD, 2, 12, 12, 12, BC_NONE);
        vu_upper(U_SUB, 2, 13, 5, 6, BC_X);
        vu_upper(U_SUB, 2, 13, 13, 7, BC_Y);
        vu_upper(U_ADD, 2, 13, 13, 8, BC_NONE);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_MUL, 14, 7, 4, 11, BC_X);
        vu_upper(U_MUL, 14, 10, 5, 11, BC_Y);
        vu_upper(U_MUL, 14, 11, 6, 11, BC_Z);
        vu_upper(U_ADD, 14, 7, 7, 10, BC_NONE);
        vu_upper(U_ADD, 14, 7, 7, 11, BC_NONE);
        vu_upper(U_MUL, 14, 8, 4, 12, BC_X);
        vu_upper(U_MUL, 14, 10, 5, 12, BC_Y);
        vu_upper(U_MUL, 14, 12, 6, 12, BC_Z);
        vu_upper(U_ADD, 14, 8, 8, 10, BC_NONE);
        vu_upper(U_ADD, 14, 8, 8, 12, BC_NONE);
        vu_upper(U_MUL, 14, 9, 4, 13, BC_X);
        vu_upper(U_MUL, 14, 10, 5, 13, BC_Y);
        vu_upper(U_MUL, 14, 13, 6, 13, BC_Z);
        vu_upper(U_ADD, 14, 9, 9, 10, BC_NONE);
        vu_upper(U_ADD, 14, 9, 9, 13, BC_NONE);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 7);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 9);
    }
#endif
}

// 100% matching!
void	njScale(NJS_MATRIX *m, Float sx, Float sy, Float sz)
{
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        mfc1      t2, %1 
        mfc1      t3, %2
        mfc1      t4, %3
        
        qmtc2     t2, vf4
        qmtc2     t3, vf5
        qmtc2     t4, vf6
    
        lqc2      vf7,  0x0(%0)
        lqc2      vf8, 0x10(%0)
        lqc2      vf9, 0x20(%0)

        vmulx.xyz vf7, vf7, vf4
        vmulx.xyz vf8, vf8, vf5
        vmulx.xyz vf9, vf9, vf6
    
        sqc2      vf7,  0x0(%0)
        sqc2      vf8, 0x10(%0)
        sqc2      vf9, 0x20(%0)
    .set reorder
    " : : "r"(m), "f"(sx), "f"(sy), "f"(sz) : 
    );
#else
    { /* asm2c */
        ee_reg R10 = {{0}}, R11 = {{0}}, R12 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.f[0] = (float)(sx);
        op2.f[0] = (float)(sy);
        op3.f[0] = (float)(sz);
        R10.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R11.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R12.sd[0] = (int32_t)(ee_f2u(op3.f[0]));
        vu_load_vf(4, &R10);
        vu_load_vf(5, &R11);
        vu_load_vf(6, &R12);
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(9, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_MUL, 14, 7, 7, 4, BC_X);
        vu_upper(U_MUL, 14, 8, 8, 5, BC_X);
        vu_upper(U_MUL, 14, 9, 9, 6, BC_X);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 7);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 9);
    }
#endif
}

// 100% matching!
void	njScaleV(NJS_MATRIX *m, NJS_VECTOR *v)
{
	if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl       a4, 0x7(%1)
        ldr       a4,   0(%1)

        lw        a5, NJS_VECTOR.z(%1) 

        pcpyld    a4, a5, a4

        qmtc2.ni  a4, vf4

        lqc2      vf5,    0(%0)
        lqc2      vf6, 0x10(%0)
        lqc2      vf7, 0x20(%0)

        vmulx.xyz vf5, vf5, vf4
        vmuly.xyz vf6, vf6, vf4
        vmulz.xyz vf7, vf7, vf4
    
        sqc2      vf5,     0(%0)
        sqc2      vf6,  0x10(%0)
        sqc2      vf7,  0x20(%0)
    .set reorder
    " : : "r"(m), "r"(v) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(v);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_MUL, 14, 5, 5, 4, BC_X);
        vu_upper(U_MUL, 14, 6, 6, 4, BC_Y);
        vu_upper(U_MUL, 14, 7, 7, 4, BC_Z);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 5);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 6);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 7);
    }
#endif
}

// 100% matching!
Bool	njInvertMatrix(NJS_MATRIX *m)
{
	if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lq          t0,     0(%0)
        lq          t1,  0x10(%0)
        lq          t2,  0x20(%0)
        lqc2        vf4, 0x30(%0)

        vmove       vf5, vf4

        vsub.xyz    vf4, vf4, vf4

        vmove       vf10, vf4

        qmfc2.ni    t3, vf4

        pextlw      t4, t1, t0
        pextuw      t5, t1, t0
 
        pextlw      t6, t3, t2
        pextuw      t7, t3, t2
 
        pcpyld      t0, t6, t4
        pcpyud      t1, t4, t6
        pcpyld      t2, t7, t5
 
        qmtc2.ni    t0, vf7
        qmtc2.ni    t1, vf8
        qmtc2.ni    t2, vf9

        vmulax.xyz  ACC, vf7, vf5

        vmadday.xyz ACC, vf8, vf5
        vmaddz.xyz  vf4, vf9, vf5

        vsub.xyz    vf4, vf10, vf4

        sq          t0,     0(%0)
        sq          t1,  0x10(%0)
        sq          t2,  0x20(%0)
        sqc2        vf4, 0x30(%0)
    .set reorder
    " : : "r"(m) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R12 = {{0}}, R13 = {{0}}, R14 = {{0}}, R15 = {{0}};
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        ee_lq(&R8, (op0.w[0] + (uint32_t)(0)));
        ee_lq(&R9, (op0.w[0] + (uint32_t)(16)));
        ee_lq(&R10, (op0.w[0] + (uint32_t)(32)));
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_move(15, 5, 4);
        vu_upper(U_SUB, 14, 4, 4, 4, BC_NONE);
        vu_move(15, 10, 4);
        vu_store_vf(&R11, 4);
        ee_pextlw(&R12, &R9, &R8);
        ee_pextuw(&R13, &R9, &R8);
        ee_pextlw(&R14, &R11, &R10);
        ee_pextuw(&R15, &R11, &R10);
        ee_pcpyld(&R8, &R14, &R12);
        ee_pcpyud(&R9, &R12, &R14);
        ee_pcpyld(&R10, &R15, &R13);
        vu_load_vf(7, &R8);
        vu_load_vf(8, &R9);
        vu_load_vf(9, &R10);
        vu_upper(U_MULA, 14, 0, 7, 5, BC_X);
        vu_upper(U_MADDA, 14, 0, 8, 5, BC_Y);
        vu_upper(U_MADD, 14, 4, 9, 5, BC_Z);
        vu_upper(U_SUB, 14, 4, 10, 4, BC_NONE);
        ee_sq((op0.w[0] + (uint32_t)(0)), &R8);
        ee_sq((op0.w[0] + (uint32_t)(16)), &R9);
        ee_sq((op0.w[0] + (uint32_t)(32)), &R10);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(48))), 4);
    }
#endif

    return TRUE;
}

// 100% matching!
void	njTransposeMatrix(NJS_MATRIX *m)
{
	if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2    vf4,    0(%0)
        lqc2    vf5, 0x10(%0)
        lqc2    vf6, 0x20(%0)

        vaddx.x vf7, vf0, vf4
        vaddx.y vf7, vf0, vf5
        vaddx.z vf7, vf0, vf6

        vaddy.x vf8, vf0, vf4
        vaddy.y vf8, vf0, vf5
        vaddy.z vf8, vf0, vf6

        vaddz.x vf9, vf0, vf4
        vaddz.y vf9, vf0, vf5
        vaddz.z vf9, vf0, vf6

        sqc2    vf7,    0(%0)
        sqc2    vf8, 0x10(%0)
        sqc2    vf9, 0x20(%0)
    .set reorder
    " : : "r"(m) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_ADD, 8, 7, 0, 4, BC_X);
        vu_upper(U_ADD, 4, 7, 0, 5, BC_X);
        vu_upper(U_ADD, 2, 7, 0, 6, BC_X);
        vu_upper(U_ADD, 8, 8, 0, 4, BC_Y);
        vu_upper(U_ADD, 4, 8, 0, 5, BC_Y);
        vu_upper(U_ADD, 2, 8, 0, 6, BC_Y);
        vu_upper(U_ADD, 8, 9, 0, 4, BC_Z);
        vu_upper(U_ADD, 4, 9, 0, 5, BC_Z);
        vu_upper(U_ADD, 2, 9, 0, 6, BC_Z);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 7);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 8);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 9);
    }
#endif
}

// 100% matching!
static float njAtan2b(float a, float b)
{
    return atan2(a, b);
}

// 100% matching!
void	njMirror(NJS_MATRIX *m,NJS_PLANE *pl)
{
    NJS_MATRIX mat; 
    int lAngleX;   
    int lAngleZ;   
    
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        move        a1, %0
        move        a0, %1
        move        s1, %2
        move        s2, %3
        
        addiu       a0, sp, 0x40
        
        vaddz.x     vf4, vf0, vf3z
        
        ld          t0, NJS_PLANE.vx(a1)
        ld          t2, NJS_PLANE.vz(a1)
        
        qmtc2       t0, vf5
    
        vmulw.x     vf4, vf4, vf2w
        
        qmtc2       t2, vf6
    
        vaddx.z     vf5, vf0, vf6x
        
        vmul.xyz    vf7, vf5, vf5
        
        vaddy.x     vf7, vf7, vf7y
        vaddz.x     vf7, vf7, vf7z
        
        vrsqrt      Q, vf0w, vf7x
    
        ldl         t0, 0x7(a1)
        ldr         t0, 0x0(a1)
        
        lw          t1, 0x8(a1)
        
        pcpyld      t0, t1, t0
    
        qmtc2       t0, vf12
    
        vwaitq
    
        vmulq.xyz   vf9, vf5, Q
    
        vaddq.x     vf6, vf0, Q
        
        vmul.xyz    vf8, vf9, vf12
        
        vsubx.x     vf10, vf0, vf8x
        vsuby.x     vf10, vf10, vf8y
        vsubz.x     vf10, vf10, vf8z
        
        vmulx.xyz   vf11, vf9, vf10x
        
        vsub.xyz    vf11, vf0, vf11
        
        qmfc2       t0, vf10
    
        bnez        t0, l_002D732C
    
        vadd.x      vf10, vf10, vf10
        
        vmulw.xyzw  vf4, vf0, vf0w
        
        vmr32.xyzw  vf5, vf4
        vmr32.xyzw  vf6, vf5
        vmr32.xyzw  vf7, vf6
        
        sqc2        vf4, 0x30(s1)
        sqc2        vf5, 0x20(s1)
        sqc2        vf6, 0x10(s1)

        jr          ra
		
        sqc2        vf7, 0x0(s1)
    
        l_002D732C:
        vmulx.xyz   vf14, vf5, vf10x
        
        vsub.xyz    vf12, vf0, vf14
        
        vaddy.x     vf11, vf0, vf5y
        vaddz.x     vf11, vf11, vf5z
        
        qmfc2       t0, vf11
    
        bnez        t0, l_002D7374
    
        sqc2        vf12, 0x30(a0)
        
        addiu       t6, zero, 0x4008
        addiu       t6, t6, 0x7FF8
        addiu       t7, zero, 0x4000
        
        mfc1        s1, f0
        
        qmfc2       t2, vf5
    
        bgtz        t2, l_002D7450
    
        mfc1        s2, f14
    
        bgtz        t2, l_002D7450
    
        mfc1        s2, f15
    
        j           l_002D7450
    
        mfc1        s2, f0
    
        l_002D7374:
        vaddz.x     vf6, vf0, vf5z
        vaddy.x     vf7, vf0, vf5y
        
        qmfc2       a0, vf6
        qmfc2       a1, vf7
    
        addi        sp, sp, -0x10 
        
        sw          v0, 0x0(sp)
        sw          v1, 0x4(sp)
        sw          a0, 0x8(sp)
        sw          a1, 0xC(sp)
        
        lwc1        f12, 0x8(sp)
        
        jal         njAtan2b
    
        lwc1        f13, 0xC(sp)
        
        swc1        f0, 0xC(sp)
        
        lw          v0, 0x0(sp)
        lw          v1, 0x4(sp)
        lw          t2, 0xC(sp)
        
        addi        sp, sp, 0x10 
    
        qmtc2       t2, vf12
        
        vmulx.x     vf12, vf12, vf4x
        
        vdiv        Q, vf12x, vf3w
    
        vmul.xyz    vf6, vf5, vf5
        
        vaddy.x     vf7, vf0, vf5y
        vaddz.x     vf7, vf7, vf5z
        
        vwaitq
        
        vaddq.x     vf12, vf0, Q
        
        vftoi0.xyzw vf12, vf12
        
        qmfc2       t3, vf12
    
        sw          t3, 0x0(s1)
        
        vsqrt       Q, vf7x          
    
        addi        sp, sp, -0x10 
        
        sw          v0, 0x0(sp)
        sw          v1, 0x4(sp)
        
        vwaitq
        
        vaddq.x     vf6, vf0, Q
        
        qmfc2       a0, vf5
        qmfc2       a1, vf6
    
        sw          a0, 0x8(sp)
        sw          a1, 0xC(sp)
        
        lwc1        f12, 0x8(sp)
        
        jal         njAtan2b
    
        lwc1        f13, 0xC(sp)
        
        swc1        f0, 0xC(sp)
        
        lw          v0, 0x0(sp)
        lw          v1, 0x4(sp)
        lw          t2, 0xC(sp)
        
        addi        sp, sp, 0x10 
    
        qmtc2       t2, vf12
        
        vmulx.x     vf12, vf12, vf4x
        
        vdiv        Q, vf12x, vf3w
    
        vwaitq
    
        vaddq.x     vf12, vf0, Q
        
        vsubx.x     vf12, vf0, vf12x
        
        vftoi0.xyzw vf12, vf12
        
        qmfc2       t3, vf12
    
        sw          t3, 0x0(s2)
    .set reorder
    " : : "r"(pl), "f"(&mat), "r"(&lAngleX), "r"(&lAngleZ) : 
    );
#else
    { /* Hand-translated; verified against the retail ELF (0x2D7260). The decomp's block is
         the original hand-written asm (register-allocated for the retail frame), so it
         cannot be compiled as-is. Two latent bugs of the original are unreachable for real
         mirror planes and not reproduced: d == 0 stores an identity matrix over the stack
         and returns early; ny == nz == 0 leaves lAngleX/lAngleZ uninitialized. */
        float nx = pl->vx, ny = pl->vy, nz = pl->vz;
        float inv = vu_clampf(1.0f / sqrtf(fabsf(nx * nx + ny * ny + nz * nz)));
        float d = -((nx * inv) * pl->px + (ny * inv) * pl->py + (nz * inv) * pl->pz);
        float d2 = d + d;
        /* angle scale: vf4.x = vf3.z * vf2.w = 65536 * 0.5, divided by vf3.w (pi) */
        float scale = g_vu0.vf[3].f[2] * g_vu0.vf[2].f[3];
        float pi = g_vu0.vf[3].f[3];

        mat[12] = -(nx * d2);
        mat[13] = -(ny * d2);
        mat[14] = -(nz * d2);
        mat[15] = 1.0f;

        if (ny == 0.0f && nz == 0.0f) {
            lAngleX = 0;
            lAngleZ = nx > 0.0f ? 0xC000 : (nx < 0.0f ? 0x4000 : 0);
        } else {
            lAngleX = (int)(((float)njAtan2b(nz, ny) * scale) / pi);
            /* the original takes sqrt(ny + nz); the squares it computes go unused */
            lAngleZ = (int)(-(((float)njAtan2b(nx, sqrtf(fabsf(ny + nz))) * scale) / pi));
        }
    }
#endif

    l_002D7450:
    njUnitRotPortion(&mat);
    
    njRotateX(&mat, lAngleX);
    njRotateZ(&mat, lAngleZ);
    
    njScale(&mat, 1.0f, -1.0f, 1.0f);
    
    njRotateZ(&mat, -lAngleZ);
    njRotateX(&mat, -lAngleX);
    
    njMultiMatrix(m, &mat);
}

// 100% matching!
void	njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps, NJS_POINT3 *pd)
{
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl         t0, 7(%1)
        ldr         t0, 0(%1)
        
        lw          t1, 8(%1)
        
        pcpyld      t0, t1, t0
    
        qmtc2       t0, vf4
    
        lqc2        vf5, 0(%0)
        lqc2        vf6, 0x10(%0)
        lqc2        vf7, 0x20(%0)
        lqc2        vf8, 0x30(%0)

        vmulax.xyz  ACC,  vf5, vf4
        
        vmadday.xyz ACC,  vf6, vf4
        vmaddaz.xyz ACC,  vf7, vf4
        vmaddw.xyz  vf18, vf8, vf0

        qmfc2       t0, vf18
    
        pcpyud      t1, t0, t0
    
        sdl         t0, 7(%2)
        sdr         t0, 0(%2)
        
        sw          t1, 8(%2)
    .set reorder
    " : : "r"(m), "r"(ps), "r"(pd) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(ps);
        op2.sd[0] = (int32_t)(uintptr_t)(pd);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(8)));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 14, 0, 5, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 6, 4, BC_Y);
        vu_upper(U_MADDA, 14, 0, 7, 4, BC_Z);
        vu_upper(U_MADD, 14, 18, 8, 0, BC_W);
        vu_store_vf(&R8, 18);
        ee_pcpyud(&R9, &R8, &R8);
        ee_sdl((op2.w[0] + (uint32_t)(7)), R8.d[0]);
        ee_sdr((op2.w[0] + (uint32_t)(0)), R8.d[0]);
        EE_SW((op2.w[0] + (uint32_t)(8)), R9.d[0]);
    }
#endif
}

// 100% matching!
void njCalcPoint4(NJS_MATRIX* pMatrix, NJS_POINT4* pSrcPoint, NJS_POINT4* pDstPoint)
{
    if (pMatrix == NULL)
    {
        pMatrix = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2        vf4, 0(%1)
        lqc2        vf5, 0(%0)
        lqc2        vf6, 0x10(%0)
        lqc2        vf7, 0x20(%0)
        lqc2        vf8, 0x30(%0)

        vmulax.xyz  ACC,  vf5, vf4x
        
        vmadday.xyz ACC,  vf6, vf4y
        vmaddaz.xyz ACC,  vf7, vf4z
        vmaddw.xyz  vf18, vf8, vf0w
    
        sqc2        vf18, 0(%2)
    .set reorder
    " : : "r"(pMatrix), "r"(pSrcPoint), "r"(pDstPoint) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pMatrix);
        op1.sd[0] = (int32_t)(uintptr_t)(pSrcPoint);
        op2.sd[0] = (int32_t)(uintptr_t)(pDstPoint);
        vu_load_vf(4, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 14, 0, 5, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 6, 4, BC_Y);
        vu_upper(U_MADDA, 14, 0, 7, 4, BC_Z);
        vu_upper(U_MADD, 14, 18, 8, 0, BC_W);
        vu_store_vf(EE_PTR((op2.w[0] + (uint32_t)(0))), 18);
    }
#endif
}

// 100% matching!
void njCalcPointCN(NJS_POINT3* pSrcPoint, NJS_POINT3* pDstPoint)
{
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl         t2, 7(%0)
        ldr         t2, 0(%0)
        
        lw          t3, 8(%0)
        
        pcpyld      t2, t3, t2
    
        qmtc2       t2, vf4
    
        vmulax.xyz  ACC, vf28, vf4
        
        vmadday.xyz ACC, vf29, vf4
        vmaddaz.xyz ACC, vf30, vf4

        vmaddw.xyz  vf18, vf31, vf0

        qmfc2       t0, vf18
    
        pcpyud      t1, t0, t0
    
        sdl         t0, 7(%1)
        sdr         t0, 0(%1)
        
        sw          t1, 8(%1)
    .set reorder
    " : : "r"(pSrcPoint), "r"(pDstPoint) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pSrcPoint);
        op1.sd[0] = (int32_t)(uintptr_t)(pDstPoint);
        R10.d[0] = ee_ldl(R10.d[0], (op0.w[0] + (uint32_t)(7)));
        R10.d[0] = ee_ldr(R10.d[0], (op0.w[0] + (uint32_t)(0)));
        R11.sd[0] = EE_LW((op0.w[0] + (uint32_t)(8)));
        ee_pcpyld(&R10, &R11, &R10);
        vu_load_vf(4, &R10);
        vu_upper(U_MULA, 14, 0, 28, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 29, 4, BC_Y);
        vu_upper(U_MADDA, 14, 0, 30, 4, BC_Z);
        vu_upper(U_MADD, 14, 18, 31, 0, BC_W);
        vu_store_vf(&R8, 18);
        ee_pcpyud(&R9, &R8, &R8);
        ee_sdl((op1.w[0] + (uint32_t)(7)), R8.d[0]);
        ee_sdr((op1.w[0] + (uint32_t)(0)), R8.d[0]);
        EE_SW((op1.w[0] + (uint32_t)(8)), R9.d[0]);
    }
#endif
}

// 100% matching!
void	njAddVector(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
#ifndef TARGET_PC
	asm volatile
    ("
    .set noreorder
        lwc1   f8, NJS_VECTOR.x(%0)
        lwc1   f9, NJS_VECTOR.y(%0)
        lwc1  f10, NJS_VECTOR.z(%0)

        lwc1  f11, NJS_VECTOR.x(%1)
        lwc1  f12, NJS_VECTOR.y(%1)
        lwc1  f13, NJS_VECTOR.z(%1)

        add.s  f8,  f8, f11
        add.s  f9,  f9, f12
        add.s f10, f10, f13

        swc1   f8, NJS_VECTOR.x(%0)
        swc1   f9, NJS_VECTOR.y(%0)
        swc1  f10, NJS_VECTOR.z(%0)
    .set reorder
    " : : "r"(vd), "r"(vs) : 
    );
#else
	{ /* asm2c */
	    float F8 = 0.0f, F9 = 0.0f, F10 = 0.0f, F11 = 0.0f, F12 = 0.0f, F13 = 0.0f;
	    ee_reg op0 = {{0}};
	    ee_reg op1 = {{0}};
	    op0.sd[0] = (int32_t)(uintptr_t)(vd);
	    op1.sd[0] = (int32_t)(uintptr_t)(vs);
	    F8 = EE_LWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, x))));
	    F9 = EE_LWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, y))));
	    F10 = EE_LWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    F11 = EE_LWF((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, x))));
	    F12 = EE_LWF((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, y))));
	    F13 = EE_LWF((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    F8 = vu_clampf(F8 + F11);
	    F9 = vu_clampf(F9 + F12);
	    F10 = vu_clampf(F10 + F13);
	    EE_SWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, x))), F8);
	    EE_SWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, y))), F9);
	    EE_SWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))), F10);
	}
#endif
}

// 100% matching!
void	njSubVector(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
#ifndef TARGET_PC
	asm volatile
    ("
    .set noreorder
        lwc1   f8, NJS_VECTOR.x(%0)
        lwc1   f9, NJS_VECTOR.y(%0)
        lwc1  f10, NJS_VECTOR.z(%0)

        lwc1  f11, NJS_VECTOR.x(%1)
        lwc1  f12, NJS_VECTOR.y(%1)
        lwc1  f13, NJS_VECTOR.z(%1)

        sub.s  f8,  f8, f11
        sub.s  f9,  f9, f12
        sub.s f10, f10, f13

        swc1   f8, NJS_VECTOR.x(%0)
        swc1   f9, NJS_VECTOR.y(%0)
        swc1  f10, NJS_VECTOR.z(%0)
    .set reorder
    " : : "r"(vd), "r"(vs) : 
    );
#else
	{ /* asm2c */
	    float F8 = 0.0f, F9 = 0.0f, F10 = 0.0f, F11 = 0.0f, F12 = 0.0f, F13 = 0.0f;
	    ee_reg op0 = {{0}};
	    ee_reg op1 = {{0}};
	    op0.sd[0] = (int32_t)(uintptr_t)(vd);
	    op1.sd[0] = (int32_t)(uintptr_t)(vs);
	    F8 = EE_LWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, x))));
	    F9 = EE_LWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, y))));
	    F10 = EE_LWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    F11 = EE_LWF((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, x))));
	    F12 = EE_LWF((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, y))));
	    F13 = EE_LWF((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    F8 = vu_clampf(F8 - F11);
	    F9 = vu_clampf(F9 - F12);
	    F10 = vu_clampf(F10 - F13);
	    EE_SWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, x))), F8);
	    EE_SWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, y))), F9);
	    EE_SWF((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))), F10);
	}
#endif
}

// 100% matching!
void	njCalcVector(NJS_MATRIX *m, NJS_VECTOR *vs, NJS_VECTOR *vd)
{
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl         t0, 7(%1)
        ldr         t0, 0(%1)
        
        lw          t1, 8(%1)
        
        pcpyld      t0, t1, t0
    
        qmtc2       t0, vf4
    
        lqc2        vf7, 0(%0)
        lqc2        vf8, 0x10(%0)
        lqc2        vf9, 0x20(%0)

        vmulax.xyz  ACC,  vf7, vf4
        
        vmadday.xyz ACC,  vf8, vf4
        vmaddz.xyz  vf18, vf9, vf4

        qmfc2       t0, vf18
    
        pcpyud      t1, t0, t0
    
        sdl         t0, 7(%2)
        sdr         t0, 0(%2)
        
        sw          t1, 8(%2)
    .set reorder
    " : : "r"(m), "r"(vs), "r"(vd) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(m);
        op1.sd[0] = (int32_t)(uintptr_t)(vs);
        op2.sd[0] = (int32_t)(uintptr_t)(vd);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(8)));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(8, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(9, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_upper(U_MULA, 14, 0, 7, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 8, 4, BC_Y);
        vu_upper(U_MADD, 14, 18, 9, 4, BC_Z);
        vu_store_vf(&R8, 18);
        ee_pcpyud(&R9, &R8, &R8);
        ee_sdl((op2.w[0] + (uint32_t)(7)), R8.d[0]);
        ee_sdr((op2.w[0] + (uint32_t)(0)), R8.d[0]);
        EE_SW((op2.w[0] + (uint32_t)(8)), R9.d[0]);
    }
#endif
}

// 100% matching!
Float	njUnitVector(NJS_VECTOR *v)
{
	float ret;

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl       a4, 0x7(%1)
        ldr       a4,   0(%1)
     
        lw        a5, NJS_VECTOR.z(%1) 
     
        pcpyld    a4, a5, a4
     
        qmtc2.ni  a4, vf4
     
        vmul.xyz  vf5, vf4, vf4
     
        vaddy.x   vf5, vf5, vf5
        vaddz.x   vf5, vf5, vf5
     
        vrsqrt    Q, vf0w, vf5
     
        vwaitq   

        vmulq.xyz vf6, vf4, Q
        vmulq.x   vf7, vf5, Q

        qmfc2.ni  a6, vf6

        pcpyud    a7, a6, a6

        sdl       a6, 0x7(%1)
        sdr       a6,   0(%1)
     
        sw        a7, NJS_VECTOR.z(%1) 
     
        qmfc2.ni  v0, vf7

        mtc1      v0, %0
    .set reorder
    " : "=r"(ret) : "r"(v) : 
    );
#else
    { /* asm2c */
        ee_reg R2 = {{0}}, R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.f[0] = (float)(ret);
        op1.sd[0] = (int32_t)(uintptr_t)(v);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_upper(U_MUL, 14, 5, 4, 4, BC_NONE);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Y);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Z);
        vu_rsqrt(0, 3, 5, 0);
        vu_upper(U_MUL, 14, 6, 4, 0, BC_Q);
        vu_upper(U_MUL, 8, 7, 5, 0, BC_Q);
        vu_store_vf(&R10, 6);
        ee_pcpyud(&R11, &R10, &R10);
        ee_sdl((op1.w[0] + (uint32_t)(7)), R10.d[0]);
        ee_sdr((op1.w[0] + (uint32_t)(0)), R10.d[0]);
        EE_SW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))), R11.d[0]);
        vu_store_vf(&R2, 7);
        op0.f[0] = ee_u2f(R2.w[0]);
        ret = op0.f[0];
    }
#endif

    return ret;
}

// 100% matching!
Float	njScalor(NJS_VECTOR *v)
{
	float ret;

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl      a4, 0x7(%1)
        ldr      a4,   0(%1)
     
        lw       a5, NJS_VECTOR.z(%1) 
     
        pcpyld   a4, a5, a4
     
        qmtc2.ni a4, vf4
     
        vmul.xyz vf5, vf4, vf4
     
        vaddy.x  vf5, vf5, vf5
        vaddz.x  vf5, vf5, vf5
     
        vsqrt    Q, vf5
     
        vwaitq   
     
        vaddq.x  vf6, vf0, Q
     
        qmfc2.ni v0, vf6

        mtc1     v0, %0
    .set reorder
    " : "=r"(ret) : "r"(v) : 
    );
#else
    { /* asm2c */
        ee_reg R2 = {{0}}, R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.f[0] = (float)(ret);
        op1.sd[0] = (int32_t)(uintptr_t)(v);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_upper(U_MUL, 14, 5, 4, 4, BC_NONE);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Y);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Z);
        vu_sqrt(5, 0);
        vu_upper(U_ADD, 8, 6, 0, 0, BC_Q);
        vu_store_vf(&R2, 6);
        op0.f[0] = ee_u2f(R2.w[0]);
        ret = op0.f[0];
    }
#endif

    return ret;
}

// 100% matching!
Float	njScalor2(NJS_VECTOR *v)
{
	float ret;

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl      a4, 0x7(%1)
        ldr      a4,   0(%1)
     
        lw       a5, NJS_VECTOR.z(%1) 
     
        pcpyld   a4, a5, a4
     
        qmtc2.ni a4, vf4
     
        vmul.xyz vf5, vf4, vf4
     
        vaddy.x  vf5, vf5, vf5
        vaddz.x  vf6, vf5, vf5
     
        qmfc2.ni v0, vf6
        
        mtc1     v0, %0
    .set reorder
    " : "=r"(ret) : "r"(v) : 
    );
#else
    { /* asm2c */
        ee_reg R2 = {{0}}, R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.f[0] = (float)(ret);
        op1.sd[0] = (int32_t)(uintptr_t)(v);
        R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_upper(U_MUL, 14, 5, 4, 4, BC_NONE);
        vu_upper(U_ADD, 8, 5, 5, 5, BC_Y);
        vu_upper(U_ADD, 8, 6, 5, 5, BC_Z);
        vu_store_vf(&R2, 6);
        op0.f[0] = ee_u2f(R2.w[0]);
        ret = op0.f[0];
    }
#endif

    return ret;
}

// 100% matching!
void	njProjectScreen(NJS_MATRIX *m, NJS_POINT3 *p3, NJS_POINT2 *p2)
{
    NJS_POINT3 Point;
    
    if (m == NULL)
    {
        m = pNaMatMatrixStuckPtr;
    }
    
    njMulMatrixCN(&NaViewScreenMatrix, m);
    
    njCalcPointCN(p3, &Point);
    
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        mfc1     t0, %2
        
        qmtc2    t0, vf4

        vdiv     Q, vf4x, vf18z

        lw       t1, fNaViwOffsetX
        lw       t2, fNaViwOffsetY
        
        qmtc2    t1, vf5
        qmtc2    t2, vf6

        vwaitq

        vaddq.z  vf8, vf0, Q

        vmulq.xy vf7, vf18, Q
        
        vaddx.x  vf8, vf7, vf5
        vaddx.y  vf8, vf7, vf6

        qmfc2    t0, vf8
    
        pcpyud   t1, t0, t0
    
        sdl      t0, 7(%0)
        sdr      t0, 0(%0)
        
        sw       t1, 8(%1)
    .set reorder
    " : : "r"(p2), "r"(&Point), "f"(_nj_screen_.dist) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(p2);
        op1.sd[0] = (int32_t)(uintptr_t)(&Point);
        op2.f[0] = (float)(_nj_screen_.dist);
        R8.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        vu_load_vf(4, &R8);
        vu_div(4, 0, 18, 2);
        R9.sd[0] = EE_LW((uint32_t)(uintptr_t)&fNaViwOffsetX);
        R10.sd[0] = EE_LW((uint32_t)(uintptr_t)&fNaViwOffsetY);
        vu_load_vf(5, &R9);
        vu_load_vf(6, &R10);
        vu_upper(U_ADD, 2, 8, 0, 0, BC_Q);
        vu_upper(U_MUL, 12, 7, 18, 0, BC_Q);
        vu_upper(U_ADD, 8, 8, 7, 5, BC_X);
        vu_upper(U_ADD, 4, 8, 7, 6, BC_X);
        vu_store_vf(&R8, 8);
        ee_pcpyud(&R9, &R8, &R8);
        ee_sdl((op0.w[0] + (uint32_t)(7)), R8.d[0]);
        ee_sdr((op0.w[0] + (uint32_t)(0)), R8.d[0]);
        EE_SW((op1.w[0] + (uint32_t)(8)), R9.d[0]);
    }
#endif
}

// 100% matching! 
Float	njOuterProduct(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *ov)
{
	float ret;

#ifndef TARGET_PC
	asm volatile
    ("
    .set noreorder
        ldl      a4, 0x7(%1)
        ldr      a4,   0(%1)
     
        lw       a5, NJS_VECTOR.z(%1) 

        ldl      a6, 0x7(%2)
        ldr      a6,   0(%2)
     
        lw       a7, NJS_VECTOR.z(%2) 
     
        pcpyld   a4, a5, a4
        pcpyld   a6, a7, a6
     
        qmtc2.ni a4, vf4
        qmtc2.ni a6, vf5

        vopmula  ACC, vf4, vf5
        vopmsub  vf6, vf5, vf4
     
        vmul.xyz vf7, vf6, vf6
     
        vaddy.x  vf7, vf7, vf7
        vaddz.x  vf7, vf7, vf7

        vsqrt    Q, vf7

        qmfc2.ni a4, vf6

        pcpyud   a5, a4, a4

        sdl      a4, 0x7(%3)
        sdr      a4,   0(%3)

        sw       a5, NJS_VECTOR.z(%3)

        vwaitq

        vaddq.x  vf8, vf0, Q
     
        qmfc2.ni v0, vf8
        
        mtc1     v0, %0
    .set reorder
    " : "=r"(ret) : "r"(v1), "r"(v2), "r"(ov) : 
    );
#else
	{ /* asm2c */
	    ee_reg R2 = {{0}}, R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
	    ee_reg op0 = {{0}};
	    ee_reg op1 = {{0}};
	    ee_reg op2 = {{0}};
	    ee_reg op3 = {{0}};
	    op0.f[0] = (float)(ret);
	    op1.sd[0] = (int32_t)(uintptr_t)(v1);
	    op2.sd[0] = (int32_t)(uintptr_t)(v2);
	    op3.sd[0] = (int32_t)(uintptr_t)(ov);
	    R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
	    R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
	    R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    R10.d[0] = ee_ldl(R10.d[0], (op2.w[0] + (uint32_t)(7)));
	    R10.d[0] = ee_ldr(R10.d[0], (op2.w[0] + (uint32_t)(0)));
	    R11.sd[0] = EE_LW((op2.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    ee_pcpyld(&R8, &R9, &R8);
	    ee_pcpyld(&R10, &R11, &R10);
	    vu_load_vf(4, &R8);
	    vu_load_vf(5, &R10);
	    vu_upper(U_OPMULA, 14, 0, 4, 5, BC_NONE);
	    vu_upper(U_OPMSUB, 14, 6, 5, 4, BC_NONE);
	    vu_upper(U_MUL, 14, 7, 6, 6, BC_NONE);
	    vu_upper(U_ADD, 8, 7, 7, 7, BC_Y);
	    vu_upper(U_ADD, 8, 7, 7, 7, BC_Z);
	    vu_sqrt(7, 0);
	    vu_store_vf(&R8, 6);
	    ee_pcpyud(&R9, &R8, &R8);
	    ee_sdl((op3.w[0] + (uint32_t)(7)), R8.d[0]);
	    ee_sdr((op3.w[0] + (uint32_t)(0)), R8.d[0]);
	    EE_SW((op3.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))), R9.d[0]);
	    vu_upper(U_ADD, 8, 8, 0, 0, BC_Q);
	    vu_store_vf(&R2, 8);
	    op0.f[0] = ee_u2f(R2.w[0]);
	    ret = op0.f[0];
	}
#endif

    return ret;
}

// 100% matching! 
Float	njInnerProduct(NJS_VECTOR *v1, NJS_VECTOR *v2)
{
	float ret;

#ifndef TARGET_PC
	asm volatile
    ("
    .set noreorder
        ldl      a4, 0x7(%1)
        ldr      a4,   0(%1)
     
        lw       a5, NJS_VECTOR.z(%1) 

        ldl      a6, 0x7(%2)
        ldr      a6,   0(%2)
     
        lw       a7, NJS_VECTOR.z(%2) 
     
        pcpyld   a4, a5, a4
        pcpyld   a6, a7, a6
     
        qmtc2.ni a4, vf4
        qmtc2.ni a6, vf5
     
        vmul.xyz vf6,  vf4, vf5
     
        vaddy.x  vf6,  vf6, vf6
        vaddz.x  vf14, vf6, vf6
     
        qmfc2.ni v0, vf14
        
        mtc1     v0, %0
    .set reorder
    " : "=r"(ret) : "r"(v1), "r"(v2) : 
    );
#else
	{ /* asm2c */
	    ee_reg R2 = {{0}}, R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
	    ee_reg op0 = {{0}};
	    ee_reg op1 = {{0}};
	    ee_reg op2 = {{0}};
	    op0.f[0] = (float)(ret);
	    op1.sd[0] = (int32_t)(uintptr_t)(v1);
	    op2.sd[0] = (int32_t)(uintptr_t)(v2);
	    R8.d[0] = ee_ldl(R8.d[0], (op1.w[0] + (uint32_t)(7)));
	    R8.d[0] = ee_ldr(R8.d[0], (op1.w[0] + (uint32_t)(0)));
	    R9.sd[0] = EE_LW((op1.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    R10.d[0] = ee_ldl(R10.d[0], (op2.w[0] + (uint32_t)(7)));
	    R10.d[0] = ee_ldr(R10.d[0], (op2.w[0] + (uint32_t)(0)));
	    R11.sd[0] = EE_LW((op2.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
	    ee_pcpyld(&R8, &R9, &R8);
	    ee_pcpyld(&R10, &R11, &R10);
	    vu_load_vf(4, &R8);
	    vu_load_vf(5, &R10);
	    vu_upper(U_MUL, 14, 6, 4, 5, BC_NONE);
	    vu_upper(U_ADD, 8, 6, 6, 6, BC_Y);
	    vu_upper(U_ADD, 8, 14, 6, 6, BC_Z);
	    vu_store_vf(&R2, 14);
	    op0.f[0] = ee_u2f(R2.w[0]);
	    ret = op0.f[0];
	}
#endif

    return ret;
}

// 100% matching! 
void njTranslateEx(NJS_VECTOR *v)
{
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl         a4, 0x7(%0)
        ldr         a4,   0(%0)

        lw          a5, NJS_VECTOR.z(%0) 

        pcpyld      a4, a5, a4

        qmtc2.ni    a4, vf4

        lqc2        vf28,    0(%1)
        lqc2        vf29, 0x10(%1)
        lqc2        vf30, 0x20(%1)
        lqc2        vf31, 0x30(%1)

        vmulax.xyz  ACC,  vf28, vf4
        
        vmadday.xyz ACC,  vf29, vf4
        vmaddaz.xyz ACC,  vf30, vf4
        vmaddw.xyz  vf31, vf31, vf0
    
        sqc2        vf31, 0x30(%1)
    .set reorder
    " : : "r"(v), "r"(pNaMatMatrixStuckPtr) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(v);
        op1.sd[0] = (int32_t)(uintptr_t)(pNaMatMatrixStuckPtr);
        R8.d[0] = ee_ldl(R8.d[0], (op0.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op0.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_load_vf(28, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(29, EE_PTR((op1.w[0] + (uint32_t)(16))));
        vu_load_vf(30, EE_PTR((op1.w[0] + (uint32_t)(32))));
        vu_load_vf(31, EE_PTR((op1.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 14, 0, 28, 4, BC_X);
        vu_upper(U_MADDA, 14, 0, 29, 4, BC_Y);
        vu_upper(U_MADDA, 14, 0, 30, 4, BC_Z);
        vu_upper(U_MADD, 14, 31, 31, 0, BC_W);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(48))), 31);
    }
#endif
}

// 100% matching! 
void njRotateEx( Angle *ang, Sint32 lv )
{
	if (lv != 0) 
    {
        njRotateY(NULL, ang[1]);
        njRotateX(NULL, ang[0]);
        njRotateZ(NULL, ang[2]);
    }
    else
    {
        njRotateZ(NULL, ang[2]);
        njRotateY(NULL, ang[1]);
        njRotateX(NULL, ang[0]);
    }
}

// 100% matching! 
void njScaleEx(NJS_VECTOR *v)
{
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2     vf28,    0(%1)
        lqc2     vf29, 0x10(%1)
        lqc2     vf30, 0x20(%1)
        
        ldl      a4, 0x7(%0)
        ldr      a4,   0(%0)
        
        lw       a5, NJS_VECTOR.z(%0) 
        
        pcpyld   a4, a5, a4
        
        qmtc2.ni a4, vf4
        
        vmulx    vf28, vf28, vf4
        vmuly    vf29, vf29, vf4
        vmulz    vf30, vf30, vf4
        
        sqc2     vf28,    0(%1)
        sqc2     vf29, 0x10(%1)
        sqc2     vf30, 0x20(%1)
    .set reorder
    " : : "r"(v), "r"(pNaMatMatrixStuckPtr) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(v);
        op1.sd[0] = (int32_t)(uintptr_t)(pNaMatMatrixStuckPtr);
        vu_load_vf(28, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(29, EE_PTR((op1.w[0] + (uint32_t)(16))));
        vu_load_vf(30, EE_PTR((op1.w[0] + (uint32_t)(32))));
        R8.d[0] = ee_ldl(R8.d[0], (op0.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op0.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op0.w[0] + (uint32_t)(offsetof(NJS_VECTOR, z))));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(4, &R8);
        vu_upper(U_MUL, 15, 28, 28, 4, BC_X);
        vu_upper(U_MUL, 15, 29, 29, 4, BC_Y);
        vu_upper(U_MUL, 15, 30, 30, 4, BC_Z);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(0))), 28);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(16))), 29);
        vu_store_vf(EE_PTR((op1.w[0] + (uint32_t)(32))), 30);
    }
#endif
}

// 100% matching!
Bool njPushMatrixEx( void )
{
    njPushMatrix(NULL);
}

// 100% matching!
Bool njPopMatrixEx( void )
{
    njPopMatrix(1);
}

// 100% matching! 
void njRotTransPers(NJS_POINT3* pPoint, NJS_SCRVECTOR* pScreen)
{
	njMulMatrixCN(&NaViewScreenMatrix, NULL);

    njCalcPointCN(pPoint, (NJS_POINT3*)&pScreen->x);

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        vdiv     Q, vf0w, vf18z

        mfc1     t0, %2

        lw       t1, fNaViwOffsetX
        lw       t2, fNaViwOffsetY

        qmtc2    t0, vf4
        qmtc2    t1, vf5
        qmtc2    t2, vf6

        vwaitq

        vmulq.x  vf8,  vf4,  Q

        vaddq.z  vf14, vf0,  Q

        vmulx.xy vf8,  vf18, vf8
        
        vaddx.x  vf14, vf8,  vf5
        vaddx.y  vf14, vf8,  vf6

        qmfc2    a6, vf14
    
        pcpyud   a7, a6, a6
    
        sdl      a6, 7(%1)
        sdr      a6, 0(%1)
        
        sw       a7, NJS_SCRVECTOR.iz(%1)
    .set reorder
    " : : "r"(pPoint), "r"(pScreen), "f"(_nj_screen_.dist) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        op1.sd[0] = (int32_t)(uintptr_t)(pScreen);
        op2.f[0] = (float)(_nj_screen_.dist);
        vu_div(0, 3, 18, 2);
        R8.sd[0] = (int32_t)(ee_f2u(op2.f[0]));
        R9.sd[0] = EE_LW((uint32_t)(uintptr_t)&fNaViwOffsetX);
        R10.sd[0] = EE_LW((uint32_t)(uintptr_t)&fNaViwOffsetY);
        vu_load_vf(4, &R8);
        vu_load_vf(5, &R9);
        vu_load_vf(6, &R10);
        vu_upper(U_MUL, 8, 8, 4, 0, BC_Q);
        vu_upper(U_ADD, 2, 14, 0, 0, BC_Q);
        vu_upper(U_MUL, 12, 8, 18, 8, BC_X);
        vu_upper(U_ADD, 8, 14, 8, 5, BC_X);
        vu_upper(U_ADD, 4, 14, 8, 6, BC_X);
        vu_store_vf(&R10, 14);
        ee_pcpyud(&R11, &R10, &R10);
        ee_sdl((op1.w[0] + (uint32_t)(7)), R10.d[0]);
        ee_sdr((op1.w[0] + (uint32_t)(0)), R10.d[0]);
        EE_SW((op1.w[0] + (uint32_t)(offsetof(NJS_SCRVECTOR, iz))), R11.d[0]);
    }
#endif

    pScreen->fog = njCalcFogPower(pScreen->z);
}

// 100% matching!
void njRotTrans(NJS_POINT3* pPoint, NJS_POINT3* pOut)
{
	njMulMatrixCN(&NaViewScreenMatrix, NULL);
    
    njCalcPointCN(pPoint, pOut);
}

// 100% matching!
void njPers(NJS_SCRVECTOR* pScreen)
{
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        ldl      t0, 7(%0)
        ldr      t0, 0(%0)
        
        lw       t1, 8(%0)
        
        pcpyld   t0, t1, t0

        qmtc2    t0, vf18

        vdiv     Q, vf0w, vf18z

        mfc1     t0, %1

        lw       t1, fNaViwOffsetX
        lw       t2, fNaViwOffsetY

        qmtc2    t0, vf4
        qmtc2    t1, vf5
        qmtc2    t2, vf6

        vwaitq

        vmulq.x  vf8, vf4, Q

        vaddq.z  vf14, vf0, Q

        vmulx.xy vf8, vf18, vf8
        
        vaddx.x  vf14, vf8, vf5
        vaddx.y  vf14, vf8, vf6

        qmfc2    t2, vf14
    
        pcpyud   t3, t2, t2
    
        sdl      t2, 7(%0)
        sdr      t2, 0(%0)
        
        sw       t3, 12(%0)
    .set reorder
    " : : "r"(pScreen), "f"(_nj_screen_.dist) : 
    );
#else
    { /* asm2c */
        ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pScreen);
        op1.f[0] = (float)(_nj_screen_.dist);
        R8.d[0] = ee_ldl(R8.d[0], (op0.w[0] + (uint32_t)(7)));
        R8.d[0] = ee_ldr(R8.d[0], (op0.w[0] + (uint32_t)(0)));
        R9.sd[0] = EE_LW((op0.w[0] + (uint32_t)(8)));
        ee_pcpyld(&R8, &R9, &R8);
        vu_load_vf(18, &R8);
        vu_div(0, 3, 18, 2);
        R8.sd[0] = (int32_t)(ee_f2u(op1.f[0]));
        R9.sd[0] = EE_LW((uint32_t)(uintptr_t)&fNaViwOffsetX);
        R10.sd[0] = EE_LW((uint32_t)(uintptr_t)&fNaViwOffsetY);
        vu_load_vf(4, &R8);
        vu_load_vf(5, &R9);
        vu_load_vf(6, &R10);
        vu_upper(U_MUL, 8, 8, 4, 0, BC_Q);
        vu_upper(U_ADD, 2, 14, 0, 0, BC_Q);
        vu_upper(U_MUL, 12, 8, 18, 8, BC_X);
        vu_upper(U_ADD, 8, 14, 8, 5, BC_X);
        vu_upper(U_ADD, 4, 14, 8, 6, BC_X);
        vu_store_vf(&R10, 14);
        ee_pcpyud(&R11, &R10, &R10);
        ee_sdl((op0.w[0] + (uint32_t)(7)), R10.d[0]);
        ee_sdr((op0.w[0] + (uint32_t)(0)), R10.d[0]);
        EE_SW((op0.w[0] + (uint32_t)(12)), R11.d[0]);
    }
#endif
}

// 100% matching!
void njCopyMatrix(NJS_MATRIX* pDstMat, NJS_MATRIX* pSrcMat) 
{
#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2 vf4, 0(%1) 
        lqc2 vf5, 0x10(%1) 
        lqc2 vf6, 0x20(%1) 
        lqc2 vf7, 0x30(%1) 
    
        sqc2 vf4, 0(%0) 
        sqc2 vf5, 0x10(%0) 
        sqc2 vf6, 0x20(%0) 
        sqc2 vf7, 0x30(%0) 
    .set reorder
    " : : "r"(pDstMat), "r"(pSrcMat) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pDstMat);
        op1.sd[0] = (int32_t)(uintptr_t)(pSrcMat);
        vu_load_vf(4, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op1.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op1.w[0] + (uint32_t)(32))));
        vu_load_vf(7, EE_PTR((op1.w[0] + (uint32_t)(48))));
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(0))), 4);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(16))), 5);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(32))), 6);
        vu_store_vf(EE_PTR((op0.w[0] + (uint32_t)(48))), 7);
    }
#endif
}

// 100% matching!
void njMulMatrixCN(NJS_MATRIX* pSrcMat1, NJS_MATRIX* pSrcMat2)
{
    if (pSrcMat2 == NULL)
    {
        pSrcMat2 = pNaMatMatrixStuckPtr;
    }

#ifndef TARGET_PC
    asm volatile
    ("
    .set noreorder
        lqc2         vf4,  0(%0)
        lqc2         vf5,  0x10(%0)
        lqc2         vf6,  0x20(%0)
        lqc2         vf7,  0x30(%0)
        lqc2         vf8,  0(%1)
        lqc2         vf9,  0x10(%1)
        lqc2         vf10, 0x20(%1)
        lqc2         vf11, 0x30(%1)

        vmulax.xyzw  ACC,  vf4, vf8
        
        vmadday.xyzw ACC,  vf5, vf8
        vmaddz.xyzw  vf28, vf6, vf8
        
        vmulax.xyzw  ACC,  vf4, vf9

        vmadday.xyzw ACC,  vf5, vf9
        vmaddz.xyzw  vf29, vf6, vf9
        
        vmulax.xyzw  ACC,  vf4, vf10

        vmadday.xyzw ACC,  vf5, vf10
        vmaddz.xyzw  vf30, vf6, vf10
        
        vmulax.xyzw  ACC,  vf4, vf11

        vmadday.xyzw ACC,  vf5, vf11
        vmaddaz.xyzw ACC,  vf6, vf11
        vmaddw.xyzw  vf31, vf7, vf0
    .set reorder
    " : : "r"(pSrcMat1), "r"(pSrcMat2) : 
    );
#else
    { /* asm2c */
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(pSrcMat1);
        op1.sd[0] = (int32_t)(uintptr_t)(pSrcMat2);
        vu_load_vf(4, EE_PTR((op0.w[0] + (uint32_t)(0))));
        vu_load_vf(5, EE_PTR((op0.w[0] + (uint32_t)(16))));
        vu_load_vf(6, EE_PTR((op0.w[0] + (uint32_t)(32))));
        vu_load_vf(7, EE_PTR((op0.w[0] + (uint32_t)(48))));
        vu_load_vf(8, EE_PTR((op1.w[0] + (uint32_t)(0))));
        vu_load_vf(9, EE_PTR((op1.w[0] + (uint32_t)(16))));
        vu_load_vf(10, EE_PTR((op1.w[0] + (uint32_t)(32))));
        vu_load_vf(11, EE_PTR((op1.w[0] + (uint32_t)(48))));
        vu_upper(U_MULA, 15, 0, 4, 8, BC_X);
        vu_upper(U_MADDA, 15, 0, 5, 8, BC_Y);
        vu_upper(U_MADD, 15, 28, 6, 8, BC_Z);
        vu_upper(U_MULA, 15, 0, 4, 9, BC_X);
        vu_upper(U_MADDA, 15, 0, 5, 9, BC_Y);
        vu_upper(U_MADD, 15, 29, 6, 9, BC_Z);
        vu_upper(U_MULA, 15, 0, 4, 10, BC_X);
        vu_upper(U_MADDA, 15, 0, 5, 10, BC_Y);
        vu_upper(U_MADD, 15, 30, 6, 10, BC_Z);
        vu_upper(U_MULA, 15, 0, 4, 11, BC_X);
        vu_upper(U_MADDA, 15, 0, 5, 11, BC_Y);
        vu_upper(U_MADDA, 15, 0, 6, 11, BC_Z);
        vu_upper(U_MADD, 15, 31, 7, 0, BC_W);
    }
#endif
}
