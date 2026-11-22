/*
GameSpy GT SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
Tel: 949.798.4200
Fax: 949.798.4299
*/

/**********************
** GameSpy Transport **
**********************/

#ifndef _GT_H_
#define _GT_H_

#include "nonport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********
** TYPES **
**********/

// Boolean.
///////////
typedef int GTBool;
#define GTFalse 0
#define GTTrue  1

// A byte.
//////////
typedef char GTByte;

// A connection to a remote system.
///////////////////////////////////
typedef struct GTIConnection * GTConnection;

// This is used to listen for incoming
// connections from remote systems.
//////////////////////////////////////
typedef struct GTIListener * GTListener;

// Possible results from a connection attempt.
//////////////////////////////////////////////
typedef enum
{
	GTAccepted,             // The connection was accepted.
	GTRejected,             // The connection was rejected.
	GTMemoryError,          // There was an error allocating memory.
	GTLocalNetworkError,    // There was a local networking error.
	GTLocalAddressError,    // There was an error with the local address.
	GTRemoteAddressError,   // There was an error with the remote address.
	GTTimedOut,             // The connection attempt timed-out.
	GTNegotiationError      // There was an error negotiating the connection.
} GTConnectResult;

// The cause of the connection being closed.
////////////////////////////////////////////
typedef enum
{
	GTLocalClose,      // The connection was closed with gtCloseConnection.
	GTRemoteClose,     // The connection was closed remotely.
	GTConnectionError, // There was an error with the connection.
	GTNotEnoughMemory  // There wasn't enough memory to store an incoming message.
} GTCloseReason;

// The cause of the listener being stopped.
///////////////////////////////////////////
typedef enum
{
	GTStopped,         // The listener was stopped with gtStopListener.
	GTListenerError    // There was an error with the listener.
} GTStopReason;

/************
** GLOBALS **
************/

// The challenge key is a 32 character string
// that is used in the authentication process.
// The key can be set before GT is used so
// that the key will be application-specific.
//////////////////////////////////////////////
extern char challengeKey[33];

/**************
** CALLBACKS **
**************/

// Called when the connect has completed.
// If the result is GTRejected,
// then message is the message that the
// listener passed to gtReject.  If the
// result is anything else, then message
// is NULL and len is 0.
/////////////////////////////////////////
typedef void (* gtConnectedCallback)
(
	GTConnection connection,       // The connection object.
	GTConnectResult result,        // Result from connect attempt.
	GTByte * message,              // If result==GTRejected, the reason.  Otherwise, NULL.
	int len                        // If result==GTRejected, the length of the reason.  Otherwise, 0.
);

// Called when a message is received.
/////////////////////////////////////
typedef void (* gtReceivedCallback)
(
	GTConnection connection,       // The connection the message was received on.
	GTByte * message,              // The message that was received.  May be NULL.
	int len,                       // The length of the message in bytes.  May be 0.
	GTBool reliable                // True if this is a reliable message.
);

// Called when the connection is closed (remotely or locally).
// The connection can no longer be referenced after this.
//////////////////////////////////////////////////////////////
typedef void (* gtClosedCallback)
(
	GTConnection connection,       // The connection that was closed.
	GTCloseReason reason           // The reason the connection was closed.
);

// Callbacks set for each connection.
// The connected callback is ignored
// when this is passed to gtAccept.
/////////////////////////////////////
typedef struct
{
	gtConnectedCallback connected; // Called when gtConnect is complete.
	gtReceivedCallback received;   // Called when a message is received.
	gtClosedCallback closed;       // Called when the connection is closed (remotely or locally).
} GTConnectionCallbacks;

// Called when a connection attempt is made.
// Nothing should be done with the connection
// object until it is either accepted or rejected.
// If rejected, the object cannot be used
// anymore.
//////////////////////////////////////////////////
typedef void (* gtConnectAttemptCallback)
(
	GTListener listener,            // The listener the connection was received on.
	GTConnection connection,        // The connection being attempted.
	unsigned int ip,                // The IP address of the remote system (in network byte order).
	unsigned short port,            // The port the attempt is coming from (in host byte order).
	GTByte * message,               // Initial message sent by the remote system.  May be NULL.
	int len                         // Length of message sent by remote system.  May be 0.
);

// Called when the listener is stopped.
// The listener can no longer be referenced after this.
///////////////////////////////////////////////////////
typedef void (* gtStoppedCallback)
(
	GTListener listener,            // The listener that was stopped.
	GTStopReason reason             // The reasont he listener was stopped.
);

