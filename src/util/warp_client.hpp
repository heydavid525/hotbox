#pragma once

#include "zmq_util.hpp"
#include "warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
#include <zmq.hpp>
#include <utility>
#include <memory>
#include <string>
#include <cstdint>
#include <map>

namespace hotbox {

struct WarpClientConfig {
  bool connect_proxy = false;
  int num_servers = 1;

  WarpClientConfig() { }

  WarpClientConfig(bool connect_proxy, int num_servers = 1) :
  connect_proxy(connect_proxy), num_servers(num_servers) { }
};

class WarpClient {
public:
  // Server binds to a socket, while clients connect to it. The order of bind
  // and connect doesn't matter (Comment: for inproc, which we don't use, order
  // matters).
  //
  // connect_proxy = true to connect to a proxy server through ipc instead of
  // hb server.
  //
  // Client router socket's identity is set internally by zmq.
  explicit WarpClient(const WarpClientConfig& config);

  // Send to one server. Return success or not. Always async.
  bool Send(const ClientMsg& msg, bool compress = true,
      int server_id = 0);
  // Broadcast to all connected servers.
  bool Broadcast(const ClientMsg& msg, bool compress = true);

  // Receive from a designated server (blocking).
  ServerMsg Recv(bool decompress = true, int server_id = 0);

  // Receive any server message. Return server_id if not nullptr.
  ServerMsg RecvAny(bool decompress = true, int* server_id = nullptr);

  // Recv from all servers.
  std::vector<ServerMsg> RecvAll(bool decompress = true);

  // Convenience method for Send and Recv. WarpClient acts like a REQ client.
  ServerMsg SendRecv(const ClientMsg& msg, bool compress = true,
      int server_id = 0);

  std::vector<ServerMsg> BroadcastRecvAll(const ClientMsg& msg,
      bool compress = true);

  // Get # of connected servers.
  int GetNumServers() const {
    return num_servers_;
  }

private:
  // Lower level Send.
  bool Send(const std::string& data, int server_id);

  // Handshake with the servers and get the client's client id. The first
  // connect and receive.
  void HandshakeWithServers();

private:
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  // server_id string --> server rank
  std::map<std::string, int> server_ids_;

  // client id is rank among all clients. It only increase monotonically, so
  // missing rank means dead client.
  int client_id_{-1};
  int num_servers_;
};

}  // namespace hotbox
