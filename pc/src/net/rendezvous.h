/* Co-op through the server, client side (docs/SERVER.md): room create/join, hole punching,
   relay fallback, end-to-end encryption. Runs on net.c's socket, driven from the game thread
   (coop_glue.c) or the CLI test client (pc/server/client.c). One session at a time. */
#ifndef RECVX_RENDEZVOUS_H
#define RECVX_RENDEZVOUS_H
#include "net.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct RdvConfig {
    int host;                   /* 1: create a room; 0: join `code` */
    NetAddr server;             /* the server's address, unless server_name is set */
    char server_name[128];      /* "host" or "host:port": looked up on a thread (rdv_frame), with a 5 s deadline and a cache */
    char code[12];              /* joining: the room code (normalized) */
    char password[64];          /* "" = none */
    int force_relay;            /* "Via server": no hole punching */
    int e2e;                    /* host: encrypt game datagrams end to end (always with a password); joiner: require it
                                   (a plaintext session only when both sides allow it) */
    int punch_ms;               /* hole punching before the relay (default 2000) */
    int no_punch;               /* test aid: drop received punches (a NAT punching cannot pass) */
    unsigned char rejoin[16];   /* joiner: our rejoin tag for this room (rdv_rejoin_tag; zero: none): our own stale place is ours again */
} RdvConfig;

enum { RDV_ST_OFF, RDV_ST_RESOLVE, RDV_ST_HELLO, RDV_ST_REQUEST, RDV_ST_WAITING, RDV_ST_PUNCHING, RDV_ST_DIRECT, RDV_ST_RELAY, RDV_ST_FAILED };

extern const NetAddr rdv_peer;  /* the address the game sees for the partner, whatever the path */

void rdv_start(const RdvConfig* c);
void rdv_stop(void);            /* tells the server (CLOSE) */
void rdv_kick(void);            /* host: removes the partner from the room (KICK); the room and its code stay, the session's keys go */
void rdv_frame(void);           /* timers: retries, keepalives, punches, the relay switch */
/* every received datagram: -1 taken here (server message, punch, dropped); 0 not ours (the
   caller handles it as before); > 0 a game datagram from the partner, decrypted into `buf`, with
   `from` set to rdv_peer */
int rdv_input(NetAddr* from, unsigned char* buf, int n, int cap);
int rdv_send(const void* buf, int n);   /* a game datagram to the partner (0 while there is no path yet) */
int rdv_active(void);
int rdv_state(void);
int rdv_overhead(void);         /* bytes the framing may add: game datagrams stay below 1200 - this */
int rdv_wire_extra(void);       /* bytes it adds now per datagram (encryption, relay header) */
const char* rdv_status(void);   /* for the status line */
const char* rdv_room(void);     /* host: the room code, "" before the server gave one */
const char* rdv_path(void);     /* "direct", "direct (LAN)", "relay", "" */
const char* rdv_join_code(void);/* the protocol-11 join code both games derive (NULL before the introduction) */
const char* rdv_error(void);    /* joining: why it failed (the session ends), NULL otherwise */
int rdv_server_parse(const char* s, NetAddr* out);   /* "host" or "host:port" (default 7141); 1 ok (blocks on DNS: the CLI) */
int rdv_server_valid(const char* s);                 /* the same, only the syntax (no DNS) */
int rdv_keys_confirmed(void);   /* the partner proved the same keys (punch MAC or an authenticated datagram) */
struct RdvSession;
const struct RdvSession* rdv_session_state(void);   /* tests: the session's keys and counters (NULL: none) */

#ifdef __cplusplus
}
#endif
#endif
