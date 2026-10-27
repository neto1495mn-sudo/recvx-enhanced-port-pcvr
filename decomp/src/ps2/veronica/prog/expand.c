#include "../../../ps2/veronica/prog/expand.h"

EXPAND_CTRL_BUF ExpandCtrlBuf;

// 100% matching!
void Init_Expand() 
{ 
    ExpandCtrlBuf.flag.abort = 0; 
}

#ifndef TARGET_PC
// 99.63% matching
int Expand(register char* s, register unsigned char* d) 
{
    register int T;

    T = 0;

    asm volatile 
    {
    
        lui     t9, 0xFFFF 
        addiu   t9, t9, 0x7F08
        addiu   t9, t9, 0x7FF8
            
        lui     t8, 0xFFFF 
        addiu   t8, t8, 0x6008
        addiu   t8, t8, 0x7FF8 
            
        lbu     t6, 0(s) 
        
        addi    s,  s,  1 
        
        sub     t7, t7, t7 
        addiu   t7, t7, 9 
        sub     t3, t3, t3 
        addu    t3, t3, d
        
        j       l_002CAEE4 
        
        l_002CAED4:
        lbu     t1, 0(s) 
        
        addi    s, s, 1 
        
        sb      t1, 0(d) 
            
        addi    d, d, 1 
        
        l_002CAEE4:
        addiu   t7, t7, -1 
        
        andi    t4, t6, 0x1 
        srl     t6, t6, 1
        
        bgtz    t7, l_002CAF10         
        vnop   
        
        lbu     t6, 0(s) 
        
        addi    s,  s,  1 
        
        sub     t7, t7, t7 
        addiu   t7, t7, 8 
        
        andi    t4, t6, 0x1 
            
        srl     t6, t6, 1 
            
        l_002CAF10:
        bgtz    t4, l_002CAED4 
    
        addiu   t7, t7, -1 
            
        andi    t4, t6, 0x1 
        srl     t6, t6, 1 
        
        bgtz    t7, l_002CAF44 
        vnop
        
        lbu     t6, 0(s)
            
        addi    s,  s,  1 
        
        sub     t7, t7, t7 
        addiu   t7, t7, 8 
        
        andi    t4, t6, 0x1 
            
        srl     t6, t6, 1 
            
        l_002CAF44:
        bgtz    t4, l_002CAFF0 
        
        sub     t0, t0, t0 
        
        addiu   t7, t7, -1 
            
        andi    t4, t6, 0x1 
        srl     t6, t6, 1
            
        bgtz    t7, l_002CAF7C 
        vnop    
        
        lbu     t6, 0(s) 
            
        addi    s,  s,  1 
        
        sub     t7, t7, t7 
        addiu   t7, t7, 8 
        andi    t4, t6, 0x1 
        srl     t6, t6, 1 
            
        l_002CAF7C:
        sll     t0, t0, 1 
        or      t0, t0, t4 
            
        addiu   t7, t7, -1 
            
        andi    t4, t6, 0x1 
        srl     t6, t6, 1
            
        bgtz    t7, l_002CAFB0 
        vnop     
        
        lbu     t6, 0(s)
            
        addi    s,  s,  1 
        
        sub     t7, t7, t7 
        addiu   t7, t7, 8 
        andi    t4, t6, 0x1 
        srl     t6, t6, 1 
            
        l_002CAFB0:
        lbu     t2, 0(s)
        
        addi    s,  s,  1 
        
        sll     t0, t0, 1 
        or      t0, t0, t4 
        or      t2, t2, t9 
    
        l_002CAFC4:
        addiu   t0, t0, 2 
        addu    t2, d,  t2 
            
        l_002CAFCC:
        lbu     t1, 0(t2) 
            
        addiu   t2, t2, 1 
        
        addiu   t0, t0, -1 
            
        sb      t1, 0(d) 
            
        addi    d, d, 1 
        nop
        
        bgtz    t0, l_002CAFCC 
        vnop    
        
        j       l_002CAEE4 
        vnop    
    
        l_002CAFF0:
        lbu     t0, 0(s) 
        lbu     t1, 1(s)
            
        addi    s,  s,  2 
        
        andi    t2, t0, 0xFF 
        sll     t1, t1, 8 
        or      t2, t2, t1 
            
        sub     t4, t4, t4 
        
        beq     t4, t2, l_002CB040 
        
        srl     t2, t2, 3 
        andi    t0, t0, 0x7 
            
        and     t4, t4, t4 
            
        or      t2, t2, t8
        
        bne     t4, t0, l_002CAFC4 
        
        lbu     t0, 0(s) 
            
        addi    s,  s,  1 
        
        addu    t2, d,  t2 
        andi    t0, t0, 0xFF 
        addiu   t0, t0, 1
            
        j       l_002CAFCC 
        nop
        
        l_002CB040:
        sub     T, d, t3 
        nop
        
    }	

    return T;
} 

#else
/* C version of Expand() provided by Clownacy (verified equivalent to the asm above) */
typedef struct Expand_State
{
    const unsigned char *source;
    unsigned char descriptor_field;
    unsigned char descriptor_bits_remaining;
} Expand_State;
 
static void Expand_RefreshDescriptorField(Expand_State* const state)
{
    state->descriptor_field = *state->source++;
    state->descriptor_bits_remaining = 8;
}
 
static int Expand_GetDescriptorBit(Expand_State* const state)
{
    int bit;
 
    if (--state->descriptor_bits_remaining == 0)
        Expand_RefreshDescriptorField(state);
 
    bit = state->descriptor_field & 1;
    state->descriptor_field >>= 1;
 
    return bit;
}
 
int Expand(register char* s, register unsigned char* d)
{
    unsigned char *destination = d;
    Expand_State state;
 
    state.source = (unsigned char*)s;
    Expand_RefreshDescriptorField(&state);
 
    // This seems like a bug, but the game's data will not decompress correctly without it.
    ++state.descriptor_bits_remaining;
 
    for (;;)
    {
        if (Expand_GetDescriptorBit(&state))
        {
            *destination++ = *state.source++;
        }
        else
        {
            unsigned int length = 0;
            int offset;
 
            if (!Expand_GetDescriptorBit(&state))
            {
                if (Expand_GetDescriptorBit(&state))
                    length += 2;
 
                if (Expand_GetDescriptorBit(&state))
                    ++length;
 
                ++length;
 
                offset = -0x100 + *state.source++;
            }
            else
            {
                const unsigned int lower = *state.source++;
                const unsigned int upper = *state.source++;
                const unsigned int whole = lower | (upper << 8);
 
                if (whole == 0)
                    break;
 
                offset = -0x2000 + (whole >> 3);
                length = whole & 7;
 
                if (length != 0)
                    ++length;
                else
                    length = *state.source++;
            }
 
            do
            {
                *destination = destination[offset];
                ++destination;
            } while (length-- != 0);
        }
    }
 
    return destination - d;
}
#endif
