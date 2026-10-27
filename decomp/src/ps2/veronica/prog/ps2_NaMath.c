#ifdef TARGET_PC
#include "ee_asm.h"
#endif
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/main.h"

static float SinTable[16384];

// 100% matching! 
void _Make_SinTable()
{ 
    int i;

    for (i = 0; i < 16384; i++) 
    { 
        SinTable[i] = sinf(0.0000958738f * i); 
    } 
}  

// 100% matching!
Float	njSin(Angle n)
{
    float ret;
    
#ifndef TARGET_PC
    asm volatile 
    ("
    .set noreorder
        move  v0, %0
        
        lw    t0, 0(%1)
        
        addi  t2, zero, 16383
    
        andi  t3, t0, 0x4000
        
        beqz  t3, l_002D7D08
        
        and   t1, t0, t2
    
        sub   t2, t2, t1
    
        muli  t2, t2, 4
    
        add   t2, t2, %2
        
        lwc1  f4, 0(t2)
        
        neg.s f6, f4
        
        mfc1  %2, f6
        
        andi  t0, t0, 0x8000
    
        mfc1  t6, f4
        
        b     l_002D7D30
    
        movz  %2, t6, t0

        l_002D7D08:
        muli  t1, t1, 4
        
        add   t1, t1, %2
        
        lwc1  f4, 0(t1)
        
        neg.s f6, f4
        
        mfc1  %2, f4
        mfc1  t6, f6
    
        andi  t0, t0, 0x8000
        
        movn  %2, t6, t0

        l_002D7D30:
        mtc1  %2, f0
    .set reorder
    " : "=r"(ret) : "r"(&n), "r"(SinTable) : 
    );
#else
    { /* hand-translated: v = SinTable[n & 0x3FFF] (mirrored in the 2nd/4th quadrant), negated for n & 0x8000 */
        int idx = n & 0x3FFF;
        float v = (n & 0x4000) ? SinTable[0x3FFF - idx] : SinTable[idx];
        ret = (n & 0x8000) ? -v : v;
        return ret; /* the asm returned through $f0 */
    }
#endif
}

// 100% matching!
Float	njCos(Angle n)
{
	float ret;

#ifndef TARGET_PC
	asm volatile 
    ("
    .set noreorder
        move  v0, %0
        
        lw    t0, 0(%1)
        
        addi  t2, zero, 16383
    
        andi  t3, t0, 0x4000
        
        beqz  t3, l_002D7D94
        
        and   t1, t0, t2
    
        muli  t1, t1, 4
    
        add   t1, t1, %2
        
        lwc1  f5, 0(t1)
        
        neg.s f7, f5
        
        mfc1  %2, f5
		mfc1  t7, f7
        
        andi  t0, t0, 0x8000

        b     l_002D7DC0

        movz  %2, t7, t0

        l_002D7D94:
		sub   t2, t2, t1

        muli  t2, t2, 4
        
        add   t2, t2, %2
        
        lwc1  f5, 0(t2)
        
        neg.s f7, f5
        
        mfc1  %2, f5
        mfc1  t7, f7
    
        andi  t0, t0, 0x8000
        
        movn  %2, t7, t0

        l_002D7DC0:
        mtc1  %2, f0
    .set reorder
    " : "=r"(ret) : "r"(&n), "r"(SinTable) : 
    );
#else
	{ /* hand-translated */
	    int idx = n & 0x3FFF;
	    if (n & 0x4000)
	        ret = (n & 0x8000) ? SinTable[idx] : -SinTable[idx];
	    else
	        ret = (n & 0x8000) ? -SinTable[0x3FFF - idx] : SinTable[0x3FFF - idx];
	    return ret; /* the asm returned through $f0 */
	}
#endif
}

