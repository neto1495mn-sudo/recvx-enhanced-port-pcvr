/*
 * Network Library
 * Copyright (c) 1999,2000 SEGA
 *
 * Library : Network Utility Library
 * File : nt_utl.h
 *  Date    : 2000.12.12
 * Version : 3.01 (ntUtl)
 *  Version : 4.04 (ntInf)
 * Version : 1.03 (ntLD)
 *
 * For Network SDK
 */

#ifndef _NT_UTL_
#define _NT_UTL_

#ifdef __cplusplus /* Support for C++ */
extern "C" {
#endif /* __cplusplus */

/* for shinobi2 */
#define bool	int
#define true	(1)
#define false	(0)

/*
 * return value
 */
#define NTD_OK                      ( 0)
#define NTD_ERR_NOTINIT             ( -1)
#define NTD_ERR_INVALIDPARAM        ( -2)
#define NTD_ERR_NOINFO              ( -3)
#define NTD_ERR_NODATA              NTD_ERR_NOINFO

/* for flag in ntUtl */
#define NTUTL_JUMP                  0x00000001
#define NTUTL_FIX                   0x00000002
#define NTUTL_UP                    0x00000004
#define NTUTL_UP_OK                 0x00000008
#define NTUTL_UP_DO                 0x00000010
#define NTUTL_DOWN                  0x00000020
#define NTUTL_DOWN_OK               0x00000040
#define NTUTL_DOWN_DO               0x00000080
#define NTUTL_VIRTUALVM             0x00000100
#define NTUTL_VER1                  0x00000000
#define NTUTL_FLAG_INIT             (NTUTL_VER1|NTUTL_DOWN_OK)

/*
 * bit definition for flag in NetworkInfo
 */
#define ISP_USE1                    0x00000000 /* Use ISP 1 information */
#define ISP_USE2                    0x00000001 /* Use ISP 2 information */
#define CHANGE_NUMBER               0x00000000 /* When the line is busy, change the number */
#define FIXED_NUMBER                0x00000002 /* The phone number is fixed. */
/* for modem setting */              
#define DIAL_TONE                   0x00000000 /* Tone dialing */
#define DIAL_PULSE                  0x00000100 /* Pulse dialing */
#define PULSE_US_10                 0x00000000 /* Pulse speed 10pps (US) */
#define PULSE_UK_10                 0x00000200 /* Pulse speed 10pps (UK) */
#define PULSE_US_20                 0x00000400 /* Pulse speed 20pps (US) */
#define PULSE_UK_20                 0x00000600 /* Pulse speed 20pps (UK) */
#define SPEAKER_VOL_L0              0x00000000 /* Speaker volume is low */
#define SPEAKER_VOL_L1              0x00000800 /* Speaker volume is low */
#define SPEAKER_VOL_L2              0x00001000 /* Speaker volume is medium */
#define SPEAKER_VOL_L3              0x00001800 /* Speaker volume is high */
#define SPEAKER_CTL_M0              0x00000000 /* Speaker always OFF */
#define SPEAKER_CTL_M1              0x00002000 /* Speaker ON from start of dialing until receiving carrier */
#define SPEAKER_CTL_M2              0x00004000 /* Speaker always ON */
#define SPEAKER_CTL_M3              0x00006000 /* Speaker OFF from end of dialing until receiving carrier */
#define BLIND_ENABLE                0x00008000 /* Enable Blind call (default: disabled) */

/* flag for NetworkInfo2; The following flags are newly added */
#define USE_LAN                     0x00000004 /* Use LAN information in lan_info */
#define VERSION_2                   0x01000000 /* Version of NetworkInfo */
                                    
/* flag for NetworkAccessInfo */    
#define USE_EXTRA_INFO              0x0001     /* Use NetworkAccessInfo2 additionally */
                                    
/* flag for NetworkAccessInfo2 */   
#define NA2_USE_MAIL1               0x00000001
#define NA2_USE_MAIL2               0x00000002
#define NA2_USE_MAIL3               0x00000004
#define NA2_USE_MAIL4               0x00000008
#define NA2_PROXY_OFF               0x00000010
#define NA2_FIXED_ADDRESS           0x00000020
#define NA2_CID_M0                  0x00000100
#define NA2_CID_M1                  0x00000200
#define NA2_CID_M2                  0x00000400
#define NA2_CID_M3                  0x00000800
#define NA2_CID_M4                  0x00001000
                                    
#define NA2_HEADERCOMP              0x00000000  // Do Use Van Jacobson header compression (default)
#define NA2_NO_HEADERCOMP           0x00000040  // Do not use Van Jacobson header compression
                                    
/* flag for TCPIPInfo */             
#define TCP_USE_DNS                 0x00000001
#define TCP_FIXED_ADDRESS           0x00000002
#define TCP_USE_PROXY               0x00000004
                                    
#define TCP_USE_GATEWAY             0x00000010  // Use the Gateway in FlashRAM, instead of ISP provided Gateway
                                    
#define TCP_DYNAMIC_DHCP            0x00000000  // Perform DHCP Query (default)
#define TCP_DYNAMIC_NO_DHCP         0x00000100  // Do not perform DHCP Query
#define TCP_DYNAMIC_PPPoE           0x00000200  // Perform PPPoE Query
#define TCP_DYNAMIC_NO_PPPoE        0x00000000  // Do not perform PPPoE Query (default)
#define TCP_DYNAMIC_HEADERCOMP      0x00000000  // Use Van Jacobson header compression (default)
#define TCP_DYNAMIC_NO_HEADERCOMP   0x00000400  // Do not use Van Jacobson header compression
#define TCP_DYNAMIC_MASK            0x00000f00  // Mask for dynamic protocols
                                    
#define TCP_LINK_NEGO_AUTO          0x00000000  // Auto negotiate the link (default)
#define TCP_LINK_NEGO_10BT          0x00001000  // Force link to 10BaseT - Half-Duplex
#define TCP_LINK_NEGO_10BTX         0x00002000  // Force link to 10BaseT - Full-Duplex 
#define TCP_LINK_NEGO_100BT         0x00003000  // Force link to 100BaseT - Half-Duplex
#define TCP_LINK_NEGO_100BTX        0x00004000  // Force link to 100BaseT - Full-Duplex
// 0x00000500 to 0x00000f00 : for future expansion
#define TCP_LINK_NEGO_MASK          0x0000f000  // Mask for link type

/* flag for NetworkCommonInfo */
#define NTINF_COMM_SUMMERTIME       0x00000001
#define NTINF_COMM_SUMMERTIME_DEF	  0x00000008	/* GMT+1 flag */
#define NTINF_COMM_EXTERNAL_MODEM   0x00000002
#define NTINF_COMM_SERIAL			      0x00000002
#define NTINF_COMM_SERIAL_B300		  0x00000010
#define NTINF_COMM_SERIAL_B1200		  0x00000020
#define NTINF_COMM_SERIAL_B2400		  0x00000030
#define NTINF_COMM_SERIAL_B4800		  0x00000040
#define NTINF_COMM_SERIAL_B9600		  0x00000050
#define NTINF_COMM_SERIAL_B14400	  0x00000060
#define NTINF_COMM_SERIAL_B19200	  0x00000070
#define NTINF_COMM_SERIAL_B38400	  0x00000080
#define NTINF_COMM_SERIAL_B57600	  0x00000090
#define NTINF_COMM_SERIAL_B115200	  0x00000000	/* default */
#define NTINF_COMM_SERIAL_B230400	  0x000000b0
#define NTINF_COMM_SERIAL_B460800	  0x000000c0
#define NTINF_COMM_SERIAL_BAUDMASK  0x000000f0
#define NTINF_COMM_SERIAL_7BIT		  0x00000004	/* unset is 8bit */
#define NTINF_COMM_SERIAL_PARITY	  0x00000100	/* set parity on */
#define NTINF_COMM_SERIAL_PAR_EVEN  0x00000200	/* set even parity */
#define NTINF_COMM_SERIAL_STOP0		  0x00000400
#define NTINF_COMM_SERIAL_STOP1		  0x00000000	/* default */
#define NTINF_COMM_SERIAL_STOP2		  0x00000c00
#define NTINF_COMM_SERIAL_STOPMASK  0x00000c00
#define NTINF_COMM_SERIAL_NFLOW		  0x00002000	/* set non flow */
#define NTINF_COMM_SERIAL_SOFTFLOW  0x00001000	/* unset is hard flow */
#define NTINF_COMM_DP3              0x80000000
/* mail account */
#define NTINF_COMM_MAIL_AUTO		0x00000000	/* ISP depend (default) */
#define NTINF_COMM_MAIL_ISP1		0x00010000	/* ISP1 (SEGA) */
#define NTINF_COMM_MAIL_SEGA		0x00010000	/* ISP1 (SEGA) */
#define NTINF_COMM_MAIL_ISP2		0x00020000	/* ISP2 (USER) */
#define NTINF_COMM_MAIL_USER		0x00020000	/* ISP2 (USER) */
#define NTINF_COMM_MAIL_LAN			0x00030000	/* LAN */
#define NTINF_COMM_MAIL_PLANET		0x00040000	/* Planetweb */
#define NTINF_COMM_MAIL_MASK		0x00070000	/* mask bit */

/* magic for NetworkAccessInfo, NetworkAccessInfo2, TCPIPInfo
    and NetworkCommonInfo */
#define NETINFO_MAGIC  0x41474553 /* magic number ("SEGA") */

#ifndef __NG_NGSOCKET_H_INCLUDED__
#ifndef USE_NETSDK_ACCESS_MODULE
/*
 * Structures
 */
struct in_addr {
  union {
    struct { unsigned char s_b1, s_b2, s_b3, s_b4; } S_un_b;
    struct { unsigned short s_w1, s_w2; } S_un_w;
    unsigned long S_addr;
  } S_un;
  #define s_addr S_un.S_addr  /* should be used for all code */
};
#else
#include <types.h>
#include <in.h>
#endif
#endif//__NG_NGSOCKET_H_INCLUDED__


typedef struct _NetworkAccessInfo {
  Uint32  magic;  /* magic number, should be 0x41474553("SEGA") */
  char    loginId[28];
  char    loginPasswd[16];
  char    accessPointNumber[3][40];
  struct  in_addr dnsServerAddress1;
  struct  in_addr dnsServerAddress2;
  char    mailAddress[48];
  char    mailServer[32];
  char    popServer[32];
  char    popId[16];
  char    popPasswd[16];
  char    proxyServer[32];
  Uint16  proxyPortNum;
  Uint16  flag;
} NetworkAccessInfo;

typedef struct _MailAccount {
  char  mailAddress[48];
  char  mailServer[32];
  char  popServer[32];
  char  popId[16];
  char  popPasswd[16];
} MailAccount;

typedef struct _NetworkAccessInfo2 {
  Uint32      magic;
  Uint32      flag;
  struct      in_addr ipAddr;
  char        parentalPasswd[12];
  MailAccount mailAccount[4];
} NetworkAccessInfo2;

typedef struct _TCPIPInfo {
  Uint32          magic;
  Uint32          flag;
  struct in_addr  ipAddr;
  struct in_addr  subnetMask;
  struct in_addr  broadcast;
  struct in_addr  dnsServerAddress1;
  struct in_addr  dnsServerAddress2;
  struct in_addr  gateway1;
  struct in_addr  gateway2;
  char            host[32];
  char            domain[64];
  char            mailAddress[140];
  char            mailServer[64];
  char            popServer[64];
  char            popId[32];
  char            popPasswd[32];
  char            proxyServer[64];
  Uint16          proxyPortNum;
  Uint16          reserved_short;
  char            loginId[48];
  char            loginPasswd[16];
  Uint32          reserved;
} TCPIPInfo;

typedef struct _NetworkSummerTime {
  Uint8   mon;
  Uint8   mweek;
  Uint8   dweek;
  Uint8   hour;
} NetworkSummerTime;

typedef struct _NetworkCommonInfo {
  Uint32            magic;
  Uint32            flag;
  char              url[52];
  NetworkSummerTime stStart;
  NetworkSummerTime stEnd;
  Sint32            stDiff;
  char              initAT[48];
} NetworkCommonInfo;

typedef struct _NetworkCIDStruct {
  char  communicationID[16];
  char  password[8];
} NetworkCIDStruct;

typedef struct _NetworkCIDInfo {
  NetworkCIDStruct isp1[5];
/* NetworkCIDStruct isp2[5];*/ /* SEGA ISP only */
/* NetworkCIDStruct lan[1];*/ /* SEGA ISP only */
} NetworkCIDInfo;

typedef struct _NetworkInfo {
  Uint32            flag;
  NetworkAccessInfo isp_info1;
  NetworkAccessInfo isp_info2;
  Uint32            reserve;
} NetworkInfo;

typedef struct _PlanetWebInfo {
  char  signature[10];    // signature         10
  Uint8 version;          // current version of this userinfo   11
  Sint8 dialArea;         // dial area code always      12
  char  outLine[16];			// outside line        28
  char 	mailAddress2[16];	// AT&T extended email address (part 2)  44
  char  callWait[16];			// disable call waitin       60
  char 	mailAddress3[16];	// AT&T extended email address (part 3)  76
  char  realName[32];     // used for mail        108
  char  pcntrl[42];       // parental control login name    150
  char  modemInit[32];    // init the modem with this string   182
  char  areaCode[32];     // local area code       214
  char  longDst[32];      // long distance (1, and/or credit card)  246
  Sint8 useProxy;         // TRUE if we should do HTTP Proxy stuff  247
  Sint8 mailHdr;          // show mail headers when retrieving mail  248
  Sint8 titlebar;         // show titlebar        249
  Sint8 toolbar;          // show tool bar        250 
  Sint8 style;            // browser theme        251
  Sint8 qwertykeyb;       // qwerty/rectangular keyboard    252
  Sint8 sticky;           // sticky keyboard       253
  Sint8 quote;            // quote orig msg in replys     254
  Sint8 sndEffects;       // sound effects        255 
  Sint8 japanese;         // japanese translation      256
  Sint8 showTime;         // show time on screen      257
  Sint8 timezone;         // sync time to gmt       258
  Sint8 daylightSavings;  // are we in daylight savings?    259
  Sint8 hideKeyb;         // always hide the keyboard     260
  Sint8 ssvroff;          // don't show screen saver     261
  Sint8 showmodem;        // modem dialing box?       262
  Sint8 ccself;           // Carbon Copy to Self?      263
  Sint8 anchorDisplay;    // Style of anchors       264
  Sint8 urlbar;           // sometimes OFF for some products   265
  Sint8 padding[25];      //           300
} PlanetWebInfo;

typedef struct _NetworkInfo3 {
  Uint32              flag;  /* Highest Byte means version of this structure */
  NetworkAccessInfo   isp_info1;
  NetworkAccessInfo   isp_info2;
  Uint32              reserve;
  NetworkAccessInfo2  ext_info1;
  NetworkAccessInfo2  ext_info2;
  TCPIPInfo           lan_info;
  NetworkCommonInfo   comm_info;
  NetworkCIDInfo      cid_info;
  PlanetWebInfo       pw_info;
} NetworkInfo3;

/*
 * prototypes
 */
/* nt_Utl */
void ntUtlInit(void);
void ntUtlExit(void);
Sint16 ntUtlSetFlag(unsigned long flag);
Sint16 ntUtlGetFlag(unsigned long *flag);
Sint16 ntUtlSetJumpNumber(long number);
Sint16 ntUtlGetJumpNumber(long *number);
Sint16 ntUtlSetFix(unsigned char *fixdata, long size);
Sint16 ntUtlGetFix(unsigned char *fixdata, long *size);
Sint16 ntUtlSetUpload(unsigned char *updata, long size);
Sint16 ntUtlGetUpload(unsigned char *updata, long *size);
Sint16 ntUtlSetDownload(unsigned char *downdata, long size);
Sint16 ntUtlGetDownload(unsigned char *downdata, long *size);
bool ntUtlIsDownload(void);
bool ntUtlIsUpload(void);
Sint16 ntUtlSetDownFlag(bool flag);
Sint16 ntUtlSetUpFlag(bool flag);
bool   ntUtlIsVirtualVM(void);
Sint16 ntUtlSetVirtualVM(bool flag);

/* sg_dpldr */
void ntLDBrowser(void);

/* nt_Inf */
Sint32 ntInfInit(NetworkInfo3 *pNetInfo, void *pWork);
Sint32 ntInfExit(void);
Sint32 ntInfIsNetworkInfo(void);
Sint32 ntInfGetNetworkAccessInfo(Sint32 isp, NetworkAccessInfo **ppNetAccInfo);
Sint32 ntInfGetFlag(Uint32 *fFlag);
Sint32 ntInfGetLoginId(Sint32 isp, char *pLoginId, Sint32 size);
Sint32 ntInfGetLoginPasswd(Sint32 isp, char *pLoginPasswd, Sint32 size);
Sint32 ntInfGetAccessPointNumber(Sint32 isp, Sint32 number, char *pAccessPointNumber, Sint32 size);
Sint32 ntInfGetDialString(Sint32 isp, Sint32 number, char *pDialString, Sint32 size);
Sint32 ntInfGetPrimaryDnsAddress(Sint32 isp, struct in_addr *pDns);
Sint32 ntInfGetSecondaryDnsAddress(Sint32 isp, struct in_addr *pDns);
Sint32 ntInfGetProxyPortNum(Sint32 isp, Uint16 *pPort);

/* nt_Inf ext */
Sint32 ntInfGetNetworkInfo3(NetworkInfo3 **ppNetInfo);
Sint32 ntInfGetMailAddress(Sint32 isp, Sint32 account, char *pMailAddress, Sint32 size);
Sint32 ntInfGetMailServer(Sint32 isp, Sint32 account, char *pMailServer, Sint32 size);
Sint32 ntInfGetPopServer(Sint32 isp, Sint32 account, char *pPopServer, Sint32 size);
Sint32 ntInfGetPopId(Sint32 isp, Sint32 account, char *pPopId, Sint32 size);
Sint32 ntInfGetPopPasswd(Sint32 isp, Sint32 account, char *pPopPasswd, Sint32 size);
Sint32 ntInfGetProxyServer(Sint32 isp, char *pProxyServer, Sint32 size);

Sint32 ntInfGetNetworkAccessInfoFlag(Sint32 isp, Uint16 *pFlag);
Sint32 ntInfBuildFlagString(char *pinit, Sint32 size);
Sint32 ntInfGetNetworkAccessInfo2(Sint32 isp, NetworkAccessInfo2 **ppNetAccInfo2);
Sint32 ntInfGetTCPIPInfo(TCPIPInfo **ppTCPIPInfo);
Sint32 ntInfGetNetworkAccessInfo2Flag(Sint32 isp, Uint32 *pFlag);
Sint32 ntInfGetIPAddr(Sint32 isp, struct in_addr *IPAddr);
Sint32 ntInfGetParentalPasswd(Sint32 isp, char *pPasswd, Sint32 size);
Sint32 ntInfGetMailAccount(Sint32 isp, Uint32 account, MailAccount **ppMailAcct);
Sint32 ntInfGetTCPIPInfoFlag(Uint32 *pFlag);
Sint32 ntInfGetSubnetMask(struct in_addr *pSubnetMask);
Sint32 ntInfGetBroadcast(struct in_addr *pBroadcast);
Sint32 ntInfGetPrimaryGateway(struct in_addr *pGw);
Sint32 ntInfGetSecondaryGateway(struct in_addr *pGw);
Sint32 ntInfGetHost(char *pHost, Sint32 size);
Sint32 ntInfGetDomain(char *pDomain, Sint32 size);

Sint32 ntInfGetISP(Sint32 *isp);

Sint32 ntInfGetNetworkCommonInfo(NetworkCommonInfo **ppCommInfo);
Sint32 ntInfGetURL(char *pURL, Sint32 size);
Sint32 ntInfGetSummerTimeStart(NetworkSummerTime *pstStart);
Sint32 ntInfGetSummerTimeEnd(NetworkSummerTime *pstEnd);
Sint32 ntInfGetSummerTimeDiff(Sint32 *pdiff);
bool ntInfSummerTime(void);
bool ntInfExternalModem(void);
Sint32 ntInfGetModemInit(Sint32 isp, char *pinit, Sint32 size);
Sint32 ntInfGetCIDStruct(Sint32 isp, Sint32 account, NetworkCIDStruct *pcidstr);
Sint32 ntInfGetCommunicationID(Sint32 isp, Sint32 account, char *pId, Sint32 size);
Sint32 ntInfGetCommunicationPasswd(Sint32 isp, Sint32 account, char *pPasswd, Sint32 size);
Sint32 ntInfGetCommonFlag(Uint32 *pFlag);
Sint32 ntInfGetIniMdm(char *at, Sint32 size);

void ntInfSplitTelNumber(Uint8 *pintel, Uint8 *poutextension, Uint8 *poutprefix, Uint8 *pouttel);
Sint32 ntInfGetPlanetWebInfo(PlanetWebInfo **ppPwInfo);

#ifdef __cplusplus
} /* End of extern "C" { */
#endif /* __cplusplus */

#endif /* _NT_UTL_ */
