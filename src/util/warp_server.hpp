#pragma once

#include <zmq.hpp>
#include <utility>
#include <memory>
#include <string>
#include <map>
#include "zmq_util.hpp"
#include "util/proto/warp_msg.pb.h"

namespace hotbox {

const std::string kServerPrefix =
  zmq_util::Convert2ZmqId("hotbox_server");
const std::string kProxyServerAddr = "ipc://hotbox_proxy";

struct WarpServerConfig {
  bool proxy_server = false;

  // support multiple (proxy) servers. ServerId is kServerPrefix +
  // std::to_string(server_id)
  //
  // Comment(wdai): No automatic port assignment for non-proxy
  // server, so multiple server on one machine would cause error.
  int server_id = 0;

  WarpServerConfig() {}

  WarpServerConfig(bool proxy_server, int server_id = 0) :
    proxy_server(proxy_server), server_id(server_id) { }
};

// WarpServer is globally unique and talks to WarpClients. Server binds to
// tcp://*:kServerPort.
class WarpServer {
public:
  // True if caller is a proxy server instead of hb server
  explicit WarpServer(const WarpServerConfig& config);

  // Send to a client.
  bool Send(int client_id, const ServerMsg& msg, bool compress = true);

  // Recv internally handles handshake. The rest is handled externally.
  ClientMsg Recv(int* client_id, bool decompress = true);

  // Get the list of active clients.
  std::vector<int> GetClientIds() const;

private:
  // Lower level implementation.
  bool Send(int client_id, const std::string& data);

private:
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  int num_clients_{0};

  // Map from ith client to zmq id string.
  std::map<int, std::string> client_id2str_;
  std::map<std::string, int> client_str2id_;
};

// Apply to all server
inline std::string MakeServerId(int server_id) {
  return kServerPrefix + std::to_string(server_id);
}

// Only for proxy server
inline std::string MakeProxyServerAddr(int server_id) {
  return kProxyServerAddr + std::to_string(server_id);
}

}  // namespace hotbox
