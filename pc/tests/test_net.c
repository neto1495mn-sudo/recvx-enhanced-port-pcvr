/* Co-op networking (pc/src/net): hosting on a port another socket holds fails as "in use" and
   leaves nothing open; the port mapping can be started and stopped again after its worker has
   finished (it used to free its job while still current: the next stop wrote into freed memory).
   No traffic: automatic mapping off. */
#include "../src/net/net.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)

void host_log(const char* fmt, ...) { (void)fmt; }

int main(void)
{
    struct sockaddr_in a;
    socklen_t al = sizeof(a);
    PortmapStatus st;
    int s, port, k;
    CHECK(net_open(0) == 1);   /* also starts Winsock */
    net_close();
    s = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    CHECK(bind(s, (struct sockaddr*)&a, sizeof(a)) == 0 && getsockname(s, (struct sockaddr*)&a, &al) == 0);
    port = ntohs(a.sin_port);
    CHECK(net_open(port) == NET_IN_USE);
    CHECK(net_recv(&(NetAddr){ 0 }, &k, sizeof(k)) == -1);   /* nothing left open */
    CHECK(net_open(0) == 1);   /* a retry on another port */
    {   /* arrival time: a datagram to ourselves read 50 ms after it came is stamped when it came */
        NetAddr me;
        char b[8];
        uint64_t sent, age;
        CHECK(net_resolve("127.0.0.1", net_local_port(), &me));
        sent = net_us();
        CHECK(net_send(&me, "ping", 4));
        SDL_Delay(50);
        CHECK(net_recv(&me, b, sizeof(b)) == 4);
        age = net_us() - net_rx_us();
        printf("net: arrival %.2f ms after sending, read %.2f ms later\n", (net_rx_us() - sent) / 1000.0, age / 1000.0);
        CHECK(net_rx_us() >= sent && age >= 45000 && age < 1000000);
        /* a datagram bigger than the buffer does not stop the read loop (Windows: WSAEMSGSIZE is
           skipped; elsewhere it comes truncated): the one behind it is read */
        {
            static char big[1500];
            int got = 0;
            CHECK(net_send(&me, big, sizeof(big)) && net_send(&me, "pong", 4));
            SDL_Delay(20);
            for (k = 0; k < 2 && !got; k++) got = net_recv(&me, b, sizeof(b)) == 4 && !memcmp(b, "pong", 4);
            CHECK(got);
        }
    }
    net_close();
    for (k = 0; k < 3; k++) {   /* start, let the worker finish, stop: its job must be gone */
        int ms;
        portmap_start(port, 0);
        for (ms = 0; ms < 3000; ms += 10) {
            portmap_status(&st);
            if (st.msg[0]) break;
            SDL_Delay(10);
        }
        CHECK(st.msg[0] && st.state == PM_OFF);
        SDL_Delay(50);
        portmap_stop();
        portmap_status(&st);
        CHECK(!st.msg[0]);
    }
    printf("net: ok\n");
    return 0;
}
