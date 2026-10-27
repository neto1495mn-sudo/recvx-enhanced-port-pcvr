/*++

  COPYRIGHT        (C) NEC        Corporation 1997

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	:  		T.Hirata(NEC Software Ltd.,)
  Module Name:		kmstruct.h

  Abstract:
	Kamui is Low-Level PowerVR Driver.
	'kmstruct.h' is kamui structure defines header.

  Environment:
	Microsoft Compiler.

  Notes:
	Kamui's structure.

  Revision History:
	97.08.19	Separeted from kmdefs.h

--*/
#if !defined(__kamui_struct_h)
#define __kamui_struct_h

#ifdef __cplusplus
extern "C" {
#endif 

/*++
 KMVERSIONINFO
 -------------
	KAMUI Library Version Information.

--*/
/*
 * Kamui Version Information structure
 */
typedef struct KMVERSIONINFO
{
	KMDWORD kmMajorVersion;  		/* ARC1 with IRIS 	= 1 */
									/* ARC1 with COSMOS	= 2	*/
									/* HOLLY			= 3	*/
	KMDWORD kmLocalVersion;
	KMDWORD kmFrameBufferSize; 		/* Total Size of TextureÅïFrameBuffer */
} KMVERSIONINFO, *PKMVERSIONINFO;


/*++
Color and Palette Structure.
----------------------------


--*/
#if defined(_X86_)
#pragma pack(1)
#endif

/*
 * 32 bit packed color
 */
typedef union _tagKMPACKEDARGB
{
	KMDWORD	dwPacked;
	struct {
		KMBYTE	bBlue;
		KMBYTE	bGreen;
		KMBYTE	bRed;
		KMBYTE	bAlpha;
	}byte;
}KMPACKEDARGB,*PKMPACKEDARGB;

/*
 * PowerVR II OnChip Palette
 */

#define KM_PALETTE_ENTRY (1024)

typedef union _tagKMPALETTEDATA
{
	KMDWORD	wPaletteData [KM_PALETTE_ENTRY];
	KMDWORD	dwPaletteData[KM_PALETTE_ENTRY];
}KMPALETTEDATA,*PKMPALETTEDATA;

#if defined(_X86_)
#pragma pack()
#endif

/*
 * FrameBuffer/TextureSurface
 */

typedef struct tagKMSURFACEDESC
{
	KMDWORD	SurfaceType;				/* 0:FrameBuffer    1:Texture	*/
	KMDWORD	BitDepth;					/* 16bpp...16   32bpp...32  etc	*/
 	KMDWORD	PixelFormat;				/* 1555,4444,etc				*/

	union{
		KMDWORD	USize;					/* USize 8-1024					*/
		KMDWORD	Width;					/* FB/Tex(Width)				*/
	}
#if defined(_STRICT_UNION_)
	u0
#endif
	;

	union{
		KMDWORD	VSize;					/* VSize 8-1024					*/
		KMDWORD	Height;					/* FB/Tex(Height)				*/
	}
#if defined(_STRICT_UNION_)
	u1
#endif
	;

	union{
		KMDWORD	dwTextureSize;			/* TextureSize					*/
		KMDWORD	dwFrameBufferSize;		/* FrameBufferSize				*/
	}
#if defined(_STRICT_UNION_)
	uSize
#endif
	;

	KMDWORD		fSurfaceFlags;			/* Surface Flags				*/
	PKMDWORD	pSurface;				/* Texture instance(Pointer on PVRII Core)	*/
	PKMDWORD	pVirtual;				/* Texture instance(Virtual  address on SH4)*/
	PKMDWORD	pPhysical;				/* Texture instance(physical address on SH4)*/

}KMSURFACEDESC;
typedef KMSURFACEDESC* PKMSURFACEDESC;
typedef PKMSURFACEDESC* PPKMSURFACEDESC;

/*
 * Base address of each VertexList.
 */
typedef struct _tagKMVERTEXBUFFERPOINTER
{
	PKMDWORD pOpaquePolygonBuffer;				/* Pointer to Top of Opaque polygon buffer					*/
	PKMDWORD pOpaqueModifierBuffer;				/* Pointer to Top of Opaque modifier polygon buffer			*/
	PKMDWORD pTransPolygonBuffer;				/* Pointer to Top of Translusent polygon buffer				*/
	PKMDWORD pTransModifierBuffer;				/* Pointer to Top of Translucent modifier polygon buffer	*/
	PKMDWORD pPunchThroughPolygonBuffer;		/* Pointer to Top of Punch through polygon buffer			*/
}KMVERTEXBUFFERPOINTER,*PKMVERTEXBUFFERPOINTER,**PPKMVERTEXBUFFERPOINTER;

/*
 * Current Vertex Buffer List Statistics
 */
typedef struct _tagKMCURRENTLISTSTATE
{
	KMDWORD					ListType;				/* List type : 0-3 								*/
	KMDWORD					GlobalParamSize;		/* Size of Global parameter : 0x20 or 0x40		*/
	KMDWORD					Reserved0;				/* Reserved 									*/
	KMDWORD					VertexBank;				/* Bank# of Vertex buffer : 0 or 1				*/
}KMCURRENTLISTSTATE,*PKMCURRENTLISTSTATE;

/*
 * VertexBuffer DESC
 */
typedef struct _tagKMVERTEXBUFFDESC
{
	PKMDWORD					*pCurrentPtr;		/* Pointer to top of CurrentVertex pointer array	*/
	PKMDWORD					pGlobalParam;		/* Pointer to Global parameter work area			*/
	PKMCURRENTLISTSTATE			pCurrentListState;	/* Pointer to KMCURRENTLISTSTATE structure			*/
	PPKMVERTEXBUFFERPOINTER		ppBuffer;			/* Pointer to Vertex buffer pointer structure		*/
	KMDWORD						fActiveList;		/* Active List	 									*/
	KMDWORD						Reserved1;			/* 32byte Align										*/
	KMDWORD						Reserved2;			/* 32byte Align										*/
	KMDWORD						Reserved3;			/* 32byte Align										*/
}KMVERTEXBUFFDESC,*PKMVERTEXBUFFDESC;

/*
 * SystemConfigration Struct
 */
typedef struct	_tagKMSYSTEMCONFIGSTRUCT {
	/* Size Of KMSYSTEMCONFIGSTRUCT */
	KMDWORD				dwSize;

	/* System Configuration Flags */
	KMDWORD				flags;

	/* for Frame Buffer */
	PPKMSURFACEDESC		ppSurfaceDescArray;			/* Array of SurfaceDesc	pointer     */
	KMUINT32			nNumOfFrameBuffer;			/* Number Of Frame Buffer			*/
	KMUINT32			nWidth;						/* Width of Frame buffer			*/
	KMUINT32			nHeight;					/* Height of Frame buffer			*/
	KMBPPMODE			nBpp;						/* Bit Per Depth for Frame buffer	*/

	/* for Texture Memory */
	KMUINT32			nTextureMemorySize;			/* Texture Memory size				*/

	/* for Vertex Buffer */
	PKMVERTEXBUFFDESC	pBufferDesc;				/* pointer to VERTEXBUFFDESC 		*/
	PKMDWORD			pVertexBuffer;				/* pointer to Vertex buffer 		*/
	KMUINT32			nVertexBufferSize;			/* size of Vertex buffer 			*/
	KMFLOAT				fBufferSize[5];				/* buffer size in percent			*/

	/* Reserve Area */
	KMDWORD				reserved01;					/* reserved for future use 			*/
	KMDWORD				reserved02;					/* reserved for future use 			*/
	KMDWORD				reserved03;					/* reserved for future use 			*/
	KMDWORD				reserved04;					/* reserved for future use 			*/
	KMDWORD				reserved05;					/* reserved for future use 			*/
	KMDWORD				reserved06;					/* reserved for future use 			*/
	KMDWORD				reserved07;					/* reserved for future use 			*/

	/* VertexBuffer Model */
	KMVERTEXBUFMODEL 	nVbufModel;					/* VertexBuffer Model				*/

} KMSYSTEMCONFIGSTRUCT, *PKMSYSTEMCONFIGSTRUCT;

/*
 * Callback functions
 */
typedef VOID (* PKMCALLBACKFUNC)( PKMVOID pArgs );

/*
 * for PAL Extension Mode
 */
typedef struct _tagKMPALEXTINFO
{
	IN	KMDISPLAYMODE	nDisplayMode;
	OUT	KMINT32			nPALExtMode;
} KMPALEXTINFO,*PKMPALEXTINFO;



#ifdef __cplusplus
}
#endif 

#endif /*  __kamui_struct_h	*/

