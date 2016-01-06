#include <unistd.h>
#include <zmq.h>
#include <cstdio>

int main() {
  int major, minor, patch;
  zmq_version(&major, &minor, &patch);
  printf("ZeroMQ %d.%d.%d\n", major, minor, patch);
  void *ctx = zmq_ctx_new();
  void *sock = zmq_socket(ctx, ZMQ_ROUTER);
  zmq_setsockopt(sock, ZMQ_IDENTITY, "server", 7);
  int mandatory = 1;
  zmq_setsockopt(sock, ZMQ_ROUTER_MANDATORY, &mandatory, sizeof(mandatory));
  zmq_bind(sock, "tcp://127.0.0.1:10000");
  char addr[256];
  char buff[256];
  while (true) {
    int len = zmq_recv(sock, addr, 256, 0);
    zmq_recv(sock, buff, 256, 0);
    printf("recv %s\n", buff);
    sleep(4);
    zmq_send(sock, addr, len, ZMQ_SNDMORE);
    zmq_send(sock, "response", 9, 0);
    printf("send response\n");
  }
  zmq_close(sock);
  zmq_term(ctx);
}