#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

// TAB WIDTH == 2

// Required defines for NexGen includes:
// -------------------------------------
#ifndef NG_COMP_SHC
#define NG_COMP_SHC
#endif
#ifndef NG_CPU_SH4
#define NG_CPU_SH4
#endif
#ifndef NG_LITTLE_ENDIAN
#define NG_LITTLE_ENDIAN
#endif
#ifndef NG_HW_SEGADC
#define NG_HW_SEGADC
#endif

// Header files:
// -------------
#include <ngdev.h>
#include <ngos\netif.h>
#include <ngmdm.h>
#include <ngsocket.h>
#include <nt_utl.h>


// Enumerated types:
// -----------------
enum {	// Bit-flags to select which devices are to be probed for upon stack initialisation...
	TR_DEVICE_PROBE_FINISHED	= 0x00,	// Probing of transport devices has finished.
	TR_DEVICE_DCLAN						=	0x01,	// Enable probing for a DC LAN card.
	TR_DEVICE_EXTMODEM				=	0x02,	// Enable probing for an external modem.
	TR_DEVICE_RASSERIALPPP		= 0x04,	// Enable probing for a RAS serial PPP connection.
	TR_DEVICE_SERIALPPP				= 0x08,	// Enable probing for a serial PPP connection.
	TR_DEVICE_INTMODEM				=	0x10,	// Enable probing for the internal modem.
};

enum {	// Bit-field for storing the various elements that can be used in a transport method...
	TR_COMPONENT_ETHERNET	= 0x1,	// The transport system is Ethernet based.
	TR_COMPONENT_SERIAL		= 0x2,	// The transport system uses the SCIF serial port.
	TR_COMPONENT_MODEM		= 0x4,	// The transport system is modem based.
	TR_COMPONENT_SCRIPT		= 0x8,	// The transport system uses a dial/connection script.
	TR_COMPONENT_PPP			= 0x10,	// The transport system uses the PPP protocol.
	TR_COMPONENT_DHCP			= 0x20,	// The transport system uses the DHCP protocol.
};

enum {	// Combinations of the above bit-flags to show the exact transport method and components in use...
// No transport method has been found yet (still polling)...
	TR_TRANSPORT_NONE					= 0,

// Tells trConnect() to use the default transport device found by the stack...
	TR_TRANSPORT_AUTO					= TR_TRANSPORT_NONE,

// Use the Dreamcast's LAN adapter (static configuration - no transport protocol selected)...
	TR_TRANSPORT_DCLAN_STATIC	= TR_COMPONENT_ETHERNET,

// Use the Dreamcast's LAN adapter (DHCP protocol enabled)...
	TR_TRANSPORT_DCLAN_DHCP		= (TR_COMPONENT_ETHERNET	| TR_COMPONENT_DHCP),

// Use the Dreamcast's LAN adapter (PPPoE protocol enabled)...
	TR_TRANSPORT_DCLAN_PPPOE	= (TR_COMPONENT_ETHERNET	| TR_COMPONENT_PPP),

// Use a normal null-modem transport method...
	TR_TRANSPORT_SERIALPPP		= (TR_COMPONENT_SERIAL		| TR_COMPONENT_PPP),

// Use a RAS compatible null-modem transport method...
	TR_TRANSPORT_RASSERIALPPP	= (TR_COMPONENT_SERIAL		| TR_COMPONENT_PPP | TR_COMPONENT_SCRIPT),

// Use an external modem connected to the serial port...
	TR_TRANSPORT_EXTMODEM			= (TR_COMPONENT_SERIAL		| TR_COMPONENT_PPP | TR_COMPONENT_SCRIPT | TR_COMPONENT_MODEM),

// Use the Dreamcast's internal modem...
	TR_TRANSPORT_INTMODEM			= (                         TR_COMPONENT_PPP | TR_COMPONENT_SCRIPT | TR_COMPONENT_MODEM),
};

enum {	// Stack state machine states...
	TR_STATE_PROBE_DCLAN = 1,	// Probing for a DC LAN card.
	TR_STATE_PROBE_EXTMODEM,	// Probing for an external modem.
	TR_STATE_PROBE_SERIALPPP,	// Probing for a serial PPP connection.
	TR_STATE_PROBE_INTMODEM,	// Probing for an internal modem.
	TR_STATE_IDLE,						// Not currently doing any processing.
	TR_STATE_POLL_DEV,				// Device connection polling state.
	TR_STATE_RESET_DEV,				// Device reset state.
	TR_STATE_POLL_PPP,				// PPP negotiation polling state.
	TR_STATE_CONNECTED,				// Application defined connection state.
};

