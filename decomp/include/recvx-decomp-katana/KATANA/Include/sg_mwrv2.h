/*
 *  Middleware Library
 *  Copyright (c) 1998 CRI,SEGA
 *  
 *      Video Render
 *
 *  Module  : Library Header File
 *  File    : sg_mwrnv.h
 *  Create  : 1999-04-01
 *  Modify  : 1998-04-01
 *  Note    : M.O
 */

#ifndef _SG_MWRNV_H_
#define _SG_MWRNV_H_

/*	Version No.	*/
#define	MWD_RNV_VER		"2.10"

/***** インクルード *****/
#include	<sg_xpt.h>
#include	"sj.h"
#include	"sg_mwmvf.h"

/***** 定数マクロ *****/

/*	テクスチャフレームのサイズ	*/
#define	MWD_RNV2_SIZE_OF_TXFRM	(16)	/*	sizeof(MWS_RNV_TXFRM)		*/

/*	テクスチャフォーマット	*/
#define	MWD_RNV2_TFMT_YUV420		(0)
#define	MWD_RNV2_TFMT_YUV422		(1)
#define	MWD_RNV2_TFMT_PLT8			(2)

/*	合成モード	*/
#define MWD_RNV2_COMPO_OPEQ			(0)	/*	不透明							*/
#define MWD_RNV2_COMPO_ADD			(1)	/*	加算合成						*/
#define MWD_RNV2_COMPO_ALPH			(2)	/*	アルファ合成					*/
#define MWD_RNV2_COMPO_INVALPH		(3)	/*	インバースアルファ合成			*/
#define MWD_RNV2_COMPO_DSTALPH		(4)	/*	相手側のアルファ合成			*/
#define MWD_RNV2_COMPO_INVDSTALPH	(5)	/*	相手側のインバースアルファ合成	*/

#define	MWD_RNV_DTYPE_AUTO		(0)			/*	自動決定		*/
#define	MWD_RNV_DTYPE_FULL		(1)			/*	フルスクリーン	*/

/*	ピクチャ描画コールバックの返り値	*/
#define MWD_RNV_DRAWCB_OK			(0)
#define MWD_RNV_DRAWCB_WAIT		(1)
#define MWD_RNV_DRAWCB_SKIP		(2)

#define	MWD_DSPPNT_NOMAL		(0)
#define	MWD_DSPPNT_ENDOFSTRIP	(1)

/*	動作状態	*/
/*	ハンドルの状態	*/
/*	ハンドルの状態		*/
/*	Status of Handle	*/
#ifndef MWE_RNV2_STAT_DEFINED
#define MWE_RNV2_STAT_DEFINED
typedef enum {
	MWE_RNV2_STAT_STOP = 0x00,		/*	STOP							*/
	MWE_RNV2_STAT_MN_WAIT = 0x01,	/*	WAIT(manual mode)				*/
	MWE_RNV2_STAT_MN_FLSH = 0x02,	/*	FLUSH(manual mode)				*/
	MWE_RNV2_STAT_MN_EXEC = 0x03,	/*	EXECUTING(manual mode)			*/
	MWE_RNV2_STAT_PEND = 0x04,		/*	PLAY END						*/
	MWE_RNV2_STAT_WAIT = 0x05,		/*	WAIT							*/
	MWE_RNV2_STAT_FLSH = 0x06,		/*	FLUSH							*/
	MWE_RNV2_STAT_EXEC = 0x07		/*	EXECUTING						*/
} MWE_RNV2_STAT;
#endif

/***** データ型の定義 *****/

/* ビデオレンダラハンドル */
#ifndef MWRNV2_DEFINED
#define MWRNV2_DEFINED
typedef void *MWRNV2;
#endif

/*	表示スクリーンポイント	*/
#ifndef MWS_RNV_DSPPNT_DEFINED
#define MWS_RNV_DSPPNT_DEFINED
typedef struct _mws_rnv_dsppnt {
	Uint32	ctrl;					/*	ストリップ制御データ				*/
	float	plx;					/*	表示フレームバッファ X 座標			*/
	float	ply;					/*	表示フレームバッファ Y 座標			*/
	float	plz;					/*	表示フレームバッファ Z 座標			*/
	float	tlx;					/*	テクスチャ U 座標					*/
	float	tly;					/*	テクスチャ V 座標					*/
	Uint32 rgb_base;				/*	ARGB基準値							*/
	Uint32 rgb_ofst;				/*	ARGBオフセット						*/
} MWS_RNV_DSPPNT;
#endif

