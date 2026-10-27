/* 
 * File: sg_Cp.h
 * Desc: system library (memory copy)
 * Auth: Jiro Terakwa(SYSTEM R&D)
 * Create: 1998/04/07 Jiro Terakawa: Ver. 0.1
 * Change:
 *
 * Func: syCpInit:       コピーライブラリの初期化
 *       syCpFinish:     コピーライブラリの終了
 *       syCpCreate:     コピーハンドラの生成
 *       syCpCopy:       コピーの実行
 *       syCpGetStat:    コピー状況の取得
 *                       
 * Note:
 *
 */
#ifndef _SG_CP_H_
#define _SG_CP_H_

#define  SYD_CP_MODE_ANY   0 /* デバイス指定なし */
#define  SYD_CP_MODE_DMA1  1 /* DMA1 を使用して転送 */
#define  SYD_CP_MODE_DMA3  3 /* DMA3 を使用して転送 */
#define  SYD_CP_MODE_CPU   4 /* CPU で転送 */
typedef Uint32 SY_CP_MODE;

#define SYD_CP_WIDTH_1BYTE     1  /* CHCR?.TS012 */
#define SYD_CP_WIDTH_2BYTE     2  /* CHCR?.TS012 */
#define SYD_CP_WIDTH_4BYTE     4  /* CHCR?.TS012 */
#define SYD_CP_WIDTH_8BYTE     8  /* CHCR?.TS012 */
#define SYD_CP_WIDTH_32BYTE    32  /* CHCR?.TS012 */
typedef Uint32 SY_CP_WIDTH;

#define  SYD_CP_STAT_DONE  0   /* コピーは正常終了した */
#define  SYD_CP_STAT_ERR   1   /* コピーはエラーにより終了した */
#define  SYD_CP_STAT_ABORT 2   /* コピーはユーザからの指示で中断した */
#define  SYD_CP_STAT_PEND  3   /* コピーは待ち行列に入っている */
#define  SYD_CP_STAT_PROG  4   /* コピーは実行中である */
#define  SYD_CP_STAT_OPEN  5   /* コピーは、ユーザのコンフィグレーション中である */
typedef Uint32 SY_CP_STAT;

struct SYS_CP{
  Void *src;           /* 転送元アドレス   */
  Void *dst;           /* 転送先アドレス   */
  Uint32 size;         /* 転送サイズ       */
  Void (*callback)(Void *);      /* コールバック関数 */
  Void *arg;           /* コールバック関数呼出し時の引数 */
  SY_CP_MODE mode; /* コピーモード     */
  SY_CP_STAT stat; /* 状態             */
  SY_CP_WIDTH width;   /* 転送単位 1, 2, 4, 8, 32 BYTE */
};

/* ハンドル */
typedef struct SYS_CP * SYCP;

EXTERN Void syCpInit(Void);

EXTERN SYCP syCpCreate(Void *src, Void *dst, Void (*callback)(Void *), SY_CP_MODE syCpMode);

/* Void syCpSetSrc(SYCP syCp); */
#define syCpSetSrc(_syCp, _src) \
  ((_syCp)->src = (_src))
/* Void syCpSetDst(SYCP syCp); */
#define syCpSetDst(_syCp, _dst) \
  ((_syCp)->dst = (_dst))
/* Void syCpSetCallback(SYCP syCp); */
#define syCpSetCallback(_syCp, _callback) \
  ((_syCp)->callback = (_callback))
/* Void syCpSetArg(SYCP syCp); */
#define syCpSetArg(_syCp, _arg) \
  ((_syCp)->arg = (_arg))
/* Void syCpSetMode(SYCP syCp); */
#define syCpSetMode(_syCp, _mode) \
  ((_syCp)->mode = (_mode))
/* Void syCpSetSize(SYCP syCp); */
#define syCpSetSize(_syCp, _size) \
  ((_syCp)->size = (_size))

EXTERN Void syCpCopy(SYCP syCp);

EXTERN Void syCpAbort(SYCP syCp);

/* SY_CP_STAT syCpGetStat(SYCP syCp);*/
#define syCpGetStat(_syCp) \
  ((_syCp)->syCpStat)

EXTERN Void syCpDestroy(SYCP syCp);

EXTERN Void syCpFinish(Void);

#endif /* _SG_CP_H_ */
