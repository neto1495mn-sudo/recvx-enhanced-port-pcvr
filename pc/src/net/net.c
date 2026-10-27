/* Non-blocking UDP (BSD sockets / Winsock2), see net.h. */
#include "net.h"
#ifndef RECVX_NET_NO_SDL
#include <SDL3/SDL.h>
#else
#include <time.h>                       /* the CLI test client (pc/server/client.c): no SDL */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define BAD_SOCK INVALID_SOCKET
#define closesock closesocket
static void set_nonblock(sock_t s) { u_long on = 1; ioctlsocket(s, FIONBIO, &on); }
static int net_startup(void)
{
    static int done;
    WSADATA wd;
    if (!done && WSAStartup(MAKEWORD(2, 2), &wd) == 0) done = 1;
    return done;
}
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
typedef int sock_t;
#define BAD_SOCK (-1)
#define closesock close
static void set_nonblock(sock_t s) { fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); }
static int net_startup(void) { return 1; }
#endif

static sock_t sock = BAD_SOCK;
static uint64_t rx_us;                  /* net_rx_us */

#ifdef _WIN32
/* Arrival times on Windows: a thread waits for the socket's FD_READ event and notes when data came;
   it reads nothing (the game thread receives). Winsock posts the event when data arrives and again
   after a recv that leaves data queued, so the thread checks that data is queued, and a note taken
   before the game thread's last empty read (drain_end) is not used. (Winsock's own receive
   timestamps, SIO_TIMESTAMPING from Windows 10 2004, depend on the network driver.) */
static WSAEVENT ts_ev = WSA_INVALID_EVENT;
static HANDLE ts_thread;
static volatile LONG ts_stop;
static uint64_t ts_stamp, drain_end;    /* the thread's note (atomic, 0: none); the game thread's last empty read */
static int drained = 1;                 /* the next datagram is the first after an empty read */
static DWORD WINAPI ts_main(LPVOID unused)
{
    (void)unused;
    while (WaitForSingleObject(ts_ev, INFINITE) == WAIT_OBJECT_0 && !ts_stop) {
        uint64_t t = net_us(), none = 0;   /* before the check: never later than the data's arrival + the check */
        u_long n = 0;
        WSAResetEvent(ts_ev);
        if (ioctlsocket(sock, FIONREAD, &n) == 0 && n > 0)
            __atomic_compare_exchange_n(&ts_stamp, &none, t, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
    }
    return 0;
}
static void ts_start(void)
{
    ts_stop = 0;
    __atomic_store_n(&ts_stamp, 0, __ATOMIC_RELAXED);
    drain_end = 0;
    drained = 1;
    if ((ts_ev = WSACreateEvent()) == WSA_INVALID_EVENT) return;
    if (WSAEventSelect(sock, ts_ev, FD_READ) != 0 || !(ts_thread = CreateThread(NULL, 0, ts_main, NULL, 0, NULL))) {
        WSACloseEvent(ts_ev);
        ts_ev = WSA_INVALID_EVENT;
    }
}
static void ts_end(void)
{
    if (ts_thread) {
        ts_stop = 1;
        WSASetEvent(ts_ev);
        WaitForSingleObject(ts_thread, INFINITE);
        CloseHandle(ts_thread);
        ts_thread = NULL;
    }
    if (ts_ev != WSA_INVALID_EVENT) WSACloseEvent(ts_ev);
    ts_ev = WSA_INVALID_EVENT;
}
#endif

int net_open(int port)
{
    struct sockaddr_in a;
    net_close();
    if (!net_startup()) return 0;
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == BAD_SOCK) return 0;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons((uint16_t)port);
    if (bind(sock, (struct sockaddr*)&a, sizeof(a)) != 0) {
#ifdef _WIN32
        int e = WSAGetLastError(), r = e == WSAEADDRINUSE ? NET_IN_USE : e == WSAEACCES ? NET_DENIED : 0;
#else
        int r = errno == EADDRINUSE ? NET_IN_USE : errno == EACCES ? NET_DENIED : 0;
#endif
        net_close();
        return r;
    }
    set_nonblock(sock);
#ifdef _WIN32
    {   /* an ICMP "port unreachable" (a partner that left) would fail the next recvfrom with WSAECONNRESET,
           every frame while we still send there, and net_recv's caller stops reading at the first failure */
        BOOL off = FALSE;
        DWORD n;
        WSAIoctl(sock, _WSAIOW(IOC_VENDOR, 12) /* SIO_UDP_CONNRESET */, &off, sizeof(off), NULL, 0, &n, NULL, NULL);
    }
    ts_start();
#else
    {   /* the kernel's receive time of every datagram (net_rx_us) */
        int on = 1;
        setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS, &on, sizeof(on));
    }