// Callbacks set for each listener.
///////////////////////////////////
typedef struct
{
	gtConnectAttemptCallback connectAttempt;  // Called when a connection attempt is made.
	gtStoppedCallback stopped;    // Called when the listener is stopped (on purpose or by an error).
} GTListenerCallbacks;

// Callback for filtering send data.
// Call gtFilteredSend with the filtered data,
// either from within the callback or later.
//////////////////////////////////////////////
typedef void (* gtSendFilterCallback)
(
	GTConnection connection,  // The connection on which the message is being sent.
	int filterID,             // Pass this ID to gtFilteredSend.
	const GTByte * message,   // The message being sent. May be NULL.
	int len,                  // The length of the message being sent, in bytes. May be 0.
	GTBool reliable           // If the message is being sent reliably.
);

// Callback for filtering receive data.
// Call gtFilteredRecieve with the filtered data,
// either from within the callback or later.
/////////////////////////////////////////////////
typedef void (* gtReceiveFilterCallback)
(
	GTConnection connection,       // The connection the message was received on.
	int filterID,                  // Pass this ID to gtFilteredReceive.
	GTByte * message,              // The message that was received.  May be NULL.
	int len,                       // The length of the message in bytes.  May be 0.
	GTBool reliable                // True if this is a reliable message.
);

/**********************
** GENERAL FUNCTIONS **
**********************/

// Do any neccessary processing of connections and listeners.
/////////////////////////////////////////////////////////////
void gtThink
(
	void
);

// Close all connections and stop all
// listeners.  This is not needed if
// everything has been closed/stopped already.
//////////////////////////////////////////////
void gtCleanup
(
	GTBool callCallbacks           // If GTTrue, call Closed and Stopped callbacks.
);

/*************************
** CONNECTION FUNCTIONS **
*************************/

// Open a connection to a remote system.
// localAddress = "[IP | hostname][:port]".
// remoteAddress = "<IP | hostname>:<port>".
// A len of -1 is equivalent to (strlen(message) + 1).
// A timeout of 0 means try once, a timeout of
// -1 means try until connected.
// For most situations, the timeout should
// be at least 5-10 seconds (5000-10000ms).
//////////////////////////////////////////////////////
GTConnection gtConnect
(
	const char * localAddress,          // The local address to bind to.  Can be NULL.
	const char * remoteAddress,         // The remote address to connect to.
	const GTByte * message,             // Initial message to send.  Can be NULL.
	int len,                            // Length of inital message.  Can be 0, or -1.
	int timeout,                        // Timeout, in milliseconds.  Can be 0, or -1.
	GTBool blocking,                    // If true, won't return until completed.
	GTConnectionCallbacks * callbacks   // Callbacks that get called when stuff happens.
);

// Closes a connection.  The connection
// cannot be used again after this call.
////////////////////////////////////////
void gtCloseConnection
(
	GTConnection connection             // The connection to close.
);

// Send a message on a connection.
// A len of -1 is equivalent to (strlen(message) + 1).
// An empty message will be sent if len is 0.
// Messages cannot be longer than 32767 bytes (32K - 1).
////////////////////////////////////////////////////////
void gtSend
(
	GTConnection connection,            // The connection to send the message on.
	const GTByte * message,             // The message to send.  Can be NULL.
	int len,                            // The length of the message to send, in bytes.  Can be 0 or -1.
	GTBool reliable                     // If true, the message is guaranteed to arrive.
);

/***********************
** LISTENER FUNCTIONS **
***********************/

// Start listening for connections from remote systems.
// localAddress = "[IP | hostname][:port]".
///////////////////////////////////////////////////////
GTListener gtListen
(
	const char * localAddress,          // The localAddress to bind to.  Can be NULL.
	GTListenerCallbacks * callbacks     // Callbacks for when stuff happens.
);

// Stops a listener.  The listener
// cannot be used again after this call.
////////////////////////////////////////
void gtStopListener
(
	GTListener listener            // The listener to stop.
);

// Accepts a connection attempt.
// The connected callback in the callbacks struct is ignored.
// If this connection was already closed by the remote
// system, then this call will return GTFalse, and this
// connection cannot be used.
/////////////////////////////////////////////////////////////
GTBool gtAccept
(
	GTConnection connection,            // The connection being accepted
	GTConnectionCallbacks * callbacks   // Callbacks called when stuff happens on the new connection.
);