/*	ハンドル生成のパラメータ構造体			*/
/*	Parameter structure to create handle	*/
#ifndef MWS_RNV2_CPRM_DEFINED
#define MWS_RNV2_CPRM_DEFINED
typedef struct {
	Sint32	tfmt;			/*	テクスチャフォーマット					*/
	Sint32	isx;			/*	横ピクセル数							*/
							/*	The number of sideways pixels		 	*/
	Sint32	isy;			/*	縦ピクセル数							*/
							/*	The number of lengthways pixels			*/
	Sint32	tex_nfrm;		/*	テクスチャフレーム数(V-RAM)				*/
							/*	The number of texture frames			*/
	Sint32	work_nfrm;		/*	ワークフレーム数						*/
							/*	The number of macro block frames		*/
	Sint32	wksize;			/*	作業領域の大きさ						*/
							/*	Size of work area						*/
	Uint8	*wk;			/*	作業領域								*/
							/*	Pointer to work area					*/
} MWS_RNV2_CPRM;
#endif

/***** 関数宣言 *****/

/*=========================================================================*
 *		ライブラリの初期化と終了処理
 *		Initialize and Finalize of Library
 *=========================================================================*/

/*	ライブラリの初期化	*/
void mwRnv2Init(void);

/*	ライブラリの終了処理	*/
void mwRnv2Finish(void);

/*	Vertexバッファの設定	*/
void mwRnv2SetVertexBuffer(void *vtxbuf);

/*	レイテンシーの設定	(nvsync:2|3)	*/
void mwRnv2SetLatency(Sint32 nvsync);

/*=========================================================================*
 *		基本動作の制御
 *		Basic Control
 *=========================================================================*/

/*	ムービーフレームの作業領域サイズの計算	*/
Sint32 mwRnv2CalcMvfWork(Sint32 tfmt, Sint32 sx, Sint32 sy, Sint32 nfrm);

/*	ムービーフレームの作業領域サイズの計算	*/
Sint32 mwRnv2CalcWork(MWS_RNV2_CPRM *cprm);

/*	ハンドルの作成	*/
MWRNV2 mwRnv2Create(SJ sj, MWS_RNV2_CPRM *cprm);

/*	ハンドルの消去	*/
void mwRnv2Destroy(MWRNV2 rnv);

/*	レンダリング開始	*/
void mwRnv2Start(MWRNV2 rnv);

/*	レンダリング停止	*/
void mwRnv2Stop(MWRNV2 rnv);

/*	サーバ関数	*/
void mwRnv2ExecServer(void);

/*	ハンドル状態の取得	*/
MWE_RNV2_STAT mwRnv2GetStat(MWRNV2 rnv);

/*	ピクチャ描画コールバック関数の登録	*/
/*	※コールバック関数の戻り値によりそのフレームを描画するか否かを決める	*/
/*	  MWD_RNV_DRAWCB_OK   :	描画											*/
/*	  MWD_RNV_DRAWCB_WAIT :	一回前のピクチャを表示							*/
/*	  MWD_RNV_DRAWCB_SKIP :	スキップ。次のピクチャあればすぐにコールバック	*/
void mwRnv2EntryDrawCallbackFn(MWRNV2 rnv,
  Sint32 (*fn)(void *obj, Sint32 time, Sint32 tunit, Sint32 dfno), void *obj);

/*	表示コールバック関数の登録	*/
void mwRnv2EntryDispCallbackFn(MWRNV2 rnv,
 void (*fn)(void *obj, Sint32 time, Sint32 tunit, Sint32 dfno), void *obj);

/*	テクスチャフレームの状態取得	*/
void mwRnv2GetNumTxfrm(MWRNV2 rnv, Sint32 *ndsp, Sint32 *nren);

/*	ロード可能なテキスチャ数の取得	*/
Sint32 mwRnv2GetNumRoomTex(MWRNV2 rnv);