#endif
    return 1;
}

void net_close(void)
{
#ifdef _WIN32
    ts_end();
#endif
    if (sock != BAD_SOCK) closesock(sock);
    sock = BAD_SOCK;
}

int net_resolve(const char* host, int port, NetAddr* out)
{
    struct addrinfo hints, *res = NULL;
    if (!net_startup()) return 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res) return 0;
    out->ip = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
    out->port = htons((uint16_t)port);
    freeaddrinfo(res);
    return 1;
}

struct NetResolve {
    char host[128];
    int port, state, refs;      /* state 0 busy, 1 ok, -1 failed (atomic); refs: the thread and the caller */
    NetAddr addr;
};

static void resolve_release(NetResolve* r) { if (__atomic_sub_fetch(&r->refs, 1, __ATOMIC_ACQ_REL) == 0) free(r); }

#ifdef _WIN32
static DWORD WINAPI resolve_thread(LPVOID p)
#else
static void* resolve_thread(void* p)
#endif
{
    NetResolve* r = p;
    NetAddr a = { 0, 0 };
    int ok = net_resolve(r->host, r->port, &a);
    r->addr = a;
    __atomic_store_n(&r->state, ok ? 1 : -1, __ATOMIC_RELEASE);
    resolve_release(r);
    return 0;
}

NetResolve* net_resolve_start(const char* host, int port)
{
    NetResolve* r = calloc(1, sizeof(*r));
    if (!r || !net_startup()) { free(r); return NULL; }
    snprintf(r->host, sizeof(r->host), "%s", host);
    r->port = port;
    r->refs = 2;
#ifdef _WIN32
    {
        HANDLE t = CreateThread(NULL, 0, resolve_thread, r, 0, NULL);
        if (!t) { free(r); return NULL; }
        CloseHandle(t);
    }
#else
    {
        pthread_t t;
        pthread_attr_t at;
        pthread_attr_init(&at);
        pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&t, &at, resolve_thread, r)) { pthread_attr_destroy(&at); free(r); return NULL; }
        pthread_attr_destroy(&at);
    }
#endif
    return r;
}

int net_resolve_poll(NetResolve* r, NetAddr* out)
{
    int s = __atomic_load_n(&r->state, __ATOMIC_ACQUIRE);
    if (!s) return 0;
    if (s > 0) *out = r->addr;
    resolve_release(r);
    return s;
}

void net_resolve_cancel(NetResolve* r) { if (r) resolve_release(r); }

int net_send(const NetAddr* to, const void* buf, int len)
{
    struct sockaddr_in a;
    if (sock == BAD_SOCK) return 0;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = to->ip;
    a.sin_port = to->port;
    return sendto(sock, (const char*)buf, len, 0, (struct sockaddr*)&a, sizeof(a)) == len;
}