// Rejects a connection attempt.
// A len of -1 is equivalent to (strlen(message) + 1).
//////////////////////////////////////////////////////
void gtReject
(
	GTConnection connection,       // The connection being rejected.
	const GTByte * message,        // Reject message (reason).  Can be NULL.
	int len                        // Length of reject message.  Can be 0, or -1.
);

/*********************
** FILTER FUNCTIONS **
*********************/

// Adds a filter to the connection's outgoing data.
// Returns GTFalse if there was an error adding the filter.
///////////////////////////////////////////////////////////
GTBool gtAddSendFilter
(
	GTConnection connection,       // The connection on which to add the filter.
	gtSendFilterCallback callback  // The callback the outgoing data is filtered through.
);

// Removes a filter from the connection's outgoing data.
////////////////////////////////////////////////////////
void gtRemoveSendFilter
(
	GTConnection connection,       // The connection on which to remove the filter.
	gtSendFilterCallback callback  // The callback to remove.
);

// Removes all of the connection's send filters.
////////////////////////////////////////////////
void gtRemoveAllSendFilters
(
	GTConnection connection  // The connection on which to remove all the filters.
);

// Called in response to a gtSendFilterCallback being called.
// It can be called from within the callback, or at any later time.
///////////////////////////////////////////////////////////////////
void gtFilteredSend
(
	GTConnection connection,  // The connection on which the message is being sent.
	int filterID,             // The ID passed to the gtSendFilterCallback.
	const GTByte * message,   // The message being sent. May be NULL.
	int len,                  // The length of the message being sent, in bytes. May be 0 or -1.
	GTBool reliable           // If the message should be sent reliably.
);

// Adds a filter to the connection's incoming data.
// Returns GTFalse if there was an error adding the filter.
///////////////////////////////////////////////////////////
GTBool gtAddReceiveFilter
(
	GTConnection connection,          // The connection on which to add the filter.
	gtReceiveFilterCallback callback  // The callback the incoming data is filtered through.
);

// Removes a filter from the connection's incoming data.
////////////////////////////////////////////////////////
void gtRemoveReceiveFilter
(
	GTConnection connection,          // The connection on which to remove the filter.
	gtReceiveFilterCallback callback  // The callback to remove.
);

// Removes all of the connection's receive filters.
///////////////////////////////////////////////////
void gtRemoveAllReceiveFilters
(
	GTConnection connection  // The connection on which to remove all the filters.
);

// Called in response to a gtReceiveFilterCallback being called.
// It can be called from within the callback, or at any later time.
///////////////////////////////////////////////////////////////////
void gtFilteredReceive
(
	GTConnection connection,       // The connection the message was received on.
	int filterID,                  // The ID passed to the gtReceiveFilterCallback.
	GTByte * message,              // The message that was received.  May be NULL.
	int len,                       // The length of the message in bytes.  May be 0.
	GTBool reliable                // True if this is a reliable message.
);

/********************
** OTHER FUNCTIONS **
********************/

// Returns GTTrue if the reliable channel for this connection
// has been established.  For connections initiated locally
// (with gtConnect), this will return GTFalse until the
// gtConnectedCallback callback is called.  If the connect
// attempt was successful, then this function will start
// returning GTTrue.  If this connection was accepted locally
// (with a listener), then this function will return GTFalse
// until gtAccept is called, then it will return GTTrue.
//////////////////////////////////////////////////////////////
GTBool gtIsReliableChannelEstablished
(
	GTConnection connection
);

// Returns GTTrue if an unreliable channel (UDP) has been
// established on this connection.  If it has not, unreliable
// data is sent over the reliable channel (TCP).
// This will always be GTFalse if the connection hasn't
// completed yet.
/////////////////////////////////////////////////////////////
GTBool gtIsUnreliableChannelEstablished
(
	GTConnection connection
);

// Returns GTTrue if this is a valid GTConnection object.
/////////////////////////////////////////////////////////
GTBool gtIsValidConnection
(
	GTConnection connection
);

// Returns GTTrue if this is a valid GTListener object.
///////////////////////////////////////////////////////
GTBool gtIsValidListener
(
	GTListener listener
);

// Gets the number of bytes of data waiting to be sent.
///////////////////////////////////////////////////////
int gtGetBufferedDataSize
(
	GTConnection connection         // The connection for which to get the number of bytes buffered.
);

// Gets the IP of the remote system
// to which a connection is connected,
// in network byte order.
// If the connection is not connected,
// this will return 0.
///////////////////////////////////////////
unsigned int gtGetRemoteIP
(
	GTConnection connection        // The connection for which to get the IP.
);

