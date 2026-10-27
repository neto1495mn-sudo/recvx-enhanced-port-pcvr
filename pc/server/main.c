/* recvx-server: the co-op rendezvous and relay server (docs/SERVER.md). UDP, IPv4, no disk
   state, runs as any user. Settings: defaults < config file < RECVX_SERVER_PORT < command line;
   SIGHUP reads the file again (limits, log level, stats interval; port, listen and max_rooms need
   a restart). Linux. */
#include "server.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int socks[SRV_LISTEN_MAX], nsocks;
static volatile sig_atomic_t quit, reload;

static uint64_t now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

static void send_to(const RdvAddr* to, const uint8_t* buf, int n)
{
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    memcpy(&a.sin_addr, to->ip, 4);
    a.sin_port = htons(to->port);
    /* non-blocking: a full send buffer drops this datagram (counted) instead of stalling every room */
    if (sendto(socks[to->via < nsocks ? to->via : 0], buf, n, MSG_DONTWAIT, (struct sockaddr*)&a, sizeof(a)) != n) srv_proc.send_failed++;
}

/* the core's log buffer to stdout without blocking: a socket (the journal) with MSG_DONTWAIT,
   anything else only when poll says it is writable, at most PIPE_BUF at a time; what does not go
   now waits in the buffer (bounded: the core drops and counts lines when it is full) */
