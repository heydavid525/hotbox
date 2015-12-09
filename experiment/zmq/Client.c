#include <unistd.h>
#include <zmq.h>
#include <cstdio>

int main() {
  int major, minor, patch;
  zmq_version(&major, &minor, &patch);
  printf("ZeroMQ %d.%d.%d\n", major, minor, patch);
  void *ctx = zmq_ctx_new();
  void *sock = zmq_socket(ctx, ZMQ_ROUTER);
  int mandatory = 1;
  zmq_setsockopt(sock, ZMQ_ROUTER_MANDATORY,
      &mandatory, sizeof(int));
  zmq_connect(sock, "tcp://127.0.0.1:10000");
  char addr[256];
  char buff[256];
  sleep(1);
  for (int i=0; i < 10; i++) {
    zmq_send(sock, "server", 7, ZMQ_SNDMORE);
    zmq_send(sock, "message", 10, 0);
    printf("send message\n");
  //}
  //for (int i=0; i < 10; i++) {
    zmq_recv(sock, addr, 256, 0);
    zmq_recv(sock, buff, 256, 0);
    printf("recv %s\n", buff);
  }
  zmq_close(sock);
  zmq_term(ctx);
}