// 100% matching!
void njSinCos(int lAngle, float* sin, float* cos)
{
#ifndef TARGET_PC
    asm volatile 
    ("
    .set noreorder
        move  v1, %3 
        
        lw    t0, 0(%0) 
        
        addi  t2, zero, 16383 
    
        andi  t3, t0, 0x4000 
        
        beqz  t3, l_002D7E4C 
        
        and   t1, t0, t2
    
        sub   t2, t2, t1 
    
        muli  t1, t1, 4 
        muli  t2, t2, 4 
        
        add   t1, t1, v1 
        add   t2, t2, v1 
    
        lwc1  f4, 0(t2) 
        lwc1  f5, 0(t1) 
        
        neg.s f6, f4 
        neg.s f7, f5
        
        mfc1  t4, f6
        mfc1  t5, f5
        mfc1  t6, f4
        mfc1  t7, f7
    
        andi  t0, t0, 0x8000
    
        movz  t4, t6, t0
        
        b     l_002D7E9C
    
        movz  t5, t7, t0
    
        l_002D7E4C:
        sub   t2, t2, t1 
        
        muli  t1, t1, 4
        muli  t2, t2, 4
        
        add   t1, t1, v1 
        add   t2, t2, v1 
    
        lwc1  f4, 0(t1)
        lwc1  f5, 0(t2)
        
        neg.s f6, f4
        neg.s f7, f5
        
        mfc1  t4, f4
        mfc1  t5, f5
        mfc1  t6, f6
        mfc1  t7, f7
    
        andi  t0, t0, 0x8000
    
        movn  t4, t6, t0
        movn  t5, t7, t0
    
        l_002D7E9C:
        sw    t4, 0(%1)
        sw    t5, 0(%2)
        
        qmtc2 t4, vf11
        qmtc2 t5, vf12
    .set reorder
    " : : "r"(&lAngle), "r"(sin), "r"(cos), "r"(SinTable) : 
    );
#else
    { /* asm2c */
        ee_reg R3 = {{0}}, R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R12 = {{0}}, R13 = {{0}}, R14 = {{0}}, R15 = {{0}};
        float F4 = 0.0f, F5 = 0.0f, F6 = 0.0f, F7 = 0.0f;
        int ee_cond;
        const ee_reg ee_zero = {{0}};
        ee_reg op0 = {{0}};
        ee_reg op1 = {{0}};
        ee_reg op2 = {{0}};
        ee_reg op3 = {{0}};
        op0.sd[0] = (int32_t)(uintptr_t)(&lAngle);
        op1.sd[0] = (int32_t)(uintptr_t)(sin);
        op2.sd[0] = (int32_t)(uintptr_t)(cos);
        op3.sd[0] = (int32_t)(uintptr_t)(SinTable);
        R3.d[0] = op3.d[0];
        R8.sd[0] = EE_LW((op0.w[0] + (uint32_t)(0)));
        R10.sd[0] = (int32_t)(ee_zero.w[0] + (uint32_t)(16383));
        R11.d[0] = R8.d[0] & (uint64_t)(uint16_t)(16384);
        ee_cond = (R11.sd[0] == 0);
        R9.d[0] = R8.d[0] & R10.d[0];
        if (ee_cond) goto l_002D7E4C;
        R10.sd[0] = (int32_t)(R10.w[0] + -R9.w[0]);
        R9.sd[0] = (int32_t)(R9.w[0] * (uint32_t)(4));
        R10.sd[0] = (int32_t)(R10.w[0] * (uint32_t)(4));
        R9.sd[0] = (int32_t)(R9.w[0] + R3.w[0]);
        R10.sd[0] = (int32_t)(R10.w[0] + R3.w[0]);
        F4 = EE_LWF((R10.w[0] + (uint32_t)(0)));
        F5 = EE_LWF((R9.w[0] + (uint32_t)(0)));
        F6 = -F4;
        F7 = -F5;
        R12.sd[0] = (int32_t)(ee_f2u(F6));
        R13.sd[0] = (int32_t)(ee_f2u(F5));
        R14.sd[0] = (int32_t)(ee_f2u(F4));
        R15.sd[0] = (int32_t)(ee_f2u(F7));
        R8.d[0] = R8.d[0] & (uint64_t)(uint16_t)(32768);
        if (R8.d[0] == 0) R12.d[0] = R14.d[0];
        if (R8.d[0] == 0) R13.d[0] = R15.d[0];
        goto l_002D7E9C;
    l_002D7E4C:;
        R10.sd[0] = (int32_t)(R10.w[0] + -R9.w[0]);
        R9.sd[0] = (int32_t)(R9.w[0] * (uint32_t)(4));
        R10.sd[0] = (int32_t)(R10.w[0] * (uint32_t)(4));
        R9.sd[0] = (int32_t)(R9.w[0] + R3.w[0]);
        R10.sd[0] = (int32_t)(R10.w[0] + R3.w[0]);
        F4 = EE_LWF((R9.w[0] + (uint32_t)(0)));
        F5 = EE_LWF((R10.w[0] + (uint32_t)(0)));
        F6 = -F4;
        F7 = -F5;
        R12.sd[0] = (int32_t)(ee_f2u(F4));
        R13.sd[0] = (int32_t)(ee_f2u(F5));
        R14.sd[0] = (int32_t)(ee_f2u(F6));
        R15.sd[0] = (int32_t)(ee_f2u(F7));
        R8.d[0] = R8.d[0] & (uint64_t)(uint16_t)(32768);
        if (R8.d[0] != 0) R12.d[0] = R14.d[0];
        if (R8.d[0] != 0) R13.d[0] = R15.d[0];
    l_002D7E9C:;
        EE_SW((op1.w[0] + (uint32_t)(0)), R12.d[0]);
        EE_SW((op2.w[0] + (uint32_t)(0)), R13.d[0]);
        vu_load_vf(11, &R12);
        vu_load_vf(12, &R13);
    }
#endif
}