/*	合成モードの設定	*/
void mwRnv2SetCompoMode(MWRNV2 rnv, Sint32 mode);

/*	テクスチャロードの実行	*/
void mwRnv2Load(MWRNV2 rnv, Sint8 *src, Sint32 time, Sint32 tunit);

/*	テクスチャの描画	*/
void mwRnv2Draw(MWRNV2 rnv);

/*	表示スイッチの設定	*/
void mwRnv2SetDispSw(MWRNV2 rnv, Sint32 sw);

/*	表示スイッチの取得	*/
Sint32 mwRnv2GetDispSw(MWRNV2 rnv);

/*	表示サーフェスの取得	*/
void *mwRnv2GetCurSrf(MWRNV2 rnv);

/***
*			表示ウィンドウの制御
***/

/*	表示位置やサイズのデフォルト値の設定	*/
/*	INPUT :ssx, ssy, isx, isy, tw, th				*/
/*	OUTPUT:plx, ply, plz, psx, psy, tw, th				*/
void mwRnv2SetWndDefConf(MWRNV2 rnv, long dtype);
/*	表示位置の設定	*/
void mwRnv2SetWndPos(MWRNV2 rnv, float lx, float ly);
/*	表示サイズの設定	*/
void mwRnv2SetWndSize(MWRNV2 rnv, float sx, float sy);
/*	表示位置の奥行き	*/
void mwRnv2SetWndZ(MWRNV2 rnv, float lz);
/*	輝度の設定	*/
void mwRnv2SetWndBright(MWRNV2 rnv, Uint32 val);
/*	輝度の取得	*/
Uint32 mwRnv2GetWndBright(MWRNV2 rnv);
/*	輝度オフセットの設定	*/
void mwRnv2SetWndBrightOfst(MWRNV2 rnv, Uint32 val);
/*	輝度オフセットの取得	*/
Uint32 mwRnv2GetWndBrightOfst(MWRNV2 rnv);

/***
*			表示サーフェスの設定
***/

/*	コントロールコードの設定	*/
void mwRnv2SetSrfCtrl(MWRNV2 rnv, Uint32 no, Uint32 ctrl);
/*	コントロールコードの取得	*/
Uint32 mwRnv2GetSrfCtrl(MWRNV2 rnv, Uint32 no);
/*	サーフェスポイント用バッファサイズの計算	*/
Sint32 mwRnv2CalcSrfBufSize(MWRNV2 rnv, Sint32 npnt);
/*	サーフェスポイント用バッファの設定	*/
void mwRnv2SetSrfPntBuf(MWRNV2 rnv, Sint32 npnt, void *buf, Sint32 bsize);
/*	表示位置の設定	*/
void mwRnv2SetSrfPos(MWRNV2 rnv, Uint32 no, float lx, float ly, float lz);
/*	表示位置の取得	*/
void mwRnv2GetSrfPos(MWRNV2 rnv, Uint32 no, float *lx, float *ly, float *lz);
/*	輝度の設定	*/
void mwRnv2SetSrfBright(MWRNV2 rnv, Uint32 no,
									float a, float r, float g, float b);
/*	輝度の取得	*/
void mwRnv2GetSrfBright(MWRNV2 rnv, Uint32 no,
									float *a, float *r, float *g, float *b);
/*	輝度オフセットの設定	*/
void mwRnv2SetSrfBrightOfst(MWRNV2 rnv, Uint32 no,
									float a, float r, float g, float b);
/*	輝度オフセットの取得	*/
void mwRnv2GetSrfBrightOfst(MWRNV2 rnv, Uint32 no,
									float *a, float *r, float *g, float *b);
/*	イメージ位置の設定	*/
void mwRnv2SetImgPos(MWRNV2 rnv, Uint32 no, float lx, float ly);

/*	イメージ位置の取得	*/
void mwRnv2GetImgPos(MWRNV2 rnv, Uint32 no, float *lx, float *ly);

/*	イメージサイズの設定	*/
void mwRnv2SetImgSize(MWRNV2 rnv, Sint32 isx, Sint32 isy);

/*	イメージサイズの取得	*/
void mwRnv2GetImgSize(MWRNV2 rnv, Sint32 *isx, Sint32 *isy);
#endif /*	_SG_MWRNV_H_	*/
