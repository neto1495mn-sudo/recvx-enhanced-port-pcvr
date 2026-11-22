/*
 *  Network Library
 *  Copyright (c) 1999,2000 SEGA
 *
 *  Library : Network Utility Library (Set functions)
 *  File    : nt_utl_set.h
 *  Date    : 2000.11.21
 *  Version : 1.00 (ntUtlSet)
 *
 *  For Network SDK
 */

#ifndef _NT_UTL_SET_
#define _NT_UTL_SET_

#ifdef __cplusplus /* Support for C++ */
extern "C" {
#endif /* __cplusplus */

Sint32 ntInfSave(void);
Sint32 ntInfSetClearIsp(Sint32 isp);
Sint32 ntInfSetFlag(Uint32 fFlag);
Sint32 ntInfSetLoginId(Sint32 isp, char *pLoginId);
Sint32 ntInfSetLoginPasswd(Sint32 isp, char *pLoginPasswd);
Sint32 ntInfSetAccessPointNumber(Sint32 isp, Sint32 number, char *pAccessPointNumber);
Sint32 ntInfSetDnsServerAddress(Sint32 isp, struct in_addr *pDns1, struct in_addr *pDns2);
Sint32 ntInfSetMailAddress(Sint32 isp, Sint32 account, char *pMailAddress);
Sint32 ntInfSetMailServer(Sint32 isp, Sint32 account, char *pMailServer);
Sint32 ntInfSetPopServer(Sint32 isp, Sint32 account, char *pPopServer);
Sint32 ntInfSetPopId(Sint32 isp, Sint32 account, char *pPopId);
Sint32 ntInfSetPopPasswd(Sint32 isp, Sint32 account, char *pPopPasswd);
Sint32 ntInfSetProxyServer(Sint32 isp, char *pProxyServer);
Sint32 ntInfSetProxyPortNum(Sint32 isp, Uint16 Port);
Sint32 ntInfSetNetworkAccessInfoFlag(Sint32 isp, Uint16 Flag);
Sint32 ntInfSetNetworkAccessInfo2Flag(Sint32 isp, Uint32 Flag);
Sint32 ntInfSetIPAddr(Sint32 isp, struct in_addr *IPAddr);
Sint32 ntInfSetParentalPasswd(Sint32 isp, char *pPasswd);
Sint32 ntInfSetTCPIPInfoFlag(Uint32 Flag);
Sint32 ntInfSetSubnetMask(struct in_addr *pSubnetMask);
Sint32 ntInfSetBroadcast(struct in_addr *pBroadcast);
Sint32 ntInfSetGateway(struct in_addr *pGw1, struct in_addr *pGw2);
Sint32 ntInfSetHost(char *pHost);
Sint32 ntInfSetDomain(char *pDomain);
Sint32 ntInfSetSocksServer(char *pSocksServer);
Sint32 ntInfSetSocksPortNum(Uint16 SocksPortNum);
Sint32 ntInfSetISP(Sint32 isp);
Sint32 ntInfSetURL(char *pURL);
Sint32 ntInfSetSummerTimeStart(NetworkSummerTime *pstStart);
Sint32 ntInfSetSummerTimeEnd(NetworkSummerTime *pstEnd);
Sint32 ntInfSetSummerTimeDiff(Sint32 diff);
Sint32 ntInfSetModemInit(Sint32 isp, char *pinit);
Sint32 ntInfSetCIDStruct(Sint32 isp, Sint32 account, NetworkCIDStruct *pcidstr);
Sint32 ntInfSetCommunicationID(Sint32 isp, Sint32 account, char *pcid);

#ifdef __cplusplus
} /* End of extern "C" { */
#endif /* __cplusplus */

#endif/*_NT_UTL_SET_*/
