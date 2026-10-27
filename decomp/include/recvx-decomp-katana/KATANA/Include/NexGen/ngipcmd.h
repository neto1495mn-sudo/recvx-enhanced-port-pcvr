/*****************************************************************************
 * $Id: ngipcmd.h,v 1.1 2000/09/05 19:04:16 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Network shell commands
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 *	work and the use of a copyright notice does not imply otherwise.
 *	This source code contains confidential, trade secret material of
 *	NexGen Software. Any attempt or participation in deciphering, decoding,
 *	reverse engineering or in any way altering the source code is
 *	strictly prohibited, unless the prior written consent of
 *	NexGen Software is obtained.
 *
 *   	This software is  supplied  under  the terms of a
 *   	license agreement or nondisclosure agreement with
 *   	NexGen Software, and may not be copied or disclosed
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 05/09/2000 - Regis Feneon
 *****************************************************************************/

#include <ngshell.h>

NG_BEGIN_DECLS

/* network commands */
extern const NGshellcmd ngShellCmd_netstat;
extern const NGshellcmd ngShellCmd_ifconfig;
extern const NGshellcmd ngShellCmd_route;
extern const NGshellcmd ngShellCmd_arp;
extern const NGshellcmd ngShellCmd_ping;

NG_END_DECLS

