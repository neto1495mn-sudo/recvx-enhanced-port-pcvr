#ifndef	_ADXT_H_INCLUDED
#define	_ADXT_H_INCLUDED
/****************************************************************************/
/*																			*/
/*			$title$ ＡＤＸデータ再生 ライブラリ								*/
/*					ADXI (ADX IOP) Library									*/
/*																			*/
/*				2000.11.20		written S.Hosaka							*/
/*																			*/
/****************************************************************************/

/*	Version number of ADXT library 		*/
#define ADXI_VER_MAJOR	(8)
#define ADXI_VER_MINOR	(22)
#define	ADXI_VER	"8.22"

#include "cri_xpt.h"

/****************************************************************************/
/*		定数マクロ															*/
/*		MACRO CONSTANT														*/
/****************************************************************************/

/****************************************************************************/
/*		データ型															*/
/*      Data type declaration												*/
/****************************************************************************/

/****************************************************************************/
/*		関数の宣言															*/
/*      Function Declaration												*/
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* $func$ サウンドのセットアップ
 * [書　式] void ADXI_SetupSnd(Sint32 core_no);
 * [入　力] core_no:CORE番号
 * [出　力] なし
 * [関数値] なし
 * [機　能] サウンドのセットアップする。
 *  Setup of sound system
 * [Inputs  ] core_no:CORE No.
 * [Outputs ] None
 * [Return  ] None
 * [Function] Setups sound system.
 */
void ADXI_SetupSnd(Sint32 core_no);

/* $func$ IOPモジュール側のADXライブラリの初期化
 * [書　式] void ADXI_Init(void);
 * [入　力] なし
 * [出　力] なし
 * [関数値] なし
 * [機　能] IOPモジュール側のADXライブラリを初期化する。
 *  Initalization ADX library for IOP module
 * [Inputs  ] None
 * [Outputs ] None
 * [Return  ] None
 * [Function] Initalizes ADX library for IOP module.
 */
void ADXI_Init(void);

/*	$func$ サーバ関数　（内部状態の更新)
 * [書　式] void ADXI_ExecServer(void);
 * [入　力] なし
 * [出　力] なし
 * [関数値] なし
 * [機　能] ライブラリの内部状態を更新する。
 *			V-Sync 毎に呼び出さなければならない。
 *  Server function
 * [Inputs  ] None
 * [Outputs ] None
 * [Return  ] None
 * [Function] Update the inside status of library.
 */
void ADXI_ExecServer(void);


#ifdef __cplusplus
}
#endif

#endif

/* end of file */
