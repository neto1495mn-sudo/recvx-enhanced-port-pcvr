/*++

  COPYRIGHT        (C) NEC        Corporation 1997

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	:  		T.Muto(NEC Software Ltd.,)
  Module Name:		kmutil.h

  Abstract:
	KAMUI = Kamui is abstruction model that is ultimate interface for PowerVR.
	'kmutil.h' utilities functions for KAMUI.

  Environment:
	Microsoft Compiler.

  Notes:

  Revision History:

--*/

#if !defined(__kamui_utility_h)
#define __kamui_utility_h

#ifdef __cplusplus
extern "C" {
#endif 

#include "kamui.h"

/*
 * KMUPASSSTATUS is kmuXxxPassTable Status.
 */
typedef KMUINT32	KMUPASSSTATUS;

/*
 *	for kmutil target
 */
#define	KMU_TARGET_ARC1			(0x00000001)
#define	KMU_TARGET_CLX1			(0x00000002)
#define	KMU_TARGET_CLX2			(0x00000003)

/*
 *	for kmuXxxPassTable
 */
#define KMU_PASS_OK							 0L
#define KMU_PASS_ERROR_VERTEXCONTEXT		 1L
#define KMU_PASS_ERROR_VERTEXCONTEXT_PASS	 2L
#define KMU_PASS_ERROR_PARAMTYPE			 3L
#define KMU_PASS_ERROR_LISTTYPE				 4L
#define KMU_PASS_ERROR_MIPMAP_D_ADJUST		 5L
#define KMU_PASS_ERROR_FOGMODE				 6L
#define KMU_PASS_ERROR_FILTERMODE			 7L
#define KMU_PASS_ERROR_TEXTURESHADINGMODE	 8L
#define KMU_PASS_ERROR_COLORTYPE			 9L
#define KMU_PASS_ERROR_SHADINGMODE			10L
#define KMU_PASS_ERROR_USERCLIPMODE			11L
#define KMU_PASS_ERROR_TRILINEAR_SETTING	12L
#define KMU_PASS_ERROR_SPRITE_SETTING		13L
#define KMU_PASS_ERROR_BLENDINGMODE_SETTING	14L
#define KMU_PASS_ERROR_MODIFIER_SETTING		15L

#define KMU_PASS_ERROR_NOT_IMPLEMENTED	   120L

/*++
kmuSetTarget

proto type:

	KMSTATUS
	kmuSetTarget(
			IN	KMDWORD				dwTarget
		);

Description:
	神威ユーティリティライブラリの動作Targetを設定します。
	(デフォルトの設定は、ビルドしたときのTargetです)

Arguments:
	IN		dwTarget		神威ユーティリティライブラリの動作Targetを設定します。
							設定は、下記の中のいずれかを指定します。
								KMU_TARGET_ARC1			動作Targetを ARC1にする
								KMU_TARGET_CLX1			動作Targetを CLX1にする
								KMU_TARGET_CLX2			動作Targetを CLX2にする

Result:
	KMSTATUS_SUCCESS				設定成功

--*/

KMSTATUS KMAPI
kmuSetTarget(
		IN	KMDWORD				dwTarget
	);



/*++
kmuCreateTwiddledTexture

proto type:

	KMSTATUS
	kmuCreateTwiddledTexture(
			PKMDWORD		pOutputTexture,
			PKMDWORD		pInputTexture,
			KMBOOLEAN		bAutoMipMap,
			KMBOOLEAN		bDither,
			KMINT32			USize,
			KMTEXTURETYPE	nTextureType
		);

Description:
	メインメモリ上のKM_TEXTURE_BMP形式(ABGR8888)のテクスチャを、
	Twiddled / Twiddled Mipmap 形式のテクスチャに変換します。
	bAutoMipMapにTRUEを指定した場合は、ミップマップの自動生成を行います。
	bDitherにTRUEを指定した場合は、ディザを行います。
	注意：ミップマップ、ディザの指定を行ったときは
			入力テクスチャデータの内容は破壊されます。

Arguments:
	OUT	pOutputTexture	変換後のテクスチャデータを書き込むメインメモリ上のアドレスです。
	IN	pInputTexture	KM_TEXTURE_BMP形式の入力テクスチャを示すポインタです。
	IN	bAutoMipMap		自動的にMIPMAPを生成するかどうかを指定します。
						TRUEが指定された場合には自動的にMIPMAPを生成します。
						(出力はKM_TEXTURE_TWIDDLED_MM形式になります)
						FALSEが指定された場合にはMIPMAP生成は行いません。
						(出力はKM_TEXTURE_TWIDDLED形式になります)
	IN	bDither			ディザをかけるかどうかを指定します。
						TRUEが指定された場合はディザをかけます。
	IN	USize			テクスチャの一辺のテクセル数を指定します。
						下記の中のいずれかを指定します。
						KM_MAPSIZE_8
						KM_MAPSIZE_16
						KM_MAPSIZE_32
						KM_MAPSIZE_64
						KM_MAPSIZE_128
						KM_MAPSIZE_256
						KM_MAPSIZE_512
						KM_MAPSIZE_1024
	IN	nTextureType	変換後のテクスチャのピクセルフォーマットを指定します。
						下記の中のいずれかを指定します。
						KM_PIXELFORMAT_ARGB1555
						KM_PIXELFORMAT_RGB565
						KM_PIXELFORMAT_ARGB4444

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプ、サイズを指定した

--*/

KMSTATUS KMAPI
kmuCreateTwiddledTexture(
	PKMDWORD		pOutputTexture,
	PKMDWORD		pInputTexture,
	KMBOOLEAN		bAutoMipMap,
	KMBOOLEAN		bDither,
	KMINT32			USize,
	KMTEXTURETYPE	nTextureType
	);


/*++
kmuConvertFBtoBMP

proto type:

	KMSTATUS
	kmuConvertFBtoBMP(
			PKMDWORD		pOutputData,
			PKMDWORD		pInputData,
			KMINT32			nWidth,
			KMINT32			nHeight,
			KMBPPMODE		nBpp
		);

Description:
	kmGetTextureでメインメモリ上に読み出したフレームバッファの内容を、
	Windows フルカラーBMP形式のピクセルデータとして、メモリ上に書き出します。
	なお、この出力にはWindows BMPのヘッダ54バイトは生成されません。

Arguments:
	OUT	pOutputData		変換後のピクセルデータを書き込むメインメモリ上のアドレスです。
	IN	pInputData		フレームバッファの内容を示すポインタです。
						kmGetTextureで、フレームバッファサーフェスへのディスクリプタを
						指定して読み出したフレームバッファのピクセルデータを指していなければ
						いけません。KAMUIテクスチャデータは指定不可能です。
	IN	nWidth,nHeight	読み出したフレームバッファのサイズを指定します。
	IN	nBpp			読み出したフレームバッファのピクセルフォーマットを指定します。
						下記の中のいずれかを指定します。
							KM_DSPBPP_RGB565
							KM_DSPBPP_RGB555
							KM_DSPBPP_ARGB4444
							KM_DSPBPP_ARGB1555

Result:
	KMSTATUS_SUCCESS					読み込み成功

--*/

KMSTATUS KMAPI
kmuConvertFBtoBMP(
		PKMDWORD		pOutputData,
		PKMDWORD		pInputData,
		KMINT32			nWidth,
		KMINT32			nHeight,
		KMBPPMODE		nBpp
		);


/*++
kmuGeneratePassTable

proto type:

	KMUPASSSTATUS
	kmuGeneratePassTable(
			IN	PKMVERTEXCONTEXT	pVertexContext,
			IN	KMUINT32			nNumContext,
			OUT	PPKMVERTEXCONTEXT	ppVertexContextTable,
			OUT	PKMUINT32			pPass
		);

Description:
	設定されたレンダリング指定(pVertexContext)をもとに、マルチパス(Trilinear)の各Passで
	必要なコンテキストを生成します。
	  pVertexContextにTrilinearの1Pass目のKMVERTEXCONTEXTを入力すると、その設定値より
	各Passで必要なKMVERTEXCONTEXTを生成します(Opaque:2Pass,Trans:3Pass)
	  TransPolygonのTrilinearの場合に3Pass目のBlendingModeの設定は任意に設定可能ですが、
	本関数を使用した場合には、下記の設定になります。
	(TransPolygonを使用した場合の3Pass目)
		SRCBlendingMode	:: KM_SRCALPHA
		DSTBlendingMode	:: KM_INVSRCALPHA
	ppVertexContextTableにNULLが指定された場合はpPassへ必要なパス数のみが返却します。

Arguments:
	IN		pVertexContext			レンダリング条件を指定するKMVERTEXCONTEXTへのポインタ
									を設定します。
	IN		nNumContext				準備したppVertexContextTableのエントリ数(パス数)を設定します。
									これが実際に必要なパス数より小さい場合は、
									KMU_PASS_ERROR_VERTEXCONTEXT_PASSが返され、
									pPassへ必要なパス数のみが返されて終了します。
	OUT		ppVertexContextTable	生成されたマルチパス用KMVERTEXCONTEXTを受け取るための、
									KMVERTEXCONTEXTへのポインタの配列へのポインタを設定します。
									ここにNULLが指定された場合はpPassへ必要なパス数のみを返します。
	OUT		pPass					設定されたレンダリング指定で必要となるマルチパス数を返します。

Result:
	KMU_PASS_OK							設定成功
	KMU_PASS_ERROR_NOT_IMPLEMENTED		現在のシステムでは、本関数は対応してません
	KMU_PASS_ERROR_VERTEXCONTEXT		pVertexContextが不正(NULL)です
	KMU_PASS_ERROR_VERTEXCONTEXT_PASS	指定したパス数(nNumContext)では不足しています
--*/

KMUPASSSTATUS KMAPI
kmuGeneratePassTable(
		IN	PKMVERTEXCONTEXT	pVertexContext,
		IN	KMUINT32			nNumContext,
		OUT	PPKMVERTEXCONTEXT	ppVertexContextTable,
		OUT	PKMUINT32			pPass
	);


/*++
kmuCheckPassTable

proto type:

	KMUPASSSTATUS
	kmuCheckPassTable(
			IN	PPKMVERTEXCONTEXT	ppVertexContextTable,
			IN	KMUINT32			nNumContext,
			OUT	PKMUINT32			pPass
		);

Description:
	指定されたKMVERTEXCONTEXTテーブルの各コンテキストの内容が正しいか否かをチェックします。

Arguments:
	IN		ppVertexContextTable	準備したKMVERTEXCONTEXTへのポインタの配列へのポインタを設定します。
	IN		nNumContext				準備したppVertexContextTableのエントリ数を設定します。
	OUT		pPass					エラーが見つかった場合には、間違っているKMVERTEXCONTENTの
									エントリ番号をセットします。

Result:
	KMU_PASS_OK								設定成功
	KMU_PASS_ERROR_VERTEXCONTEXT			KMVERTEXCONTEXTが不正(NULL)です
	KMU_PASS_ERROR_VERTEXCONTEXT_PASS		nNumContextが不正(1未満)です
	KMU_PASS_ERROR_PARAMTYPE				KMVERTEXCONTEXTの ParamTypeが不正です
	KMU_PASS_ERROR_LISTTYPE					KMVERTEXCONTEXTの ListTypeが不正です
	KMU_PASS_ERROR_MIPMAP_D_ADJUST			KMVERTEXCONTEXTの MipMapAdjustが不正です
	KMU_PASS_ERROR_FOGMODE					KMVERTEXCONTEXTの FogModeが不正です
	KMU_PASS_ERROR_FILTERMODE				KMVERTEXCONTEXTの FilterModeが不正です
	KMU_PASS_ERROR_TEXTURESHADINGMODE		KMVERTEXCONTEXTの TextureShadingModeが不正です
	KMU_PASS_ERROR_COLORTYPE				KMVERTEXCONTEXTの ColorTypeが不正です
	KMU_PASS_ERROR_SHADINGMODE				KMVERTEXCONTEXTの ShadingModeが不正です
	KMU_PASS_ERROR_USERCLIPMODE				KMVERTEXCONTEXTの UserClipModeが不正です
	KMU_PASS_ERROR_TRILINEAR_SETTING		Trilinear設定の組み合わせが不正です
											 KMVERTEXCONTEXTの ListType/ShadingMode/FilterMode
											SRCBlendingMode/DSTBlendingMode/bSRCSel/bSRCSel
											のいずれかが、Trilinearで使用できない設定です。
	KMU_PASS_ERROR_SPRITE_SETTING			Sprite設定の組み合わせが不正です
											 KMVERTEXCONTEXTの ShadingMode/ColorType/UVFormat
											のいずれかが、Spriteで使用できない設定です。
	KMU_PASS_ERROR_BLENDINGMODE_SETTING		BlendingMode設定の組み合わせが不正です
											 OpaquePolygon使用時(ListType:KM_OPAQUE_POLYGON)に
											KMVERTEXCONTEXTの SRCBlendingMode/DSTBlendingModeで
											設定不可の組み合わせが指定された場合です
											(SRCBlendingMode:KM_ONE , DSTBlendingMode:KM_ZERO以外)
	KMU_PASS_ERROR_MODIFIER_SETTING			Modifier設定の組み合わせが不正です
											 Modifier使用時(ParamType:KM_MODIFIERVOLUME)に
											KMVERTEXCONTEXTの ModifierInstructionが不正な場合です。

--*/

KMUPASSSTATUS KMAPI
kmuCheckPassTable(
		IN	PPKMVERTEXCONTEXT	ppVertexContextTable,
		IN	KMUINT32			nNumContext,
		OUT	PKMUINT32			pPass
	);



#ifdef __cplusplus
}
#endif 

#endif