// 100% matching! 
Float	njFraction  (Float n) 
{ 
    return n - floorf(n); 
} 

// 100% matching! 
Float	njSqrt(Float n)
{
	float ret;

	ret = 0;
    
#ifndef TARGET_PC
    asm volatile
    ("
        mfc1    t0, f12

        qmtc2   t0, vf8
    
        vsqrt   Q,  vf8

        vwaitq 

        vaddq.x vf8, vf0, Q
        
        qmfc2   v0,  vf8
    
        mtc1    v0, %0
    " : "=f"(ret) : : 
    );
#else
    { /* hand-translated: Q = sqrt(|n|); vf8 = { Q, sext(n), -, - } as left by the asm */
        union { float f; int32_t i; } in = { n };
        g_vu0.vf[8].f[0] = n; g_vu0.vf[8].i[1] = in.i >> 31;
        vu_sqrt(8, 0);
        g_vu0.vf[8].f[0] = vu_clampf(0.0f + g_vu0.q);
        ret = g_vu0.vf[8].f[0];
    }
#endif

    return ret;
}

// 100% matching! 
Float	njInvertSqrt(Float n)
{
    float ret;

    ret = 0;
    
#ifndef TARGET_PC
    asm volatile
    ("
        mfc1    t0, f12

        qmtc2   t0, vf8
    
        vrsqrt  Q,  vf0w, vf8

        vwaitq 

        vaddq.x vf8, vf0, Q
        
        qmfc2   v0,  vf8
    
        mtc1    v0, %0
    " : "=f"(ret) : : 
    );
#else
    { /* hand-translated: Q = 1/sqrt(|n|); vf8 = { Q, sext(n), -, - } as left by the asm */
        union { float f; int32_t i; } in = { n };
        g_vu0.vf[8].f[0] = n; g_vu0.vf[8].i[1] = in.i >> 31;
        vu_rsqrt(0, 3, 8, 0);
        g_vu0.vf[8].f[0] = vu_clampf(0.0f + g_vu0.q);
        ret = g_vu0.vf[8].f[0];
    }
#endif

    return ret;
}