// Gets the port a listener is listening on.
////////////////////////////////////////////
unsigned short gtGetListenerPort
(
	GTListener listener            // The listener for which to get the port.
);

// This function gets an _approximate_ ping time for this connection.
// The ping is not suitable for user-display, and should only be used
// internally as a general performance measurement.
// The time is returned in milliseconds (ms).  -1 is returned if the
// unrealiable channel is not yet established.  This can be checked
// with gtIsUnreliableChannelEstablished.
int gtGetConnectionPing
(
	GTConnection connection        // The connection for which to get the ping.
);

// Gets user-data for the connection.
/////////////////////////////////////
void * gtGetConnectionData
(
	GTConnection connection             // The connection for which to get the data.
);

// Sets user-data for the connection.
/////////////////////////////////////
void gtSetConnectionData
(
	GTConnection connection,            // The connection for which to set the data.
	void * userData                     // The data.
);

// Gets user-data for the listener.
/////////////////////////////////////
void * gtGetListenerData
(
	GTListener listener            // The listener for which to get the data.
);

// Sets user-data for the listener.
///////////////////////////////////
void gtSetListenerData
(
	GTListener listener,           // The listener for which to set the data.
	void * userData                // The data.
);

// Converts an IP and a port into a text string.
// The IP must be in network byte order, and
// the port in host byte order.
// The string must be able to hold at least 22
// characters (including the NUL).
// "XXX.XXX.XXX.XXX:XXXXX"
// The string is returned.  If the string paramater
// is NULL, then an internal static string will be
// used.  There are two of these internal strings,
// so this call can be used for both addresses in
// gtConnect().
///////////////////////////////////////////////////
const char * gtAddressToString
(
	unsigned int ip,                // IP in network byte order.
	unsigned short port,            // Port in host byte order.
	char string[22]                 // String will be placed in here.  Can be NULL.
);

// Converts a string address into an IP and a port.
// The IP is stored in network byte order, and the
// port is stored in host byte order.
// If there is an error parsing the string,
// the function will return false.
// Possible string forms:
// NULL => all IPs, any port (localAddress only).
// "" => all IPs, any port (localAddress only).
// "1.2.3.4" => 1.2.3.4 IP, any port (localAddress only).
// "host.com" => host.com's IP, any port (localAddress only).
// ":2786" => all IPs, 2786 port (localAddress only).
// "1.2.3.4:0" => 1.2.3.4 IP, any port (localAddress only).
// "host.com:0" => host.com's IP, any port (localAddress only).
// "0.0.0.0:2786" => all IPs, 2786 port (localAddress only).
// "1.2.3.4:2786" => 1.2.3.4 IP, 2786 port (localAddress or remoteAddress).
// "host.com:2786" => host.com's IP, 2786 port (localAddress or remoteAddress).
///////////////////////////////////////////////////////////////////////////
GTBool gtStringToAddress
(
	const char * string,            // The string to convert.
	unsigned int * ip,              // The IP is stored here, in network byte order.  Can be NULL.
	unsigned short * port           // The port is stored here, in host byte order.  Can be NULL.
);

// Looks up the hostname for a given IP (in network byte order).
// The return string is a static string that must not be
// modified or freed.  It should be copied off if it will be
// used later.  The next call to this function will overwrite
// the string.  Returns NULL if no hostname can be found.
////////////////////////////////////////////////////////////////
const char * gtIPToHostname
(
	unsigned int ip
);

// Convert an unsigned int from network
// byte order into host byte order.
///////////////////////////////////////
unsigned int gtNetworkToHostInt
(
	unsigned int i                 // The int to convert.
);

// Convert an unsigned int from host
// byte order into newtork byte order.
//////////////////////////////////////
unsigned int gtHostToNetworkInt
(
	unsigned int i                 // The int to convert.
);

// Convert an unsigned short from network
// byte order into host byte order.
////////////////////////////////////////
unsigned short gtNetworkToHostShort
(
	unsigned short s               // The short to convert.
);

// Convert an unsigned short from host
// byte order into newtork byte order.
//////////////////////////////////////
unsigned short gtHostToNetworkShort
(
	unsigned short s               // The short to convert.
);

#ifdef _DEBUG
// This allows the application to
// get some basic debugging info.
//////////////////////////////////
void gtDebugInfo
(
	void (* callback)(const char * output)
);
#endif

#ifdef __cplusplus
}
#endif

#endif