enum {	// Stack status values returned in status callback (after connection has shutdown or attempt failed)...
// Errors relating to setting up the stack's connection info...
	TR_STATUS_ERR_DIAL_PARAMS = 1,			// Dial params sent to the stack were not valid.
	TR_STATUS_ERR_LOGIN_PARAMS,					// Authentication params sent to the stack were not valid.
	TR_STATUS_ERR_DNS_PARAMS,						// Could not retrieve the DNS addresses from the stack.

// Errors relating to a DC LAN connection attempt...
	TR_STATUS_ERR_STATIC_FAILED,				// Could not set-up a static DC LAN configuration.
	TR_STATUS_ERR_DCLAN_TIMEOUT,				// Timeout looking for a DHCP/PPPoE server.
	TR_STATUS_ERR_DHCP_INIT,						// Could not initialise the DHCP system.
	TR_STATUS_ERR_DHCP_FAILED,					// Could not establish a connection to a DHCP server.
	TR_STATUS_ERR_PPPOE_INIT,						// Could not initialise the PPPoE system.
	TR_STATUS_ERR_PPPOE_FAILED,					// Could not find a PPPoE server.

// Errors relating to the running of the modem script...
	TR_STATUS_ERR_SCR_INIT,							// Connection attempt failed during device or modem script initialisation.
	TR_STATUS_ERR_SCR_NO_CARRIER,				// No signal carrier was detected.
	TR_STATUS_ERR_SCR_NO_ANSWER,				// The remote modem did not answer the call.
	TR_STATUS_ERR_SCR_NO_DIALTONE,			// No dial tone was detected by the modem.
	TR_STATUS_ERR_SCR_LINE_BUSY,				// The line was busy (engaged?).
	TR_STATUS_ERR_SCR_TIMEOUT,					// The modem script timed out waiting for a response to an action.

// Errors relating to PPP negotiation...
	TR_STATUS_ERR_PPP_INIT,							// PPP negotiation could not be started.
	TR_STATUS_ERR_PPP_NEG,							// PPP negotiation failed during negotiation.

// Reason for disconnection...
	TR_STATUS_USER_CANCELLED,						// The user cancelled a connection in progress.
	TR_STATUS_USER_DISCONNECTED,				// The user chose to disconnect.
	TR_STATUS_LINE_DISCONNECTED,				// The line was dropped, forcing a disconnection.
};


// Public function prototypes:
// ---------------------------
typedef void (TR_FUNC)(void);	  // Function type.
typedef TR_FUNC *TR_FUNC_PTR;	  // Function pointer type.

// ---------------------------------------------------------------------------------------------------------------
//	trInit()
//	--------
//	Description	:	Initialises the stack state machine and prepares all necessary data.  Sets-up to start probing
//								for devices as soon as trYield() is called for the first time.
//
//	Arguments		:	'probe_devices'	-	Bit-field containing the 'TR_DEVICE_XXX' to probe for.  You can OR the various
//																	values together to specify all transport devices to probe for.
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trInit(int probe_devices);




// ---------------------------------------------------------------------------------------------------------------
//	trSetOnProbeFinishedCallback()
//	------------------------------
//	Description	:	This callback is called to inform the application that all device probing has finished and the
//								stack is about to enter an idle state.  At this point the application may call
//								'trGetDevicesFound()' to check which devices were detected during probing and, if need be, call
//								'trChooseTransportDevice()' to have the stack select an appropriate transport device for
//								connection.
//
//	Arguments		:	'user_cb' - Pointer to the function to be called (type 'TR_FUNC_PTR').
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trSetOnProbeFinishedCallback(TR_FUNC_PTR user_cb);




// ---------------------------------------------------------------------------------------------------------------
//	trSetOnDialFailureCallback()
//	----------------------------
//	Description	:	This callback is called to inform the application that a dial attempt has failed so that it may
//								update the user interface.  If the user has configured the flash RAM settings to attempt dialing
//								of a backup number then the stack will proceed to do so once this function returns, until either
//								a connection has succeeded or all attempts have failed (whereupon the disconnect status callback
//								will be called).
//
//	Arguments		:	'user_cb' - Pointer to the function to be called (type 'TR_FUNC_PTR').
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trSetOnDialFailureCallback(TR_FUNC_PTR user_cb);




// ---------------------------------------------------------------------------------------------------------------
//	trSetConnectionCallback()
//	-------------------------
//	Description	: Sets the user-defined function to be called while the stack is connected to an ISP.  This
//								callback is called repeatedly once the state machine has successfully established a connection
//								to an ISP.  It is not necessary to use this callback for your own application specific code as
//								you may wish to call a seperate function in the main program loop.
//
//	Arguments		:	'user_cb' - Pointer to the function to be called (type 'TR_FUNC_PTR').
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trSetConnectionCallback(TR_FUNC_PTR user_cb);




