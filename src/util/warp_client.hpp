#pragma once

#include "zmq_util.hpp"
#include "warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
#include <zmq.hpp>
#include <utility>
#include <memory>
#include <string>
#include <cstdint>

namespace hotbox {

class WarpClient {
public:
  // Server binds to a socket, while clients connect to it. The order of bind
  // and connect doesn't matter (Comment: for inproc, which we don't use, order
  // matters).
  //
  // Client router socket's identity is set internally by zmq.
  explicit WarpClient();

  // Return success or not. Always async.
  bool Send(const ClientMsg& msg);

  // Receive from server (blocking).
  ServerMsg Recv();

  // Convenience method for Send and Recv. WarpClient acts like a REQ client.
  ServerMsg SendRecv(const ClientMsg& msg);

private:
  // Lower level Send.
  bool Send(const std::string& data);

  // Handshake with the server and get the client's client id. The first
  // connect and receive.
  void HandshakeWithServer();

private:
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  std::string server_id_;

  // client id is rank among all clients. It only increase monotonically, so
  // missing rank means dead client.
  int client_id_{-1};
};

}  // namespace hotbox
