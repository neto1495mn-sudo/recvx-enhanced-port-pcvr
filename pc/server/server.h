/* recvx-server core (docs/SERVER.md): rooms, codes, introductions, relay, limits, metrics,
   the config file. No sockets and no clock of its own: main.c feeds datagrams and the time,
   pc/tests/test_rdv.c does the same. */
#ifndef RECVX_SERVER_H
#define RECVX_SERVER_H
#include "../src/net/rdv_proto.h"
#include <stdio.h>

#define SRV_LISTEN_MAX 8
#define SRV_CONFIG_PATH "/etc/recvx-server.conf"

typedef struct SrvConfig {
    /* the config file keys (srv_config_*; recvx-server.conf.example explains each) */
    int port;               /* UDP port */
    int nlisten;            /* local IPv4 addresses to listen on; 0 = all */
    uint8_t listen[SRV_LISTEN_MAX][4];
    int max_rooms;          /* open rooms at once (1..65535); a restart applies a change */
    int rooms_per_ip;       /* open rooms one IP address may host */
    int req_per_sec;        /* requests per IP and second (HELLO, CREATE, JOIN, KEEPALIVE, CLOSE) */
    int joins_per_min;      /* failed joins (unknown code, wrong password) per IP and minute */
    int creates_per_min;    /* new rooms (CREATE) per IP and minute: churn fills the code grace table */
    int req_total_per_sec;  /* requests from all addresses together, per second (admission: floods, spoofed sources) */
    int joins_total_per_min;/* failed joins from all addresses together, per minute (guessing with many addresses) */
    int relay_kbps;         /* per player: KB/s (1024 bytes) relayed; above it datagrams are dropped and logged */
    int relay_pps;          /* per player: datagrams per second relayed */
    int ip_kbps, ip_pps;    /* per IP address, all its players and rooms; kept across rooms and members (churn) */
    int total_kbps;         /* all relayed traffic, KB/s (0 = no limit) */
    int total_pps;          /* all relayed datagrams per second (0 = no limit) */
    int room_timeout_s;     /* a room without the host's keepalive closes */
    int member_timeout_s;   /* a joiner without keepalive or relay traffic leaves (its place is free for a rejoin) */
    int grace_s;            /* a closed room's code is not handed out again for this long */
    int verbose;            /* log level: 0 normal, 1 verbose (per-request and per-room events) */
    int stats_s;            /* the metrics summary line every this many seconds (0 = off) */
} SrvConfig;
void srv_defaults(SrvConfig* c);

/* config file / command line (config.c). Each returns the number of errors (0 = fine) and prints
   them, and warnings for unknown keys, to `log` (NULL: stderr); *warnings counts the warnings. */
int srv_config_text(SrvConfig* c, const char* text, const char* source, int* warnings, FILE* log);
int srv_config_file(SrvConfig* c, const char* path, int* warnings, FILE* log);   /* -1: cannot read it */
/* one setting, by key ("relay_kb_per_sec") or flag spelling ("relay-kb-per-sec"); val NULL for a
   bare flag (booleans). 1 ok, 0 bad value (msg says why), -1 unknown key. */
int srv_config_set(SrvConfig* c, const char* key, const char* val, char* msg, int cap);
int srv_config_is_bool(const char* key);
void srv_config_help(FILE* f);   /* the keys, their flags and defaults */

typedef struct SrvStats {
    /* counters since the start */
    unsigned long long requests;                    /* everything but RELAY that parsed */
    unsigned long long relay_in_dgrams, relay_in_bytes, relay_out_dgrams, relay_out_bytes;
    unsigned long long drop_rate, drop_pps, drop_bw, drop_ip, drop_total, drop_invalid, drop_unknown_room, drop_foreign;
    unsigned long long drop_relay_auth, drop_replay, drop_busy;   /* RELAY with a wrong MAC / a repeated counter; the address table or randomness refused */
    unsigned long long log_suppressed;
    unsigned created, closed, expired, joined, join_failed, code_retries;
    unsigned rejoined, kicked, member_expired;      /* a joiner took its own stale place back; the host removed its partner; a joiner timed out */
    unsigned sessions_direct, sessions_relayed;     /* ended or still running; relayed = used the relay at all */
    /* gauges */
    unsigned rooms, members;                        /* open rooms; rooms with a partner (players = rooms + members) */
    double req_rate, relay_in_rate, relay_out_rate; /* per second, over the last ~10 s */
} SrvStats;

/* filled by main.c once a second (the core has no clock and no OS calls) */
typedef struct SrvProc { double uptime_s, cpu_user_s, cpu_sys_s; long rss_kb; unsigned long long send_failed; } SrvProc;
extern SrvProc srv_proc;

typedef void (*SrvSend)(const RdvAddr* to, const uint8_t* buf, int n);
int srv_init(const SrvConfig* c, SrvSend send);   /* 0: out of memory / no randomness */
void srv_reconfig(const SrvConfig* c);            /* SIGHUP: new limits and log level (not max_rooms, port, listen) */
void srv_free(void);
void srv_input(const RdvAddr* from, const uint8_t* buf, int n, uint64_t now_ms);
void srv_tick(uint64_t now_ms);                   /* at least once a second: time-outs, rates */
const SrvStats* srv_stats(void);
int srv_status_text(char* out, int cap);          /* the metrics, "name value" lines (Prometheus text format) */
int srv_summary(char* out, int cap);              /* the metrics in one log line */

/* test hook (the fuzz target): the cookie this server gives address `a` now, so mutated CREATE /
   JOIN requests get past the cookie check into the room logic */
void srv_test_cookie(const RdvAddr* a, uint64_t now, uint8_t out[16]);
/* test hook: the randomness room codes are made from (NULL: the OS's) */
extern void (*srv_code_rng)(uint8_t out[RDV_CODE_LEN]);
/* The log. With srv_log set (tests) lines go there at once; otherwise into a bounded buffer that
   main.c writes out between packet batches without blocking (srv_log_pending / srv_log_done), so a
   stalled log reader can never stall the relay: when the buffer is full, lines are dropped and
   counted. srv_logf: a line outside the event budget (start, reload, the metrics line). */
extern FILE* srv_log;
void srv_logf(const char* fmt, ...);
const char* srv_log_pending(int* n);
void srv_log_done(int n);
#endif
