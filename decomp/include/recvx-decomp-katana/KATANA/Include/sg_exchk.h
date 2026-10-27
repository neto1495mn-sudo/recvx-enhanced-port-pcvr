/*
 *  Shinobi Library
 *  Copyright (c) 1998, 1999, 2000 SEGA Enterprises
 *  
 *  EXTENTION Check Functuion
 *
 *  Module  : Library Header File
 *  File    : sg_exchk.h
 *  Create  : 2000-06-02
 *  Modify  : 2000-06-07
 *  Version : 2.01
 *  Note    :
 */

#ifndef _SG_SYEXTCHK_H_
#define _SG_SYEXTCHK_H_

/* Initialize */
#define SYD_EXT_CHK_INIT_OK                 (0)
#define SYD_EXT_CHK_INIT_NG                 (-1)

/* Exit */
#define SYD_EXT_CHK_EXIT_OK                 (0)
#define SYD_EXT_CHK_EXIT_NG                 (-1)

/* Peripheral */
#define SYD_EXT_CHK_TYPE_MODEM              (1)
#define SYD_EXT_CHK_TYPE_MMBZIP             (2)
#define SYD_EXT_CHK_TYPE_MMBUSB             (3)
#define SYD_EXT_CHK_TYPE_LAN                (4)

/* Status */
#define SYD_EXT_CHK_CONNECT_OK              (0)
#define SYD_EXT_CHK_CONNECT_NG              (-1)
#define SYD_EXT_CHK_CONNECT_ERROR           (-2)

/* Modem country */
#define SYD_EXT_CHK_MODEM_COUNTRY_USA       (1)
#define SYD_EXT_CHK_MODEM_COUNTRY_EU        (2)
#define SYD_EXT_CHK_MODEM_COUNTRY_JAPAN     (3)
#define SYD_EXT_CHK_MODEM_COUNTRY_AUSTRALIA (4)
#define SYD_EXT_CHK_MODEM_COUNTRY_UNKNOWN   (-1)
#define SYD_EXT_CHK_MODEM_COUNTRY_ERROR     (-2)

/* Modem speed */
#define SYD_EXT_CHK_MODEM_SPEED_33600       (1)
#define SYD_EXT_CHK_MODEM_SPEED_56K         (2)
#define SYD_EXT_CHK_MODEM_SPEED_UNKNOWN     (-1)
#define SYD_EXT_CHK_MODEM_SPEED_ERROR       (-2)


/* Function prototype */
Sint32 syExtChkConnect( Uint32 );
Sint32 syExtChkExit( void );
Sint32 syExtChkInit( void );
Sint32 syExtChkModemCountry( void );
Sint32 syExtChkModemSpeed( void );


#endif
