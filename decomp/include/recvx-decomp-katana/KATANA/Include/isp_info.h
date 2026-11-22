#ifndef _ISP_INFO_H_
#define	_ISP_INFO_H_

// TAB WIDTH == 2

// Enumerated types:
// -----------------
enum	// Error return values...
{
	ISP_OK								=	 0,	// The function call was successful.
	ISP_ERR_ALREADY_INIT	= -1,	// The library has already been initialised.
	ISP_ERR_NOT_INIT			= -2,	// The library has not been initialised.
	ISP_ERR_BUF_SIZE			= -3,	// There was insufficient buffer space to copy the data into.
	ISP_ERR_NO_INFO				= -4,	// The requested information was not found in flash RAM.
	ISP_ERR_ALLOC					= -5,	// Could not allocate memory to store the PlanetWeb info.
};


// Public function prototypes:
// ---------------------------

// --------------------------------------------------------------------------------------------------------------
//	ispInit()
//	---------
//	Description	:	Initialises the library for reading ISP info from flash RAM.
//
//	Arguments		:	None.
//
//	Returns			:	'ISP_ERR_ALREADY_INIT'	- The library has already been initialised.
//								'ISP_ERR_ALLOC'					- There was insufficient memory to intialise the buffers.
//								'ISP_ERR_NO_INFO'				- No valid ISP information could be found in flash RAM.
//								'ISP_OK'								- The library was successfully initialised.
// --------------------------------------------------------------------------------------------------------------
int ispInit(void);




