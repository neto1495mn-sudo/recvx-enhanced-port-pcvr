/*++

  COPYRIGHT        (C) NEC        Corporation 1997

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	 : 		T.Hirata(NEC Software Ltd.,)
  Module Name:		kmstatus.h

  Abstract:
	KAMUI = Kamui is Abstruction Model that is Ultimate Interface for PowerVR.
	'kmstatus.h' is kamui status header.

  Environment:

  Notes:
	KMSTATUS_XXXXX style

  Revision History:

--*/
#if !defined(__kamui_status_h)
#define __kamui_status_h


/*
 * Kamui Function result is OK. means NO ERROR.
 */
#define	KMSTATUS_SUCCESS                     		0L


/*
 *	
 */
#define KMSTATUS_INVALID_DISPLAY_MODE        		1L

/*
 *
 */
#define KMSTATUS_INVALID_VIDEO_MODE        			2L

/*
 *
 */
#define KMSTATUS_NOT_ENOUGH_MEMORY           		3L

/*
 * 
 */
#define KMSTATUS_INVALID_TEXTURE_TYPE				4L

/*
 * 
 */
#define KMSTATUS_INVALID_MIPMAPED_TEXTURE			5L

/*
 * 
 */
#define KMSTATUS_CANT_FLIP_SURFACE					6L

/*
 * 
 */
#define KMSTATUS_INVALID_ADDRESS					7L

/*
 * 
 */
#define KMSTATUS_HARDWARE_NOT_PRESENTED				8L


/*
 * 
 */
#define KMSTATUS_INVALID_SETTING					9L

/*
 * 
 */
#define KMSTATUS_INVALID_VERTEX_TYPE				10L

/*
 * Vertex List type.
 */
#define KMSTATUS_INVALID_LIST_TYPE					11L

/*
 * for kmLoadTextureBlock
 */
#define KMSTATUS_INVALID_BLOCKNUMBER				12L

/*
 * for kmQueryFinishFlushVertexBuffer
 */
#define KMSTATUS_LAST_LIST_UNDER_DMA				13L

/*
 * for kmActivateFrameBuffer
 */
#define KMSTATUS_INVALID_SURFACE					14L

#define KMSTATUS_INVALID_SURFACE_TYPE				15L

#define KMSTATUS_INVALID_SURFACE_SIZE				16L

/*
 * for kmQueryFinishLastTextureDMA
 */
#define KMSTATUS_NOT_FINISH_DMA						17L

/*
 * passed value is out of range. 
 */
#define KMSTATUS_OUT_OF_RANGE						18L

/*
 * called function is not implemented.
 */
#define KMSTATUS_NOT_IMPLEMENTED					120L

/*
 * Invalid parameter ... temporary error code....
 *	玉虫色のエラーコードだから、将来は破棄する。
 */
#define KMSTATUS_INVALID_PARAMETER					0x0ddddL

#endif
