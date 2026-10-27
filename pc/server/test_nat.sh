#!/bin/sh
# ctest rdv_nat: NAT types in network namespaces, rootless (user namespace + nftables). Topology:
#   "internet" (this namespace), routing between the routers and the server
#   srv: recvx-server on 203.0.113.1:7141, behind its own link (netem: farther away than the players)
#   natA, natB: routers, WAN 198.51.100.2 / 198.51.101.2, nftables masquerade (cone) or
#               masquerade fully-random (symmetric: a new port per destination)
#   hostA (10.0.1.2) behind natA, hostB (10.0.2.2) behind natB, hostC (10.0.3.2) also behind natA
# Each case runs two recvx-clients (the game's client code) and checks the path they end up with;
# the last two add netem delay and loss on every link and compare the round trip direct vs relayed.
# usage: test_nat.sh BIN_DIR   (re-executes itself under `unshare -rn`; exit 77 when that is not allowed)
bin=$(cd "$1" && pwd)
if [ -z "$RDV_NAT_INSIDE" ]; then
    RDV_NAT_INSIDE=1 unshare -rn true 2>/dev/null || exit 77
    RDV_NAT_INSIDE=1 exec unshare -rn sh "$0" "$bin"
fi
set -u
fails=0; pids=""
box() { unshare -n sleep 600 & eval "$1=$!"; pids="$pids $!"; }
ns() { p=$1; shift; nsenter -t "$p" -n "$@"; }
link() {   # link A_PID|- B_PID A_IF A_ADDR B_IF B_ADDR
    ip link add "$3" type veth peer name "$5" || exit 77
    if [ "$1" != - ]; then ip link set "$3" netns "$1"; fi
    ip link set "$5" netns "$2"
    if [ "$1" != - ]; then ns "$1" ip addr add "$4" dev "$3"; ns "$1" ip link set "$3" up
    else ip addr add "$4" dev "$3"; ip link set "$3" up; fi
    ns "$2" ip addr add "$6" dev "$5"; ns "$2" ip link set "$5" up
}
cleanup() { kill $pids 2>/dev/null; }
trap cleanup EXIT
ip link set lo up
echo 1 > /proc/sys/net/ipv4/ip_forward 2>/dev/null
box natA; box natB; box hostA; box hostB; box hostC; box srv
sleep 0.2
link - "$natA" iA 198.51.100.1/24 wA 198.51.100.2/24
link - "$natB" iB 198.51.101.1/24 wB 198.51.101.2/24
link - "$srv" sA 192.0.2.1/30 sS 192.0.2.2/30
link "$natA" "$hostA" lA 10.0.1.1/24 eA 10.0.1.2/24
link "$natB" "$hostB" lB 10.0.2.1/24 eB 10.0.2.2/24
link "$natA" "$hostC" lC 10.0.3.1/24 eC 10.0.3.2/24
for n in "$natA" "$natB" "$hostA" "$hostB" "$hostC" "$srv"; do ns "$n" ip link set lo up; done
ns "$srv" ip addr add 203.0.113.1/32 dev lo
ns "$srv" ip route add default via 192.0.2.1
ip route add 203.0.113.1/32 via 192.0.2.2
ns "$natA" ip route add default via 198.51.100.1
ns "$natB" ip route add default via 198.51.101.1
ns "$hostA" ip route add default via 10.0.1.1
ns "$hostB" ip route add default via 10.0.2.1
ns "$hostC" ip route add default via 10.0.3.1
for n in "$natA" "$natB"; do ns "$n" sh -c 'echo 1 > /proc/sys/net/ipv4/ip_forward' || exit 77; done

nat() {   # nat PID WAN_IF cone|symmetric
    ns "$1" nft flush ruleset
    ns "$1" nft add table ip nat
    ns "$1" nft 'add chain ip nat post { type nat hook postrouting priority 100 ; }'
    if [ "$3" = symmetric ]; then ns "$1" nft add rule ip nat post oifname "$2" masquerade random,fully-random
    else ns "$1" nft add rule ip nat post oifname "$2" masquerade; fi
    # a home router's firewall: nothing new comes in from the WAN. Without it an early punch is
    # delivered to the router itself, its conntrack entry blocks the port the partner's mapping
    # would use, and the mapping moves to another port (no router behaves that way: they drop it)
    ns "$1" nft add table inet fw
    ns "$1" nft 'add chain inet fw in { type filter hook input priority 0 ; }'
    ns "$1" nft add rule inet fw in iifname "$2" ct state new drop
    ns "$1" sh -c 'command -v conntrack >/dev/null && conntrack -F' 2>/dev/null
}

ns "$srv" "$bin/recvx-server" --port 7141 --listen 203.0.113.1 --rooms-per-ip-per-min 1000 > /tmp/rdv_nat_server.txt 2>&1 &
pids="$pids $!"
sleep 0.3

# case NAME NAT_A NAT_B JOINER_PID WANT
case_() {
    nat "$natA" wA "$2"; nat "$natB" wB "$3"
    ns "$hostA" "$bin/recvx-client" 203.0.113.1 host --secs 7 ${6:-} > /tmp/rdv_nat_h.txt 2>&1 &
    h=$!
    code=""
    for i in 1 2 3 4 5 6 7 8 9 10; do
        code=$(sed -n 's/.*rdv: room \([A-Z0-9-]*\).*/\1/p' /tmp/rdv_nat_h.txt | head -1)
        [ -n "$code" ] && break
        sleep 0.2
    done
    ns "$4" "$bin/recvx-client" 203.0.113.1 join "$code" --secs 5 ${6:-} > /tmp/rdv_nat_j.txt 2>&1
    jr=$?
    wait $h
    hr=$?   # both ends must succeed: answers came back on each side
    res=$(grep RESULT /tmp/rdv_nat_j.txt)
    if [ $jr -eq 0 ] && [ $hr -eq 0 ] && echo "$res" | grep -q "path=$5 " && ! echo "$res" | grep -q "answers=0 "; then echo "ok   $1: $res"
    else echo "FAIL $1 (want $5, joiner exit $jr, host exit $hr): $res"; cat /tmp/rdv_nat_h.txt /tmp/rdv_nat_j.txt; fails=$((fails + 1)); fi
}
case_ "cone NAT / cone NAT" cone cone "$hostB" direct
case_ "port-restricted cone NAT / symmetric NAT" cone symmetric "$hostB" relay   # needs a full-cone side or port prediction
case_ "symmetric NAT / symmetric NAT" symmetric symmetric "$hostB" relay
case_ "same router (LAN)" cone cone "$hostC" "direct (LAN)"
# latency and loss: 20 ms each way on each router's internet link, 15 ms more to the server, 2% loss
for d in iA wA iB wB; do
    if [ "${d#i}" != "$d" ]; then tc qdisc add dev "$d" root netem delay 20ms loss 2% || exit 77
    else ns "$([ "$d" = wA ] && echo "$natA" || echo "$natB")" tc qdisc add dev "$d" root netem delay 20ms loss 2%; fi
done
tc qdisc add dev sA root netem delay 15ms
ns "$srv" tc qdisc add dev sS root netem delay 15ms
case_ "netem 20 ms + 2% per link, automatic" cone cone "$hostB" direct
case_ "netem 20 ms + 2% per link, via server" cone cone "$hostB" relay --relay
exit $fails