// 100% matching! 
void	njLinear(Float *idata, Float *odata, NJS_SPLINE *attr, Float frame)
{
#ifndef TARGET_PC
	asm volatile
    ("
	    addi      a4, %0, 12

		mfc1      a5, %3

        ldl       a6, 0x7(%0)
        ldr       a6,   0(%0)
     
        lw        a7,   8(%0) 

        ldl       t4, 0x7(a4)
        ldr       t4,   0(a4)
     
        lw        t5,   8(a4) 
     
        pcpyld    a6, a7, a6
        pcpyld    t4, t5, t4
     
        qmtc2.ni  a6, vf10
        qmtc2.ni  t4, vf11
		qmtc2.ni  a5, vf9
		
		vsub.xyz  vf12, vf11, vf10

		vmulx.xyz vf12, vf12, vf9

		vadd.xyz  vf4,  vf12, vf10

		qmfc2.ni  a6, vf4

		pcpyud    a7, a6, a6

		sdl       a6, 0x7(%1)
        sdr       a6,   0(%1)
     
        sw        a7,   8(%1) 
    " : : "r"(idata), "r"(odata), "r"(attr), "f"(frame) : 
    );
#else
	{ /* asm2c */
	    ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R12 = {{0}}, R13 = {{0}};
	    ee_reg op0 = {{0}};
	    ee_reg op1 = {{0}};
	    ee_reg op3 = {{0}};
	    op0.sd[0] = (int32_t)(uintptr_t)(idata);
	    op1.sd[0] = (int32_t)(uintptr_t)(odata);
	    op3.f[0] = (float)(frame);
	    R8.sd[0] = (int32_t)(op0.w[0] + (uint32_t)(12));
	    R9.sd[0] = (int32_t)(ee_f2u(op3.f[0]));
	    R10.d[0] = ee_ldl(R10.d[0], (op0.w[0] + (uint32_t)(7)));
	    R10.d[0] = ee_ldr(R10.d[0], (op0.w[0] + (uint32_t)(0)));
	    R11.sd[0] = EE_LW((op0.w[0] + (uint32_t)(8)));
	    R12.d[0] = ee_ldl(R12.d[0], (R8.w[0] + (uint32_t)(7)));
	    R12.d[0] = ee_ldr(R12.d[0], (R8.w[0] + (uint32_t)(0)));
	    R13.sd[0] = EE_LW((R8.w[0] + (uint32_t)(8)));
	    ee_pcpyld(&R10, &R11, &R10);
	    ee_pcpyld(&R12, &R13, &R12);
	    vu_load_vf(10, &R10);
	    vu_load_vf(11, &R12);
	    vu_load_vf(9, &R9);
	    vu_upper(U_SUB, 14, 12, 11, 10, BC_NONE);
	    vu_upper(U_MUL, 14, 12, 12, 9, BC_X);
	    vu_upper(U_ADD, 14, 4, 12, 10, BC_NONE);
	    vu_store_vf(&R10, 4);
	    ee_pcpyud(&R11, &R10, &R10);
	    ee_sdl((op1.w[0] + (uint32_t)(7)), R10.d[0]);
	    ee_sdr((op1.w[0] + (uint32_t)(0)), R10.d[0]);
	    EE_SW((op1.w[0] + (uint32_t)(8)), R11.d[0]);
	}
#endif
}

