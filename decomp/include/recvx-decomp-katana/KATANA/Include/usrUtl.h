#ifndef _USRUTL_H_
#define	_USRUTL_H_

#ifdef __cplusplus /* Support for C++ */
extern "C" {
#endif /* __cplusplus */

// Function prototypes:
// --------------------
// NOTE: All functions here that return type 'char *' will return NULL if they fail...
bool					 UUInitISPInfo(void);							// 'True' if ISP info found, otherwise 'false'.
char					*UUGetModemInitString(void);			// AT command string to initialise modem.	(Max. 42	bytes)
char					*UUGetPrimaryDialString(void);		// Main AT dial string.										(Max. 105 bytes)
char					*UUGetSecondaryDialString(void);	// First alternative AT dial string.			(Max. 105	bytes)
char					*UUGetTertiaryDialString(void);		// Second alternative AT dial string.			(Max. 105	bytes)
bool					 UUDialAlternateNumber(void);			// 'True' if the backup numbers should be dialled on failure.
char					*UUGetLoginID(void);							// ISP login username.										(Max. 28	bytes)
char					*UUGetPassword(void);							// ISP login password.										(Max. 16	bytes)
unsigned long	 UUGetPrimaryDNS(void);						// Primary DNS IP address									(Network byte order)
unsigned long	 UUGetSecondaryDNS(void);					// Secondary DNS IP address.							(Network byte order)
char					*UUGetMailAddress(void);					// User's game email address.							(Max. 48	bytes)
char					*UUGetMailServer(void);						// SMTP server's hostname.								(Max. 32	bytes)
char					*UUGetPopServer(void);						// POP3 server's hostname.								(Max. 32	bytes)
char					*UUGetPopId(void);								// POP3 account username.									(Max. 16	bytes)
char					*UUGetPopPassword(void);					// POP3 account password.									(Max. 16	bytes)
char					*UUGetProxyServer(void);					// Proxy server's hostname.								(Max. 32	bytes)
int						 UUGetProxyPortNum(void);					// Proxy server's port number.						(-1 if no proxy)

#ifdef __cplusplus
} /* End of extern "C" { */
#endif /* __cplusplus */

#endif	// _USRUTL_H_
