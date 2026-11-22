/*
 *  Overlay Library
 *  Copyright (c) 1998 SEGA
 *
 *      Overlay
 *
 *  Module  : Library Header File
 *  File    : sg_ov.h
 *  Create  : 1998-02-20
 *  Modify  : 1998-05-12
 *  Version : 0.41
 *  Note    :
 */
#ifndef _SG_OV_H_
#define _SG_OV_H_

#include	<sg_xpt.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*	Universally Unique Identifier	*/
#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef struct _UUID
{
    Uint32	Data1;
    Uint16	Data2;
    Uint16	Data3;
    Uint8	Data4[8];
} UUID;
typedef UUID Uuid;
#endif

/*	ライブラリIDの定義		*/
#define OVD_LID { \
	0xba7e0a00, 0xbda9, 0x11d1, 0x8a, 0xeb, 0x00, 0x60, 0x08, 0x95, 0x62, 0x5f\
}\

/*  ライブラリのバージョン(Ver.0.41)		*/
#define	OVD_VERSION_NO			(0x00)		/*  バージョン番号				*/
#define	OVD_REVISION_NO			(0x41)		/*  リビジョン番号				*/

/*	エラーコード	*/
typedef enum {
	OVE_ERR_OK				= 0,		/*  正常終了					*/
	OVE_ERR_INVALID_FORMAT	= -1,		/*  不正なフォーマット			*/
	OVE_ERR_INVALID_LIB_ID	= -2,		/*  不正なライブラリID			*/
	OVE_ERR_INVALID_LIB_HDL	= -3		/*  不正なライブラリハンドル	*/
} OVE_ERR;


/*	ライブラリハンドル	*/
typedef struct {
	struct _ov_lif	*lif;			/*	ライブラリインタフェース			*/
	Uint8			ver;			/*	バージョン番号						*/
	Uint8			revi;			/*	リビジョン番号						*/
	Sint16			rsv1;			/*	予約 1								*/
	Sint32			rsv2;			/*	予約 2								*/
	Sint32			rsv3;			/*	予約 3								*/
	UUID    		lid;			/*	ライブラリID						*/
} OVS_LIB_OBJ;
typedef OVS_LIB_OBJ *OVLIB;

/*	各ライブラリが必ず作成するライブラリインタフェース関数  				*/
typedef struct _ov_lif {
	void (*QueryInterface)();		/*		for COM compatibility			*/
	void (*AddRef)();				/*		for COM compatibility			*/
	void (*Release)();				/*		for COM compatibility			*/
	void (*Init)(OVLIB ovl);		/*		初期化							*/
	void (*Finish)(OVLIB ovl);		/*		終了処理						*/
	void (*Exec)(OVLIB ovl);		/*		実行							*/
} OVS_LIB_IF;
typedef OVS_LIB_IF *OVLIB_IF;

/***
*		実行モジュールのための関数
***/

/*	実行モジュールハンドル	*/
typedef struct {
	Sint32	rsv[8];
	OVS_LIB_OBJ libobj[1];
} OVS_MDL_OBJ;
typedef OVS_MDL_OBJ *OVMDL;

/***
*			ライブラリ内部のための定義
***/

/*	クリティカルセクション	*/
typedef Sint32 OV_CS;

/*	ライブラリ管理テーブルへのポインタのアドレス	*/
#define	OVD_LIBTBL_ADR_PTR	(0x8C00FFF0)
/*	ライブラリ管理テーブルの最大登録数へのアドレス	*/
#define	OVD_LIBTBL_ADR_MAX	(0x8C00FFF4)
/*	ライブラリ管理テーブルのアドレス	*/
#define	OVD_DEF_LIBTBL_ADR	(0x8C00FC00)

/*	ライブラリ管理テーブルの最大登録数	*/
#define	OVD_DEF_LIBTBL_MAX	(120)

/*	ライブラリ管理テーブル	*/
typedef struct {
	Sint32	nlib;				/* 登録されているライブラリ数				*/
	Sint32	rsv1;				/* 予約１									*/
	struct {
		OVLIB	lib;			/* ライブラリハンドル						*/
		Sint32	ref;			/* 参照カウンタ								*/
	} entry[1];
} OVS_LIBTBL;

/***
*		関数宣言
***/

/*	ライブラリハンドルの取得												*/
OVLIB ovLibOpen(UUID *lid);
/*	ライブラリハンドルの開放												*/
void ovLibClose(UUID *lid);
/*	モジュールファイルの読み込み											*/
OVE_ERR ovMdlEntryTbl(OVMDL ovmdl);
/*	モジュールファイルの開放												*/
OVE_ERR ovMdlDeleteTbl(OVMDL ovmdl);
/*  実行モジュールの実行													*/
Sint32 ovMdlExec(OVMDL ovmdl);

/*  ライブラリインタフェースの取得											*/
OVLIB_IF ovLibGetLibIf(OVLIB lib);
/*  ライブラリのバージョンの取得											*/
void ovLibGetVersion(OVLIB lib, Uint8 *ver, Uint8 *revi);
/*  ライブラリIDの取得														*/
UUID *ovLibGetUuid(OVLIB lib);

/*  ライブラリ管理ライブラリの初期化 (通常使用しない）						*/
void ovInit(void *ovltbl, Sint32 nlib);
/*  ライブラリ管理ライブラリの終了処理 (通常使用しない）					*/
void ovFinish(void);
/*	ライブラリハンドルの登録												*/
OVE_ERR ovLibEntryTbl(OVLIB lib);
/*	ライブラリハンドルの削除												*/
OVE_ERR ovLibDeleteTbl(OVLIB lib);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif     /*  _SG_OV_H_	*/
