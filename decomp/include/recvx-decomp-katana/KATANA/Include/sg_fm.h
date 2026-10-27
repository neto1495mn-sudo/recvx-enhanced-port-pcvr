/*
 *  Shinobi Library
 *  Copyright (c) 1998 SEGA
 *
 *  Library : Flash Memory Manage Library
 *  Module  : Library Interface Header
 *  File    : sg_fm.h
 *  Date    : 1998-11-27
 *  Author  : T.S. (SEGA, SYSTEM_R&D)
 *  Version : 1.02
 *
 */


#ifndef _SG_FM_H_
#define _SG_FM_H_


/***** macros *****/

#define FMD_STAT_COMPLETE  (1)
#define FMD_STAT_ERR       (3)
#define FMD_OK          (  0)

/***** prototype *****/

Sint32 fmMngRead(Sint32 offset, void *buf);
Sint32 fmMngExecServer(void);


#endif /* _SG_FM_H_ */

/* eof */