// ---------------------------------------------------------------------------------------------------------------
//	trSetOnDisconnectCallback()
//	---------------------------
//	Description	: Sets the user-defined function to be called when a disconnection event has occured.  This
//								callback is called every time a connection dies or a connection attempt fails.  At this point
//								it is recommended that 'trGetDisconnectReason()' is called to find out the reason for the
//								disconnection.
//
//	Arguments		:	'user_cb' - Pointer to the function to be called (type 'TR_FUNC_PTR').
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trSetOnDisconnectCallback(TR_FUNC_PTR user_cb);




// ---------------------------------------------------------------------------------------------------------------
//	trSetOnDeviceResetCallback()
//	----------------------------
//	Description	: Sets the user-defined function to be called after the disconnection procedure has completely
//								finished.  This callback is called straight after a device reset to indicate to the application
//								that a connection is now dead and the state machine is idle.
//
//	Arguments		:	'user_cb' - Pointer to the function to be called (type 'TR_FUNC_PTR').
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trSetOnDeviceResetCallback(TR_FUNC_PTR user_cb);




// ---------------------------------------------------------------------------------------------------------------
//	trGetDevice()
//	-------------
//	Description	: Returns a pointer to the stack's device data.  To be used only with other NexGenIP functions
//								(incorrect usage could corrupt data and cause your application to crash).
//
//	Arguments		:	None.
//
//	Returns			:	Pointer to the NGdev data structure.
// ---------------------------------------------------------------------------------------------------------------
const NGdev *trGetDevice(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetDeviceCb()
//	---------------
//	Description	: Returns a pointer to the stack's device control block.  To be used only with other NexGenIP
//								functions (incorrect usage could corrupt data and cause your application to crash).
//
//	Arguments		:	None.
//
//	Returns			:	Pointer to the NGdevcb data structure.
// ---------------------------------------------------------------------------------------------------------------
const NGdevcb *trGetDeviceCb(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetInterface()
//	----------------
//	Description	: Returns a pointer to the stack's selected network interface.  To be used only with other
//								NexGenIP functions (incorrect usage could corrupt data and cause your application to crash).
//
//	Arguments		:	None.
//
//	Returns			:	Pointer to the NGifnet data structure.
// ---------------------------------------------------------------------------------------------------------------
const NGifnet *trGetInterface(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetModemState()
//	-----------------
//	Description	: Returns a pointer to the stack's modem state data.  To be used only with other NexGenIP
//								functions (incorrect usage could corrupt data and cause your application to crash).
//
//	Arguments		:	None.
//
//	Returns			:	Pointer to the NGmdmstate data structure.
// ---------------------------------------------------------------------------------------------------------------
const NGmdmstate *trGetModemState(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetDevicesFound()
//	-------------------
//	Description	: Returns a bit-field of the transport devices found by the stack during probing.
//
//	Arguments		:	None.
//
//	Returns			:	'0' if no transport devices have been found.
//								If not as above then the value returned is a bit-field of the devices found ('TR_DEVICE_XXX').
// ---------------------------------------------------------------------------------------------------------------
int trGetDevicesFound(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetCurrTransport()
//	--------------------
//	Description	: Returns the current transport device being used.  If no transport device is being used then
//								you can either call trGetTransportDevice() to have the library select a suitable transport
//								device, or you can specify your own transport device when calling trConnect().
//
//	Arguments		:	None.
//
//	Returns			:	'TR_TRANSPORT_NONE' if a suitable transport device has not been selected yet.
//								If not as above then the value returned is the current transport device being used.
// ---------------------------------------------------------------------------------------------------------------
int trGetCurrTransport(void);




// ---------------------------------------------------------------------------------------------------------------
//	trYield()
//	---------
//	Description	: Runs the state machine engine to process the current stack state.  This function calls
//								ngYield() internally so it should act as the main polling function in your application.
//
//	Arguments		:	None.
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trYield(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetStackState()
//	-----------------
//	Description	: Gets the current state of the stack state machine.
//
//	Arguments		:	None.
//
//	Returns			:	'TR_STATE_IDLE' if the state machine is not currently running (idle state).
//								If not as above then the value returned is the current state of the stack ('TR_STATE_XXX').
// ---------------------------------------------------------------------------------------------------------------
int trGetStackState(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetFlashNetInfo()
//	-------------------
//	Description	: Reads network info stored in flash RAM into a structure that can be passed to trConnect().  If
//								either of the parameters passed are NULL then memory is allocated internally by the library
//								using malloc() and MUST be freed by calling trFreeFlashNetInfo().
//
//	Arguments		:	'work_area'	-	Pointer to an area of memory needed to read the network info data.
//								'net_info'	- Pointer to a structure needed to store the network info.
//
//	Returns			: 'NTD_ERR_NOTINIT'				-	The network info area of flash RAM has not been properly initialised.
//								'NTD_ERR_INVALIDPARAM'	- Internal memory allocation failed (this is only relevant if either
//																					parameter passed to the function is NULL).
//								'NTD_OK'								- Successfully read the network info from flash RAM into 'net_info'.
// ---------------------------------------------------------------------------------------------------------------
int trGetFlashNetInfo(void *work_area, NetworkInfo3 *net_info);




// ---------------------------------------------------------------------------------------------------------------
//	trFreeFlashNetInfo()
//	--------------------
//	Description	: Free any memory dynamically allocated by the library to store the network info found
//								in flash RAM.  If the application handled the memory allocation itself then this function
//								should still be called in order to clean up internal library flags.
//
//	Arguments		:	None.
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trFreeFlashNetInfo(void);




// ---------------------------------------------------------------------------------------------------------------
//	trChooseTransportDevice()
//	-------------------------
//	Description	: Instructs the library to select a suitable transport device based on critera such as the
//								existence of the device and how it is configured in flash RAM.
//
//	Arguments		:	None.
//
//	Returns			:	'TR_TRANSPORT_NONE' if no suitable transport device could be selected.
//								If not as above then the value returned is a valid transport device that can be used to connect.
// ---------------------------------------------------------------------------------------------------------------
int trChooseTransportDevice(void);




// ---------------------------------------------------------------------------------------------------------------
//	trConnect()
//	-----------
//	Description	: Initiates a connection to the specified ISP.  The rest of the process will be handled
//								automatically by the stack as long as 'trYield()' is called regularly.
//
//	Arguments		:	'device'		-	A transport device to use during the connection procedure ('TR_TRANSPORT_XXX').
//								'net_info'	- Pointer to the network info either retrieved from flash RAM or manually edited
//															by the user.
//
//	Returns			: 'NG_EOK' if there was no error, otherwise a NexGenIP error value.
// ---------------------------------------------------------------------------------------------------------------
int trConnect(int device, NetworkInfo3 *net_info);




// ---------------------------------------------------------------------------------------------------------------
//	trGetConnectSpeed()
//	-------------------
//	Description	:	Retrieves the connection or link speed of the transport device being used.
//
//	Arguments		:	None.
//
//	Returns			: Pointer to the string containing the connection/link speed, or NULL on error.
// ---------------------------------------------------------------------------------------------------------------
const char *trGetConnectSpeed(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetDnsServers()
//	-----------------
//	Description	:	Retrieves the IP addresses of the DNS servers being used by the stack.
//
//	Arguments		:	'dns1' - Pointer filled with the address of the primary DNS server.
//								'dns2' - Pointer filled with the address of the secondary DNS server.
//
//	Returns			: 'NG_EFAULT' if the DNS system is disabled (no primary DNS server), else 'NG_EOK'.
// ---------------------------------------------------------------------------------------------------------------
int trGetDnsServers(NGuint *dns1, NGuint *dns2);




// ---------------------------------------------------------------------------------------------------------------
//	trDisconnect()
//	--------------
//	Description	: Forces the stack to either disconnect from an ISP or abort the connection attempt if
//								one is already in progress.  The device reset callback will be triggered once
//								disconnection is complete and the stack has returned to an idle state.
//
//	Arguments		:	None.
//
//	Returns			: No return value.
// ---------------------------------------------------------------------------------------------------------------
void trDisconnect(void);




// ---------------------------------------------------------------------------------------------------------------
//	trGetDisconnectReason()
//	-----------------------
//	Description	:	Returns a reason for the last disconnection event.  If more than one reason was registered
//								during the disconnection procedure then only the frist reason is returned by this function.
//
//	Arguments		:	None.
//
//	Returns			: One of the 'TR_STATUS_XXX' values defined above.
// ---------------------------------------------------------------------------------------------------------------
int trGetDisconnectReason(void);




// ---------------------------------------------------------------------------------------------------------------
//	trExit()
//	--------
//	Description	:	Shuts down the stack and transport library and frees any resources allocated by them.  This
//								function should not be called unless the stack is in an idle state.
//
//	Arguments		:	None.
//
//	Returns			: 'NG_EBUSY' if the stack is not in an idle state, otherwise 'NG_EOK'.
// ---------------------------------------------------------------------------------------------------------------
int trExit(void);


#endif	// _TRANSPORT_H_