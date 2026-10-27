/*++

  COPYRIGHT		   (C) NEC		  Corporation 1997

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	:		T.Hirata(NEC Software Ltd.)
  Module Name:		kmsrvice.h

  Abstract:
	Kamui is Low-Level PowerVR Driver.
	'kmsrvice.h' is kamui defines header.

  Environment:
	Microsoft Compiler.

  Notes:
	Kamui's enum

  Revision History:

--*/
#if !defined(__kamui_service_h)
#define __kamui_service_h

/*++
	PREFIX:
		Utility library functionfor kamui.	No Depend Hardware./ No Depend Environment
			kmuXxxxXxxxxx	-> please describe kmutil.c / kmutil.h

		No Depend Hardware./ Depend Environment
			kmwXxxxXxxxxx		Win32
			kmfXxxxXxxxxx		Flat

		No Depend Hardware./ Depend Environment
			kmxXxxxXxxxxx

--*/
#if defined(_FLAT_MODEL_)

/*++
kmfSetVertexBufferBaseAddress

proto type:

	KMSTATUS
	kmfSetVertexBufferBaseAddress(
										KMDWORD dwBaseAddress ,
										KMDWORD dwVertexBufferSize
								  );

Description:
	Set Base Address/Buffer Size for Vertex Buffer.


	Keep following Sequence.

		kmInitDevice();
			:
		kmfSetVertexBufferBaseAddress();
			:
		kmCreateVertexBuffer();

Arguments:
	IN	dwBaseAddress
			Base Address for VertexBuffer.

	IN	dwVertexBufferSize
			Size Of VertexBuffer.

Result:
	KMSTATUS_SUCCESS	...	Succeed.


--*/
KMSTATUS
kmfSetVertexBufferBaseAddress(
								IN	KMDWORD dwBaseAddress ,		/* Base Address for Vertex Buffer.*/
								IN	KMDWORD dwVertexBufferSize	/* Size of	Vertex Buffer.		  */
							  );


typedef struct _tagKMISYSTEMMETRICS
{
	struct _tagNativeParams{
		KMDWORD NativeITPBase;
		KMDWORD	NativeOBJBase;
		KMDWORD NativeRegBase;
		KMDWORD NativeITPLimit;
		KMDWORD NativeOBJLimit;
		KMDWORD NativeLastITPCurrent;
	}NativeParams[2];

	struct _tagFBParams{
		KMDWORD FBStartAddr[2];
		KMDWORD FBLineStride[2];
		KMDWORD XClip;
		KMDWORD YClip;
	}FBParams;

	KMDWORD RenderTimeOutCount;
	KMDWORD VertexTimeOutCount;

	KMDWORD	OBJOverflowCount;
	KMDWORD	ITPOverflowCount;

	KMDWORD DMADataAddress[2][5];
	KMDWORD InternalVertexBufSize[5];
	KMDWORD MaxVertexBufSize[5];

	KMDWORD LastTATime;
	KMDWORD	LastRenderTime;
	KMDWORD Reserved2;
	KMDWORD Reserved3;
	KMDWORD Reserved4;
	KMDWORD Reserved5;
	KMDWORD Reserved6;
	KMDWORD Reserved7;

}KMISYSTEMMETRICS, *PKMISYSTEMMETRICS;

/*++
KMSTATUS kmfGetInternalSystemMetrics(PKMISYSTEMMETRICS pSystemMetrics)

Get the internal status which described in KMISYSTEMMETRICS struct.

--*/
KMSTATUS
kmfGetInternalSystemMetrics(PKMISYSTEMMETRICS pSystemMetrics);


KMSTATUS
kmfSetTimeOutCount(KMDWORD nVertexTimeOut, KMDWORD nRenderTimeOut);

#define KMF_YUVCONVERT_ONE_TEXTURE   0
#define KMF_YUVCONVERT_NUMOF_TEXTURE 1

KMSTATUS
kmfInitializeYUVConverter(
							PKMSURFACEDESC pSurface,	/* YUV420 Texture Surface Desc */
							KMDWORD TextureType,		/* KM_TEXTURE_YUV422 or KM_TEXTURE_YUV420 */
							KMDWORD ConvertMode,		/* One Texture Mode or Many Texture Mode */
							KMDWORD xMBSize,			/* X MacroBlock Size */
							KMDWORD yMBSize				/* Y BacroBlock Size */
						 );

KMSTATUS
kmfSetCommonModifier(
							PKMDWORD pCommonBuffer, 	/* Modifier Common Buffer Top Addr(Must be 32byte Aligned */
							KMDWORD BufferSize			/* Buffer Size (must be 0x20 multiple ) */
					);

/*
	Fatal Error発生時のコールバック関数の登録 
*/

KMSTATUS
kmfSetFatalErrorCallback(
		PKMCALLBACKFUNC pFatalErrorCallback
		);

/* Fatal Errorの定義
 * Fatal Errorが発生した場合、コールバック関数のPVOID引数に下記のいづれかの値がセット
 * されます。ボーダーカラーはデフォルトのコールバック関数での挙動です。
 *（ユーザー設定のコールバックが存在する場合には、下記ボーダーカラーの切り替え現象は起こりません。）
 *	 define						ボーダーカラー	エラーの内容
 * KMI_FATAL_ERR_NOQEUE_INTR	明るい青(0xFF)	割り込みハンドラ内からDMAを起動しようとしたがQUEUEに空きがなかった
 * KMI_FATAL_ERR_OVERFLW_VTXBUF 明るい赤(0xFF)	kmSurface内でチェックしたところ登録した頂点数がバッファをオーバーフローしていた
 * KMI_FATAL_ERR_TOO_MANY_VTX	明るい緑(0xFF)	NativeBufferが根本的に足りない。（登録ISP/TSPバッファがアロケートされた
 *																				  Nativeバッファより大きい)
 * KMI_FATAL_ERR_V_NOT_RESPOND	明るい紫(0xFF)	VSYNCウェイト内でシーケンスがまったく進まなくなっている.
 */

#define KMI_FATAL_ERR_NOQEUE_INTR		(1)
#define KMI_FATAL_ERR_OVERFLW_VTXBUF	(2)
#define KMI_FATAL_ERR_TOO_MANY_VTX		(3)
#define KMI_FATAL_ERR_V_NOT_RESPOND		(4)

#endif

#if defined(WIN32)

#endif

#endif

