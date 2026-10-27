# recvx-server in a container (docs/SERVER.md "Running a server"). From the repository root:
#   docker build -f pc/server/Dockerfile -t recvx-server .
#   docker run -d --name recvx-server --restart unless-stopped -p 7141:7141/udp --read-only --cap-drop ALL \
#       -v "$PWD/recvx-server.conf:/etc/recvx-server.conf:ro" recvx-server
# The image carries the example config (the defaults) as /etc/recvx-server.conf; mount your own
# over it (read-only) or append flags after the image name (they override the file). Reload:
# docker kill -s HUP recvx-server. A static musl binary in an empty image, run as nobody.
FROM alpine:3.20 AS build
RUN apk add --no-cache build-base cmake ninja git linux-headers
COPY CMakeLists.txt /src/
COPY cmake/version.cmake /src/cmake/
COPY pc/server /src/pc/server
COPY pc/src/net/rdv_proto.c pc/src/net/rdv_proto.h pc/src/net/rendezvous.c pc/src/net/rendezvous.h pc/src/net/net.c pc/src/net/net.h /src/pc/src/net/
RUN cmake -S /src/pc/server -B /b -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build /b --target recvx-server && strip /b/recvx-server

FROM scratch
COPY --from=build /b/recvx-server /recvx-server
COPY pc/server/recvx-server.conf.example /etc/recvx-server.conf
USER 65534:65534
EXPOSE 7141/udp
ENTRYPOINT ["/recvx-server", "--config", "/etc/recvx-server.conf"]
