/****************************************************************************
 *
 *	SH4 specific support - equivalent to _builtin_ Hitachi support routines
 *	(see machine.h in sh/include)
 *
 *****************************************************************************/

#ifndef _SH4_H
#define _SH4_H


/*****************************************************************************
 *
 *	Get status register
 *
 *****************************************************************************/

static inline unsigned int get_sr(void)
{
	register unsigned int sr = 0;
	
	asm
	{
		STC	SR,	sr
	};

	return sr;
}


/*****************************************************************************
 *
 *	Set status register
 *
 *****************************************************************************/
static inline void set_sr(unsigned int sr)
{
	register unsigned int value = sr;
	
	asm
	{
		LDC	value,	SR
	};
}



/*****************************************************************************
 *
 *	Get interrupt mask
 *
 *****************************************************************************/
static inline unsigned int get_imask(void)
{

	register unsigned int imask = 0;
	
	asm
	{
		STC	SR,	imask
	};


	imask = (imask >> 4) & 0x0f;

	return imask;
}



/*****************************************************************************
 *
 * Set interrupt mask
 *
 *****************************************************************************/
static inline void set_imask(int level)
{
	register unsigned int sr = 0;
		
	asm
	{
		STC	SR, sr
	};

	sr = (sr & 0xffffff0f) | ((level & 0x0f) << 4);

	asm
	{
		LDC	sr, SR
	};
}


/*****************************************************************************
 *
 * Set status register BL bit (interrupt block)
 *
/*****************************************************************************/
static inline void set_sr_bl(void)
{
	register unsigned int sr = 0;
		
	asm
	{
		STC	SR, sr
	};
		
	sr |= 0x10000000;
		
	asm
	{
		LDC	sr, SR
	};
}



/*****************************************************************************
 *
 * Clear status register BL bit (interrupt block)
 *
/*****************************************************************************/
static inline void clear_sr_bl(void)
{
	register unsigned int sr = 0;
		
	asm
	{
		STC	SR, sr
	};
		
	sr &= ~0x10000000;
		
	asm
	{
		LDC	sr, SR
	};
}



/*****************************************************************************
 *
 * Set r0-r7 register bank to 1
 *
 *****************************************************************************/
static inline void set_sr_rb(void)
{
	register unsigned int sr = 0;
		
	asm
	{
		STC	SR, sr
	};
		
	sr |= 0x20000000;
		
	asm
	{
		LDC	sr, SR
	};
}



/*****************************************************************************
 *
 * Set r0-r7 register bank to 0
 *
 *****************************************************************************/
static inline void clear_sr_rb(void)
{
	register unsigned int sr = 0;
		
	asm
	{
		STC	SR,	sr
	};
		
	sr &= ~0x20000000;
		
	asm
	{
		LDC	sr,	SR
	};
}


/*****************************************************************************
 *
 * Get vbr register
 *
 *****************************************************************************/
static inline unsigned int get_vbr(void)
{
	register unsigned int value;
		
	asm
	{
		STC	VBR, value
	};

	return value;
}

/*****************************************************************************
 *
 * prefetch
 *
 *****************************************************************************/
static inline void prefetch(register void* p)
{
	asm
	{
		PREF @p;
	};

}

/*****************************************************************************
 *
 *	fake strcmp intrinsic
 *
 *****************************************************************************/


static inline int _builtin_strcmp(const char * str1, const char * str2)
{
	const	unsigned char * p1 = (unsigned char *) str1;
	const	unsigned char * p2 = (unsigned char *) str2;
			unsigned char	c1, c2;
	
	while ((c1 = *p1++) == (c2 = *p2++))
		if (!c1)
			return(0);

	return(c1 - c2);
}

/*	Intrinsic functions	*/

#define set_fpscr(cr)                  __set_fpscr(cr)
#define get_fpscr()                    __get_fpscr()
#define fipr(vec1, vec2)               __fipr(vec1, vec2)
#define ftrv(vec1, vec2)               __ftrv(vec1, vec2)
#define ftrvadd(vec1, vec2, vec3)      __ftrvadd(vec1, vec2, vec3)
#define ftrvsub(vec1, vec2, vec3)      __ftrvsub(vec1, vec2, vec3)
#define add4(vec1, vec2, vec3)         __add4(vec1, vec2, vec3)
#define sub4(vec1, vec2, vec3)         __sub4(vec1, vec2, vec3)
#define mtrx4mul(mx1, mx2)             __mtrx4mul(mx1, mx2)
#define mtrx4muladd(mx1, mx2, mx3)     __mtrx4muladd(mx1, mx2, mx3)
#define mtrx4mulsub(mx1, mx2, mx3)     __mtrx4mulsub(mx1, mx2, mx3)
#define ld_ext(mx)                     __ld_ext(mx)
#define st_ext(mx)                     __st_ext(mx)

/*	Private intrisics	*/
#ifdef fsca
#undef fsca
#define fsca(rad,sinv,cosv)   __fsca(rad,sinv,cosv)
#endif
#ifdef fsrra
#undef fsrra
#define fsrra(v)              __fsrra(v)
#endif

/*	maps hitachi's strcpy intrinsic to ours	*/
#define _builtin_strcpy(s1,s2)	__strcpy(s1,s2)

#endif	// _SH4_H
