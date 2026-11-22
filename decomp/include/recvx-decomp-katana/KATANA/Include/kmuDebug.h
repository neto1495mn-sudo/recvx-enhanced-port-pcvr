//-----------------------------------------------------------------------------
// Debug Print for Kamui2 Ver 0.01 2000/2/4
// Kamui2を使用したデバッグ文字出力ユーティリティです。
//-----------------------------------------------------------------------------

#ifndef _KMDEBUG_H_
#define _KMDEBUG_H_

#include <stdio.h>
#include <stdarg.h>
#include <machine.h>

//-----------------------------------------------------------------------------

KMVOID kmuInitPrint( PKMVOID pVertexBuffDesc );
KMVOID kmuExitPrint( KMVOID );

KMVOID kmuPrintTexture( PKMCHAR8 pTexture, KMDWORD nSize, KMDWORD nAttr );
KMVOID kmuPrintSize( KMFLOAT nSize );
KMVOID kmuPrintColor( KMDWORD nColor );
KMVOID kmuPrintf( KMINT32 nX, KMINT32 nY, const char *fmt, ... );

//-----------------------------------------------------------------------------
#endif
