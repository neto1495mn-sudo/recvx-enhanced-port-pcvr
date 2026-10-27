/*++

  COPYRIGHT        (C) NEC        Corporation 1998 - 1999

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author     :		M.Imanishi(NEC Semiconductor,)
  Module Name:		kmymemory.h

  Abstract:
	Memory Copy And Memset Functions

  Environment:
	Hitachi C - Compiler

  Notes:
	Copy funcions for Memory to Memory.

  Revision History:

--*/

#if !defined(__kmyMemory_h)
#define __kmyMemory_h

#ifdef __cplusplus
extern "C" {
#endif

/*++
kmyMemCopy

proto type:

	KMVOID KMAPI
	kmyMemCopy(
			IN	KMVOID *dst,
			IN	KMVOID *src,
			IN	KMDWORD size
		);

Description:
	メモリーメモリ間コピーを行います。コピーするサイズ、アラインメントを
	チェックしてエラーが起こらないよう転送します。

Arguments:

Result:
	void
--*/
KMVOID KMAPI
kmyMemCopy (
		IN	KMVOID *dst,
		IN	KMVOID *src,
		IN	KMDWORD size
	);

/*++
kmyZeroMemroy

proto type:

	KMVOID KMAPI
	kmiZeroMemory(
			IN	KMVOID *addr,
			IN	KMDWORD size
		);

Description:
	サイズ、アラインメントをチェックしてエラーが起こらないようFillします。

Arguments:

Result:
	void
--*/

KMVOID KMAPI
kmyZeroMemory (
		IN	KMVOID	*addr,
		IN	KMDWORD	size
	);

/*++
kmyPurgeCache

proto type:

	KMVOID KMAPI
	kmyPurgeCache(
			PKMVOID addr
		);

Description:
	addrを含むキャッシュラインのパージを行います。

Arguments:

Result:
	void
--*/
KMVOID KMAPI
kmyPurgeCache (
		IN	PKMVOID addr
	);
#ifdef __cplusplus
}
#endif
#endif