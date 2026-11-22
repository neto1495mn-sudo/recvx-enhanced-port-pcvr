/*++

  COPYRIGHT        (C) NEC        Corporation 1997

  All rights reserved by NEC Corporation. This program must be
  used solely for the purpose for which it was furnished by NEC
  Corporation. No part of this program may be reproduced or
  disclosed to others, in any form, without the prior written
  permission of NEC Corporation.
  Use of copyright notice does not evidence publication of this
  program.

  Author	:  		T.Hirata(NEC Software Ltd.,)
  Module Name:		kmdefs.h

  Abstract:
	Kamui is Low-Level PowerVR Driver.
	'kmdefs.h' is kamui defines header.

  Environment:
	Microsoft Compiler.

  Notes:
	Kamui's enum , define and structure.

  Revision History:
	??.??.??	Initial Version
	?? ?? ??	Add vertex format by T.Kamei.
	?? ?? ??	BOOL -> BOOLEAN
				prefix chenge 	BOOL	f	->	b
								FLOAT	e	->	f

--*/
#if !defined(__kamui_defines_h)
#define __kamui_defines_h

#include "kmenum.h"
#include "kmstruct.h"
#include "kmvertex.h"

/*
 * Alias
 */
typedef KMVERTEX0	 KMVERTEX_00,*PKMVERTEX_00;
typedef KMVERTEX1	 KMVERTEX_01,*PKMVERTEX_01;
typedef KMVERTEX2	 KMVERTEX_02,*PKMVERTEX_02;
typedef KMVERTEX3	 KMVERTEX_03,*PKMVERTEX_03;
typedef KMVERTEX4	 KMVERTEX_04,*PKMVERTEX_04;
typedef KMVERTEX5	 KMVERTEX_05,*PKMVERTEX_05;
typedef KMVERTEX6	 KMVERTEX_06,*PKMVERTEX_06;
typedef KMVERTEX7	 KMVERTEX_07,*PKMVERTEX_07;
typedef KMVERTEX8	 KMVERTEX_08,*PKMVERTEX_08;
typedef KMVERTEX9	 KMVERTEX_09,*PKMVERTEX_09;
typedef KMVERTEX10	 KMVERTEX_10,*PKMVERTEX_10;
typedef KMVERTEX11	 KMVERTEX_11,*PKMVERTEX_11;
typedef KMVERTEX12	 KMVERTEX_12,*PKMVERTEX_12;
typedef KMVERTEX13	 KMVERTEX_13,*PKMVERTEX_13;
typedef KMVERTEX14	 KMVERTEX_14,*PKMVERTEX_14;
typedef KMVERTEX15	 KMVERTEX_15,*PKMVERTEX_15;
typedef KMVERTEX16	 KMVERTEX_16,*PKMVERTEX_16;
typedef KMVERTEX17	 KMVERTEX_17,*PKMVERTEX_17;


#define KM_CULLACW	KM_CULLCCW
#endif /*  __kamui_defines_h  */
