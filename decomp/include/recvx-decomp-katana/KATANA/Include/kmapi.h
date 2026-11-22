/*++

  COPYRIGHT        (C) NEC        Corporation 1997 - 1998

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	:  		T.Hirata(NEC Software Ltd.,)
  Module Name:		kmapi.h

  Abstract:
	KAMUI = Kamui is abstruction model that is ultimate interface for PowerVR.
	'kmapi.h' define prototype of kamui function.

  Environment:
	Microsoft Compiler.

  Notes:
	define Kamui's api proto-types.

  Revision History:

--*/

#if !defined(__kamui_api_h)
#define __kamui_api_h

#ifdef __cplusplus
extern "C" {
#endif 

/*++
kmInitDevice

proto type:

	KMSTATUS
	kmInitDevice (KMVIDEOMODE nVideoMode);

Description:
	ハードウェアの初期化を行います。ビデオ信号を出力し、BLANK画面を表示します。

Arguments:
	IN	nVideoMode
		ビデオモードを指定します。下記のいずれかが指定できます。

			KM_NTSC	... NTSC圏(北米、日本など)
			KM_PAL	... PAL圏 (ヨーロッパなど)
			KM_VGA	... VGA (ARC1)

		ARC1では、どれを指定してもKM_VGAが指定されたものとみなされます。



Result:

	KMSTATUS_SUCCESS				デバイス初期化成功
	KMSTATUS_INVALID_VIDEOMODE		不正なビデオモードを指定した。
	KMSTATUS_HARDWARE_NOT_PRESENTED	ハードウェアが使用できない。

--*/
KMSTATUS KMAPI
kmInitDevice (
		IN	KMVIDEOMODE nVideoMode
	);


/*++
kmSetDisplayMode

proto type:

	KMSTATUS
	kmSetDisplayMode(
			IN	KMDISPLAYMODE	nDisplayMode,
			IN	KMBPPMODE		nBpp,
			IN	KMBOOLEAN		fDither,
			IN	KMBOOLEAN		fAntiAlias
		);

Description:
	Frame Bufferの表示モードを設定します。

Arguments:
	IN	nDisplayMode
	ディスプレイモードを指定します。
		KM_DSPMODE_VGA				VGA(640x480) 60Hz
		KM_DSPMODE_NTSCNI320x240	320x240 ノンインタレース60Hz
		KM_DSPMODE_NTSCI320x240		320x240 インタレース 30Hz
		KM_DSPMODE_NTSCNI320x480	320x240 疑似ノンインターレース 60Hz
		KM_DSPMODE_NTSCNI320x480FF	320x240 疑似ノンインターレース 60Hz フリッカーフリー
		KM_DSPMODE_NTSCI320x480		320x240 インタレース 30Hz
		KM_DSPMODE_NTSCNI640x240	640x240 ノンインタレース60Hz
		KM_DSPMODE_NTSCI640x240		640x240 インタレース30Hz
		KM_DSPMODE_NTSCNI640x480	640x480 疑似ノンインターレース 60Hz
		KM_DSPMODE_NTSCNI640x480FF	640x480 疑似ノンインターレース 60Hz フリッカーフリー
		KM_DSPMODE_NTSCI640x480		640x480 インタレース 30Hz

		KM_DSPMODE_PALNI320x240		320x240 ノンインタレース50Hz
		KM_DSPMODE_PALI320x240		320x240 インタレース25Hz
		KM_DSPMODE_PALNI320x480		320x480 疑似ノンインターレース 50Hz
		KM_DSPMODE_PALNI320x480FF	320x480 疑似ノンインターレース 50Hz フリッカーフリー
		KM_DSPMODE_PALI320x480		320x480 インタレース25Hz
		KM_DSPMODE_PALNI640x240		640x240 ノンインタレース50Hz
		KM_DSPMODE_PALI640x240	 	640x240 インタレース25Hz
		KM_DSPMODE_PALNI640x480		640x480 疑似ノンインターレース 50Hz
		KM_DSPMODE_PALNI640x480FF	640x480 疑似ノンインターレース 50Hz フリッカーフリー
		KM_DSPMODE_PALI640x480		640x480 インタレース25Hz

	IN	nBpp
	FrameBufferのカラーモードを指定します。
	以下の定義済み定数を使用することができます。
		KM_DSPBPP_RGB565		RGB565 
		KM_DSPBPP_RGB555		RGB555
		KM_DSPBPP_ARGB1555		ARGB1555
		KM_DSPBPP_RGB888		RGB888
		KM_DSPBPP_ARGB8888		ARGB8888

	IN	bDither
		PowerVR が レンダリング結果を 16bitの Frame Buffer に書き込む際の
		Dither の有無を決定します。レンダリング先の Frame Buffer が 
		RGB888、ARGB8888 である場合、このフラグは無視されます。

			TRUE	Dither を使用する。
			FALSE	Dither を使用しない。

	IN	bAntiAlias
		アンチエリアシングフィルタの使用可否を決定します。
		アンチエイリアシングフィルタを有効にした場合動作速度が
		低下する場合があります。

			TRUE 	Anti Aliasing Filter を使用する。
			FALSE	Anti Aliasing Filter を使用しない。


Result:
	KMSTATUS_SUCCESS				成功
	KMSTATUS_INVALID_DISPLAY_MODE	不正なディスプレイモードが指定された。
									初期化時に指定されたディスプレイモードと
									矛盾するディスプレイモードが指定された。


--*/

KMSTATUS KMAPI
kmSetDisplayMode(
		IN	KMDISPLAYMODE	nDisplayMode,
		IN	KMBPPMODE		nBpp,
		IN	KMBOOLEAN		fDither,
		IN	KMBOOLEAN		fAntiAlias
	);


/*++
kmChangeDisplayFilterMode

proto type:

	KMSTATUS 
	kmChangeDisplayFilterMode(	
			KMBOOLEAN fDither,
			KMBOOLEAN fAntiAlias
		);

Description:
	kmSetDisplayMode で設定したディザやアンチエリアシングフィルタの Enable / Disable を
	後で変更する為に使用します。

Arguments:
	IN	bDither
		PowerVR が レンダリング結果を 16bitの Frame Buffer に書き込む際の
		Dither の有無を決定します。レンダリング先の Frame Buffer が 
		RGB888、ARGB8888 である場合、このフラグは無視されます。

			TRUE	Dither を使用する。
			FALSE	Dither を使用しない。

	IN	bAntiAlias
		アンチエリアシングフィルタの使用可否を決定します。
		アンチエイリアシングフィルタを有効にした場合動作速度が
		低下する場合があります。

			TRUE 	Anti Aliasing Filter を使用する。
			FALSE	Anti Aliasing Filter を使用しない。

Result:
	KMSTATUS_SUCCESS				成功
	KMSTATUS_INVALID_DISPLAY_MODE	不正なディスプレイモードが指定された。
									初期化時に指定されたディスプレイモードと
									矛盾するディスプレイモードが指定された。

--*/

KMSTATUS  KMAPI
kmChangeDisplayFilterMode(	
		IN	KMBOOLEAN fDither,
		IN	KMBOOLEAN fAntiAlias
	);

/*++
kmBlankScreen

proto type:

	KMSTATUS
	kmBlankScreen(	KMBOOLEAN bBlanking	);

Description:
	フレームバッファの画面表示を中止し、ブランキングにする。
	＜< 1回の V-Sync Callback 中で 1回の呼び出しのみ対応する。>＞

Arguments:
	IN	bBlanking	画面をブランキングするかどうかの指定。
			TRUE 	ブランキング開始
            FALSE	ブランキング解除

Result:
	KMSTATUS_SUCCESS		成功

--*/
KMSTATUS KMAPI
kmBlankScreen(
			IN	KMBOOLEAN bBlanking
		);

/*++
kmAdjustDisplayCenter

proto type:

	KMSTATUS
	kmAdjustDisplayCenter( 	KMINT32 nXAdjust, 
							KMINT32 nYAdjust 
						);

Description:
	画面上におけるFrame buffer の表示位置の調整を行います。

Arguments:
	IN	nXAdjust	横方向の画面描画位置を調整する値
	IN	nYAdjust	縦方向の画面描画位置を調整する値

Result:
	KMSTATUS_SUCCESS		成功
	KMSTATUS_OUT_OF_RANGE	有効範囲外の値を指定した。

--*/
KMSTATUS KMAPI
kmAdjustDisplayCenter( 
			IN	KMINT32 nXAdjust, 
			IN	KMINT32 nYAdjust 
		);


/*++
kmSetSystemConfiguration

proto type:
	KMSTATUS kmSetSystemConfiguration(
			PKMSYSTEMCONFIGSTRUCT pSystemConfigStruct
	)

Description:
		KMSYSTEMCONFIGSTRUCT型構造体に指定したパラメータに従い、神威の各種システム構成を設定します。
		Frame Buffer / Texture Buffer / Native Buffer の関係

		 Texture/Frame Buffer 全体の容量  -  (指定された最大テクスチャ使用サイズ + 表示用Frame Buffer)
				=	 Native Buffer

  注意：また、テクスチャサーフェスの確保は、
		本関数呼び出し以前に後で呼び出してください。
		この関数は、従来(神威Ver1.28以前)の 以下の関数に代わるものです。

				kmCreateFrameBufferSurface
				kmCreateVertexBuffer
				kmCreateTABuffer
				kmActivateFrameBuffer 

		kmSetSystemConfigurationを使用する場合は上記の関数は呼び出さないでください。

Arguments:
	IN	OUT	pSystemConfigStruct	KMSYSTEMCONFIGSTRUCT型構造体へのポインタを指定します。
								本構造体のメンバは以下の様に設定します。

				dwSize		KMSYSTEMCONFIGSTRUCT 構造体のサイズ を代入します。
							pSystemConfigStruct->dwSize = sizeof(KMSYSTEMCONFIGSTRUCT);

				flags		システムの動作を指定します。
								KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER	Frame Buffer 確保時にバッファの内容をクリアします。
								KM_CONFIGFLAG_ENABLE_STRIPBUFFER		Strip Buffer を有効にします。
								KM_CONFIGFLAG_ENABLE_2V_LATENCY			2V Laytency Mode にします。
								KM_CONFIGFLAG_NOWAITVSYNC				V-SYNC を待たないモードです。
								KM_CONFIGFLAG_USEDIRECTMODE				kmXxxxXxxxDirect 系の 関数を使用する。
																		このフラグを指定した場合、頂点データ登録/レンダリング開始は、
																		以下の関数を使用してタイルアクセラレータへ直接データを
																		書き込むことによって実現します。
																			kmStartVertexStripDirect
																			kmSetVertexDirect
																			kmSetUserClippingDirect
																			kmSetEndOfListDirect
																			kmRenderDirect
								KM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMA	TextureをDMA転送した場合に、DMAの終了を待たないようにします。
								                                        DMAの終了検出はkmQueryFinishLastTextureDMAを使用して行ってください。
								                                        また、DMA中のDMA転送元メモリへのアクセスは必ず避けるようにしてください。

				ppSurfaceDescArray/	nNumOfFrameBuffer;
							KMSURFACEDESC 構造体のポインタ配列へのポインタとポインタ配列の要素数を与えます。
							以下のように設定します。

									KMSURFACEDESC Surface1;
									KMSURFACEDESC Surface2;
									PKMSURFACEDESC ppSurfaceArray[nNumOfFrameBuffer];
									ppSurfaceArray[0] = &Surface1;
									ppSurfaceArray[1] = &Surface2;

									pSystemConfigStruct->nNumOfFrameBuffer  = nNumOfFrameBuffer;
									pSystemConfigStruct->ppSurfaceDescArray = ppSurfaceArray;

				nWidth / nHeight / nBpp
							確保する Frame Bufferの大きさ/ BPP です。
							nWidth / nBpp は常に無視します。
							nHeight は Strip バッファ使用時以外には、無視します。

				nTextureMemorySize
							使用するテクスチャメモリのサイズを指定します。

				pBufferDesc
						KMVERTEXBUFFDESC 構造体へのポインタです。

				pVertexBuffer
				nVertexBufferSize;
						頂点バッファへのベースアドレス/サイズを指定します。

				fBufferSize[5]		与えた頂点バッファ内で各リストが占める割合を % で指定します。
									合計値 =< 100% になるように設定してください。
Result:
	KMSTATUS_SUCCESS			成功
	KMSTATUS_NOT_ENOUGH_MEMORY	Native Buffer/Frame Buffer のためのメモリ容量が足りません

--*/

KMSTATUS KMAPI
kmSetSystemConfiguration(
		IN	OUT	PKMSYSTEMCONFIGSTRUCT pSystemConfigStruct
	);


/*++
kmCreateFrameBufferSurface

proto type:

	KMSTATUS 
	kmCreateFrameBufferSurface (
			PKMSURFACEDESC pSurfaceDesc,
			KMINT32 nWidth, 
			KMINT32 nHeight,
			KMBOOLEAN fStripBuffer,
			KMBOOLEAN fBufferClear
		);

Description:
	表示用のSurfaceを Frame Buffer メモリ上に確保し、
	初期化します。この関数を呼び出す前にkmSetDisplayModeを行う必要があります。

Arguments:
	OUT	pSurfaceDesc		KMSURFACEDESC 構造体の領域を示すポインタです。
							kmCreateFrameBufferSurface で作成したサーフィスの情報が格納されます。
							KMSTATUS に KMSTATUS_NOT_ENOUGH_MEMORYが返された場合には不定となります。


	IN	nWidth, nHeight		確保する Surfaceの幅/高さを指定します。0,0を設定した場合には、kmSetDisplayModeで
							指定した画面サイズを使用します。
							HOLLYは Strip Bufferを使用することができます。Strip Bufferを使用する場合には、
							nHeight に 32の倍数を指定する必要があります。Strip Bufferの機能はARC1にはありません。
							ARC1で使用する場合には常に0,0を指定してください。


	IN	bStripBuffer		Strip Buffer の作成有無を指定します。Strip Bufferの機能はARC1にはありません。
							ARC1で使用する場合には常にFALSEを指定してください。
								TRUE		StripBuffer形式の Frame Buffer を生成します。 
								FALSE		Frame Buffer を生成します。

	IN	bBufferClear		Frame Buffer 作成時に初期化の有無を指定します。
								TRUE	Surfaceを作成したときに0クリアします。
								FALSE	Sufraceを0クリアしません。

Result:
	KMSTATUS_SUCCESS				Frame Buffer 作成成功
	KMSTATUS_NOT_ENOUGH_MEMORY		失敗、メモリ容量が足りない
	

--*/


KMSTATUS KMAPI
kmCreateFrameBufferSurface (
		OUT	PKMSURFACEDESC 	pSurfaceDesc,
		IN	KMINT32 		nWidth, 
		IN	KMINT32 		nHeight,
		IN	KMBOOLEAN 		fStripBuffer,
		IN	KMBOOLEAN 		fBufferClear
	);


/*++
kmCreateTextureSurface

proto type:
	KMSTATUS 
	kmCreateTextureSurface(
			PKMSURFACEDESC	pSurfaceDesc,
			KMINT32			nWidth, 
			KMINT32			nHeight, 
			KMTEXTURETYPE	nTextureType
		);

Description:
	テクスチャ用のSurfaceを確保します。

Arguments:
	OUT	pSurfaceDesc		KMSURFACEDESC 構造体の領域を示すポインタです。
							確保した Texture Surface の情報が返されます。
							KMSTATUS にKMSTATUS_NOT_ENOUGH_MEMORYが返された場合には不定となります。

	IN	nWidth, nHeight		テクスチャの横サイズ、縦サイズを指定します。
								MIPMAPを使用する場合にはトップレベルのテクスチャサイズを指定します。
								Twiddled, VQ, Palettizedなどの正方形テクスチャを使用する場合には 
									nWidthに指定した値のみが使用されます。
								長方形テクスチャを使用する場合には、nWidth, nHeight共に有効です。
								ARC1、HOLLYは正方形テクスチャサイズとして 8x8～1024x1024までのテクスチャを使用できますが、
								nWidth, nHeightに指定する値は,8, 16, 32, 64, 128, 256, 512, 1024の
								いずれかでなければなりません。

	IN	nTextureType		テクスチャの形式を指定します。
							以下のカテゴリコード、ピクセルフォーマットコードから
							一つずつ選び出し、論理和(or)して与えます

								-カテゴリコード
									KM_TEXTURE_TWIDDLED			// Twiddledフォーマット
									KM_TEXTURE_TWIDDLED_MM		// TwiddledフォーマットMipmapあり
									KM_TEXTURE_VQ				// VQ圧縮形式
									KM_TEXTURE_VQ_MM			// VQ圧縮形式Mipmapあり
									KM_TEXTURE_PALETTIZE4		// 4bppパレット形式
									KM_TEXTURE_PALETTIZE4_MM	// 4bppパレット形式Mipmapあり
									KM_TEXTURE_PALETTIZE8		// 8bppパレット形式
									KM_TEXTURE_PALETTIZE8_MM	// 8bppパレット形式Mipmapあり
									KM_TEXTURE_RECTANGLE		// 矩形

								- ピクセルフォーマットコード
									KM_TEXTURE_1555
									KM_TEXTURE_565
									KM_TEXTURE_4444
									KM_TEXTURE_YUV422
									KM_TEXTURE_BUMP
									KM_TEXTURE_8888

							注)  KM_TEXTURE_8888はKM_TEXTURE_PALETTIZE4or8との組み合わせのみ有効です。

Result:
		KMSTATUS_SUCCESS					テクスチャメモリ確保成功
		KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した
		KMSTATUS_NOT_ENOUGH_MEMORY			メモリ容量が足りない
		KMSTATUS_INVALID_MIPMAPED_TEXTURE	MIPMAPできないテクスチャにMIPMAPを指定した。

--*/

KMSTATUS KMAPI
kmCreateTextureSurface(
		OUT	PKMSURFACEDESC	pSurfaceDesc,
		IN	KMINT32			nWidth, 
		IN	KMINT32			nHeight, 
		IN	KMTEXTURETYPE	nTextureType
	);

/*++
kmCreateCombinedTextureSurface

proto type:
	KMSTATUS 
	kmCreateCombinedTextureSurface(
			PKMSURFACEDESC	pSurfaceDesc1,
			PKMSURFACEDESC	pSurfaceDesc2,
			KMINT32			nWidth, 
			KMINT32			nHeight, 
			KMTEXTURETYPE	nTextureType
		);

Description:
	VQテクスチャ用のSurfaceを 2つ確保します。nTextureTypeにに設定するテクスチャカテゴリは
	KM_TEXTURE_VQ,KM_TEXTURE_VQ_MMのどちらかでなければなりません。

Arguments:
	OUT	pSurfaceDesc1		KMSURFACEDESC 構造体の領域を示す 1 つめのポインタです。
							確保した Texture Surface の情報が返されます。
							KMSTATUS にKMSTATUS_NOT_ENOUGH_MEMORYが返された場合には不定となります。

	OUT	pSurfaceDesc2		KMSURFACEDESC 構造体の領域を示す 2 つめのポインタです。
							確保した Texture Surface の情報が返されます。
							KMSTATUS にKMSTATUS_NOT_ENOUGH_MEMORYが返された場合には不定となります。

	IN	nWidth, nHeight		テクスチャの横サイズ、縦サイズを指定します。
							MIPMAPを使用する場合にはトップレベルのテクスチャサイズを指定します。
							ARC1、HOLLYは正方形テクスチャサイズとして 8x8～1024x1024までのテクスチャを使用できますが、
							nWidth, nHeightに指定する値は,8, 16, 32, 64, 128, 256, 512, 1024の
							いずれかでなければなりません。

	IN	nTextureType		テクスチャの形式を指定します。
							以下のカテゴリコード、ピクセルフォーマットコードから
							一つずつ選び出し、論理和(or)して与えます

								-カテゴリコード
									KM_TEXTURE_VQ				// VQ圧縮形式
									KM_TEXTURE_VQ_MM			// VQ圧縮形式Mipmapあり

								- ピクセルフォーマットコード
									KM_TEXTURE_ARGB1555
									KM_TEXTURE_ARGB4444

Result:
		KMSTATUS_SUCCESS					テクスチャメモリ確保成功
		KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した
		KMSTATUS_NOT_ENOUGH_MEMORY			メモリ容量が足りない

--*/

KMSTATUS KMAPI
kmCreateCombinedTextureSurface(
		OUT	PKMSURFACEDESC	pSurfaceDesc1,
		OUT	PKMSURFACEDESC	pSurfaceDesc2,
		IN	KMINT32			nWidth, 
		IN	KMINT32			nHeight, 
		IN	KMTEXTURETYPE	nTextureType
	);


/*++
kmSetAlphaThreshold

proto type:

	KMSTATUS 
	kmSetAlphaThreshold(	
			KMINT32 nThreshold	
		);

Description:
	1555サーフェスをレンダリング用Surfaceとした場合にαの1bitを決定するためのスレッショルド値です。
	レンダリング結果が、ここでセットした値以上となった場合、αビットを1にします。
	αビットはRAMDACなどでクロマキー合成をする場合に使用されます。

Arguments:
	IN	nThreshold		0～255の値のスレッショルド値。
						0以下または255以上を指定された場合は、それぞれ0、255 にクランプされます。


Result:
	KMSTATUS_SUCCESS		設定成功
--*/

KMSTATUS KMAPI
kmSetAlphaThreshold(	
		IN	KMINT32 nThreshold	
	);

/*++
kmActivateFrameBuffer

proto type:

	KMSTATUS 
	kmActivateFrameBuffer(
			PKMSURFACEDESC	pPrimarySurfaceDesc,
			PKMSURFACEDESC	pBackBufferSufaceDesc, 
			KMBOOLEAN 		fStripBuffer,
			KMBOOLEAN 		fWaitVSync
		);

Description:
	 Frame Buffer の表示を開始します。
Arguments:

	IN	pPrimarySurfaceDesc		表示に使用するSurfaceのSurface構造体です。 
								kmCreateFrameBufferSurface でSurfaceを確保した時点で得られた
								Surface構造体を設定してください。

	IN	pBackBufferSurfaceDesc	レンダリング対象SurfaceのSurface構造体です。

	IN	bStripBuffer			StripBufferを使用する場合TRUEを指定します。
	IN	bWaitVSync				Surfaceの表示をVsyncまでWaitするかどうかを指定します。
								TRUEの場合本関数はVsyncタイミングまでウェイトし、
								表示を開始します。


Result:
	KMSTATUS_SUCCESS			表示切り替え成功

--*/

KMSTATUS KMAPI
kmActivateFrameBuffer(
		IN	PKMSURFACEDESC	pPrimarySurfaceDesc,
		IN	PKMSURFACEDESC	pBackBufferSufaceDesc, 
		IN	KMBOOLEAN		fStripBuffer,
		IN	KMBOOLEAN		fWaitVSync
	);

/*++
kmFlipFrameBuffer

proto type:

	KMSTATUS
	kmFlipFrameBuffer( KMVOID );

Description:
	 Frame Buffer をダブルバッファリングした場合に、表示用のPrimarySurfaceと 
	レンダリングターゲットの BackBuffer を入れ替えます。

	Flipコマンドはキューイングされます。
	BackBuffer へのレンダリングが終了している場合、
	kmFlipFrameBuffer() を実行した
	次のVsyncタイミングで Flipが実行されます。

	なお、StripBuffer使用時には、ソフトウェアによるFlipは必要ありません。

Arguments:
		none

Result:
		KMSTATUS_SUCCESS			Flipコマンド発行成功
		KMSTATUS_CANT_FLIP_SURFACE 	Flipコマンド発行失敗

--*/
KMSTATUS KMAPI
kmFlipFrameBuffer( KMVOID );

/*++
kmSetWaitVsyncCount

proto type:

	KMSTATUS
	kmSetWaitVsyncCount( KMINT32 nCount );

Description:
		フレームレートを定率にするために、前回の Flip後、
		次にFlip 可能になるまでのV-Sync数を指定します。

Arguments:
		IN	nCount		V-Sync を待つ回数を指定します。前回の Flip後、
						次にFlip 可能になるまでのV-Sync数を指定します。

Result:
		KMSTATUS_SUCCESS			成功
--*/
KMSTATUS KMAPI
kmSetWaitVsyncCount(
		IN	KMINT32 nCount 
		);


/*++
kmSetCullingRegister

proto type:

	KMSTATUS
	kmSetCullingRegister(
			KMFLOAT fCullVal
		);

Description:
	小さい Polygon に対するカリングのスレッショルド値を設定します。

Arguments:
		IN	fCullVal	平面パラメータのデタミナント値を設定します。

Result:
		KMSTATUS_SUCCESS			Flipコマンド発行成功
		KMSTATUS_CANT_FLIP_SURFACE 	Flipコマンド発行失敗

--*/

KMSTATUS KMAPI
kmSetCullingRegister(
		IN	KMFLOAT fCullVal
	);


/*++
kmSetColorClampValue

proto type:

	KMSTATUS
	kmSetColorClampValue(
			KMPACKEDARGB MaxVal,
			KMPACKEDARGB MinVal 
		);

Description:
	カラーのクランプ値を指定します。カラークランプはフォグの前段で掛けられます。
	レンダリング中にクランプ色を変更するときには、
	必ずレンダリング終了のコールバック関数内などで変更を行ってください。
	それ以外のタイミングで変更を行った場合、画面イメージが不正となる場合が有ります。

Arguments:
	IN	MaxVal			Packedの ARGB 32bitカラーで、クランプの最大値を指定します。
						輝度128でRGB色を指定する場合は0x00808080のように指定します。

	IN	MinVal			Packedの ARGB 32bitカラーで、クランプの最大値を指定します。
						輝度20でRGB色を指定する場合は0x00141414のように指定します。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetColorClampValue(
		IN	KMPACKEDARGB MaxVal,
		IN	KMPACKEDARGB MinVal 
	);

/*++
kmSetFogTableColor

proto type:

	KMSTATUS
	kmSetFogTableColor(
			IN	KMPACKEDARGB FogTableColor
		);

Description:
	フォグカラーを指定します。HOLLYではFogVertex、FogTableに別々のフォグカラーを指定できますが、
	ARC1では同一のためFogVertex、FogTableに別々の値を設定しても、後から設定された値が有効となります。
	レンダリング中にフォグカラーを変更するときには、必ずレンダリング終了のコールバック関数内などで変更を
	行ってください。それ以外のタイミングで変更を行った場合、画面イメージが不正となる場合があります。

Arguments:
	IN	FogTableColor		FogTable に使用するPackedの32bitカラーです。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetFogTableColor(
		IN	KMPACKEDARGB FogTableColor
	);

/*++
kmSetFogVertexColor

proto type:

	KMSTATUS 
	kmSetFogVertexColor(
			KMPACKEDARGB FogVertexColor
		);

Description:
	フォグカラーを指定します。HOLLYではFogVertex、FogTableに別々のフォグカラーを指定できますが、
	ARC1では同一のためFogVertex、FogTableに別々の値を設定しても、後から設定された値が有効となります。
	レンダリング中にフォグカラーを変更するときには、必ずレンダリング終了のコールバック関数内などで変更を
	行ってください。それ以外のタイミングで変更を行った場合、画面イメージが不正となる場合があります。

Arguments:
	IN	FogVertexColor		FogVertexに使用するPackedの32bitカラーです。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS  KMAPI
kmSetFogVertexColor(
		IN	KMPACKEDARGB FogVertexColor
	);

/*++
kmSetFogTable

proto type:

	KMSTATUS
	kmSetFogTable(
			PKMFLOAT pfFogTable
		);

Description:
	FogTableを登録します。256段階のfloat値の格納された配列へのポインタを引数として渡します。
	

Arguments:
	IN	pfFogTable	フォグテーブル値配列へのポインタ。256段階のパラメータを指定します。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetFogTable(
		IN	PKMFLOAT pfFogTable
	);

/*++
kmSetFogDensity

proto type:

	KMSTATUS
	kmSetFogDensity(KMDWORD fogDensity)

Description:
	TSP Fog の係数を与える。

Arguments:
	IN	fogDensity			TSP Fog の係数

Result:
	KMSTATUS_SUCCESS		設定成功

--*/
KMSTATUS	KMAPI
kmSetFogDensity(
		IN	KMDWORD fogDensity
	);

/*++
kmSetPaletteMode

proto type:

	KMSTATUS 
	kmSetPaletteMode(	
			KMPALETTEMODE Palettemode
		);

Description:
	オンチップパレットのモード指定をします。

Arguments:
	IN	PaletteMode		パレットの BPP モード
						KM_PALETTE_16BPP	16BPPモード 565, 555, 1555, 4444を使用します。
						KM_PALETTE_32BPP 	32BPPモード 8888を使用します。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/
KMSTATUS  KMAPI
kmSetPaletteMode(	
		IN	KMPALETTEMODE Palettemode
	);

/*++
kmSetPaletteData

proto type:
	KMSTATUS
	kmSetPaletteData(
			PKMPALETTEDATA pPaletteTable
		);

Description:
	オンチップパレットの設定を行います。

Arguments:
	IN	pPaletteTable	Palette設定用配列のポインタを指定します。
						Palette設定用配列の内容は以下のようになります。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetPaletteData(
		IN	PKMPALETTEDATA pPaletteTable
	);


/*++
kmSetPaletteBankData

proto type:
	KMSTATUS 
	kmSetPaletteBankData(
			IN KMINT32		PaletteEntry,
			IN KMINT32		DataSize,
			IN PKMPALETTEDATA	pPaletteTable
		);

Description:
	Palettizeテクスチャで使用される On Chip Palette の、
	一部分を指定したパレットデータで書き換えます。
	パレットの構造についてはkmSetPaletteDataを参照願います。
	パレットモードが4bpp/8bppに関わらず PaletteEntryに指定可能な値は、0から1023のいずれかです。
	バンクの境界に関係なく、任意のエントリからデータを書き込むことができます。
	パレットのPaletteEntry番目のエントリから、PaletteEntry　＋　DataSize番目のエントリまでに、
	pPaletteTableで指される領域の先頭のデータから順にセットしていきます。
	パレット番号 1023 より大きいパレット番号に対応するデータは無視されます。
	すなわち設定が PaletteEntry　＋　DataSize　＞　1024　の場合は、パレット番号1023までで停止します。

Arguments:
	IN PaletteEntry
		データを書き込むパレットの先頭エントリ番号を指定します。
		 0 から 1023 の中のいずれかを指定します。
		指定されたエントリ番号からのパレットの内容を書き換えます。

	IN DataSize
		書き込むデータのサイズ(エントリ数)を指定します。
		1 から最大1024 が指定できます。

	IN pPaletteTable
		Palette設定用配列のポインタを指定します。
		Palette設定用配列の内容は以下のようになります。

		KMPALETTEDATA		PaletteTable;

		このパレットデータの要素数はDataSize個以上なければなりません。
		これがDataSize個以上なかった場合の動作は保証しません。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/
KMSTATUS 
kmSetPaletteBankData(
		IN KMINT32			PaletteEntry,
		IN KMINT32			DataSize,
		IN PKMPALETTEDATA	pPaletteTable
	);

/*++
kmSetBorderColor

proto type:
	KMSTATUS
	kmSetBorderColor(
			KMPACKEDARGB BorderColor
		);

Description:
	ボーダー(表示画面外の部分)カラーの設定を行います。

Arguments:
	IN	BorderColor		PackedのARGBカラーです。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetBorderColor(
		IN	KMPACKEDARGB BorderColor
	);

/*++
kmSetBackGroundRenderState

proto type:
	KMSTATUS
	kmSetBackGroundRenderState(
			PKMVERTEXCONTEXT	pVertexContext
		);

Description:
	kmProcessVertexRenderState で設定した KMVERTEXCONTEXT構造体の以下のメンバを
	Background Plane のレンダリングパラメータとしてシステムに登録する。
	本関数呼び出し以降の kmSetBackGroundPlane() で指定する ＢＧ面の設定は、
	引数、pVertexContext で設定した KMVERTEXCONTEXT の値が有効になる。

	参照するメンバ：
				pVertexContext->GLOBALPARAMBUFFER
				pVertexContext->ISPPARAMBUFFER
				pVertexContext->TSPPARAMBUFFER
				pVertexContext->TexturePARAMBUFFER


Arguments:
	IN	pVertexContext
						Vertex Context へのポインタを与える。

Result:
	KMSTATUS_SUCCESS	設定成功

--*/

KMSTATUS KMAPI
kmSetBackGroundRenderState(
		IN	PKMVERTEXCONTEXT	pVertexContext
	);

/*++
kmSetBackGroundPlane

proto type:
	KMSTATUS
	kmSetBackGroundPlane(
				PVOID			pVertex[3],
				KMVERTEXTYPE	VertexType,
				KMINT32			StructSize
			);

Description:
	バックグラウンド平面の設定を行います。
	この前に、kmSetBackGroundRenderState を呼び出す必要があります。

Arguments:
	IN	pVertex[3]		バックグラウンドプレーン上の座標を示す頂点データ構造体へのポインタ
						詳細は、 kmSetVertex 参照

	IN	VertexType		頂点データのデータタイプを示す。
						詳細は kmSetVertex 参照
	IN	StructSize		頂点データのデータタイプを示す。
						詳細は kmSetVertex 参照

Result:
	KMSTATUS_SUCCESS			設定成功

--*/

KMSTATUS KMAPI
kmSetBackGroundPlane(
			IN	PVOID			pVertex[3],
			IN	KMVERTEXTYPE	VertexType,
			IN	KMINT32			StructSize
		);

/*++
kmSetAutoSortMode

proto type:
	KMSTATUS
	kmSetAutoSortMode(
			KMBOOLEAN fEnable
		);

Description:
	半透明面のオートソートモードのON/OFFを制御します。

Arguments:
	IN	bEnable		Auto Sort mode の有効化フラグ
				TRUE	半透明面のソートにオートソートモードを設定します。
				FALSE	従来型と同じソフトウェアによるソーティングをエミュレーションします。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetAutoSortMode(
		IN	KMBOOLEAN fEnable
	);

/*++
kmSetPixelClipping

proto type:
	KMSTATUS
	kmSetPixelClipping(
				KMINT32	Xmin,
				KMINT32	Ymin,
				KMINT32	Xmax,
				KMINT32	Ymax
			);

Description:
	 Frame Buffer へのレンダリングの出力に対する、ピクセル単位のクリッピングを設定します。

Arguments:
	IN	Xmin, Ymin, Xmax, Ymax
				ピクセル単位のクリッピング領域の左上、右下座標を指定します。
				(Xmin,Ymin)-(Xmax,Ymax)は、画面サイズより小さくなければいけません。
				また、画面モードが24bppの時は、クリッピング領域の座標は偶数でなければいけません
				(すなわち 2ピクセル単位でしかクリッピング領域を設定できません)。
				偶数でなかった場合には、(Xmin,Ymin)に対しては指定より 1 大きい値、
										(Xmax,Ymax)に対しては指定より 1 小さい値が設定されます。

Result:
	KMSTATUS_SUCCESS		設定成功
	KMSTATUS_INVALID_PARAMETER	パラメータが不正

--*/

KMSTATUS KMAPI
kmSetPixelClipping(
		IN	KMINT32	Xmin,
		IN	KMINT32	Ymin,
		IN	KMINT32	Xmax,
		IN	KMINT32	Ymax
	);

/*++
kmSetPixelClipping

proto type:
	KMSTATUS
	kmSetStrideWidth(
			KMINT32 nWidth
		);

Description:
	ストライドテクスチャのストライドサイズを指定します。ストライドサイズは、
	32 の倍数でなければならない。

Arguments:
	IN	nWidth		ストライドサイズを入力する。
					ストライドサイズは 32の倍数でなければならない。

Result:
	KMSTATUS_SUCCESS			設定成功
	KMSTATUS_INVALID_PARAMETER	パラメータが不正

--*/

KMSTATUS KMAPI
kmSetStrideWidth(
		KMINT32 nWidth
	);

/*++
kmSetForwardClipping

proto type:

	KMSTATUS
	kmSetForwardClipping(
			IN	KMFLOAT fDepth
		);

Description:
	前面クリッピングを設定します。 fDepth の値が負の場合には、
	値は無視され、クリッピングは行われない。また、1/w が IEEE 754
	の精度限界を超える領域でのクリッピングは行われません。
	本設定はニアクリップを行うものではありませんので、頂点の 1/w の
	座標値が非常に大きいところのニアクリップは、アプリケーションで
	行う必要がある。

Arguments:
	IN	fDepth
		前面クリッピングの Depth値を設定します。

Result:

	KMSTATUS_SUCCESS				成功

--*/
KMSTATUS KMAPI
kmSetForwardClipping(
		IN	KMFLOAT fDepth
		);

/*++
kmResetRenderer

proto type:

	KMSTATUS
	kmResetRenderer( VOID );

Description:
	レンダリングパイプラインのソフトウェアリセットを行う。

	使用する場合：
		・StripBuffer使用時にストリップ内のデータを描ききれなかった
		  場合の強制リセットする。

Arguments:
	NONE

Result:

	KMSTATUS_SUCCESS				成功

--*/

KMSTATUS KMAPI
kmResetRenderer( VOID );

/*++
kmSetUserClipLevelAdjust

proto type:

	KMSTATUS
	kmSetUserClipLevelAdjust( KMADJUSTTYPE	Adjust,
				  PKMINT32	pLine
				 );

Description:
	水平方向の画面表示モードを設定します。
	これを使用することにより、水平方向のユーザークリッピング領域の境界を、
	画面のほぼ中央に来るようにすることができます。対戦ゲームなどで画面を
	上下 2 分割して使用する場合に設定します。

	神威で画面を上下 2 つに分割したい場合は、この関数でKM_LEVEL_ADJUST_HALFを指定した上で、
	ユーザークリップ領域のＹ座標を下記のように設定してください。すると、このクリップ領域の
	水平方向の境界が極力画面の中央となるよう、表示領域を調整します。

		画面の縦方向の解像度	ユーザークリップ領域のＹ座標
			240						0 ～ 119　と　120 ～ 239
			480						0 ～ 255　と　256 ～ 479

	なお、この場合、画面の上部何ラインかは表示しなくなります。表示され
	なくなるライン数は、pLineから得ることができます。

Arguments:
	IN	Adjust	水平方向の画面表示モードを指定します。
			以下のいずれかが指定できます。
			KMADJUSTTYPE
			KM_LEVEL_ADJUST_NORMAL		通常表示用の表示モード指定します。
			KM_LEVEL_ADJUST_HALF		2分割表示用の表示モードを指定します。


	OUT	pLine
			AdjustにKM_LEVEL_ADJUST_HALFを指定した場合、神威はここに画面の上下方向の
			シフト量をピクセル数で返します。画面の最上部から、ここで指定されたライン数は
			表示されなくなります。頂点登録時には各頂点のＹ座標値に、この値を加算してください。
			pLine に NULL を指定してもかまいませんが、その場合、ユーザは頂点のＹ座標に加算すべき
			ライン数を取得することができません。

			AdjustにKM_LEVEL_ADJUST_NORMALを指定した場合は、pLineが指す領域の内容は変更しません。


Result:

	KMSTATUS_SUCCESS				成功

--*/
KMSTATUS KMAPI
kmSetUserClipLevelAdjust(
		IN	KMADJUSTTYPE	Adjust,
		OUT	PKMINT32	pLine
	);

/*++
kmGetGunTriggerPos

proto type:
	KMSTATUS KMAPI
	kmGetGunTriggerPos(
			PKMDWORD pHPos,
			PKMDWORD pVPos
		);

Description:
	Gun Peripheral のトリガが押された位置を返します。
	ARC1 では実装されません。

Arguments:
	OUT	pHPos	H-Blank Out からの水平位置
    OUT	pVPos	V-Blank Out からの垂直位置

Result:
	KMSTATUS_SUCCESS		成功

--*/

KMSTATUS KMAPI
kmGetGunTriggerPos(
		OUT	PKMDWORD pHPos,
		OUT	PKMDWORD pVPos
	);

/*++
kmProcessVertexRenderState

proto type:
	KMSTATUS
	kmProcessVertexRenderState(
				PKMVERTEXCONTEXT pVertexContext
			);

Description:
	頂点(ストリップ)ごとに有効となるレンダリング・パラメータを設定します
	(pVertexContextの設定値をシステムに登録するための前処理を行います)。

	本関数はpVertexContextに設定された値をもとに以下の値を生成します。
			Global Parameter
			Combined ISP/TSP
			Instruction Word
			TSP Control Word
			Texture Control Bits

	本関数が生成した上記データは、KMVERTEXCONTEXT内の以下に示すメンバに保存します。
			pVertexContext->GLOBALPARAMBUFFER
			pVertexContext->ISPPARAMBUFFER
			pVertexContext->TSPPARAMBUFFER
			pVertexContext->TexturePARAMBUFFER

	本関数により、前処理を行った pVertexContext の上記メンバをレンダリングに使用する
	パラメータとして、システムに登録するためには、kmSetVertexRenderStateを呼び出すこと必要があります。

	VERTEXCONTEXTを最初に使用する時には、VERTEXCONTEXTのすべてのメンバの値を指定(初期化)する
	必要があります。

	注：
		pVertexContext->RenderState にすべてのフラグを設定し、すべてのパラメータを定義してください。
		未定義のビットがあった場合の動作は保証しません。

Arguments:
	IN	pVertexContext		コンテキストへのポインタを設定します。

Result:
	KMSTATUS_SUCCESS		設定成功
	KMSTATUS_INVALIDSETTING	許可されていないモード設定

--*/

KMSTATUS KMAPI
kmProcessVertexRenderState(
		IN	PKMVERTEXCONTEXT pVertexContext
	);

/*++
kmSetVertexRenderState

proto type:

	KMSTATUS
	kmSetVertexRenderState (
			PKMVERTEXCONTEXT pVertexContext
		);

Description:
	kmProcessVertexRenderStateで処理した pVertexContext 内の以下のメンバ及び関連する値を
	レンダリングに使用するパラメータとしてシステムに登録します。

			pVertexContext->GLOBALPARAMBUFFER
			pVertexContext->ISPPARAMBUFFER
			pVertexContext->TSPPARAMBUFFER
			pVertexContext->TexturePARAMBUFFER

	本関数の呼び出しで設定したパラメータは 本関数の後で呼び出される kmStartVertexStrip 
	以降の頂点(Strip)登録で有効となります。


Arguments:
	IN	pVertexContext		KMVERTEXCONTEXT 構造体へのポインタを設定します。

Result:
	KMSTATUS_SUCCESS		レンダリングパラメータ登録成功

--*/


KMSTATUS KMAPI
kmSetVertexRenderState (
		IN	PKMVERTEXCONTEXT pVertexContext
	);

/*++
kmSetModifierRenderState

proto type:
	KMSTATUS
	kmSetModifierRenderState(
			PKMVERTEXCONTEXT	pVertexContext
		);

Description:
	kmProcessVertexRenderStateで設定したKMVERTEXCONTEXT内の以下のメンバを、
	 Modifier Volume の影響を受ける Polygonの、2 番目のレンダリングパラメータ
	としてシステムに登録します。

				pVertexContext->TSPPARAMBUFFER
				pVertexContext->TexturePARAMBUFFER

	以降の Modifier Volume の影響を受ける Polygon の登録では、本関数で指定した
	KMVERTEXCONTEXTが 2 番目のレンダリングパラメータとして使用します。

	KMVERTEXCONTEXT構造体のうち以下のメンバは参照しません(無視します)。
	(kmSetVertexRenderStateでの設定値が有効となります)
	// for Global Parameter
	KMPARAMTYPE			ParamType			// Parameter Type
	KMLISTTYPE			ListType			// List Type
	KMSTRIPLENGTH		StripLength			// Strip Length
	KMUSERCLIPMODE		UserClipMode		// UserClip Mode
	KMCOLORTYPE			ColorType			// Color Type
	KMUVFORMAT			UVFormat			// UV format

	// for ISP/TSP Instruction Word
	KMDEPTHMODE			DepthMode;			// DepthMode指定
	KMCULLINGMODE		CullingMode;		// Culling Mode
	KMSCREENCORDINATION	ScreenCoordination;	// Screen Coordination
	KMSHADINGMODE		ShadingMode;		// Shading Mode
	KMMODIFIER			SelectModifier;		// Modifier Volume Valiant
	KMBOOLEAN			bZWriteDisable;		// Z Write Disable

	// for ModifierInstruction
	KMDWORD				ModifierInstruction;// ModifierInstruction
	KMFLOAT				fBoundingBoxXmin;	// BoundingBoxXmin(ShadowVolume)
	KMFLOAT				fBoundingBoxYmin;	// BoundingBoxYmin(ShadowVolume)
	KMFLOAT				fBoundingBoxXmax;	// BoundingBoxXmax(ShadowVolume)
	KMFLOAT				fBoundingBoxYmax;	// BoundingBoxYmax(ShadowVolume)

Arguments:
	IN	pVertexContext		レンダリングコンテキストへのポインタを設定します。

Result:
	KMSTATUS_SUCCESS		レンダリングパラメータ登録成功


--*/

KMSTATUS KMAPI
kmSetModifierRenderState(
		IN	PKMVERTEXCONTEXT	pVertexContext
	);

/*++
kmSetGlobalClipping

proto type:
	KMSTATUS
	kmSetGlobalClipping(
				KMINT32 nWidth, 
				KMINT32 nHeight
			);
Description:
	グローバルクリッピング領域を指定します。0,0オリジンでWidth、Heightで指定された
	領域以外のところにはレンダリングされません。

Arguments:
	IN	nWidth, nHeight				タイルサイズの倍数を指定します。
									HOLLYの場合は32x32のタイルサイズのため、
									たとえば128x64の領域を指定したい場合には Width = 4,Height= 2とします。
									ARC1の場合は32x8のタイルサイズですが、 
									HOLLYでのパラメータ指定を神威内部でARC1に合わせて変換(Height×4)します。

Result:
	KMSTATUS_SUCCESS					設定成功
	KMSTATUS_INVALID_PARAMETER			設定失敗

--*/

KMSTATUS KMAPI
kmSetGlobalClipping(
		IN	KMINT32 nWidth, 
		IN	KMINT32 nHeight
	);

/*++
kmUseAnotherModifier

proto type:
	KMSTATUS
	kmUseAnotherModifier(
			KMLISTTYPE kmModifierListType
			);

Description:
	kmModifierListType で指定した Modifier を他方の 
	Modifier としても使用する。

Arguments:
	IN	kmModifierListType		KM_OPAQUE_MODIFIER	Opaque Modifier を Trans Modifier  として使用する。
								KM_TRANS_MODIFIER	Trans Modifier  を Opaque Modifier として使用する。

Result:
	KMSTATUS_SUCCESS				設定成功
	KMSTATUS_INVALID_LIST_TYPE		設定失敗

--*/

KMSTATUS KMAPI
kmUseAnotherModifier(
		IN	KMLISTTYPE kmModifier
			);

/*++
kmSetCheapShadowMode

proto type:
	KMSTATUS
	kmSetCheapShadowMode(
			KMINT32 nIntensity
			);

Description:
	チープ(簡易)シャドウモードを選択します。CheapShadowModeとは、あるPolygonが Modifier Volume にさしかかった場合、
   その部分の輝度を下げることにより影を表現する機能です。この関数で値が設定された後は、すべてのModifierVolumeの設定は
   チープシャドウモードに設定されます。シーン内で 2 パラメータPolygonとの共存はできません。
   CheapShadowModeを終了させるには引数に負の数を入れてこの関数を呼び出してください。

Arguments:
	IN	nIntensity		モディファイアボリューム内に入ったPolygonの輝度を設定します。0から255の値が使用できます。
						実際にはこれを256で割った値がPolygonのベースカラーとオフセットカラーに乗算されます。従って、
						128を指定すると128/256 = 0.5が乗算されることになります。また、負の値を入力した場合、
						そのシーンから CheapShadowMode の設定を終了し、通常の 2 パラメータPolygonが有功となります。

Result:
	KMSTATUS_SUCCESS					設定成功
	KMSTATUS_INVALID_PARAMETER			パラメータが不正

--*/

KMSTATUS KMAPI
kmSetCheapShadowMode(
		IN	KMINT32 nIntensity
			);

/*++
kmSetStripLength

proto type:
	KMSTATUS
	kmSetStripLength(
		IN	KMSTRIPLENGTH	StripLength
		);

Description:
	Strip分割数の既定値を設定します。タイルアクセラレータが、入力された頂点データストリップを Strip分割処理する
	際の最大ストリップ長を指定します。
	デフォルト(システム初期値)は KM_STRIP_06です。
	ただし、kmProcessVertexRenderState関数の引数として使用する、PKMVERTEXCONTEXT 構造体の StripLengthメンバを
	有効にした場合には、本関数で設定した値を無視し、StripLengthメンバに設定した値を使用します。

Arguments:
	IN	StripLength		Strip分割数の既定値を指定します。以下のいずれかが指定できます。
						KMSTRIPLENGTH	KM_STRIP_01		= 0		// Strip長1に分割します
										KM_STRIP_02		= 1		// Strip長2に分割します
										KM_STRIP_04		= 2		// Strip長4に分割します
										KM_STRIP_06		= 3		// Strip長6に分割します

Result:
	KMSTATUS_SUCCESS	設定成功

--*/

KMSTATUS KMAPI
kmSetStripLength(
		IN	KMSTRIPLENGTH	StripLength
	);

/*++
kmSetUserClipping

proto type:
	KMSTATUS
	kmSetUserClipping(
			PKMVERTEXBUFFDESC pBufferDesc,
			KMLISTTYPE ListType,
			KMINT32	Xmin,
			KMINT32	Ymin,
			KMINT32	Xmax,
			KMINT32 Ymax 
		);

Description:
	ユーザークリッピング領域を設定します。KMVERTEXCONTEXT 構造体のUserClipModeメンバに、
	KM_USERCLIP_INSIDEまたはKM_USERCLIP_OUTSIDEを指定したPolygonに対しては、
	ここで指定したユーザークリッピング領域が有効となります。

	注意：  頂点ストリップの登録の途中でストリップの一部だけをクリップすることはできません。
			具体的には、kmStartVertexStrip で 開始した頂点ストリップの登録を
			kmSetVertexで ENDOFSTRIP を登録するまでの間に、kmSetUserClippingを発行してはいけません。

Arguments:
	IN	pBufferDesc
						PKMVERTEXBUFFDESC型の、Vertex Buffer Descriptorへのポインタを入力します。

	IN	ListType		頂点データリストの種類を指定します。以下のいずれかが指定できます。
						KMLISTTYPE	KM_OPAQUE_POLYGON		= 0	// 不透明Polygon
									KM_OPAQUE_MODIFIER		= 1	// 不透明 Modifier Volume 
									KM_TRANS_POLYGON		= 2	// 半透明/透明Polygon
									KM_TRANS_MODIFIER		= 3	// 半透明/透明 Modifier Volume 
									KM_PUNCHTHROUGH_POLYGON = 4	// パンチスルーPolygon(CLX2のみ)

	IN	Xmin, Ymin, Xmax, Ymax
						ユーザークリップ領域の左上、右下座標を指定します。
						ここで指定する値はタイル単位です。(1 = 32pixel)
						Xmin,Xmaxは、下位6bitのみ有効です。
						Ymin,Ymaxは、下位4bitのみ有効です。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetUserClipping(
		IN	PKMVERTEXBUFFDESC pBufferDesc,
		IN	KMLISTTYPE 		ListType,
		IN	KMINT32			Xmin,
		IN	KMINT32			Ymin,
		IN	KMINT32			Xmax,
		IN	KMINT32			Ymax
	);

/*++
kmSetUserClippingDirect

proto type:
	KMSTATUS
	kmSetUserClippingDirect(
			KMINT32	Xmin, 
			KMINT32	Ymin,
			KMINT32	Xmax, 
			KMINT32	Ymax
		);

Description:
	ユーザークリッピング領域を設定します。VERTEXCONTEXTのUserClipModeメンバに、KM_USERCLIP_INSIDEまたはKM_USERCLIP_OUTSIDEを
	指定したPolygonに対しては、ここで指定したユーザークリッピング領域が有効となります。

注意： kmStartVertexStripDirectとkmSetVertexDirectの間で、 kmSetUserClippingDirectを発行してはいけません。

Arguments:

	IN	Xmin, Ymin, Xmax, Ymax
						ユーザークリップ領域の左上、右下座標を指定します。
						ここで指定する値はタイル単位です。(1 = 32pixel)
						Xmin,Xmaxは、下位6bitのみ有効です。
						Ymin,Ymaxは、下位4bitのみ有効です。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmSetUserClippingDirect(
		IN	KMINT32	Xmin,
		IN	KMINT32	Ymin,
		IN	KMINT32	Xmax,
		IN	KMINT32	Ymax
	);

/*++
kmGetCurrentVertexOffset

proto type:
	KMDWORD
	kmGetCurrentVertexOffset(
			KMLISTTYPE ListType
		);

Description:
	現在、指定のリストタイプのVertexBuffer中で頂点データがストアされている32ビットワード数を返り値として返します。
	本関数で得た値を、kmChangeVertexOffsetやkmChangeVertexPCWと組み合わせて使用することにより、VertexBuffer中の値を
	変更することができます。

Arguments:
	IN	ListType		頂点データリストを指定します。以下のいずれかが指定できます。
						KMLISTTYPE	KM_OPAQUE_POLYGON	= 0	// 不透明Polygon
									KM_OPAQUE_MODIFIER	= 1	// 不透明 Modifier Volume 
									KM_TRANS_POLYGON	= 2	// 半透明/透明Polygon
									KM_TRANS_MODIFIER	= 3	// 半透明/透明 Modifier Volume 

Result:
	指定リストタイプの現在の書き込み位置を返します。値は指定リストタイプのVertexBufferの、最初のポインタからの
	32bitワードオフセット値(現在の指定リストタイプVertexBufferの使用容量)です。
	Vertex定義にDirect系の関数を使用している場合には使用できません。

--*/

KMDWORD KMAPI
kmGetCurrentVertexOffset(
		KMLISTTYPE ListType
	);

/*++
kmChangeVertexOffset

proto type:
	KMSTATUS
	kmChangeVertexOffset(
			KMLISTTYPE ListType,
			KMDWORD    VertexOffset
		);

Description:
	指定リストタイプのVertexBufferの、現在の書き込み位置を変更します。
	Vertex定義にDirect系の関数を使用している場合には使用できません。


Arguments:

	IN	ListType		頂点データリストを指定します。以下のいずれかが指定できます。
						KMLISTTYPE	KM_OPAQUE_POLYGON		= 0	// 不透明Polygon
									KM_OPAQUE_MODIFIER		= 1	// 不透明 Modifier Volume 
									KM_TRANS_POLYGON		= 2	// 半透明/透明Polygon
									KM_TRANS_MODIFIER		= 3	// 半透明/透明 Modifier Volume 
									KM_PUNCHTHROUGH_POLYGON = 4	// パンチスルーPolygon(CLX2のみ)

	IN	VertexOffset	指定リストタイプの書き込み位置を指定します。kmGetCurrentVertexOffsetなどで取得した値を使用します。

Result:
	KMSTATUS_SUCCESS		設定成功
	KMSTATUS_INVALID_VALUE	指定できない値を設定した

--*/

KMSTATUS KMAPI
kmChangeVertexOffset(
		KMLISTTYPE ListType,
		KMDWORD    VertexOffset
	);

/*++
kmChangeVertexPCW

proto type:
	KMSTATUS
	kmChangeVertexPCW(
			KMLISTTYPE ListType,
			KMDWORD    VertexPCW,
			KMDWORD    IncPtr
		);

Description:
	指定のリストタイプVertexBufferの、現在の書き込み位置にVertexPCWで指定された値を書き込みます。この場合
	VertexBufferのポインタはIncPtrだけ、インクリメントされます。三角形のストリップ定義終了後、途中でストリップを
	中断する場合などに使用します。本関数はVertexBuffer内の整合性を破壊する可能性がありますので使用する場合には十分
	注意してください。また、Vertex定義にDirect系の関数を使用している場合には使用できません。

Arguments:
	IN	ListType		頂点データリストを指定します。以下のいずれかが指定できます。
						KMLISTTYPE	KM_OPAQUE_POLYGON		= 0	// 不透明Polygon
									KM_OPAQUE_MODIFIER		= 1	// 不透明 Modifier Volume 
									KM_TRANS_POLYGON		= 2	// 半透明/透明Polygon
									KM_TRANS_MODIFIER		= 3	// 半透明/透明 Modifier Volume 
									KM_PUNCHTHROUGH_POLYGON = 4	// パンチスルーPolygon(CLX2のみ)
	IN	VertexPCW		頂点定義のParameterControlWordを指定します。
									KM_VERTEXPARAM_NORMAL		= 0xE0000000	// 通常の頂点データ
									KM_VERTEXPARAM_ENDOFSTRIP	= 0xF0000000	// ストリップ終端の頂点データ

	IN	IncPtr			VertexPCWで指定されたパラメータ書き込み後に、VertexBufferのポインタをいくら進めるのかを指定します。
						通常はsizeof(...)/4のように使用しているVertexTypeのサイズを指定します。

Result:
	KMSTATUS_SUCCESS		設定成功

--*/

KMSTATUS KMAPI
kmChangeVertexPCW(
		KMLISTTYPE ListType,
		KMDWORD    VertexPCW,
		KMDWORD    IncPtr
	);

/*++
kmChangeContextColorType

proto type:
	KMSTATUS
	kmChangeContextColorType(
			KMCOLORTYPE Color
		);

Description:
	頂点のカラーフォーマットを変更します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextColorType(
		KMCOLORTYPE Color
	);

/*++
kmChangeContextDepthMode

proto type:
	KMSTATUS 
	kmChangeContextDepthMode(
			KMDEPTHMODE DepthMode
		);

Description:
	Z値比較モードを選択します。

Arguments:

Result:

--*/

KMSTATUS  KMAPI
kmChangeContextDepthMode(
		KMDEPTHMODE DepthMode
	);

/*++
kmChangeContextCullingMode

proto type:
	KMSTATUS
	kmChangeContextCullingMode(
			KMCULLINGMODE CullingMode
		);

Description:
	カリングモードを選択します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextCullingMode(
		KMCULLINGMODE CullingMode
	);

/*++
kmChangeContextZWriteDisable

proto type:
	KMSTATUS
	kmChangeContextZWriteDisable(
			KMBOOLEAN fZWriteDisable
		);

Description:
	Z値の更新を禁止/許可します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextZWriteDisable(
		IN	KMBOOLEAN fZWriteDisable
	);

/*++
kmChangeContextSRCBlendMode

proto type:
	KMSTATUS
	kmChangeContextSRCBlendMode(
			KMBLENDINGMODE SRCBlend
		);

Description:
	ソースのブレンディングモードを選択します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextSRCBlendMode(
		IN	KMBLENDINGMODE SRCBlend
	);

/*++
kmChangeContextDSTBlendMode

proto type:
	KMSTATUS
	kmChangeContextDSTBlendMode(
			KMBLENDINGMODE DSTBlend
		);

Description:
	ディステネーションのブレンディングモードを選択します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextDSTBlendMode(
		IN	KMBLENDINGMODE DSTBlend
	);

/*++
kmChangeContextFogMode

proto type:
	KMSTATUS
	kmChangeContextFogMode(
			KMFOGMODE FogMode
		);

Description:
	フォグモードを選択します。

Arguments:

Result:

--*/
KMSTATUS KMAPI
kmChangeContextFogMode(
		IN	KMFOGMODE FogMode
	);

/*++
kmChangeContextFlipUV

proto type:
	KMSTATUS
	kmChangeContextFlipUV(
			KMFLIPMODE FlipMode
		);

Description:
	テクスチャのFlipを設定します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextFlipUV(
		IN	KMFLIPMODE FlipMode
	);

/*++
kmChangeContextClampUV

proto type:
	KMSTATUS
	kmChangeContextClampUV(
			KMCLAMPMODE ClampMode
		);

Description:
	テクスチャのClampを設定します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextClampUV(
		IN	KMCLAMPMODE ClampMode
	);

/*++
kmChangeContextFilterMode

proto type:
	KMSTATUS
	kmChangeContextFilterMode(
			KMFILTERMODE FilterMode
		);

Description:
	テクスチャフィルタを設定します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextFilterMode(
		IN	KMFILTERMODE FilterMode
	);

/*++
kmChangeContextSuperSample

proto type:
	KMSTATUS
	kmChangeContextSuperSample(
			KMBOOLEAN fEnable
		);

Description:
	引数がTRUEの場合anisotoropic Filterを使用します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextSuperSample(
		IN	KMBOOLEAN fEnable
	);

/*++
kmChangeContextTextureShadingMode

proto type:
	KMSTATUS
	kmChangeContextTextureShadingMode(
			KMTEXTURESHADINGMODE TexShadingmode
		);

Description:
	テクスチャブレンディングモードを指定します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextTextureShadingMode(
		IN	KMTEXTURESHADINGMODE TexShadingmode
	);

/*++
kmChangeContextColorClamp

proto type:
	KMSTATUS
	kmChangeContextColorClamp(
			KMBOOLEAN fColorClamp
		);

Description:
	引数がTRUEの場合グローバルレジスタの値に従って色がクランプされます。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextColorClamp(
		IN	KMBOOLEAN fColorClamp
	);

/*++
kmChangeContextPaletteBank

proto type:
	KMSTATUS
	kmChangeContextPaletteBank(
			KMDWORD Bank
		);

Description:
	現在のパレットバンクを変更します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextPaletteBank(
		IN	KMDWORD Bank
	);


/*++
kmChangeContextStripLength

proto type:
	KMSTATUS
	kmChangeContextStripLength(
			KMSTRIPLENGTH		 StripLength
		);

Description:
	入力された頂点ストリップデータを、タイルアクセラレータ内部で、何Polygonずつのストリップに分解するかを指定します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextStripLength(
		IN	KMSTRIPLENGTH		 StripLength
	);


/*++
kmChangeContextUserClipMode

proto type:
	KMSTATUS
	kmChangeContextUserClipMode(
			KMUSERCLIPMODE 		UserClipMode
		);

Description:
	KmSetUserClippingで指定したクリッピング領域に対する効果を指定します。

Arguments:

Result:

--*/

KMSTATUS KMAPI
kmChangeContextUserClipMode(
		IN	KMUSERCLIPMODE 		UserClipMode
	);


/*++
kmCreateVertexBuffer

proto type:
	KMSTATUS
	kmCreateVertexBuffer(
			PKMVERTEXBUFFDESC pBufferDesc,
			KMINT32 OpaquePolygonBuffer,
			KMINT32 OpaqueModifierBuffer,
			KMINT32 TransPolygonBuffer,
			KMINT32 TransModifierBuffer
		);

Description:
	頂点データ登録に使用するバッファ(以下 Vertex Buffer)と、Vertex Buffer に登録された頂点データを、
	タイルアクセラレータがレンダリングに使用するパラメータとして展開するためのバッファ(以下 Native Buffer)を
	以下の場所にそれぞれダブルバッファとして確保します。

			Vertex Buffer	...  メインメモリ上
			Native Buffer	...	  Frame Buffer メモリ

	Native Buffer のサイズは、引数で指定された各リストのサイズをもとに算出します。

	頂点データ登録は、 kmStartVertexStrip, kmSetVertex を使用して行います。
	各Buffer へ 1シーン分のデータの登録完了後、kmRender でまとめてＨＷへ送ることによってレンダリングを開始します。

	注： 本関数は互換性のために用意されているので kmSetSystemConfiguration を使用してください。

		レンダリング中にＴＡの展開を行うために ダブルバッファとして Native Data Buffer を確保します。
		なお、CLX2よりサポートされる Punchthrough Polygon用Buffer の容量を指定することはできません、
		また、kmCreateVertexBufferとkmCreateTABufferを同時に使用することはできません。

Arguments:
	I/O	pBufferDesc		PKMVERTEXBUFFDESC型の、Vertex Buffer Descriptorへのポインタを入力します。
						Vertex Buffer Descriptorの各メンバには、ここで確保された
						4 つのバッファの先頭アドレスが返されます。

	IN	OpaquePolygonBuffer
						不透明Polygonを積み上げる頂点データリストのサイズ(DWORD数)。

	IN	OpaqueModifierBuffer
						不透明 Modifier Volume を積み上げる頂点データリストのサイズ(DWORD数)。

	IN	TransPolygonBuffer
						半透明/透明Polygonを積み上げる頂点データリストのサイズ(DWORD数)。

	IN	TransModifierBuffer
						半透明/透明 Modifier Volume を積み上げる頂点データリストのサイズ(DWORD数)。

Result:
	KMSTATUS_SUCCESS			頂点データリスト確保成功
	KMSTATUS_NOT_ENOUGH_MEMORY	メモリ容量が足りない

--*/

KMSTATUS KMAPI
kmCreateVertexBuffer(
		IN OUT	PKMVERTEXBUFFDESC pBufferDesc,
		IN		KMINT32 OpaquePolygonBuffer,
		IN		KMINT32 OpaqueModifierBuffer,
		IN		KMINT32 TransPolygonBuffer,
		IN		KMINT32 TransModifierBuffer
	);


/*++
kmDiscardVertexBuffer

prototype:
KMSTATUS KMAPI
kmDiscardVertexBuffer(
		PKMVERTEXBUFFDESC pBufferDesc
	);

Description:
	kmCreateVertexBufferにて確保した頂点データ登録用の 4種類のバッファ(ダブルバッファ)を開放します。
	同時に Frame Buffer 上に確保したタイルアクセラレータ出力用バッファ(Native Data Buffer)も開放します。

Arguments:
	I/O pBufferDesc			PKMVERTEXBUFFDESC型の、Vertex Buffer Descriptorへのポインタを入力します。

Result:
	KMSTATUS_SUCCESS		頂点データリスト開放成功

--*/
KMSTATUS KMAPI
kmDiscardVertexBuffer(
		PKMVERTEXBUFFDESC pBufferDesc
	);


/*++
kmCreateTABuffer

proto type:
	KMSTATUS
	kmCreateTABuffer(
		IN	PKMVERTEXBUFFDESC pBufferDesc,
		IN	KMINT32 TABuffer
		);

Description:
	タイルアクセラレータ出力用バッファ(Native Data Buffer)を、 Frame Buffer メモリ上に確保します。
	頂点データ登録は、以下の関数を使用してタイルアクセラレータへ直接データを送り込むことで実現します。

				kmStartVertexStripDirect
				kmSetVertexDirect
				kmSetUserClippingDirect
				kmSetEndOfListDirect
				kmRenderDirect

注意：ひとつのアプリケーション内で kmCreateVertexBuffer と kmCreateTABuffer を同時に使用することはできません。
      また、テクスチャサーフェスの確保は、本関数呼び出し以前に後で呼び出してください。
	  (CLX1/2)では、ネィティブデータバッファの生成方法が変わるため、この関数は使用できません。
	  (CLX1/2)では、kmSetSystemConfiguration を使用してください。

Arguments:

	IN	pBufferDesc		PKMVERTEXBUFFDESC型の、Vertex Buffer Descriptorへのポインタです。

	IN	TABuffer		タイルアクセラレータ出力(PowerVRネイテイブデータ)を保存するバッファのサイズ(バイト数)。

Result:
	KMSTATUS_SUCCESS			頂点データリスト確保成功
	KMSTATUS_NOT_ENOUGH_MEMORY	メモリ容量が足りない

--*/

KMSTATUS KMAPI
kmCreateTABuffer(
		IN	PKMVERTEXBUFFDESC pBufferDesc,
		IN	KMINT32 TABuffer
	);

/*++
kmFlushVertexBuffer

proto type:
	KMSTATUS
	kmFlushVertexBuffer(
		IN	KMLISTTYPE ListType
		);

Description:
	ListType で指定した 頂点データリストの内容を Tiling Accelerator に転送します。

注意：

Arguments:
	IN	ListType		Flush する頂点データリストを指定します。

Result:
	KMSTATUS_SUCCESS	転送成功？

--*/

KMSTATUS KMAPI
kmFlushVertexBuffer(
		IN	KMLISTTYPE ListType
		);

/*++
kmStartVertexStrip

proto type:
	KMSTATUS
	kmStartVertexStrip( 
			PKMVERTEXBUFFDESC pBufferDesc
		);

Description:
	神威に、頂点データストリップの登録を開始することを通知します。
	以下の関数の呼び出しでシステムに登録した

		kmSetVertexRenderState
		kmSetModifierRenderState

	レンダリングパラメータを、5種の頂点データリストのいずれかへ書き込みます。
	書き込む頂点データリストは、 KMVERTEXCONTEXTの
	ListTypeメンバから決定されます。

注意：ここで書き込んだレンダリングパラメータの情報と、これに続くkmSetVertexで送る
	  頂点パラメータの形式の組み合わせが正しくない場合の動作は保証されません。

	<本関数は高速性を要求されるためInline展開されるマクロとして定義されます。>

	マクロを使用する場合には、以下のように設定してください。

		#define _KM_USE_VERTEX_MACRO_
		#include <kamui.h>
		#include <kamuix.h> 

Arguments:
	IN	pBufferDesc				PKMVERTEXBUFFDESC型の、Vertex Buffer Descriptorへのポインタです。
								データを登録する頂点データリストの情報を示す。

Result:
	KMSTATUS_SUCCESS			レンダリングパラメータ 書き込み成功
	KMSTATUS_NOT_ENOUGH_MEMORY	頂点データリストの容量が足りない

--*/
#if !defined(_KM_USE_VERTEX_MACRO_)
KMSTATUS KMAPI
kmStartVertexStrip( 
		IN	PKMVERTEXBUFFDESC pBufferDesc
	);
#endif

/*++
kmStartVertexStripDirect

proto type:
	KMSTATUS
	kmStartVertexStripDirect(VOID);

Description:
	頂点データストリップの登録を開始します。kmProcessVertexRenderState で作成後 kmSetModifierRenderState で
	システムに登録したレンダリングパラメータをタイルアクセラレータへ直接書き込みます。
	同一シーンの頂点データをタイルアクセラレータに送る場合、頂点の種類毎にまとめて入力する必要があります。

	例： Opaque Polygon を登録後  Translucent Polygon を登録し、再度Opaque Polygon を登録する ことはできません。

	注意：	ここで書き込んだレンダリングパラメータの情報と、これに続くkmSetVertexで書き込む
			頂点パラメータの形式の組み合わせが正しくない場合の動作は保証されません。

			リストへの頂点登録が完了し、別のリストの登録を開始する場合、 kmSetEndOfListDirect を使用して
			リストの完了をタイルアクセラレータに通知する必要があります。


Result:
	KMSTATUS_SUCCESS		レンダリングパラメータ書き込み成功

Arguments:
	IN	pBufferDesc		KMVERTEXBUFFDESC型のVertex Buffer Descriptorへのポインタです。

Result:
	KMSTATUS_SUCCESS		レンダリングパラメータ書き込み成功

--*/

KMSTATUS KMAPI
kmStartVertexStripDirect(VOID);

/*++
kmSetVertex

proto type:
	KMSTATUS
	kmSetVertex(
			PKMVERTEXBUFFDESC pBufferDesc,
			PVOID pVertex,
			KMVERTEXTYPE VertexType,
			KMINT32 StructSize
		);

Description:
	pVertexが指す頂点データを、システムに登録済みのレンダリンパラメータの ListType で指定した頂点データリストへ書き込みます。
	ストリップの最後となる頂点データのParameter Control Word がKM_VERTEXPARAM_ENDOFSTRIP でなかったときの動作は保証しません。

	注意：	ここで書き込む頂点パラメータの形式と、この前のkmSetVertexRenderStateで設定したグローバルパラメータの形式の
			組み合わせが正しくない場合の動作は保証しません。

	<本関数は高速性を要求されるためInline展開されるマクロとして定義されます。>

	マクロを使用する場合には、以下のように設定してください。

		#define _KM_USE_VERTEX_MACRO_
		#include <kamui.h>
		#include <kamuix.h> 

Arguments:
	IN	pBufferDesc		PKMVERTEXBUFFDESC型の、Vertex Buffer Descriptorへのポインタです。
	IN	pVertex			頂点データ構造体へのポインタです。
	IN	VertexType		頂点データのデータタイプを示します。下記のいずれかを指定します。
						KM_VERTEXTYPE_00			// 頂点データ Type 0
						KM_VERTEXTYPE_01			// 頂点データ Type 1
						KM_VERTEXTYPE_02			// 頂点データ Type 2
						KM_VERTEXTYPE_03			// 頂点データ Type 3
						KM_VERTEXTYPE_04			// 頂点データ Type 4
						KM_VERTEXTYPE_05			// 頂点データ Type 5
						KM_VERTEXTYPE_06			// 頂点データ Type 6
						KM_VERTEXTYPE_07			// 頂点データ Type 7
						KM_VERTEXTYPE_08			// 頂点データ Type 8
						KM_VERTEXTYPE_09			// 頂点データ Type 9
						KM_VERTEXTYPE_10			// 頂点データ Type 10
						KM_VERTEXTYPE_11			// 頂点データ Type 11
						KM_VERTEXTYPE_12			// 頂点データ Type 12
						KM_VERTEXTYPE_13			// 頂点データ Type 13
						KM_VERTEXTYPE_14			// 頂点データ Type 14
						KM_VERTEXTYPE_15			// 頂点データ Type 15
						KM_VERTEXTYPE_16			// 頂点データ Type 16
						KM_VERTEXTYPE_17			// 頂点データ Type 17

	IN	StructSize		頂点データのデータタイプサイズを示します。頂点データに使用するタイプに合わせて、
						sizeof(KMVERTEX_01)の様に指定してください。

Result:
	KMSTATUS_SUCCESS			頂点データ書き込み成功
	KMSTATUS_NOT_ENOUGH_MEMORY	頂点データリストの容量が足りない

--*/
#if !defined(_KM_USE_VERTEX_MACRO_)
KMSTATUS KMAPI
kmSetVertex(
		IN	PKMVERTEXBUFFDESC pBufferDesc,
		IN	PVOID pVertex,
		IN	KMVERTEXTYPE VertexType,
		IN	KMINT32 StructSize
	);
#endif

/*++
kmSetVertexDirect

proto type:
	KMSTATUS
	kmSetVertexDirect(
			PVOID pVertex,
			KMVERTEXTYPE VertexType,
			KMINT32 StructSize
		);

Description:
	pVertexが指す頂点データをタイルアクセラレータへ直接書き込みます。
	同一シーンの頂点データをタイルアクセラレータに対送る場合、頂点の種類毎にまとめて入力する必要があります。

	注意：	ストリップの最後となる頂点データの Parameter Control Word がKM_VERTEXPARAM_ENDOFSTRIP で
			なかったときの動作は保証しません。
			ここで書き込む頂点データのデータタイプと、この前のkmSetVertexRenderStateで設定した
			グローバルパラメータの形式の組み合わせが正しくない場合の動作は保証しません。

Arguments:
	IN	pVertex			頂点データ構造体へのポインタです。
	IN	VertexType		頂点データのデータタイプを示します。下記のいずれかを指定します。
						KM_VERTEXTYPE_00			// 頂点データ Type 0
						KM_VERTEXTYPE_01			// 頂点データ Type 1
						KM_VERTEXTYPE_02			// 頂点データ Type 2
						KM_VERTEXTYPE_03			// 頂点データ Type 3
						KM_VERTEXTYPE_04			// 頂点データ Type 4
						KM_VERTEXTYPE_05			// 頂点データ Type 5
						KM_VERTEXTYPE_06			// 頂点データ Type 6
						KM_VERTEXTYPE_07			// 頂点データ Type 7
						KM_VERTEXTYPE_08			// 頂点データ Type 8
						KM_VERTEXTYPE_09			// 頂点データ Type 9
						KM_VERTEXTYPE_10			// 頂点データ Type 10
						KM_VERTEXTYPE_11			// 頂点データ Type 11
						KM_VERTEXTYPE_12			// 頂点データ Type 12
						KM_VERTEXTYPE_13			// 頂点データ Type 13
						KM_VERTEXTYPE_14			// 頂点データ Type 14
						KM_VERTEXTYPE_15			// 頂点データ Type 15
						KM_VERTEXTYPE_16			// 頂点データ Type 16
						KM_VERTEXTYPE_17			// 頂点データ Type 17

	IN	StructSize		頂点データのデータタイプサイズを示します。頂点データに使用するタイプに合わせて、
						sizeof(KMVERTEX_01)の様に指定してください。
Result:
	KMSTATUS_SUCCESS		頂点データ書き込み成功

--*/

KMSTATUS KMAPI
kmSetVertexDirect(
		IN	PVOID 				pVertex,
		IN	KMVERTEXTYPE		VertexType,
		IN	KMINT32				StructSize
	);


/*++
kmSetEndOfListDirect

proto type:
	KMSTATUS
	kmSetEndOfListDirect( VOID );

Description:
	特定の種類のリストの終了を、タイルアクセラレータへ直接通知します。

	現在登録中のリストの最後に kmSetEndOfListDirect() を発行して、
	現在のリストタイプのデータの登録が終了したことを通知します。

	注意：kmSetVertexを使用して頂点登録を行っている場合には、本関数を呼び出してはいけません。

Arguments:
	None

Result:
	KMSTATUS_SUCCESS		通知成功

--*/

KMSTATUS KMAPI
kmSetEndOfListDirect( VOID );


/*++
kmRender

proto type:
	KMSTATUS
	kmRender( VOID );

Description:
	タイルアクセラレータに対して、1シーン分の頂点データの登録が終了したことを神威に通知し,
	データ展開完了後、Back Buffer に対するレンダリングを開始します。

Arguments:
	None

Result:
	KMSTATUS_SUCCESS		通知成功

--*/

KMSTATUS KMAPI
kmRender( VOID );

/*++
kmRenderDirect

proto type:
	KMSTATUS
	kmRenderDirect(VOID);

Description:
	タイルアクセラレータに対して、1シーン分の頂点データの登録が終了したことを神威に通知し,
	頂点データ展開完了後、Back Buffer に対するレンダリングを開始します。

Arguments:
	None

Result:
	KMSTATUS_SUCCESS		通知成功

--*/

KMSTATUS KMAPI
kmRenderDirect(VOID);

/*++
kmRenderTexture

proto type:
	KMSTATUS
	kmRenderTexture(	
			PKMSURFACEDESC pTextureSurface
		);

Description:
	タイルアクセラレータに対して、1シーン分の頂点データの登録が終了したことを神威に通知します。
	頂点データ展開完了後、引数(pTextureSurface) で指定されれたテクスチャに対するレンダリングを開始します。(
	なお、描画先に指定するテクスチャは、Rectangle/Stride形式でなければなりません。

Arguments:
	OUT	pTextureSurface		レンダリング結果を格納するテクスチャです。

Result:
	KMSTATUS_SUCCESS			通知成功
	KMSTATUS_INVALID_TEXTURE	無効なテクスチャを指定した

--*/

KMSTATUS KMAPI
kmRenderTexture(
		PKMSURFACEDESC pTextureSurface
	);

/*++
kmRenderTextureDirect

proto type:
	KMSTATUS
	kmRenderTextureDirect(
			PKMSURFACEDESC pTextureSurface
		);

Description:
	タイルアクセラレータに対して、1シーン分の頂点データの登録が終了したことを神威に通知します。
	頂点データ展開完了後、引数(pTextureSurface) で指定されれたテクスチャに対するレンダリングを開始します。(
	なお、描画先に指定するテクスチャは、Rectangle/Stride形式でなければなりません。

Arguments:
	OUT	pTextureSurface		レンダリング結果を格納するテクスチャです。

Result:
	KMSTATUS_SUCCESS		通知成功
	KMSTATUS_INVALID_TEXTURE	無効なテクスチャを指定した

--*/

KMSTATUS KMAPI
kmRenderTextureDirect(
		PKMSURFACEDESC pTextureSurface
	);

/*++
kmSetEORCallback

proto type:

	KMSTATUS 
	kmSetEORCallback(
			PKMCALLBACKFUNC pEORCallback,
			PVOID pCallbackArguments
		);

Description:
	レンダリング終了時に呼ばれるコールバック関数を登録します。
	コールバック関数は、以下の形式でコーディングしてください。

		VOID EORCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pEORCallback	レンダリング終了時に呼ばれる関数のポインタを指定します。
	IN	pCallbackArguments
						callback 時に呼出される関数へ渡す、引数へのポインタを指定します。


Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS  KMAPI
kmSetEORCallback(
		PKMCALLBACKFUNC pEORCallback,
		PVOID 			pCallbackArguments
	);

/*++
kmSetVSyncCallback

proto type:
	KMSTATUS
	kmSetVSyncCallback(
			PKMCALLBACKFUNC pVSyncCallback,
			PVOID pCallbackArguments
		);

Description:
	垂直帰線区間(Vsync)突入時に呼び出されるコールバック関数を登録します。
	コールバック関数は、以下の形式でコーディングしてください。

		VOID VSyncCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pVSyncCallback	Vsync突入時に呼ばれる関数のポインタを指定します。
	IN	pCallbackArguments
						callback 時に呼出される関数へ渡す、引数へのポインタを指定します。

Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetVSyncCallback(
		PKMCALLBACKFUNC pVSyncCallback,
		PVOID 			pCallbackArguments
	);

/*++
kmSetHSyncCallback

proto type:
	KMSTATUS
	kmSetHSyncCallback(
			PKMCALLBACKFUNC pHSyncCallback,
			PVOID pCallbackArguments
		);

Description:
	水平帰線区間(Hsync)突入時に呼び出されるコールバック関数を登録します。
	コールバック関数は、以下の形式でコーディングしてください。

	VOID HSyncCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pHSyncCallback		Hsync突入時に呼ばれる関数のポインタを指定します。

	pCallbackArguments		callback 時に呼出される関数へ渡す、引数へのポインタを指定します。
							KmSetHSyncLine で指定したライン番号は渡されないので、
							kmSetHSyncLine で設定した値をこのポインタが示す領域に保持するか、
							または、kmGetCurrentScanline()を使用して、現在の Scanline 数を取得してください。

Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetHSyncCallback(
		PKMCALLBACKFUNC pHSyncCallback,
		PVOID pCallbackArguments
	);

/*++
kmSetHSyncLine

proto type:
	KMSTATUS
	kmSetHSyncLine(
			KMINT32 nInterruptLine
		);

Description:
	表示何ライン目で割り込みを起こすかを指定します。 

Arguments:
	IN	nInterruptLine	何ライン目で割り込みを起こすか指定します。0～240/480の値を設定してください。 

Result:
	KMSTATUS_SUCCESS			設定成功
	KMSTATUS_ILLEGAL_PARAMETER	値が不正

--*/

KMSTATUS KMAPI
kmSetHSyncLine(
		KMINT32 nInterruptLine
	);

/*++
kmGetCurrentScanline

proto type:
	KMSTATUS
	kmGetCurrentScanline(
			PKMINT32 pScanline
		);

Description:
	現在のH-Syncラインを読み出します。 

Arguments:
	OUT	pScanline	現在のH-Syncラインを格納するKMINT32へのポインタです。 

Result:
	KMSTATUS_SUCCESS			設定成功

--*/

KMSTATUS KMAPI
kmGetCurrentScanline(
		PKMINT32 pScanline
	);

/*++
kmSetTexOverflowCallback

proto type:
	KMSTATUS
	kmSetTexOverflowCallback(
			PKMCALLBACKFUNC pTexOverflowCallback,
			PVOID pCallbackArguments
		);

Description:
	テクスチャメモリが残っていない状態で、テクスチャを登録しようとした場合に呼ばれるコールバック関数を
	登録します。コールバック関数は、以下の形式でコーディングしてください。

		VOID TexOverflowCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pTexOverflowCallback	テクスチャオーバーフロー時のコールバック関数へのポインタを指定します。
	IN	pCallbackArguments		callback 時に呼出される関数へ渡す、引数へのポインタを指定します。
	
Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetTexOverflowCallback(
		PKMCALLBACKFUNC pTexOverflowCallback,
		PVOID pCallbackArguments
	);

/*++
kmSetStripOverRunCallback

proto type:
	KMSTATUS
	kmSetStripOverRunCallback (
			PKMCALLBACKFUNC	pStripOverRunCallback,
			PVOID			pCallbackArguments
		);

Description:
	StripBufferの縦サイズの表示期間中に次のStripのレンダリングが終了しなかった場合に呼ばれる
	コールバック関数を登録します。コールバック関数は、以下の形式でコーディングしてください。

			VOID StripOverRunCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pStripOverRunCallback	コールバック関数へのポインタを指定します。
	IN	pCallbackArguments		callback 時に呼出される関数へ渡す、引数へのポインタを指定します。

Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetStripOverRunCallback (
		PKMCALLBACKFUNC	pStripOverRunCallback,
		PVOID			pCallbackArguments
	);

/*++
kmSetEndOfVertexCallback

proto type:
	KMSTATUS
	kmSetEndOfVertexCallback (
			PKMCALLBACKFUNC pEndOfVertexCallback,
			PKMVOID			pCallbackArguments
		);

Description:
	神威からレンダリングハードウェアへの、1シーンのデータ転送が終了した場合に呼ばれるコールバック関数を
	登録します。コールバック関数は、以下の形式でコーディングしてください。

			VOID EndOfVertexCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pEndOfVertexCallback	コールバック関数へのポインタを指定します。
	IN	pCallbackArguments		callback 時に呼出される関数へ渡す、引数へのポインタを指定します。

Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetEndOfVertexCallback (
		PKMCALLBACKFUNC pEndOfVertexCallback,
		PKMVOID 		pCallbackArguments
	);

/*++
kmSetEndOfYUVCallback

proto type:
	KMSTATUS
	kmSetEndOfYUVCallback(
			PKMCALLBACKFUNC pEndOfYUVCallback,
			PVOID pCallbackArguments
		);

Description:
	CLXのYUVコンバータがコンバートを終了した際に発行する、YUV終了割り込みに対する
	コールバック関数を登録します。

		VOID EndOfYUVFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pEndOfYUVCallback		YUVコンバート終了時のコールバック関数へのポインタを指定します。
	IN	pCallbackArguments		callback 時に呼出される関数へ渡す、引数へのポインタを指定します。
	
Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetEndOfYUVCallback(
		PKMCALLBACKFUNC pEndOfYUVCallback,
		PVOID pCallbackArguments
	);

/*++
kmSetWaitVsyncCallback

proto type:
	KMSTATUS
	kmSetWaitVsyncCallback(
			PKMCALLBACKFUNC pWaitVsyncCallback,
			PVOID pCallbackArguments
		);

Description:
	VsyncのWAIT中に呼ばれるコールバック関数を登録します。CD-ROMからの読み出しをバックグラウンド
	等で行う場合や、その他V非同期処理を行うためのコールバック等として使用します。
	ただし、あまり大きな関数、コールバック内での無限ループ等は行わないでください。
	
		VOID WaitVsyncCallbackFunc(PVOID pCallbackArguments);

	pCallbackArguments(入力)：登録時に指定されたパラメータへのポインタです。

Arguments:
	IN	pWaitVsyncCallback		Vsyncウェイト中のコールバック関数へのポインタを指定します。
	IN	pCallbackArguments		callback 時に呼出される関数へ渡す、引数へのポインタを指定します。
	
Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetWaitVsyncCallback(
		PKMCALLBACKFUNC pWaitVsyncCallback,
		PVOID pCallbackArguments
	);

/*++
kmSetPALEXTCallback

proto type:
	KMSTATUS
	kmSetPALEXTCallback(
			PKMCALLBACKFUNC pPalExtCallback,
			PVOID 			Reserved
		);

Description:
	kmSetDisplayMode を呼び出した際に、	PAL ??? EXT を指定した場合、
	縦の表示比率を決定するための定数を返すためのコールバック関数です。

		VOID pPalExtCallback(PVOID pData);

	pData には、システム内部の KMPALEXTINFO 構造体へのポインタが渡されます。
	nPALExtMode メンバに対して適切な値を代入してください。
	KMPALEXTINFO 構造体の nDisplayModeには、
	kmSetDisplayMode の引数が代入されています。

Arguments:
	IN	pFaltalErrorCallback	回復不能なエラーが発生した場合に呼び出されるコールバック関数です。
	IN	reserved				NULL を入れてください。

Result:
	KMSTATUS_SUCCESS		登録成功

--*/

KMSTATUS KMAPI
kmSetPALEXTCallback( 
		IN	PKMCALLBACKFUNC pPalExtCallback,
		IN	PVOID			pCallbackArguments
	);

/*++
kmStopDisplayFrameBuffer

proto type:
	KMSTATUS
	kmStopDisplayFrameBuffer ( VOID )

Description:
	Frame Bufferの表示を停止します。但し、CRTコントローラが表示を止めるだけですので、
	Frame Bufferの状態は変化しません。

Arguments:
	None

Result:
	KMSTATUS_SUCCESS		成功

--*/
KMSTATUS KMAPI
kmStopDisplayFrameBuffer ( VOID );

/*++
kmGetVersionInfo

proto type:
	KMSTATUS
	kmGetVersionInfo(
			PKMVERSIONINFO pVersionInfo
		);

Description:
	ライブラリのバージョン情報を取得します。
	バージョン情報構造体の内容については別途構造体一覧をご覧ください。

Arguments:
	OUT	pVersionInfo	あらかじめ確保しておいたKMVERSIONINFO構造体へのポインタを示します。

Result:
	KMSTATUS_SUCCESS		成功

--*/

KMSTATUS KMAPI
kmGetVersionInfo(
		PKMVERSIONINFO pVersionInfo
	);

/*++
kmLoadTexture

proto type:

	KMSTATUS
	kmLoadTexture(
			PKMSURFACEDESC pSurfaceDesc,
			PKMDWORD pTexture,
			KMBOOLEAN fAutoMipMap,
			KMBOOLEAN fDither
		);

Description:
	kmCreateTextureSurfaceで確保したテクスチャメモリ上の領域に、 
	pTextureで指定されたメインメモリ上のテクスチャを読み込みます。

Arguments:
	OUT	pSurfaceDesc	kmCreateTextureSurfaceで確保したテクスチャサーフェスです。
	IN	pTexture		メインメモリ上のテクスチャを示すポインタです。
	IN	bAutoMipMap		自動的にMIPMAPを生成するかどうかを指定します。
						TRUEが指定された場合には自動的にMIPMAPを生成します。
	IN	bDither			MIPMAPの自動生成を指定した場合に、自動生成中にディザをかけてMIPMAPを
						生成するかどうかを指定します。TRUEが指定された場合はディザをかけます。
						このフラグはbAutoMipMapにTRUEが指定されたときのみ有効です。

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した
	KMSTATUS_INVALID_MIPMAPED_TEXTURE	MIPMAPできないテクスチャに
										MIPMAPを使用しようとした

--*/
KMSTATUS KMAPI
kmLoadTexture(
		PKMSURFACEDESC pSurfaceDesc,
		PKMDWORD pTexture,
		KMBOOLEAN fAutoMipMap,
		KMBOOLEAN fDither
	);

/*++
kmLoadTextureBlock

proto type:

	KMSTATUS KMAPI
	kmLoadTextureBlock(
		PKMSURFACEDESC	pSurfaceDesc,
		PKMDWORD		pTexture,
		KMUINT32		nBlockNum,
		KMUINT32		nBlockSize
	)

Description:
	kmCreateTextureSurfaceでアロケートしたテクスチャメモリ上の領域に、 
	pTextureで指定されたメインメモリ上のテクスチャブロックを読み込みます。

	テクスチャデータをいくつかのブロックに分割して読み込みます。
	これにより、メインメモリ上に大きなワークエリアを確保することなく、
	サイズの大きなテクスチャを読み込むことができます。

	テクスチャ全体のサイズがブロックサイズで割り切れなくても、正しく読み込まれます。

	読み込むテクスチャのフォーマット形式、サイズなどは、pSurfaceDesc
	が指すサーフェスディスクリプタより判断します。
	実際のテクスチャのフォーマット形式などが、pSurfaceDescが指すサーフェスディスクリプタ
	の内容と異なっている場合は、表示が不正となります。

	なお、この関数はSmall VQ形式のテクスチャには対応していません。
	Small VQ形式のpSurfaceDescを指定した場合には、KMSTATUS_INVALID_TEXTURE_TYPEを返します。


	また、ARC1では、VQ、Mipmapフォーマットには対応していません。

Arguments:
	IN	pSurfaceDesc	kmCreateTextureSurface/kmCreateCombinedTextureSurfaceで
						アロケートしたテクスチャサーフェスです。
	IN	pTexture		メインメモリ上のテクスチャブロックの先頭を示すポインタです。
	IN	nBlockNum		テクスチャのブロック番号を 0 から n ( nはフォーマット/サイズによる)で指定します。
	IN	nBlockSize		ブロックのサイズを 32バイト単位で指定します。
						ブロックサイズ(バイト)を 32 で割った値を指定してください。
						なお、テクスチャ全体のサイズがブロックサイズで割り切れる必要はありません。

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_BLOCKNUMBER		ブロック番号が不正
	KMSTATUS_INVALID_ADDRESS			指定されたアドレスを先頭に持つメモリブロックは
										アロケートされていない
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した

--*/

KMSTATUS KMAPI
kmLoadTextureBlock(
		PKMSURFACEDESC	pSurfaceDesc,
		PKMDWORD		pTexture,
		KMUINT32		nBlockNum,
		KMUINT32		nBlockSize
	);

/*++
kmLoadTexturePart

proto type:

	KMSTATUS KMAPI
	kmLoadTexturePart(
			PKMSURFACEDESC	pSurfaceDesc,
			PKMDWORD		pTexture,
			KMUINT32		nOffset,
			KMUINT32		nSize
		)

Description:
	kmCreateTextureSurfaceでアロケートしたテクスチャメモリ上の領域に、 
	pTextureで指定されたメインメモリ上のテクスチャ(部分)を読み込みます。

	テクスチャデータをいくつかの部分に分割して読み込みます。
	これにより、メインメモリ上に大きなワークエリアを確保することなく、
	サイズの大きなテクスチャを読み込むことができます。

	kmLoadTextureBlockと異なり、一つのテクスチャを、「サイズの異なる」
	複数の部分に分けて読み込むことができます。ただし、今までに読み込んだ
	テクスチャのサイズ(先頭からのオフセット量)は、ユーザ側で管理する
	必要があります。

	読み込むテクスチャのフォーマット形式、サイズなどは、pSurfaceDesc
	で指定されるサーフェスディスクリプタより判断されます。
	実際のテクスチャのフォーマット形式などが、pSurfaceDescで指定される
	サーフェスディスクリプタの内容と異なっている場合は、表示が不正となります。

	なお、この関数はSmall VQ形式のテクスチャには対応していません。
	Small VQ形式のpSurfaceDescを指定した場合には、KMSTATUS_INVALID_TEXTURE_TYPEを返します。


	また、ARC1では、VQ、Mipmapフォーマットには対応していません。

Arguments:
	IN	pSurfaceDesc	kmCreateTextureSurface/kmCreateCombinedTextureSurfaceで
						アロケートしたテクスチャサーフェスです。
	IN	pTexture		メインメモリ上のテクスチャブロックの先頭を示すポインタです。
	IN	nOffset			すでに読み込んだテクスチャのサイズ(先頭からのオフセット量)を
						バイト単位で指定します。これは、今回読み込むテクスチャデータが
						転送される、テクスチャ領域の先頭アドレスとなります。
	IN	nSize			今回読み込む部分のサイズをバイト単位で指定します。
						なお、nSizeが、テクスチャの残りサイズ ( = テクスチャサイズ - nOffset)
						より大きい場合は、残りサイズのみを正しく読み込んで正常終了します。

Usage:
	nOffset = 0;
	nSize   = ***;
	pTextureにテクスチャデータの最初のnSizeバイトをロード;
	while(KMSTATUS_SUCCESS == kmLoadTexturePart(...);
		nOffset = nOffset + nSize;
		nSize   = ????;
		pTextureに次のnSizeバイトをロード;
	}

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_ADDRESS			nOffsetがテクスチャサイズを越えている
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した

--*/

KMSTATUS KMAPI
kmLoadTexturePart(
	IN	PKMSURFACEDESC	pSurfaceDesc,
	IN	PKMDWORD		pTexture,
	IN	KMUINT32		nOffset,
	IN	KMUINT32		nSize
	);


/*++
kmLoadRectangleTexturePart

proto type:

	KMSTATUS KMAPI
	kmLoadRectangleTexturePart(
			PKMSURFACEDESC	pSurfaceDesc,
			PKMDWORD		pTexture,
			KMUINT32		width,
			KMUINT32		height,
			KMUINT32		dst_X,
			KMUINT32		dst_Y,
			KMUINT32		src_X,
			KMUINT32		src_Y,
			KMUINT32		src_U
			)

Description:
	kmCreateTextureSurfaceでアロケートしたRectangle/Stride形式
	テクスチャメモリ上の矩形領域に、pTextureで指定されたメイン
	メモリ上のテクスチャの矩形領域(部分)を書き込みます。

	Rectangle/Stride形式テクスチャの一部を書き換えることができます。

	転送先の矩形領域は、( dst_X, dst_Y )-( dst_X + width, dst_Y + height )です。
	転送元の矩形領域は、( src_X, src_Y )-( src_X + width, src_Y + height )です。
	src_Uがゼロであった場合は、転送元テクスチャのサイズは矩形領域の幅と
	高さ(width,height)と等しいものとみなします。

	なお、この関数はRectangle/Stride形式テクスチャにのみ対応しています。
	これら以外のpSurfaceDescを指定した場合には、
	KMSTATUS_INVALID_TEXTURE_TYPEを返します。

	矩形領域は、転送先・転送元テクスチャの領域内に完全に含まれていなけ
	ればなりません。
	そうでない場合には、転送先テクスチャを含め、他のテクスチャのデータ
	が破壊される場合があります。
	神威では、このチェックは行いません。

Arguments:
	IN	pSurfaceDesc	kmCreateTextureSurface/kmCreateCombinedTextureSurfaceで
						アロケートしたRectangle/Stride形式のテクスチャサーフェスです。
						(転送先テクスチャ)
	IN	pTexture		メインメモリ上のテクスチャデータの先頭を示すポインタです。
						(転送元テクスチャ)
	IN	width,height	転送する矩形領域の幅と高さをテクセル数で指定します。
	IN	dst_X,dst_Y		転送先テクスチャに於ける矩形領域のX,Y座標です。
						転送先テクスチャの左下(先頭)テクセルからの
						相対テクセル数で指定します。
	IN	src_X,src_Y		転送元テクスチャに於ける矩形領域のX,Y座標です。
						転送元テクスチャの左下(先頭)テクセルからの
						相対テクセル数で指定します。
	IN	src_U			転送元テクスチャの幅をテクセル数で指定します。
						これがゼロであった場合は、転送元テクスチャのサイズは
						矩形領域の幅と高さ(width,height)と等しいものとみなします。
						このときは、src_X,src_Yの値は無視されます。(0とみなされます)

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した

--*/

KMSTATUS KMAPI
kmLoadRectangleTexturePart(
		PKMSURFACEDESC	pSurfaceDesc,
		PKMDWORD		pTexture,
		KMUINT32		width,
		KMUINT32		height,
		KMUINT32		dst_X,
		KMUINT32		dst_Y,
		KMUINT32		src_X,
		KMUINT32		src_Y,
		KMUINT32		src_U
	);


/*++
kmFreeTexture

proto type:
	KMSTATUS
	kmFreeTexture(
			PKMSURFACEDESC pSurfaceDesc
		);

Description:
	指定したテクスチャサーフェスを解放します。

Arguments:
	I/O	pSurfaceDesc	kmCreateTextureSurfaceで確保したテクスチャサーフェスです。

Result:
	KMSTATUS_SUCCESS				解放成功
	KMSTATUS_INVALID_PARAMETER		解放失敗、テクスチャの指定が不正

--*/

KMSTATUS KMAPI
kmFreeTexture(
		PKMSURFACEDESC pSurfaceDesc
	);


/*++
kmGetFreeTextureMem

proto type:
	KMSTATUS
	kmGetFreeTextureMem(
		PKMUINT32 pSizeOfTexture,
		PKMUINT32 pMaxBlockSizeOfTexture
		);

Description:
	テクスチャメモリの残容量を返します。

Arguments:
	OUT	pSizeOfTexture
			テクスチャメモリの残容量を返すKMDWORD領域へのポインタです。
	OUT	pMaxBlockSizeOfTexture
			テクスチャメモリの空きブロックの最大容量を返すKMDWORD領域へのポインタです。
Result:
	KMSTATUS_SUCCESS		成功

--*/

KMSTATUS KMAPI
kmGetFreeTextureMem(
		PKMUINT32 pSizeOfTexture,
		PKMUINT32 pMaxBlockSizeOfTexture
	);

/*++
kmReLoadMipmap

proto type:
	KMSTATUS
	kmReLoadMipmap(
			PKMSURFACEDESC 	pSurfaceDesc,
			PKMVOID	 		pTexture,
			KMINT32	 		nMipmapCount
		);

Description:
	kmCreateTextureSurfaceで確保したテクスチャメモリ上の領域に、 
	pTextureで指定されたメインメモリ上のミップマップテクスチャを上書き読み込みします。

Arguments:
	I/O	pSurfaceDesc	kmCreateTextureSurfaceで確保したテクスチャサーフェスです。
						＜Reload先です＞

	IN	pTexture		メインメモリ上のミップマップテクスチャを示すポインタです。
						＜Reload元です＞

	IN	nMipmapCount	読み込むミップマップテクスチャのレベルを指定します。
						下記のいずれかを指定することができます。

						nMipmapCount			Texture Size
						KM_MAPSIZE_1x1				   1x1
						KM_MAPSIZE_2x2				   2x2
						KM_MAPSIZE_4x4				   4x4
						KM_MAPSIZE_8x8				   8x8
						KM_MAPSIZE_16x16			  16x16
						KM_MAPSIZE_32x32			  32x32
						KM_MAPSIZE_64x64			  64x64
						KM_MAPSIZE_128x128			 128x128
						KM_MAPSIZE_256x256			 256x256
						KM_MAPSIZE_512x512			 512x512
						KM_MAPSIZE_1024x1024		1024x1024
	

Result:
	KMSTATUS_SUCCESS			成功
	KMSTATUS_INVALID_PARAMETER	パラメータが不正
	KMSTATUS_INVALID_TEXTURE	無効なテクスチャを指定した

--*/
KMSTATUS KMAPI
kmReLoadMipmap(
		PKMSURFACEDESC 	pSurfaceDesc,
		PKMVOID		 	pTexture,
		KMINT32	 		nMipmapCount
	);

/*++
kmGetTexture

proto type:

	KMSTATUS
	kmGetTexture(
		PKMDWORD		pTexture,
		PKMSURFACEDESC	pSurfaceDesc
		);

Description:
	pSurfaceDescで指定したテクスチャメモリ上のテクスチャを、 
	pTextureで指定されたメインメモリ上に読み出します。
	出力には、KAMUIテクスチャフォーマットに従った16バイトのヘッダが付加されます。

Arguments:
	OUT	pTexture		メインメモリ上のテクスチャを保存する領域を示すポインタです。
	IN	pSurfaceDesc	テクスチャが保存されているテクスチャサーフェスです。

Result:
	KMSTATUS_SUCCESS					読み出し成功
	KMSTATUS_INVALID_ADDRESS			指定されたテクスチャサーフェスは
										確保されていない

--*/

KMSTATUS KMAPI
kmGetTexture(
		PKMDWORD		pTexture,
		PKMSURFACEDESC	pSurfaceDesc
	);


/*++
kmGarbageCollectTexture

proto type:
	KMSTATUS
	kmGarbageCollectTexture(VOID);

Description:
	 Frame Buffer メモリのガベージコレクションを行います。

Arguments:

Result:
	KMSTATUS_SUCCESS					ガベージコレクション成功
	KMSTATUS_NOT_ENOUGH_MEMORY			メモリ容量が足りない

--*/

KMSTATUS KMAPI
kmGarbageCollectTexture(KMVOID);



/*++
kmSetFramebufferTexture

proto type:
	KMSTATUS 
	kmSetFramebufferTexture(
		IN	OUT	PKMSURFACEDESC	pSurfaceDesc
	);

Description:
	 Frame Buffer の情報を、テクスチャ用のSurfaceDescに読み出します。
	直前の Frame Buffer の内容が、Stride-Rectanglar-TextureとしてSurfaceDescに設定されます。
	このSurfaceDescを使用してテクスチャマップすることにより、直前の Frame Buffer 
	の内容をテクスチャとして使用することができます。
	テクスチャとして使用しようとする Frame Buffer は、下記の設定である必要があります。

	・kmSetDisplayModeで、BitDepth が下記のいづれかが指定されていること。
		KM_DSPBPP_ARGB1555
		KM_DSPBPP_RGB565

Arguments:
	IN	OUT	pSurfaceDesc	KMSURFACEDESC 構造体の領域を示すポインタです。
							 Frame Buffer の情報が返されます。

Result:
		KMSTATUS_SUCCESS				 Frame Buffer の情報読み出し成功
		KMSTATUS_INVALID_TEXTURE_TYPE	現在の Frame Buffer の設定はテクスチャとして使用できない

Sample:
	kmSetStrideWidth(nScreenWidth);					// Set screen width
	....(Setup vertex)....
	kmRender();										// Generate previous Frame-Buffer
	kmSetFramebufferTexture(pSurfaceDesc);			// Get Framebuffer information
	VertexContext.pTextureSurfaceDesc	= &pSurfaceDesc;
	kmProcessVertexRenderState(&VertexContext);		// Use Framebuffer as texture
	kmSetVertexRenderState(&VertexContext);			// Set new vertex-context
	....(Setup vertex)....
	kmRender();										// Render!!

--*/

KMSTATUS KMAPI
kmSetFramebufferTexture(
		PKMSURFACEDESC	pSurfaceDesc
	);


/*++
kmLoadVQCodebook

proto type:

	KMSTATUS
	kmLoadVQCodebook(
			PKMSURFACEDESC pSurfaceDesc,
			PKMDWORD pTexture
		);

Description:
	pSurfaceDescで指定されたVQテクスチャサーフェスに、pTextureで指定された
	メインメモリ上のVQテクスチャのコードブック部分のみを読み込みます。
	ロード済みのVQテクスチャの、コードブック(800hバイト)のみを書き換えて
	カラーパレット効果を出したいときに使用します。

Arguments:
	OUT	pSurfaceDesc	kmCreateTextureSurfaceで確保したテクスチャサーフェスです。
						このサーフェスのカテゴリは、KM_TEXTURE_VQまたはKM_TEXTURE_VQ_MM
						のどちらかでなければなりません。
	IN	pTexture		メインメモリ上のテクスチャ(コードブック)を示すポインタです。
						これは完全なVQテクスチャである必要はありませんが、
						少なくとも先頭に800hバイトのコードブックを含んでいる必要があります。

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した

--*/

KMSTATUS KMAPI
kmLoadVQCodebook(
		PKMSURFACEDESC	pSurfaceDesc,
		PKMDWORD		pTexture
	);


/*++
kmCreateContiguousTextureSurface

proto type:
	KMSTATUS 
	kmCreateContiguousTextureSurface(
			PKMSURFACEDESC	pSurfaceDesc,
			KMINT32			nTexture,
			KMINT32			nWidth, 
			KMINT32			nHeight, 
			KMTEXTURETYPE	nTextureType
		);

Description:
	 Frame Buffer の連続したアドレス領域に、同時に複数のテクスチャ用Surfaceを確保します。
	これは、Tile AcceleratorのYUVコンバータを使用して、連続して複数のYUV422形式のテクスチャ
	を読み込むときに使用します。(kmLoadYUVTextureをご参照ください)
	YUV422以外のテクスチャ用Surfaceの確保にも使用することができます。

	なお、ARC1では、このAPIでVQ,VQ-mipmap,Twiddled-mipmapのサーフェスを確保する
	ことはできません。

Arguments:
	IN	OUT	pSurfaceDesc		KMSURFACEDESC 構造体の配列へのポインタです。
								確保した Texture Surface の情報が返されます。
								KMSTATUS にKMSTATUS_NOT_ENOUGH_MEMORYが返された場合には不定となります。

	IN	nTexture				連続して確保する Texture Surface の数を指定します。

	IN	nWidth, nHeight			テクスチャの横サイズ、縦サイズを指定します。
								nWidth, nHeightに指定する値は,8, 16, 32, 64, 128, 256, 512, 1024の
								いずれかでなければなりません。

	IN	nTextureType			テクスチャの形式を指定します。
								以下のカテゴリコード、ピクセルフォーマットコードから
								一つずつ選び出し、論理和(or)して与えます

								-カテゴリコード
									KM_TEXTURE_TWIDDLED				// Twiddledフォーマット
									KM_TEXTURE_TWIDDLED_MM			// TwiddledフォーマットMipmapあり(ARC1では指定できません)
									KM_TEXTURE_TWIDDLED_RECTANGLE	// Twiddled-Rectangle			(ARC1では使用できません)
									KM_TEXTURE_VQ					// VQ圧縮フォーマット			(ARC1では指定できません)
									KM_TEXTURE_VQ_MM				// VQ圧縮フォーマットMipmapあり	(ARC1では指定できません)
									KM_TEXTURE_PALETTIZE4			// 4bppパレット形式				(ARC1では使用できません)
									KM_TEXTURE_PALETTIZE4_MM		// 4bppパレット形式Mipmapあり	(ARC1では使用できません)
									KM_TEXTURE_PALETTIZE8			// 8bppパレット形式				(ARC1では使用できません)
									KM_TEXTURE_PALETTIZE8_MM		// 8bppパレット形式Mipmapあり	(ARC1では使用できません)
									KM_TEXTURE_RECTANGLE			// 矩形
									KM_TEXTURE_STRIDE				// 矩形(ストライド指定有り)

								- ピクセルフォーマットコード
									KM_TEXTURE_ARGB1555
									KM_TEXTURE_RGB565
									KM_TEXTURE_ARGB4444
									KM_TEXTURE_YUV422				//	(ARC1では使用できません)
									KM_TEXTURE_BUMP					//	(ARC1では使用できません)

Result:
		KMSTATUS_SUCCESS					テクスチャメモリ確保成功
		KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した
		KMSTATUS_NOT_ENOUGH_MEMORY			メモリ容量が足りない

--*/

KMSTATUS KMAPI
kmCreateContiguousTextureSurface(
	IN	OUT	PKMSURFACEDESC	pSurfaceDesc,
	IN		KMINT32			nTexture,
	IN		KMINT32			nWidth,
	IN		KMINT32			nHeight,
	IN		KMTEXTURETYPE	nTextureType
	);


/*++
kmLoadYUVTexture

proto type:

	KMSTATUS
	kmLoadYUVTexture(
		PPKMSURFACEDESC	ppSurfaceDesc,
		PKMDWORD		pTexture,
		KMINT32			nWidth,
		KMINT32			nHeight,
		KMINT32			nFormat,
		KMBOOLEAN		bWaitEndOfDMA
		);

Description:
	kmCreateTextureSurface/kmCreateCombinedTextureSurface/kmCreateContiguousTextureSurface
	で確保したテクスチャメモリ上の領域に、pTextureで指定されたメインメモリ上の
	YUV420-data/YUV422-dataを、Non-Twiddled YUV422テクスチャに変換して読み込みます。
	CLX1の Tile Accelerator に内蔵されている YUV-data converter を使用して、変換しながら
	読み込みます。

	YUV-data converter の出力は Non-Twiddled であるため、このAPIで指定する読み込み先の
	Texture Surface の形式は、以下のいずれかでなければなりません。
		KM_TEXTURE_RECTANGLE | KM_TEXTURE_YUV422	// 矩形
		KM_TEXTURE_STRIDE    | KM_TEXTURE_YUV422	// 矩形(ストライド指定有り)

	複数のYUV-dataを、一度に連続して読み込む場合(nTexture > 1 の時)は、個々のテクスチャの
	サイズは16x16テクセルでなければなりません。このAPIで指定する読み込み先のTexture Surface
	のサイズ指定にご注意ください。
	また、この場合は、読み込み先のTexture Surfaceは Frame Buffer 上で連続したアドレスに
	確保されていることが必要です。"kmCreateContiguousTextureSurface" APIを使用して
	確保したTexture Surfaceを指定してください。

Arguments:
	IN	ppSurfaceDesc	アロケート済みのテクスチャサーフェスを示すKMSURFACEDESC 構造体への
						ポインタ配列のポインタです。
	IN	pTexture		メインメモリ上のYUV420-data/YUV422-dataを示すポインタです。
	IN	nWidth,
	IN	nHeight,
						連続して読み込む Texture マクロブロックの数を縦と横の数で指定します。
						指定できる値は、それぞれ 1 から 64 のいずれかです。
	IN	nFormat			読み込むデータの形式を指定します。以下のいずれかを指定します。
						KM_TEXTURE_YUV420		入力データがYUV420-dataであることを示します。
						KM_TEXTURE_YUV422		入力データがYUV422-dataであることを示します。
	IN	bWaitEndOfDMA	TRUEの場合、YUVコンバータへのデータ転送のDMAが完了するのを待ちます。
									DMAが終了するまで、本APIは終了しません。
						FALSEの場合、DMAの終了を待ちません。DMAの終了を検知したい場合は
									コールバック"kmSetEndOfYUVCallback"を使用してください。

Result:
	KMSTATUS_SUCCESS					読み込み成功
	KMSTATUS_INVALID_TEXTURE_TYPE		無効なテクスチャタイプを指定した

--*/

KMSTATUS KMAPI
kmLoadYUVTexture(
		PKMSURFACEDESC	pSurfaceDesc,
		PKMDWORD		pTexture,
		KMINT32			nWidth,
		KMINT32			nHeight,
		KMINT32			nFormat,
		KMBOOLEAN		bWaitEndOfDMA
		);


/*++
kmChangeLatencyModel
proto type:
	KMSTATUS
	kmChangeLatencyModel(
			KMLATENCYMODEL LatencyMode,
			KMVERTEXBUFMODEL VbufModel
		);

Description:
	現在のレイテンシモデルを変更します。

Arguments:
 IN  LatencyMode  Latency Modelを指定します
     KM_LATENCYMODEL_3V  3V Latency Modelを指定します
     KM_LATENCYMODEL_2V  2V Latency Modelを指定します

 IN VbufModel     VertexBufferをどのように使用するかを指定します
	KM_VERTEXBUFMODEL_NORMAL 					3V Latencyの場合には常にこれを指定してください
	KM_VERTEXBUFMODEL_FLUSH_OPAQUE			  	2V LatencyでOpaqueのリストをFlushする場合に指定します
	KM_VERTEXBUFMODEL_FLUSH_OPAQUE_MODIFIER  	2V LatencyでOpaque ModifierのリストをFlushする場合に指定します
	KM_VERTEXBUFMODEL_FLUSH_TRANS  				2V LatencyでTranslucentのリストをFlushする場合に指定します
	KM_VERTEXBUFMODEL_FLUSH_TRANS_MODIFIER  	2V LatencyでTranslucnet ModifierのリストをFlushする場合に指定します
	KM_VERTEXBUFMODEL_FLUSH_PUNCH_THROUGH 	 	2V LatencyでPunch ThroughのリストをFlushする場合に指定します
	KM_VERTEXBUFMODEL_NOBUF_OPAQUE  			2V LatencyでOpaqueのリストをTA直接転送する場合に指定します
	KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER  	2V LatencyでOpaque ModifierのリストをTA直接転送する場合に指定します
	KM_VERTEXBUFMODEL_NOBUF_TRANS  				2V LatencyでTranslucentのリストをTA直接転送する場合に指定します
	KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER  	2V LatencyでTranslucnet ModifierのリストをTA直接転送する場合に指定します
	KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH 	 	2V LatencyでPunch ThroughのリストをTA直接転送する場合に指定します

Result:

--*/
KMSTATUS KMAPI
kmChangeLatencyModel(
		KMLATENCYMODEL LatencyModel, 
		KMVERTEXBUFMODEL VbufModel
		);

/*++
kmQueryFinishFlushVertexBuffer

proto type:
	KMSTATUS
	kmQueryFinishFlushVertexBuffer(KMVOID);

Description:
	前回のkmFlushVertexBuffer()による指定ListTypeのDMA転送が終了したかどうかを
	チェックします。終了している場合にはKMSTATUS_SUCCESSを返します。
	KMSTATUS_LAST_LIST_UNDER_DMAを返した場合には、前回のFlushがDMA中である事を
	示します。前回のDMAが終了するまで待つ場合には、
	while( kmQueryFinishFlushVertexBuffer() == KMSTATUS_LAST_LIST_UNDER_DMA);
	の様に使用します。

Arguments:
	なし
Result:
	KMSTATUS_SUCCESS				前回のDMAは終了している
	KMSTATUS_LAST_LIST_UNDER_DMA	前回のDMAは終了していない

--*/
KMSTATUS KMAPI
kmQueryFinishFlushVertexBuffer(KMVOID);



/*++
kmQueryFinishLastTextureDMA

proto type:
	KMSTATUS
	kmQueryFinishLastTextureDMA(KMVOID);

Description:
	前回のLoadTextureによるDMAが終了したかどうかをチェックします。
	kmSetSystemConfigurationでflagにKM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMAが
	場合にのみ有効です。それ以外の場合には、常にKMSTATUS_SUCCESSを返します。

Arguments:
	なし
Result:
	KMSTATUS_SUCCESS				前回のDMAは終了している
	KMSTATUS_NOT_FINISH_DMA			前回のDMAは終了していない

--*/
KMSTATUS KMAPI
kmQueryFinishLastTextureDMA(KMVOID);



#ifdef __cplusplus
}
#endif 

#endif
