/*
 *  Shinobi Library
 *  Copyright (c) 1998 SEGA
 *
 *  Library : k_cfg
 *  Module  :
 *  File    : kcfg.h
 *  Date    : 1998-07-11
 *  Author  : T.S. (SEGA, SYSTEM_R&D)
 *  Version : 0.50
 *
 */


#ifndef _KCFG_H_

#define _KCFG_H_


/***** macros *****/

#define KBD_CFGINI    (0x00)

#define KBD_CFG_OK   ( 0)
#define KBD_CFG_ERR  (-1)

/***** structures *****/


/***** prototype *****/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Sint32 kbCfgIni(Sint32 permit);
Sint32 kbCfgGetIcon(Sint32 number, void *pbuf);
Sint8 *kbCfgGetId(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _KCFG_H_ */

/* eof */