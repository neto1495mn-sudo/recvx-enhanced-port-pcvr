/*++

  COPYRIGHT        (C) NEC        Corporation 1997 - 1998

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	:       T.Hirata(NEC Software Ltd.,)
  Module Name:      kmenum.h

  Abstract:
    Kamui is Low-Level PowerVR Driver.
    'kmenum.h' is kamui defines header.

  Environment:
    Microsoft Compiler.

  Notes:
    Kamui's enum

  Revision History:

--*/
#if !defined(__kamui_enum_h)
#define __kamui_enum_h

#ifdef __cplusplus
extern "C" {
#endif 


/*
// KM_DSPMODE_XXXX
// ---------------
//  Bit order : Little Endian:?
//  xxxx xxxx xxxx xxxx
//             ||| ||||
//             ||| |||+---- VGA	= 1     ; 0x01  = KM_VGA
//             ||| ||+----- NTSC= 0     ; 0x00  = KM_NTSC
//             ||| ||       PAL = 1     ; 0x02  = KM_PAL
//             ||| |+------ NI  = 0     ; 0x00  = Enable Interlace Flicker Fliter on NTSC/PAL
//             ||| |        I   = 1     ; 0x04  = Interlace
//             ||| +--------PNI = 1     : 0x08  = pseudo-non-interlace
//             ||+--------- 320 = 0     ; 0x00  
//             ||           640 = 1     ; 0x10  
//             |+---------- 240 = 0     ; 0x00  
//             |            480 = 1     ; 0x20  
//             +----------- EXT = 1     ; 0x40  = PAL DisplayArea Extension
//
*/
typedef enum
{
    KM_NTSC = 0,	        /*  xxxx xx00b	*/
    KM_VGA  = 1,	        /*  xxxx xxx1b	*/
    KM_PAL  = 2 	        /*	xxxx xx10b	*/

}KMVIDEOMODE;

/*
 * KMVIDEOMODE
 *
 *	(@1) ... Same line display    		: Even / Odd Frame:
 *	(@2) ... Pseudo-Non-Interlace		: Disable Flicker Filter
 *	(@3) ... Non-Interlace Flicker Free	: Enable Interlace Flicker Fliter (Pseudo-None-Interlace...)
 *           If you select this mode then you have to update each frame data  in 16.66ms.
 *           Your scene data should be able to finish renderring in 16.66ms by PowerVR2 Renderring Engine.
 */

#define KM_DSPMODE_NONINTERLACE			(0x00)      /*  xxxx 00xxb	*/
#define KM_DSPMODE_INTERLACE    		(0x04)      /*  xxxx 01xxb	*/
#define KM_DSPMODE_PSEUDONONINTERLACE	(0x08)      /*  xxxx 10xxb	*/

#define KM_DSPMODE_WIDTH_320    (0x000)           /*  xxx0 xxxxb	*/
#define KM_DSPMODE_WIDTH_640    (0x010)           /*  xxx1 xxxxb	*/

#define KM_DSPMODE_HEIGHT_240   (0x000)           /*  xx0x xxxxb	*/
#define KM_DSPMODE_HEIGHT_480   (0x020)           /*  xx1x xxxxb	*/
#define KM_DSPMODE_HEIGHT_NRM   (0x000)           /*  x0xx xxxxb	*/
#define KM_DSPMODE_HEIGHT_EXT   (0x040)           /*  x1xx xxxxb	*/

#define KM_DSPMODE_320x240      (KM_DSPMODE_WIDTH_320 | KM_DSPMODE_HEIGHT_240)           /*  xx00 xxxxb	*/
#define KM_DSPMODE_640x240      (KM_DSPMODE_WIDTH_640 | KM_DSPMODE_HEIGHT_240)           /*  xx01 xxxxb	*/
#define KM_DSPMODE_320x480      (KM_DSPMODE_WIDTH_320 | KM_DSPMODE_HEIGHT_480)           /*  xx10 xxxxb	*/
#define KM_DSPMODE_640x480      (KM_DSPMODE_WIDTH_640 | KM_DSPMODE_HEIGHT_480)           /*  xx11 xxxxb	*/

#define KM_DSPMODE_640x240_EXT  (KM_DSPMODE_WIDTH_640 | KM_DSPMODE_HEIGHT_240 | KM_DSPMODE_HEIGHT_EXT)
																					     /*  x110 xxxxb	*/
#define KM_DSPMODE_640x480_EXT  (KM_DSPMODE_WIDTH_640 | KM_DSPMODE_HEIGHT_480 | KM_DSPMODE_HEIGHT_EXT)
																					     /*  x111 xxxxb	*/

typedef enum
{
    KM_DSPMODE_VGA	            =	KM_VGA, 												 	   		/* VGA  640x480 60Hz	              			*/
    KM_DSPMODE_NTSCNI320x240	=	(KM_NTSC | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_320x240),		/* NTSC 320x240 60Hz Non-Interlace		  (@1)	*/
    KM_DSPMODE_NTSCI320x240	    =	(KM_NTSC | KM_DSPMODE_INTERLACE          | KM_DSPMODE_320x240),		/* NTSC 320x240 30Hz Interlace					*/
    KM_DSPMODE_NTSCNI640x240	=	(KM_NTSC | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_640x240),		/* NTSC 640x240 60Hz Non-Interlace		  (@1)	*/
    KM_DSPMODE_NTSCI640x240	    =	(KM_NTSC | KM_DSPMODE_INTERLACE          | KM_DSPMODE_640x240),		/* NTSC 640x240 30Hz Interlace					*/
    KM_DSPMODE_NTSCNI320x480FF	=	(KM_NTSC | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_320x480),		/* NTSC 320x480 60Hz Non-Interlace FF     (@3)	*/
    KM_DSPMODE_NTSCNI320x480	=	(KM_NTSC | KM_DSPMODE_PSEUDONONINTERLACE | KM_DSPMODE_320x480),		/* NTSC 320x480 60Hz Pseudo-Non-Interlace (@2)	*/
    KM_DSPMODE_NTSCI320x480	    =	(KM_NTSC | KM_DSPMODE_INTERLACE          | KM_DSPMODE_320x480),		/* NTSC 320x480 30Hz Interlace					*/
    KM_DSPMODE_NTSCNI640x480FF  =	(KM_NTSC | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_640x480),		/* NTSC 640x480 60Hz Non-Interlace FF     (@3)	*/
    KM_DSPMODE_NTSCNI640x480    =	(KM_NTSC | KM_DSPMODE_PSEUDONONINTERLACE | KM_DSPMODE_640x480),		/* NTSC 640x480 60Hz Pseudo-Non-Interlace (@2)	*/
    KM_DSPMODE_NTSCI640x480	    =	(KM_NTSC | KM_DSPMODE_INTERLACE          | KM_DSPMODE_640x480),		/* NTSC 640x480 30Hz Interlace					*/
    KM_DSPMODE_PALNI320x240	    =	(KM_PAL  | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_320x240),		/* PAL  320x240 50Hz Non-Interlace				*/
    KM_DSPMODE_PALI320x240	    =	(KM_PAL  | KM_DSPMODE_INTERLACE          | KM_DSPMODE_320x240),		/* PAL  320x240 25Hz Interlace					*/
    KM_DSPMODE_PALNI640x240	    =	(KM_PAL  | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_640x240),		/* PAL  640x240 50Hz Non-Interlace		  (@1)	*/
    KM_DSPMODE_PALNI640x240EXT  =	(KM_PAL  | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_640x240_EXT),	/* PAL  640x240 50Hz Non-Interlace		  (@1)	*/
    KM_DSPMODE_PALI640x240	    =	(KM_PAL  | KM_DSPMODE_INTERLACE          | KM_DSPMODE_640x240),		/* PAL  640x240 25Hz Interlace					*/
    KM_DSPMODE_PALNI320x480FF   =	(KM_PAL  | KM_DSPMODE_NONINTERLACE       | KM_DSPMODE_320x480),		/* PAL  320x480 50Hz Non-Interlace FF     (@3)	*/
    KM_DSPMODE_PALNI320x480	    =	(KM_PAL  | KM_DSPMODE_PSEUDONONINTERLACE | KM_DSPMODE_320x480),		/* PAL  320x480 50Hz Pseudo-Non-Interlace (@2)	*/
    KM_DSPMODE_PALI320x480	    =	(KM_PAL  | KM_DSPMODE_INTERLACE          | KM_DSPMODE_320x480),		/* PAL  320x480 25Hz Interlace					*/
    KM_DSPMODE_PALNI640x480FF   =	(KM_PAL  | KM_DSPMODE_NONINTERLACE		 | KM_DSPMODE_640x480),		/* PAL  640x480 50Hz Non-Interlace FF     (@3)	*/
    KM_DSPMODE_PALNI640x480	    =	(KM_PAL  | KM_DSPMODE_PSEUDONONINTERLACE | KM_DSPMODE_640x480),		/* PAL  640x480 50Hz Pseudo-Non-Interlace (@2)	*/
    KM_DSPMODE_PALI640x480	    =	(KM_PAL  | KM_DSPMODE_INTERLACE          | KM_DSPMODE_640x480), 	/* PAL  640x480 25Hz Interlace					*/
    KM_DSPMODE_PALNI640x480EXT  =	(KM_PAL  | KM_DSPMODE_PSEUDONONINTERLACE | KM_DSPMODE_640x480_EXT),	/* PAL  640x480 50Hz Pseudo-Non-Interlace (@2)	*/
    KM_DSPMODE_PALI640x480EXT   =	(KM_PAL  | KM_DSPMODE_INTERLACE          | KM_DSPMODE_640x480_EXT) 	/* PAL  640x480 25Hz Interlace					*/
}KMDISPLAYMODE;


/*
 * KMBPPMODE
 */
typedef enum {
    KM_DSPBPP_RGB565	= 0,
    KM_DSPBPP_RGB555	= 1,
/*  KM_DSPBPP_ARGB4444	= 2, Deleted.*/
    KM_DSPBPP_ARGB1555	= 3,
    KM_DSPBPP_RGB888	= 4,
    KM_DSPBPP_ARGB8888	= 5
}KMBPPMODE;

/*
 * DISPLAY Holizontal Level Adjust Mode for User Clipping.
 */
typedef enum _tagKMADJUSTTYPE
{
	KM_LEVEL_ADJUST_NORMAL,
	KM_LEVEL_ADJUST_HALF
}KMADJUSTTYPE;

/*
 * PAL Extended mode Virtical Height
 */
typedef enum {
	KM_PALEXT_HEIGHT_RATIO_1_033 = 0,	/* 247 || 495 */
	KM_PALEXT_HEIGHT_RATIO_1_066 = 1,	/* 255 || 511 */
	KM_PALEXT_HEIGHT_RATIO_1_100 = 2,	/* 263 || 527 */
	KM_PALEXT_HEIGHT_RATIO_1_133 = 3,	/* 271 || 543 */
	KM_PALEXT_HEIGHT_RATIO_1_166 = 4,	/* 279 || 559 */
}KMPALEXTHEIGHT;

/*
 *	SystemConfigration flags
 *         xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx 
 *                      |    |             ||   ||
 *                      |    |             ||   |+--- Enable FrameBuffer Clear 
 *                      |    |             ||   +---- Enable Strip Buffer.
 *                      |    |             |+-------- Enable 2VLaytency Model.
 *                      |    |             +--------- No Wait VSync in kmFlipFrame Buffer.
 *                      |    +----------------------- Use Direct Mode. cf. kmXxxxXxxxDirect();
 *                      +---------------------------- Enable DMAWait
 */
#define KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER	(0x00000001)	/* FrameBuffer: NoClear */
#define KM_CONFIGFLAG_ENABLE_STRIPBUFFER		(0x00000002)	/* Enable Strip Buffer	*/
#define KM_CONFIGFLAG_ENABLE_2V_LATENCY			(0x00000010)	/* Enable 2V Latency	*/
#define KM_CONFIGFLAG_NOWAITVSYNC				(0x00000020)	/* No WaitVsync			*/
#define KM_CONFIGFLAG_USEDIRECTMODE				(0x00010000)	/* DirectMode			*/
#define KM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMA	(0x00100000)	/* Detect FinishTexDMA  */

/*
 * TEXTURE FORMAT TYPE
 */
#define KM_TEXTURE_TWIDDLED	            (0x0100)
#define KM_TEXTURE_TWIDDLED_MM	        (0x0200)
#define KM_TEXTURE_VQ	                (0x0300)
#define KM_TEXTURE_VQ_MM	            (0x0400)
#define KM_TEXTURE_PALETTIZE4	        (0x0500)
#define KM_TEXTURE_PALETTIZE4_MM	    (0x0600)
#define KM_TEXTURE_PALETTIZE8	        (0x0700)
#define KM_TEXTURE_PALETTIZE8_MM	    (0x0800)
#define KM_TEXTURE_RECTANGLE	        (0x0900)
#define KM_TEXTURE_RECTANGLE_MM	        (0x0A00)	/* Reserved : Can't use. */
#define KM_TEXTURE_STRIDE	            (0x0B00)
#define KM_TEXTURE_STRIDE_MM	        (0x0C00)	/* Reserved : Can't use. */
#define KM_TEXTURE_TWIDDLED_RECTANGLE   (0x0D00)	/* CLX1 Only.*/
#define KM_TEXTURE_BMP                  (0x0E00)	/* Converted to Twiddled */
#define KM_TEXTURE_BMP_MM               (0x0F00)	/* Converted to Twiddled MM */
#define KM_TEXTURE_SMALLVQ              (0x1000)
#define KM_TEXTURE_SMALLVQ_MM           (0x1100)

/*
 * TEXTURE COLOR FORMAT
 */
#define KM_TEXTURE_ARGB1555             (0x00)
#define KM_TEXTURE_RGB565               (0x01)
#define KM_TEXTURE_ARGB4444             (0x02)
#define KM_TEXTURE_YUV422               (0x03)
#define KM_TEXTURE_BUMP                 (0x04)
#define KM_TEXTURE_RGB555               (0x05) 	/* for PCX compatible only.	*/
#define KM_TEXTURE_YUV420               (0x06) 	/* for CLX1 YUV converter */

/*
 * Surface Type:
 */
#define KM_SURFACETYPE_FRAMEBUFFER			1
#define KM_SURFACETYPE_TEXTURE				2
#define KM_SURFACETYPE_SMALLVQ_TEXTURE		3


/*
 * Bit Depth
 */
#define KM_BITDEPTH_16		1
#define KM_BITDEPTH_24		2
#define KM_BITDEPTH_32		3

/*
 * Pixel Format.
 */
#define KM_PIXELFORMAT_ARGB1555			(0x00000000)
#define KM_PIXELFORMAT_RGB565			(0x08000000)
#define KM_PIXELFORMAT_ARGB4444			(0x10000000)
#define KM_PIXELFORMAT_YUV422			(0x18000000)
#define KM_PIXELFORMAT_BUMP				(0x20000000)
#define KM_PIXELFORMAT_PALETTIZED_4BPP	(0x28000000)
#define KM_PIXELFORMAT_PALETTIZED_8BPP	(0x30000000)

/*
 * Texture Suraface paramter.
 */
#define KM_SURFACEFLAGS_MIPMAPED		0x001
#define KM_SURFACEFLAGS_CHROMAKEYFIX	0x002
#define KM_SURFACEFLAGS_TWIDDLED		0x004
#define KM_SURFACEFLAGS_VQ				0x008
#define KM_SURFACEFLAGS_STRIDE			0x010
#define KM_SURFACEFLAGS_PALETTIZED		0x020

/*
 * Palette Color Depth
 */
typedef enum{
	KM_PALETTE_16BPP_ARGB1555,
	KM_PALETTE_16BPP_RGB565,
	KM_PALETTE_16BPP_ARGB4444,
	KM_PALETTE_32BPP_ARGB8888
}KMPALETTEMODE;


/*
 * Texture Map size:
 */
#define KM_MAPSIZE_1		1
#define KM_MAPSIZE_2		2
#define KM_MAPSIZE_4		4
#define KM_MAPSIZE_8		8
#define KM_MAPSIZE_16		16
#define KM_MAPSIZE_32		32
#define KM_MAPSIZE_64		64
#define KM_MAPSIZE_128		128
#define KM_MAPSIZE_256		256
#define KM_MAPSIZE_512		512
#define KM_MAPSIZE_1024		1024

/*
 *	MipMap D Adjust
 */
#define KM_MIPMAP_D_ADJUST_0_25		(0x00000001)	/* D=0.25 */
#define KM_MIPMAP_D_ADJUST_0_50		(0x00000002)	/* D=0.50 */
#define KM_MIPMAP_D_ADJUST_0_75		(0x00000003)	/* D=0.75 */
#define KM_MIPMAP_D_ADJUST_1_00		(0x00000004)	/* D=1.00 */
#define KM_MIPMAP_D_ADJUST_1_25		(0x00000005)	/* D=1.25 */
#define KM_MIPMAP_D_ADJUST_1_50		(0x00000006)	/* D=1.50 */
#define KM_MIPMAP_D_ADJUST_1_75		(0x00000007)	/* D=1.75 */
#define KM_MIPMAP_D_ADJUST_2_00		(0x00000008)	/* D=2.00 */
#define KM_MIPMAP_D_ADJUST_2_25		(0x00000009)	/* D=2.25 */
#define KM_MIPMAP_D_ADJUST_2_50		(0x0000000A)	/* D=2.50 */
#define KM_MIPMAP_D_ADJUST_2_75		(0x0000000B)	/* D=2.75 */
#define KM_MIPMAP_D_ADJUST_3_00		(0x0000000C)	/* D=3.00 */
#define KM_MIPMAP_D_ADJUST_3_25		(0x0000000D)	/* D=3.25 */
#define KM_MIPMAP_D_ADJUST_3_50		(0x0000000E)	/* D=3.50 */
#define KM_MIPMAP_D_ADJUST_3_75		(0x0000000F)	/* D=3.75 */

/*
 * ParamType Data for KMVERTEXCONTEXT structure
 */
#define KM_PARAM_POLYGON   			(0x04)	/* 100	*/
#define KM_PARAM_MODIFIERVOLUME		(0x04)	/* 100	*/
#define KM_PARAM_SPRITE				(0x05)	/* 101  */

/*
 * KMPARAMTYPE
 */
typedef enum {
    KM_POLYGON	        = 0,	/* Triange Polygon				*/
    KM_MODIFIERVOLUME	= 1,	/* Modifier Volume(Shadow/Light)*/
    KM_SPRITE	        = 2	    /* Sprite(Quad)					*/
}KMPARAMTYPE;

/*
 * KMCOLORTYPE
 */
typedef enum {
    KM_PACKEDCOLOR	            = 0,	/* 32bit ARGB packed color format	*/
    KM_FLOATINGCOLOR	        = 1,	/* 32bit * 4 floating color format	*/
    KM_INTENSITY	            = 2,	/* Intensity format					*/
    KM_INTENSITY_PREV_FACE_COL	= 3		/* Intensity format(Prev Face Color)*/
}KMCOLORTYPE;

/*
 * KMUVFORMAT
 */
typedef enum {
    KM_32BITUV	                = 0,	/* 32bit float format(UV)	*/
    KM_16BITUV	                = 1	    /* 16bit float format(UV)	*/
}KMUVFORMAT;

/*
 * Modifier Volume Valiant
 */
typedef enum {
    KM_NOMODIFIER	    = 0,	/* No Modifier */
    KM_MODIFIER_A	    = 1	    /* ModifierA   */
}KMMODIFIER;

/*
 * KMLISTTYPE
 */
typedef	enum
{
    KM_OPAQUE_POLYGON		=0,	    /*	Opaque Polygon				*/
    KM_OPAQUE_MODIFIER		=1,	    /*	Opaque Modifier Volume		*/
    KM_TRANS_POLYGON		=2,	    /*	Translucent Polygon			*/
    KM_TRANS_MODIFIER		=3,	    /*	Translucent Modifier Volume	*/
#if defined(_CLX2_)
	KM_PUNCHTHROUGH_POLYGON	=4		/*  PunchThrough Polygon(CLX2)	*/
#else
	KM_PUNCHTHROUGH_POLYGON =2		/*	PunchThrough Polygon -> Trans Polygon(ARC/CLX1)	*/
#endif
} KMLISTTYPE;

/*
 * KMDEPTHMODE
 */
typedef enum
{
    KM_IGNORE	            = 0,
    KM_LESS	                = 1,
    KM_EQUAL	            = 2,
    KM_LESSEQUAL	        = 3,
    KM_GREATER	            = 4,
    KM_NOTEQUAL	            = 5,
    KM_GREATEREQUAL	        = 6,
    KM_ALWAYS	            = 7
} KMDEPTHMODE;

/*
 * KMCULLINGMODE 
 */
typedef enum
{
    KM_NOCULLING	            = 0,    /* No Culling				*/
    KM_CULLSMALL	            = 1,    /* Small Polygon Culling	*/
    KM_CULLCCW	                = 2,    /* Cull Counter Clockwise	*/
    KM_CULLCW	                = 3	    /* Cull Clockwise			*/
} KMCULLINGMODE;


/*
 * KMSCREENCOORRDINATION
 */
typedef enum
{
    KM_SCREEN	                = 0,    /* Screen Coordination		*/
    KM_PROJECTIVE	            = 1     /* Projective Coordination	*/
} KMSCREENCOORDINATION;

/*
 * KMSHADINGMODE
 */
typedef enum
{
    KM_NOTEXTUREFLAT	        = 0,
    KM_NOTEXTUREGOURAUD	        = 1,
    KM_TEXTUREFLAT	            = 2,
    KM_TEXTUREGOURAUD	        = 3
} KMSHADINGMODE;

/*
 * KMFOGMODE
 */
typedef enum
{
    KM_FOGTABLE		=	0,
    KM_FOGVERTEX	=	1,
    KM_NOFOG		=	2,
	KM_FOGTABLE_2	=	3
} KMFOGMODE;

/*
 * KMBLENDINGMODE
 */
typedef enum
{
    KM_BOTHINVSRCALPHA,
    KM_BOTHSRCALPHA,
    KM_DESTALPHA,
    KM_DESTCOLOR,
    KM_INVDESTALPHA,
    KM_INVDESTCOLOR,
    KM_INVSRCALPHA,
    KM_INVSRCCOLOR,
    KM_SRCALPHA,
    KM_SRCCOLOR,
    KM_ONE,
    KM_ZERO

} KMBLENDINGMODE;

/*
 * KMCLAMPMODE
 */

typedef enum
{
    KM_NOCLAMP  = 0,
    KM_CLAMP_V  = 1,
    KM_CLAMP_U  = 2,
    KM_CLAMP_UV = 3
} KMCLAMPMODE;

/*
 * KMFLIPMODE
 */
typedef enum
{
    KM_NOFLIP	= 0,
    KM_FLIP_V   = 1,
    KM_FLIP_U   = 2,
    KM_FLIP_UV  = 3
} KMFLIPMODE;

/*
 * KMFILTERMODE
 */
typedef enum
{
    KM_POINT_SAMPLE     = 0,	/* 00 */
    KM_TRILINEAR_A      = 1,	/* 01 */
    KM_BILINEAR         = 2,	/* 10 */
    KM_TRILINEAR_B      = 3		/* 11 */
} KMFILTERMODE;

/*
 * Texture Shading mode
 */
typedef enum
{
	KM_DECAL			= 0,
    KM_MODULATE         = 1,
    KM_DECAL_ALPHA      = 2,
    KM_MODULATE_ALPHA   = 3
} KMTEXTURESHADINGMODE;

/*
 *  Rendering Context Definisions
 *	    Must be describe PerVertex definisions.
 */
typedef enum
{
    KM_PARAMTYPE            = 0x00100000, /* Parameter Type. */
    KM_LISTTYPE             = 0x00200000, /* List Type. */
	KM_STRIPLENGTH			= 0x08000000, /* Strip Length */
	KM_USERCLIPMODE			= 0x10000000, /* UserClip Mode */
    KM_COLORTYPE            = 0x00400000, /* Color Type. */
    KM_UVFORMAT				= 0x00800000, /* UV Format. */
    KM_DEPTHMODE            = 0x00000001, /* Set Depth mode. */
    KM_CULLINGMODE          = 0x00000002, /* Set Culling mode  */
    KM_SCREENCOORDINATION	= 0x00000004, /* Set Coordinate type */
    KM_SHADINGMODE          = 0x00000008, /* Texture Gouraud, Texture Flat, Non-Texture Gouraud */
    KM_MODIFIER             = 0x00000010, /* Modifier Volume Valiant No or A */
    KM_ZWRITEDISABLE        = 0x00000020, /* Z Write Disable or not */
    KM_SRCBLENDINGMODE      = 0x00000040, /* Blending Mode */
    KM_DSTBLENDINGMODE      = 0x00000080, /* Blending Mode */
    KM_SRCSELECT			= 0x01000000, /* Blending Mode */
    KM_DSTSELECT			= 0x02000000, /* Blending Mode */
	KM_FOGMODE              = 0x00000100, /* Fog Non or Vertex or Table */
    KM_USESPECULAR          = 0x00000200, /* Specular Highlighted or not */
    KM_USEALPHA             = 0x00000400, /* Alpha Blended or not */
    KM_IGNORETEXTUREALPHA   = 0x00000800, /* Ignore Texture Alpha */
    KM_CLAMPUV              = 0x00001000, /* Texture Clamping */
    KM_FLIPUV               = 0x00002000, /* Texture Flipping */
    KM_FILTERMODE           = 0x00004000, /* Point-sample or Bilinear or Trilinear */
    KM_SUPERSAMPLE          = 0x00008000, /* Anisotoropic Filter */
    KM_MIPMAPDADJUST        = 0x00010000, /* MipMap D Adjust */
    KM_TEXTURESHADINGMODE   = 0x00020000, /* Modulate or Decal Alpha or Modulate Alpha */
    KM_COLORCLAMP           = 0x00040000, /* Color Clamping */
    KM_PALETTEBANK          = 0x00080000, /* Palette Bank */
    KM_DCALCEXACT	        = 0x04000000  /* D Calc Exact */
} KMRENDERSTATE;

/*
 * Modifier
 */

#if defined(_CLX_)

#define	KM_MODIFIER_NORMAL_POLY			(0)
#define	KM_MODIFIER_INCLUDE_FIRST_POLY	(KM_MODIFIER_NORMAL_POLY)
#define	KM_MODIFIER_EXCLUDE_FIRST_POLY	(KM_MODIFIER_NORMAL_POLY)
#define	KM_MODIFIER_INCLUDE_LAST_POLY	(1 << 29)
#define	KM_MODIFIER_EXCLUDE_LAST_POLY	(2 << 29)

#else

#define	KM_MODIFIER_NORMAL_POLY			(0)
#define	KM_MODIFIER_INCLUDE_FIRST_POLY	(1 << 29)
#define	KM_MODIFIER_EXCLUDE_FIRST_POLY	(2 << 29)
#define	KM_MODIFIER_INCLUDE_LAST_POLY	(KM_MODIFIER_NORMAL_POLY)
#define	KM_MODIFIER_EXCLUDE_LAST_POLY	(KM_MODIFIER_NORMAL_POLY)

#endif


/*
 * StripType
 */
typedef	enum
{
    KM_STRIP_01  =0,    /* Strip 1	*/
    KM_STRIP_02  =1,    /* Strip 2	*/
    KM_STRIP_04  =2,    /* Strip 4	*/
    KM_STRIP_06  =3	    /* Strip 6	*/
} KMSTRIPLENGTH;

/*
 * UserClip 
 */
typedef	enum
{
    KM_USERCLIP_DISABLE =0,     /*  UserClip Disable	*/
    KM_USERCLIP_RESERVE =1,     /*  Reserved			*/
    KM_USERCLIP_INSIDE  =2,     /*  UserClip Inside		*/
    KM_USERCLIP_OUTSIDE =3	    /*  UserClip Outside	*/
} KMUSERCLIPMODE;


/*
 * Kamui's Vertex Type.
 */
typedef enum {
    KM_VERTEXTYPE_00,           /* Vertex data Type 0	*/
    KM_VERTEXTYPE_01,           /* Vertex data Type 1	*/
    KM_VERTEXTYPE_02,           /* Vertex data Type 2	*/
    KM_VERTEXTYPE_03,           /* Vertex data Type 3	*/
    KM_VERTEXTYPE_04,           /* Vertex data Type 4	*/
    KM_VERTEXTYPE_05,           /* Vertex data Type 5	*/
    KM_VERTEXTYPE_06,           /* Vertex data Type 6	*/
    KM_VERTEXTYPE_07,           /* Vertex data Type 7	*/
    KM_VERTEXTYPE_08,           /* Vertex data Type 8	*/
    KM_VERTEXTYPE_09,           /* Vertex data Type 9	*/
    KM_VERTEXTYPE_10,           /* Vertex data Type 10	*/
    KM_VERTEXTYPE_11,           /* Vertex data Type 11	*/
    KM_VERTEXTYPE_12,           /* Vertex data Type 12	*/
    KM_VERTEXTYPE_13,           /* Vertex data Type 13	*/
    KM_VERTEXTYPE_14,           /* Vertex data Type 14	*/
    KM_VERTEXTYPE_15,           /* Vertex data Type 15	*/
    KM_VERTEXTYPE_16,           /* Vertex data Type 16	*/
    KM_VERTEXTYPE_17            /* Vertex data Type 17	*/
}KMVERTEXTYPE;

/*
 * ActiveListType
 */
#define	KM_ACTIVE_OPAQUE_POLYGON		(	1 << KM_OPAQUE_POLYGON			)
#define	KM_ACTIVE_OPAQUE_MODIFIER		(	1 << KM_OPAQUE_MODIFIER			)
#define	KM_ACTIVE_TRANS_POLYGON			(	1 << KM_TRANS_POLYGON			)
#define	KM_ACTIVE_TRANS_MODIFIER		(	1 << KM_TRANS_MODIFIER			)
#define	KM_ACTIVE_PUNCHTHROUGH_POLYGON	(	1 << KM_PUNCHTHROUGH_POLYGON	)

/*
 * Latency Model
 */
typedef	enum
{
    KM_LATENCYMODEL_3V =0,     /*  3V Latency Model(Default)	*/
    KM_LATENCYMODEL_2V =1      /*  2V Latency Model			*/
} KMLATENCYMODEL;

/*
 * VertexBuffer Model
 */
typedef	enum
{
	KM_VERTEXBUFMODEL_NORMAL 					= 0x00,
	KM_VERTEXBUFMODEL_FLUSH_OPAQUE			  	= 0x10,
	KM_VERTEXBUFMODEL_FLUSH_OPAQUE_MODIFIER  	= 0x11,
	KM_VERTEXBUFMODEL_FLUSH_TRANS  				= 0x12,
	KM_VERTEXBUFMODEL_FLUSH_TRANS_MODIFIER  	= 0x13,
	KM_VERTEXBUFMODEL_FLUSH_PUNCH_THROUGH 	 	= 0x14,
	KM_VERTEXBUFMODEL_NOBUF_OPAQUE  			= 0x20,
	KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER  	= 0x21,
	KM_VERTEXBUFMODEL_NOBUF_TRANS  				= 0x22,
	KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER  	= 0x23,
	KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH 	 	= 0x24
} KMVERTEXBUFMODEL;

/*
 * Vertex Flag(Normal or EndofStrip)
 */
#define KM_VERTEXPARAM_NORMAL			(0xE0000000)
#define KM_VERTEXPARAM_ENDOFSTRIP		(0xF0000000)

#ifdef __cplusplus
}
#endif 

#endif /*  __kamui_enum_h	*/