int net_recv(NetAddr* from, void* buf, int cap)
{
    struct sockaddr_in a;
    int n;
    if (sock == BAD_SOCK) return -1;
#ifdef _WIN32
    {
        int al = sizeof(a);
        uint64_t s;
        int e, tries = 0;
        /* a datagram too big for buf (WSAEMSGSIZE: its rest is discarded) or an ICMP error for an
           earlier send (WSAECONNRESET / WSAENETRESET): skip it, the next one may be ours */
        while ((n = recvfrom(sock, (char*)buf, cap, 0, (struct sockaddr*)&a, &al)) < 0 && ++tries < 64 &&
               ((e = WSAGetLastError()) == WSAEMSGSIZE || e == WSAECONNRESET || e == WSAENETRESET))
            al = sizeof(a);
        if (n < 0) {   /* nothing queued (or a failure): what the thread notes from now on is new */
            drain_end = net_us();
            __atomic_store_n(&ts_stamp, 0, __ATOMIC_RELEASE);
            drained = 1;
            return -1;
        }
        s = drained ? __atomic_exchange_n(&ts_stamp, 0, __ATOMIC_ACQUIRE) : 0;
        rx_us = s && s >= drain_end ? s : net_us();
        drained = 0;
    }
#else
    {
        struct timespec ts = { 0, 0 };
        union { char b[CMSG_SPACE(sizeof(struct timespec))]; struct cmsghdr align; } ctl;
        struct iovec io = { buf, (size_t)cap };
        struct msghdr mh;
        struct cmsghdr* c;
        int tries = 0;
        memset(&mh, 0, sizeof(mh));
        mh.msg_name = &a;
        mh.msg_namelen = sizeof(a);
        mh.msg_iov = &io;
        mh.msg_iovlen = 1;
        mh.msg_control = ctl.b;
        mh.msg_controllen = sizeof(ctl.b);
        /* an ICMP error for an earlier send (ECONNREFUSED: a peer not yet listening): skip it */
        while ((n = (int)recvmsg(sock, &mh, 0)) < 0 && ++tries < 64 && (errno == ECONNREFUSED || errno == EHOSTUNREACH || errno == ENETUNREACH))
            mh.msg_namelen = sizeof(a), mh.msg_controllen = sizeof(ctl.b);
        if (n < 0) return -1;   /* EWOULDBLOCK */
        rx_us = net_us();
        for (c = CMSG_FIRSTHDR(&mh); c; c = CMSG_NXTHDR(&mh, c))
            if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SCM_TIMESTAMPNS) memcpy(&ts, CMSG_DATA(c), sizeof(ts));
        if (ts.tv_sec) {   /* a wall-clock time: how long ago, taken off our monotonic clock */
            struct timespec now;
            int64_t ago;
            clock_gettime(CLOCK_REALTIME, &now);
            ago = ((int64_t)now.tv_sec - ts.tv_sec) * 1000000 + (now.tv_nsec - ts.tv_nsec) / 1000;
            if (ago >= 0 && ago < 60000000) rx_us -= (uint64_t)ago;   /* (a slow frame waits seconds; else the clock was set: when it was read) */
        }
    }
#endif
    from->ip = a.sin_addr.s_addr;
    from->port = a.sin_port;
    return n;
}

uint64_t net_rx_us(void) { return rx_us; }

const char* net_addr_str(const NetAddr* a)
{
    static char s[32];
    const unsigned char* b = (const unsigned char*)&a->ip;
    snprintf(s, sizeof(s), "%u.%u.%u.%u:%u", b[0], b[1], b[2], b[3], ntohs(a->port));
    return s;
}

#ifndef RECVX_NET_NO_SDL
uint32_t net_ms(void) { return (uint32_t)SDL_GetTicks(); }
uint64_t net_us(void) { return SDL_GetTicksNS() / 1000; }
#else
uint32_t net_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}
uint64_t net_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000 + (uint64_t)t.tv_nsec / 1000;
}
#endif

int net_local_port(void)
{
    struct sockaddr_in a;
    socklen_t al = sizeof(a);
    if (sock == BAD_SOCK || getsockname(sock, (struct sockaddr*)&a, &al) != 0) return 0;
    return ntohs(a.sin_port);
}

int net_local_ip(char* out, int cap)
{
    /* a connected UDP socket picks the outgoing interface; connect() sends nothing */
    struct sockaddr_in a;
    socklen_t al = sizeof(a);
    sock_t s;
    int ok;
    out[0] = 0;
    if (!net_startup() || (s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == BAD_SOCK) return 0;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(0x08080808);   /* any public address */
    a.sin_port = htons(53);
    ok = connect(s, (struct sockaddr*)&a, sizeof(a)) == 0 && getsockname(s, (struct sockaddr*)&a, &al) == 0 &&
         a.sin_addr.s_addr != 0;
    if (ok) {
        const unsigned char* b = (const unsigned char*)&a.sin_addr.s_addr;
        snprintf(out, cap, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
    }
    closesock(s);
    return ok;
}
