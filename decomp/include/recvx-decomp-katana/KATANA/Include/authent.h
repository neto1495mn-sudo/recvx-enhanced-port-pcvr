//------------------------------------------------------------------------
//	DREAMCAST DREAMARENA AUTHENTICATION LIBRARY (Client Side)
//
//  Author : Thomas Szirtes
//	
//  Copyright Sega Europe Ltd, 2000. All rights reserved.
//  Confidential. This code is for internal use only, and should not be
//  distributed to external developers.
//
//
//	Version : 2.1   
//	(based on v2.1 of Integrated Online Gaming authentication service
//	 document)
//
//	Notes :
//  Developed with Shinobi library set (1.60) with SHC compiler
//	Written using Codewright.
//
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//
//
//	Defines
//
//
//--------------------------------------------------------------------------


// Possible service types
#define	AUTHENT_SERVICE_GAMECONNECT		"gmsv"
#define	AUTHENT_SERVICE_LOBBY			"lobb"
#define	AUTHENT_SERVICE_CHAT			"chat"
#define	AUTHENT_SERVICE_GAMESPY			"gmsp"


//--------------------------------------------------------------------------
//
//
//	Type Definitions
//
//
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//	Enumerated Types
//--------------------------------------------------------------------------

typedef enum {
	AER_SYSTEM_ERROR=0,				// A system error has occurred (like memory failure)
	AER_CONNECT_FAIL=1,				// Was unable to connect to server (maybe due to timeout)
	AER_NOT_REGISTERED=2,			// Dreamcast has not been registered with DreamArena yet
	AER_LOGIN_FAIL=3,				// Bad password was entered too many times
	AER_BAD_PROTCOL=4,				// Client/Server protocol version different
	AER_NO_SERVERS=5,				// No servers could be found for this game
	AER_NO_AVAIL_SERVERS=6,			// Servers could be found but they are all unavailable at present
	AER_TIMEOUT=7,
	AER_UNDEFINED					// Dunno! (usually dodgy server)
} AUTHENT_ERROR;

typedef enum {
	AUTHENT_STAT_OK,								// Everything okay
	AUTHENT_STAT_BUSY,								// Still working - poll authent again
	AUTHENT_STAT_FAILED,							// Something went wrong check AUTHENT_ERROR for reason
	AUTHENT_STAT_COMPLETE,							// Completed successfully no need to call authent again
	AUTHENT_STAT_BADPARAM,							// Bad parameter passed
	AUTHENT_STAT_ACTION_USERNAME,					// Password must be entered
	AUTHENT_STAT_ACTION_GAMERNAME_NEW,				// Gamer name must be entered for first time
	AUTHENT_STAT_ACTION_GAMERNAME_CHECK,			// Gamer name needs to be checked
	AUTHENT_STAT_ACTION_GAMERNAME_SUGGEST,			// Gamer name already taken - server has suggested other
	AUTHENT_STAT_ACTION_INFOPACK,					// Infopack has been returned
	AUTHENT_STAT_ACTION_SERVER_REQUEST,			// Please request a server
	AUTHENT_STAT_ACTION_SERVER_RETURN,				// Server has been returned
	AUTHENT_STAT_TERMINATED							// authent has already finished
} AUTHENT_STAT;




//--------------------------------------------------------------------------
//	Simple Types
//--------------------------------------------------------------------------

// Game identifier
typedef	char*	AUTHENT_GAMEID;



//--------------------------------------------------------------------------
//	Callback StructuredTypes
//--------------------------------------------------------------------------

// Userlist callback structure - generated when server returns a list of names for user to
// chose from
typedef struct {
	int			in_numusers;		// number of users provided between 1 and 4
	char		in_users[4][20]; 	// 4 User names to select from
	int			out_userselect;		// * put in here the user selected
	char		out_password[10];	// * put in here the password for that user
} AUTHENT_USERLIST_CBSTR;


// Gamer name callback
typedef struct {
	char		in_gamername[20];
	char		out_gamername[20];
} AUTHENT_GAMERNAME_CBSTR;

typedef struct {
	char		in_shortname[16];
	char		in_userid[10];
	char		in_preferredlanguage[3];
	char		in_countryid[2];
	char		in_age[3];
	char		in_chatname[20];
	char		in_gamername[20];
	char		in_emailaddress[65];
} AUTHENT_INFOPACK_CBSTR;

typedef struct {
	char*		in_servicetype;				// service type
	char*		in_description;				// description string
	char*		in_address;					// IP address
	char*		in_port;					// port
} AUTHENT_SERVERENUM_CBSTR;




//--------------------------------------------------------------------------
//
//
//		PROTOTYPES FOR NETWORK BINDING INTERFACE
//
//
//--------------------------------------------------------------------------

typedef enum { AU_NET_OK, AU_NET_ERROR, AU_NET_NOTCOMPLETE } AUTHENTNET_STAT;

AUTHENTNET_STAT	authentnet_Receive( char **buffer );

AUTHENTNET_STAT	authentnet_Send( char *buffer );

AUTHENTNET_STAT	authentnet_Connect( char *ipaddress, unsigned short portnumber);

AUTHENTNET_STAT	authentnet_Disconnect( void );



//--------------------------------------------------------------------------
//
//
//		PUBLIC API FUNCTIONS
//
//
//--------------------------------------------------------------------------

AUTHENT_STAT	ntAuInit(
							char*				 	service_type,
							char*					game_identifier,
							AUTHENT_USERLIST_CBSTR	*userlist_str,
							AUTHENT_GAMERNAME_CBSTR	*gamername_str,
							AUTHENT_INFOPACK_CBSTR	*infopack_str
						 );

AUTHENT_STAT	ntAuTotalServers( int *num_servers );

int				ntAuRequestServers( int max_servers,
									AUTHENT_SERVERENUM_CBSTR *serverenum_str );


AUTHENT_STAT	ntAuComplete( void );

void			ntAuTerminate( void );

AUTHENT_ERROR	ntAuGetError( void );


// Non-blocking Asynchronous call
AUTHENT_STAT	ntAuPoll( void );
