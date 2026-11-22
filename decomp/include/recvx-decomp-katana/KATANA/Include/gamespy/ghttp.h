 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2000 GameSpy Industries, Inc

Suite E-204
2900 Bristol Street
Costa Mesa, CA 92626
(714)549-7689
Fax(714)549-0757
*/

#ifndef _GHTTP_H_
#define _GHTTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

// Boolean.
///////////
typedef enum
{
	GHTTPFalse,
	GHTTPTrue
} GHTTPBool;

// The current state of an http request.
////////////////////////////////////////
typedef enum
{
	GHTTPHostLookup,            // Resolving hostname to IP.
	GHTTPConnecting,            // Waiting for socket connect to complete.
	GHTTPSendingRequest,        // Sending the request.
	GHTTPWaiting,               // Waiting for a response.
	GHTTPReceivingStatus,       // Receiving the response status.
	GHTTPReceivingHeaders,      // Receiving the headers.
	GHTTPReceivingFile          // Receiving the file.
} GHTTPState;

// The result of an http request.
/////////////////////////////////
typedef enum
{
	GHTTPSuccess,               // Successfully retrieved file.
	GHTTPOutOfMemory,           // A memory allocation failed.
	GHTTPBufferOverflow,        // The user-supplied buffer was too small to hold the file.
	GHTTPParseURLFailed,        // There was an error parsing the URL.
	GHTTPHostLookupFailed,      // Failed looking up the hostname.
	GHTTPSocketFailed,          // Failed to create/initialize/read/write a socket.
	GHTTPConnectFailed,         // Failed connecting to the http server.
	GHTTPBadResponse,           // Error understanding a response from the server.
	GHTTPRequestRejected,       // The request has been rejected by the server.
	GHTTPUnauthorized,          // Not authorized to get the file.
	GHTTPForbidden,             // The server has refused to send the file.
	GHTTPFileNotFound,          // Failed to find the file on the server.
	GHTTPServerError,           // The server has encountered an internal error.
	GHTTPFileWriteFailed        // An error occured writing to the local file (for ghttpSaveFile[Ex]).
} GHTTPResult;

// Represents an http file request.
///////////////////////////////////
typedef int GHTTPRequest;

// Called with updates on the current state of the request.
// The buffer should not be accessed once this callback returns.
// If ghttpGetFile[Ex] was used, buffer contains all of the data that has been 
//   received so far, and bufferSize is the total number of bytes received.
// If ghttpSaveFile[Ex] was used, buffer only contains the most recent data
//   that has been received.  This same data is saved to the file.  The buffer
//   will not be valid after this callback returns.
// If ghttpStreamFileEx was used, buffer only contains the most recent data
//   that has been received.  This data will be lost once the callback
//   returns, and should be copied if it needs to be saved.  bufferSize
//   is the number of bytes in the current block of data.
//////////////////////////////////////////////////////////////////////////////
typedef void (* ghttpProgressCallback)
(
	GHTTPRequest request,       // The request.
	GHTTPState state,           // The current state of the request.
	const char * buffer,        // The file's bytes so far, NULL if state<GHTTPReceivingFile.
	int bufferLen,              // The number of bytes in the buffer, 0 if state<GHTTPReceivingFile.
	int bytesReceived,          // The total number of bytes receivied, 0 if state<GHTTPReceivingFile.
	int totalSize,              // The total size of the file, -1 if unknown.
	void * param                // User-data.
);

// Called when the entire file has been received.
// If ghttpStreamFileEx or ghttpSaveFile[Ex] was used,
//   buffer is NULL, bufferLen is the number of bytes
//   in the file, and the return value is ignored.
// If ghttpGetFile[Ex] was used, return true to have
//   the buffer freed, false if the app will free the
//   buffer.  If true, the buffer cannot be accessed
//   once the callback returns.  If false, the app can
//   use the buffer even after this call returns, but
//   must free it at some later point.  There will always
//   be a file, even if there was an error, although for
//   errors it may be an empty file.
////////////////////////////////////////////////////////
typedef GHTTPBool (* ghttpCompletedCallback)
(
	GHTTPRequest request,       // The request.
	GHTTPResult result,         // The result (success or an error).
	char * buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
	int bufferLen,              // The file's length.
	void * param                // User-data.
);

// Cleans up any resources being used by this library.
// This should be called once at program exit, but
// it can be called at any time.  It will cancel any
// outstanding file requests.
//////////////////////////////////////////////////////
void ghttpCleanup
(
	void
);

// Get a file from an http server.
// Returns -1 if an error occurs.
//////////////////////////////////
GHTTPRequest ghttpGetFile
(
	const char * URL,           // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Get a file from an http server.
// Returns -1 if an error occurs.
// Allows an optional user-supplied buffer to be used,
// optional extra http headers,
// and an optional progress callback.
// The optional headers must be 0 or more HTTP headers,
//   each terminated by a CR-LF pair (0xD, 0xA).
// If using a user-supplied buffer:
//   set buffer to the buffer to use,
//   set bufferSize to the size of the buffer in bytes.
// To have the library allocate a buffer:
//   set buffer to NULL, set bufferSize to 0
///////////////////////////////////////////////////////
GHTTPRequest ghttpGetFileEx
(
	const char * URL,           // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * headers,       // Optional headers to pass with the request.  Can be NULL or "".
	char * buffer,              // Optional user-supplied buffer.  Set to NULL to have one allocated.
	int bufferSize,             // The size of the user-supplied buffer in bytes.  0 if buffer is NULL.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpProgressCallback progressCallback,    // Called periodically with progress updates.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Gets a file and saves it to disk.
// Returns -1 if an error occurs.
////////////////////////////////////
GHTTPRequest ghttpSaveFile
(
	const char * URL,           // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * filename,      // The path and name to store the file as locally.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Gets a file and saves it to disk.
// Returns -1 if an error occurs.
// Allows optional extra http headers and
// an optional progress callback.
/////////////////////////////////////////
GHTTPRequest ghttpSaveFileEx
(
	const char * URL,           // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * filename,      // The path and name to store the file as locally.
	const char * headers,       // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpProgressCallback progressCallback,    // Called periodically with progress updates.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Streams a file from an http server.
// Returns -1 if an error occurs.
// Allows optional extra http headers.
//////////////////////////////////////
GHTTPRequest ghttpStreamFileEx
(
	const char * URL,           // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * headers,       // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPBool blocking,         // If true, this call doesn't return until the file has finished streaming.
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
);

// Processes all current http request.
//////////////////////////////////////
void ghttpThink
(
	void
);

// Cancels the request.
///////////////////////
void ghttpCancelRequest
(
	GHTTPRequest request
);

// Gets the current state of this request.
//////////////////////////////////////////
GHTTPState ghttpGetState
(
	GHTTPRequest request
);

// Gets the status code and status string for a request.
// A pointer to the status string is returned, or NULL on error.
// Only valid if the GHTTPState for this request
// is greater than GHTTPReceivingStatus.
////////////////////////////////////////////////////////////////
const char * ghttpGetResponseStatus
(
	GHTTPRequest request,       // The request to get the response state of.
	int * statusCode            // If not NULL, the status code is stored here.
);

// Gets headers returned by the http server.
// Only valid if the GHTTPState for this
// request is GHTTPReceivingFile.
////////////////////////////////////////////
const char * ghttpGetHeaders
(
	GHTTPRequest request
);

// Gets the URL for a given request.
////////////////////////////////////
const char * ghttpGetURL
(
	GHTTPRequest request
);

#ifdef __cplusplus
}
#endif

#endif