// 100% matching! 
void	njOverhauserSpline(Float *idata, Float *odata, NJS_SPLINE *attr, Float frame)
{
	float ftmp; 

	ftmp = 0.5f;

#ifndef TARGET_PC
	asm volatile
    ("
		mfc1      a4, %3

		vaddw.x   vf12,  vf0, vf0
		vaddw.x   vf12, vf12, vf0

		qmtc2.ni  a4, vf4

		vmul.x    vf5,  vf4, vf4
		vmul.x    vf6,  vf5, vf4

		vadd.x    vf8,  vf5, vf5

		vsub.x    vf8,  vf8, vf6
		vsub.x    vf8,  vf8, vf4

		vmulz.x   vf10, vf5, vf2
		vmul.x    vf9,  vf6, vf2

		vsub.x    vf9,  vf9, vf10

		vadd.x    vf9,  vf9, vf12

		vsub.x    vf7,  vf0, vf2

		vmulx.x   vf7,  vf6, vf7
		vmuly.x   vf10, vf5, vf2

		vadd.x    vf10, vf10, vf4
		vadd.x    vf10, vf10, vf7

		vsub.x    vf11, vf6, vf5

		mfc1      a5, %4
		nop

		qmtc2.ni  a5, vf12

        ldl       a4,  0x7(%0)
        ldr       a4,    0(%0)
     
        lw        a5,    8(%0) 

        ldl       a6, 0x13(%0)
        ldr       a6,  0xC(%0)
     
        lw        a7,   20(%0) 

		ldl       t4, 0x1F(%0)
        ldr       t4, 0x18(%0)
     
        lw        t5,   32(%0) 

        ldl       t6, 0x2B(%0)
        ldr       t6, 0x24(%0)
     
        lw        t7,   44(%0) 
     
		pcpyld    a4, a5, a4
		pcpyld    a6, a7, a6
        pcpyld    t4, t5, t4
        pcpyld    t6, t7, t6
     
		qmtc2.ni  a4, vf4
		qmtc2.ni  a6, vf5
		qmtc2.ni  t4, vf6
		qmtc2.ni  t6, vf7
		
		vmulx.xyz vf4, vf4, vf8
		vmulx.xyz vf5, vf5, vf9
		vmulx.xyz vf6, vf6, vf10
		vmulx.xyz vf7, vf7, vf11

		vadd.xyz  vf4, vf4, vf5
		vadd.xyz  vf4, vf4, vf6
		vadd.xyz  vf4, vf4, vf7

		vmulx.xyz vf4, vf4, vf12

		qmfc2.ni  a6, vf4

		pcpyud    a7, a6, a6

		sdl       a6, 0x7(%1)
        sdr       a6,   0(%1)
     
        sw        a7,   8(%1) 
    " : : "r"(idata), "r"(odata), "r"(attr), "f"(frame), "f"(ftmp) : 
    );
#else
	{ /* asm2c */
	    ee_reg R8 = {{0}}, R9 = {{0}}, R10 = {{0}}, R11 = {{0}}, R12 = {{0}}, R13 = {{0}}, R14 = {{0}}, R15 = {{0}};
	    ee_reg op0 = {{0}};
	    ee_reg op1 = {{0}};
	    ee_reg op3 = {{0}};
	    ee_reg op4 = {{0}};
	    op0.sd[0] = (int32_t)(uintptr_t)(idata);
	    op1.sd[0] = (int32_t)(uintptr_t)(odata);
	    op3.f[0] = (float)(frame);
	    op4.f[0] = (float)(ftmp);
	    R8.sd[0] = (int32_t)(ee_f2u(op3.f[0]));
	    vu_upper(U_ADD, 8, 12, 0, 0, BC_W);
	    vu_upper(U_ADD, 8, 12, 12, 0, BC_W);
	    vu_load_vf(4, &R8);
	    vu_upper(U_MUL, 8, 5, 4, 4, BC_NONE);
	    vu_upper(U_MUL, 8, 6, 5, 4, BC_NONE);
	    vu_upper(U_ADD, 8, 8, 5, 5, BC_NONE);
	    vu_upper(U_SUB, 8, 8, 8, 6, BC_NONE);
	    vu_upper(U_SUB, 8, 8, 8, 4, BC_NONE);
	    vu_upper(U_MUL, 8, 10, 5, 2, BC_Z);
	    vu_upper(U_MUL, 8, 9, 6, 2, BC_NONE);
	    vu_upper(U_SUB, 8, 9, 9, 10, BC_NONE);
	    vu_upper(U_ADD, 8, 9, 9, 12, BC_NONE);
	    vu_upper(U_SUB, 8, 7, 0, 2, BC_NONE);
	    vu_upper(U_MUL, 8, 7, 6, 7, BC_X);
	    vu_upper(U_MUL, 8, 10, 5, 2, BC_Y);
	    vu_upper(U_ADD, 8, 10, 10, 4, BC_NONE);
	    vu_upper(U_ADD, 8, 10, 10, 7, BC_NONE);
	    vu_upper(U_SUB, 8, 11, 6, 5, BC_NONE);
	    R9.sd[0] = (int32_t)(ee_f2u(op4.f[0]));
	    vu_load_vf(12, &R9);
	    R8.d[0] = ee_ldl(R8.d[0], (op0.w[0] + (uint32_t)(7)));
	    R8.d[0] = ee_ldr(R8.d[0], (op0.w[0] + (uint32_t)(0)));
	    R9.sd[0] = EE_LW((op0.w[0] + (uint32_t)(8)));
	    R10.d[0] = ee_ldl(R10.d[0], (op0.w[0] + (uint32_t)(19)));
	    R10.d[0] = ee_ldr(R10.d[0], (op0.w[0] + (uint32_t)(12)));
	    R11.sd[0] = EE_LW((op0.w[0] + (uint32_t)(20)));
	    R12.d[0] = ee_ldl(R12.d[0], (op0.w[0] + (uint32_t)(31)));
	    R12.d[0] = ee_ldr(R12.d[0], (op0.w[0] + (uint32_t)(24)));
	    R13.sd[0] = EE_LW((op0.w[0] + (uint32_t)(32)));
	    R14.d[0] = ee_ldl(R14.d[0], (op0.w[0] + (uint32_t)(43)));
	    R14.d[0] = ee_ldr(R14.d[0], (op0.w[0] + (uint32_t)(36)));
	    R15.sd[0] = EE_LW((op0.w[0] + (uint32_t)(44)));
	    ee_pcpyld(&R8, &R9, &R8);
	    ee_pcpyld(&R10, &R11, &R10);
	    ee_pcpyld(&R12, &R13, &R12);
	    ee_pcpyld(&R14, &R15, &R14);
	    vu_load_vf(4, &R8);
	    vu_load_vf(5, &R10);
	    vu_load_vf(6, &R12);
	    vu_load_vf(7, &R14);
	    vu_upper(U_MUL, 14, 4, 4, 8, BC_X);
	    vu_upper(U_MUL, 14, 5, 5, 9, BC_X);
	    vu_upper(U_MUL, 14, 6, 6, 10, BC_X);
	    vu_upper(U_MUL, 14, 7, 7, 11, BC_X);
	    vu_upper(U_ADD, 14, 4, 4, 5, BC_NONE);
	    vu_upper(U_ADD, 14, 4, 4, 6, BC_NONE);
	    vu_upper(U_ADD, 14, 4, 4, 7, BC_NONE);
	    vu_upper(U_MUL, 14, 4, 4, 12, BC_X);
	    vu_store_vf(&R10, 4);
	    ee_pcpyud(&R11, &R10, &R10);
	    ee_sdl((op1.w[0] + (uint32_t)(7)), R10.d[0]);
	    ee_sdr((op1.w[0] + (uint32_t)(0)), R10.d[0]);
	    EE_SW((op1.w[0] + (uint32_t)(8)), R11.d[0]);
	}
#endif
}

// 100% matching! 
void	njBezierSpline(Float *idata, Float *odata, NJS_SPLINE *attr, Float frame) 
{
    unsigned int ulCnt; 
    unsigned int ulMax; 
    float fFactMax;     
    float fResult;      
    
    odata[0] = 0;
    odata[1] = 0;
    odata[2] = 0;
    
    ulMax = *attr->iparam - 1;
    
    fResult = njFactorial(ulMax);

    for (ulCnt = 0; ulCnt <= ulMax; ulCnt++)  
    {            
        fFactMax = (powf(frame, ulCnt) * (fResult / (njFactorial(ulCnt) * njFactorial(ulMax - ulCnt)))) * powf(1.0f - frame, ulMax - ulCnt);
        
        odata[0] += fFactMax * *idata++;
        odata[1] += fFactMax * *idata++;
        odata[2] += fFactMax * *idata++; 
    }
}

// 100% matching! 
unsigned int njFactorial(unsigned int ulN) 
{ 
    unsigned int ulResult;

    ulResult = 1;
    
    for ( ; ulN != 0; ulN--)
    { 
        ulResult *= ulN; 
    }
    
    return ulResult; 
}