// --------------------------------------------------------------------------------------------------------------
//	ispExit()
//	---------
//	Description	:	Shuts down the library and cleans up any internal data being used.
//
//	Arguments		:	None.
//
//	Returns			: None.
// --------------------------------------------------------------------------------------------------------------
void ispExit(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetInitString()
//	----------------------
//	Description	:	Retrieves the modem init string in the form of an AT command (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetInitString(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetPrimaryDialString()
//	-----------------------------
//	Description	:	Returns a fully formatted, browser independent primary dial string (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetPrimaryDialString(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetSecondaryDialString()
//	-------------------------------
//	Description	:	Returns a fully formatted, browser independent secondary dial string (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetSecondaryDialString(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetTertiaryDialString()
//	------------------------------
//	Description	:	Returns a fully formatted, browser independent tertiary dial string (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetTertiaryDialString(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_DialBackupNumber()
//	-------------------------
//	Description	:	Returns whether the application should attempt to redial the primary number or dial a backup
//								number instead.
//
//	Arguments		:	None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE attempts to dial the two backup numbers.
//								'false'	-	The user has pre-configured to always attempt to redial the primary number.
// --------------------------------------------------------------------------------------------------------------
bool ispMdm_DialBackupNumber(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetLoginId()
//	-------------------
//	Description	:	Retrieves the user's PPP login ID from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetLoginId(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetLoginPassword()
//	-------------------------
//	Description	:	Retrieves the user's PPP login password from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetLoginPassword(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetStaticIpAddr()
//	------------------------
//	Description	:	Retrieves the client machine's static IP address from flash RAM (modem area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispMdm_GetStaticIpAddr(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_UseStaticIpAddr()
//	------------------------
//	Description	:	Returns whether the application should use a static IP address when connected to the ISP.
//
//	Arguments		: None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE use of the static IP address.
//								'false'	-	The user has pre-configured to DISABLE use of the static IP Address.
// --------------------------------------------------------------------------------------------------------------
bool ispMdm_UseStaticIpAddr(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetPrimaryDns()
//	----------------------
//	Description	:	Retrieves the ISP's primary DNS address from flash RAM (modem area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispMdm_GetPrimaryDns(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetSecondaryDns()
//	------------------------
//	Description	:	Retrieves the ISP's primary DNS address from flash RAM (modem area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispMdm_GetSecondaryDns(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetMailAddress()
//	-----------------------
//	Description	:	Retrieves the user's client-side email address from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetMailAddress(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetMailServer()
//	----------------------
//	Description	:	Retrieves the ISP's SMTP server host name from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetMailServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetPopServer()
//	---------------------
//	Description	:	Retrieves the ISP's POP3 server host name from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetPopServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetPopId()
//	-----------------
//	Description	:	Retrieves the user's POP3 account login ID (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetPopId(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetPopPassword()
//	-----------------------
//	Description	:	Retrieves the user's POP3 account login password (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetPopPassword(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetProxyServer()
//	-----------------------
//	Description	:	Retrieves the ISP's proxy server host name from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetProxyServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetProxyPortNum()
//	------------------------
//	Description	:	Returns the HTTP port number on the proxy server (modem area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The proxy server's HTTP port number.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetProxyPortNum(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_UseProxy()
//	-----------------
//	Description	:	Returns whether the application should use the proxy server for network traffic.
//
//	Arguments		:	None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE use of the proxy server.
//								'false'	-	The user has pre-configured to DISABLE use of the proxy server.
// --------------------------------------------------------------------------------------------------------------
bool ispMdm_UseProxy(void);




// --------------------------------------------------------------------------------------------------------------
//	ispMdm_GetParentalPassword()
//	----------------------------
//	Description	:	Retrieves the parental password (child lock) string from flash RAM (modem area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispMdm_GetParentalPassword(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetStaticIpAddr()
//	------------------------
//	Description	:	Retrieves the client machine's static IP address from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetStaticIpAddr(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_UseStaticIpAddr()
//	------------------------
//	Description	:	Returns whether the application should use a static IP address when connected to the ISP.
//
//	Arguments		: None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE use of the static IP address.
//								'false'	-	The user has pre-configured to DISABLE use of the static IP Address.
// --------------------------------------------------------------------------------------------------------------
bool ispLan_UseStaticIpAddr(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetSubnetMask()
//	----------------------
//	Description	:	Retrieves the network's subnet mask from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetSubnetMask(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetBroadcastAddr()
//	-------------------------
//	Description	:	Retrieves the network-wide broadcast address from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetBroadcastAddr(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetPrimaryDns()
//	----------------------
//	Description	:	Retrieves the ISP's primary DNS address from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetPrimaryDns(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetSecondaryDns()
//	------------------------
//	Description	:	Retrieves the ISP's secondary DNS address from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetSecondaryDns(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_UseDns()
//	---------------
//	Description	:	Returns whether the application should override the ISP provided DNS servers.
//
//	Arguments		: None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE use of the static DNS server details.
//								'false'	-	The user has pre-configured to DISABLE use of the static DNS server details.
// --------------------------------------------------------------------------------------------------------------
bool ispLan_UseDns(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetPrimaryGateway()
//	--------------------------
//	Description	:	Retrieves the ISP's primary gateway address from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetPrimaryGateway(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetSecondaryGateway()
//	----------------------------
//	Description	:	Retrieves the ISP's secondary gateway address from flash RAM (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The IP address in network-byte order.
// --------------------------------------------------------------------------------------------------------------
unsigned long ispLan_GetSecondaryGateway(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetHostName()
//	--------------------
//	Description	:	Retrieves the client machine's host name from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetHostName(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetDomainName()
//	----------------------
//	Description	:	Retrieves the ISP's network domain name from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetDomainName(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetMailAddress()
//	-----------------------
//	Description	:	Retrieves the user's client-side email address from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetMailAddress(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetMailServer()
//	----------------------
//	Description	:	Retrieves the ISP's SMTP server host name from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetMailServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetPopServer()
//	---------------------
//	Description	:	Retrieves the ISP's POP3 server host name from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetPopServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetPopId()
//	-----------------
//	Description	:	Retrieves the user's POP3 account login ID (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetPopId(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetPopPassword()
//	-----------------------
//	Description	:	Retrieves the user's POP3 account login password (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetPopPassword(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetProxyServer()
//	-----------------------
//	Description	:	Retrieves the ISP's proxy server host name from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetProxyServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetProxyPortNum()
//	------------------------
//	Description	:	Returns the HTTP port number on the proxy server (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The proxy server's HTTP port number.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetProxyPortNum(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_UseProxy()
//	-----------------
//	Description	:	Returns whether the application should use the proxy server for network traffic.
//
//	Arguments		:	None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE use of the proxy server.
//								'false'	-	The user has pre-configured to DISABLE use of the proxy server.
// --------------------------------------------------------------------------------------------------------------
bool ispLan_UseProxy(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetSocksServer()
//	-----------------------
//	Description	:	Retrieves the ISP's SOCKS server host name from flash RAM (TCP/IP area).
//
//	Arguments		:	'buf'		- Pointer to the buffer to store the data into. If this value is NULL then the function
//													returns the maximum number of bytes needed to store the data (excluding the NULL
//													terminating character).
//								'size'	- Pointer to the size of the buffer passed.  If the buffer parameter is NULL then upon
//													returning the 'size' value contains the maximum number of bytes needed to store the
//													data.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								'ISP_ERR_BUF_SIZE'	-	The size of the buffer supplied was invalid (see above).
//								'ISP_OK'						-	The function succeeded.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetSocksServer(char *buf, int *size);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_GetSocksPortNum()
//	------------------------
//	Description	:	Returns the port number of the SOCKS server (TCP/IP area).
//
//	Arguments		:	None.
//
//	Returns			: 'ISP_ERR_NOT_INIT'	- The library has not been initialised.
//								'ISP_ERR_NO_INFO'		-	No info of this type was found in flash RAM.
//								Any positive value	-	The port number of the SOCKS server.
// --------------------------------------------------------------------------------------------------------------
int ispLan_GetSocksPortNum(void);




// --------------------------------------------------------------------------------------------------------------
//	ispLan_UseSocks()
//	-----------------
//	Description	:	Returns whether the application should use the SOCKS server.
//
//	Arguments		:	None.
//
//	Returns			: 'true'	-	The user has pre-configured to ENABLE use of the SOCKS server.
//								'false'	-	The user has pre-configured to DISABLE use of the SOCKS server.
// --------------------------------------------------------------------------------------------------------------
bool ispLan_UseSocks(void);


#endif	// _ISP_INFO_H