static void flush_log(void)
{
    int n;
    const char* p = srv_log_pending(&n);
    while (n > 0) {
        ssize_t w = send(1, p, n, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (w < 0 && errno == ENOTSOCK) {
            struct pollfd q = { 1, POLLOUT, 0 };
            if (poll(&q, 1, 0) != 1 || !(q.revents & POLLOUT)) return;
            w = write(1, p, n < 4096 ? n : 4096);
        }
        if (w <= 0) return;
        srv_log_done((int)w);
        p = srv_log_pending(&n);
    }
}

static void usage(FILE* f)
{
    fprintf(f, "recvx-server " RECVX_VERSION ": co-op rendezvous and relay server (docs/SERVER.md)\n"
               "usage: recvx-server [--config PATH] [-v] [--KEY VALUE ...]\n"
               "  --config PATH              the config file (default %s if it exists)\n"
               "  -v, --verbose              log requests, joins and relay changes too\n"
               "  -h, --help                 this text\n"
               "  --version                  print the version\n"
               "Every config key is also a flag (flags override the file):\n", SRV_CONFIG_PATH);
    srv_config_help(f);
}

static void on_signal(int s) { if (s == SIGHUP) reload = 1; else quit = 1; }

/* defaults, the file, the environment, the flags; the number of errors (printed) */
static int load(SrvConfig* c, int argc, char** argv, const char** path)
{
    const char* env = getenv("RECVX_SERVER_PORT");
    char msg[160];
    int i, errs = 0, warn = 0;
    srv_defaults(c);
    *path = NULL;
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--config") && i + 1 < argc) *path = argv[++i];
        else if (!strncmp(argv[i], "--config=", 9)) *path = argv[i] + 9;
    if (*path) errs += srv_config_file(c, *path, &warn, stderr) != 0;   /* named: it must be there and valid */
    else if (access(SRV_CONFIG_PATH, F_OK) == 0) {
        *path = SRV_CONFIG_PATH;
        errs += srv_config_file(c, *path, &warn, stderr) != 0;
    }
    if (env && srv_config_set(c, "port", env, msg, sizeof(msg)) != 1) { fprintf(stderr, "RECVX_SERVER_PORT: %s\n", msg); errs++; }
    for (i = 1; i < argc; i++) {
        char key[64];
        const char* a = argv[i];
        const char* val = NULL;
        int r;
        if (!strcmp(a, "--config")) { i++; continue; }
        if (!strncmp(a, "--config=", 9)) continue;
        if (!strcmp(a, "-v")) { c->verbose = 1; continue; }
        if (strncmp(a, "--", 2) || !a[2]) { fprintf(stderr, "unknown argument '%s' (--help)\n", a); errs++; continue; }
        snprintf(key, sizeof(key), "%.*s", (int)strcspn(a + 2, "="), a + 2);
        if (a[2 + strlen(key)] == '=') val = a + 3 + strlen(key);
        else if (!srv_config_is_bool(key)) {
            if (i + 1 >= argc) { fprintf(stderr, "%s: a value is missing\n", a); errs++; continue; }
            val = argv[++i];
        }
        r = srv_config_set(c, key, val, msg, sizeof(msg));
        if (r < 0) { fprintf(stderr, "unknown option %s (--help)\n", a); errs++; }   /* a typo on the command line: refuse */
        else if (!r) { fprintf(stderr, "%s\n", msg); errs++; }
    }
    return errs;
}

static int open_sockets(const SrvConfig* c)
{
    int i, n = c->nlisten ? c->nlisten : 1;
    for (i = 0; i < n; i++) {
        struct sockaddr_in a;
        memset(&a, 0, sizeof(a));
        a.sin_family = AF_INET;
        if (c->nlisten) memcpy(&a.sin_addr, c->listen[i], 4);
        a.sin_port = htons((uint16_t)c->port);
        socks[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socks[i] >= 0) fcntl(socks[i], F_SETFL, fcntl(socks[i], F_GETFL) | O_NONBLOCK);
        if (socks[i] < 0 || bind(socks[i], (struct sockaddr*)&a, sizeof(a)) != 0) {
            fprintf(stderr, "cannot listen on %s:%d (UDP): %s\n", inet_ntoa(a.sin_addr), c->port, strerror(errno));
            return 0;
        }
        nsocks = i + 1;
    }
    return 1;
}

static const char* listen_str(const SrvConfig* c)
{
    static char s[SRV_LISTEN_MAX * 24];
    int i, k = 0;
    if (!c->nlisten) snprintf(s, sizeof(s), "0.0.0.0:%d", c->port);
    for (i = 0; i < c->nlisten; i++)
        k += snprintf(s + k, sizeof(s) - k, "%s%u.%u.%u.%u:%d", i ? ", " : "", c->listen[i][0], c->listen[i][1], c->listen[i][2], c->listen[i][3], c->port);
    return s;
}

static void describe(const SrvConfig* c, const char* path, const char* what)
{
    char total[64], stats[32];
    snprintf(total, sizeof(total), "%d KB/s and %d datagrams/s", c->total_kbps, c->total_pps);   /* 0: no limit */
    snprintf(stats, sizeof(stats), c->stats_s ? "every %d s" : "off", c->stats_s);
    srv_logf("recvx-server " RECVX_VERSION ": %s: protocol %d, config %s, listening on %s, up to %d rooms (%d per address), relay per player %d KB/s and %d "
             "datagrams/s, per address %d KB/s and %d datagrams/s, all relayed traffic %s (0 = no limit), %d requests/s and %d failed "
             "joins/min per address, room time-out %d s, member time-out %d s, code grace %d s, log %s, metrics line %s",
             what, RDV_VERSION, path ? path : "none", listen_str(c), c->max_rooms, c->rooms_per_ip, c->relay_kbps, c->relay_pps, c->ip_kbps,
             c->ip_pps, total, c->req_per_sec, c->joins_per_min, c->room_timeout_s, c->member_timeout_s, c->grace_s, c->verbose ? "verbose" : "normal", stats);
}

static void proc_info(uint64_t start)
{
    struct rusage u;
    FILE* f;
    long pages = 0, rss = 0;
    srv_proc.uptime_s = (now_ms() - start) / 1000.0;
    if (getrusage(RUSAGE_SELF, &u) == 0) {
        srv_proc.cpu_user_s = u.ru_utime.tv_sec + u.ru_utime.tv_usec / 1e6;
        srv_proc.cpu_sys_s = u.ru_stime.tv_sec + u.ru_stime.tv_usec / 1e6;
        srv_proc.rss_kb = u.ru_maxrss;   /* the peak, if /proc is not there */
    }
    if ((f = fopen("/proc/self/statm", "r"))) {
        if (fscanf(f, "%ld %ld", &pages, &rss) == 2) srv_proc.rss_kb = rss * (sysconf(_SC_PAGESIZE) / 1024);
        fclose(f);
    }
}

int main(int argc, char** argv)
{
    SrvConfig c;
    const char* path;
    int i;
    uint64_t start = now_ms(), last_tick = 0, last_stats;
    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) { usage(stdout); return 0; }
        else if (!strcmp(argv[i], "--version")) { puts("recvx-server " RECVX_VERSION); return 0; }
    if (load(&c, argc, argv, &path)) { fprintf(stderr, "recvx-server: not started (configuration errors above)\n"); return 2; }
    if (!srv_init(&c, send_to)) { fprintf(stderr, "out of memory or no randomness\n"); return 1; }
    if (!open_sockets(&c)) return 1;
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGHUP, on_signal);
    describe(&c, path, "started");
    flush_log();
    proc_info(start);
    last_stats = now_ms();
    while (!quit) {
        struct pollfd p[SRV_LISTEN_MAX];
        uint64_t now;
        for (i = 0; i < nsocks; i++) { p[i].fd = socks[i]; p[i].events = POLLIN; p[i].revents = 0; }
        if (poll(p, nsocks, 1000) > 0)
            for (i = 0; i < nsocks; i++) {
                int k;
                if (!(p[i].revents & POLLIN)) continue;
                for (k = 0; k < 256; k++) {   /* a batch, then the timers */
                    uint8_t buf[RDV_DGRAM_MAX + 1];
                    struct sockaddr_in f;
                    socklen_t fl = sizeof(f);
                    RdvAddr from;
                    int n = (int)recvfrom(socks[i], buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&f, &fl);
                    if (n < 0) break;
                    memset(&from, 0, sizeof(from));
                    from.fam = 4;
                    memcpy(from.ip, &f.sin_addr, 4);
                    from.port = ntohs(f.sin_port);
                    from.via = (uint8_t)i;
                    srv_input(&from, buf, n > RDV_DGRAM_MAX ? RDV_DGRAM_MAX + 1 : n, now_ms());
                }
            }
        now = now_ms();
        if (now - last_tick >= 1000) { proc_info(start); srv_tick(now); last_tick = now; }
        if (c.stats_s > 0 && now - last_stats >= (uint64_t)c.stats_s * 1000) {
            char t[2048];
            srv_summary(t, sizeof(t));
            srv_logf("stats: %s", t);
            last_stats = now;
        }
        flush_log();   /* between batches: never inside packet processing */
        if (reload) {   /* SIGHUP: the file again; a bad file keeps the running settings */
            SrvConfig n;
            const char* np;
            reload = 0;
            if (load(&n, argc, argv, &np)) { srv_logf("recvx-server: reload failed (errors on stderr), the settings stay as they were"); continue; }
            if (n.port != c.port || n.nlisten != c.nlisten || memcmp(n.listen, c.listen, sizeof(c.listen)) || n.max_rooms != c.max_rooms)
                srv_logf("recvx-server: port, listen and max_rooms keep their values until a restart");
            n.port = c.port; n.nlisten = c.nlisten; memcpy(n.listen, c.listen, sizeof(c.listen)); n.max_rooms = c.max_rooms;
            c = n;
            srv_reconfig(&c);
            describe(&c, np, "reloaded");
        }
    }
    srv_logf("recvx-server: stopped");
    flush_log();
    srv_free();
    return 0;
}
