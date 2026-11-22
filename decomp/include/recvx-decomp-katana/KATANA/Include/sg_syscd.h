/********************************************************************/
/*                                                                  */
/*  Shinobi Scheduler Library Header(scheduler.h)                   */
/*                                                                  */
/*  98/02/13 r0.00                                                  */
/*  98/03/06 r0.01:update all                                       */
/*  98/05/07 r0.02:(1)新コーディング基準に対応                      */
/********************************************************************/

#ifndef _SG_SYSCD_H_
#define _SG_SYSCD_H_

/********************************************************************/
/*  インクルードファイル                                            */
/********************************************************************/
#include <machine.h>                        /*                      */
#include <sg_xpt.h>                         /* 共通ヘッダ           */
#include <sg_int.h>                         /*                      */
#include "sg_syfbr.h"                       /* ファイバヘッダ       */

/********************************************************************/
/*  定数定義                                                        */
/********************************************************************/
#define SYD_SCD_CPUMAX (0xFFFFFFFF)      /* CPU割り当て時間最大値 */

#define SYD_SCD_PROC_NONE (0)            /* null     */
#define SYD_SCD_PROC_EXEC (1)            /* 実行可能 */
#define SYD_SCD_PROC_EXIT (2)            /* exit     */

#define SYD_SCD_SLEEPFOREVER   ((Uint32)-1) /* 起こされるまでスリープ*/

#define SYD_SCD_OK   (0)
#define SYD_SCD_NG   (-1)
#define SYD_SCD_NULL (NULL)
#define SYD_SCD_MINFIBER (1)

/********************************************************************/
/*  型定義                                                          */
/********************************************************************/
/* 関数登録情報         */
typedef struct SyScdS_FuncAttr_tag{
    Uint32 lOrder  ;                        /* 登録順序  0:最後に追加 / 1～:指定した位置に追加 */
    void*  lParam   ;                       /* ファイバ起動時に渡すパラメータ  */
    void*  pStack   ;                       /* ファイバが使用するスタックエリア*/
    Uint32 lStackSize;                      /* ファイバのスタックサイズ        */
    Uint32 lTime    ;                       /* CPU最大割り当て時間             */
}SYS_SCD_FUNCATTR ;                           /* 0  :無限大:自らCPU放棄するまで  */
                                            /* 1～SyScdYCPUTIME_MAX:100u秒単位 */
/* ファイバ情報         */
typedef struct SyScdS_FiberObj_tag{
    SYFBR   hFiber ;                        /* スケジューリングするファイバのハンドル */
    Uint32  lTime  ;                        /* CPU最大割り当て時間             */
    Uint32  lStat  ;                        /* ファイバの状態(実行中,実行可能,...)*/
    Uint32  lExec  ;                        /* スリープカウント（あと何回待つか）*/
}SYS_SCD_FIBEROBJ;

/* スケジューラテーブル */
typedef struct SyScdTblObj_tag{
/*  Bool    bUsed     ;                   */ /* 使用中フラグ(0:未使用 1:使用中)  */
    Sint32  sFiberMax ;                     /* ファイバ登録最大数               */
    Sint32  sFiberNum ;                     /* ファイバ登録数                   */
    Sint32  sCurrent  ;                     /* 実行中のファイバ番号             */
    SYFBR   hFiberMain ;                    /* スケジューラのファイバ           */
    SYS_SCD_FIBEROBJ fibers[1];             /* 登録されたファイバ情報 */
} SYS_SCD_TBLOBJ;

/* ハンドルの定義       */
typedef SYS_SCD_TBLOBJ* SYSCD ;                /* スケジューラハンドル */

/********************************************************************/
/*  関数マクロ  　　                                                */
/********************************************************************/
                                    /* x個のファイバを動作させるためのバッファ数*/
#define SYD_SCD_BUFSIZE(x)   (sizeof(SYS_SCD_TBLOBJ)+ \
                              ((x)-1) * sizeof(SYS_SCD_FIBEROBJ))


/********************************************************************/
/*  関数定義                                                        */
/********************************************************************/
#ifdef __cplusplus
extern "C"{                                 /* for C++ Interface    */
#endif  /* __cplusplus */

extern  Sint32      syScdInit   ( void ) ;
extern  void        syScdFinish ( void ) ;
extern  SYSCD       syScdCreate ( void*, Sint32 ) ;
extern  Sint32      syScdDelete ( SYSCD ) ; 

extern  Sint32      syScdAddFunc( SYSCD, void (*func)(void*),SYS_SCD_FUNCATTR*, SYFBR* ) ;
extern  Sint32      syScdSleep  ( SYSCD, Uint32) ; 
extern  Sint32      syScdWakeupFiber( SYSCD, SYFBR) ; 
extern  Sint32      syScdExit   ( SYSCD ) ;
extern  Sint32      syScdExecyte( SYSCD ) ;
extern  void        syScdTerminateFiber( SYSCD ) ;

extern  void        syScdSaveContext( void ) ;
extern  void        syScdJumpContext( void ) ;

#ifdef __cplusplus
}                                           /* for C++ Interface    */
#endif /* __cpulspuls  */

#endif /* __SG_SYSCD_H__ */
/********************************************* End of sg_syscd.h *******/
