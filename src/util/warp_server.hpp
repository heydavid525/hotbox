#pragma once

#include <zmq.hpp>
#include <utility>
#include <memory>
#include <string>
#include <map>
#include "zmq_util.hpp"
#include "util/proto/warp_msg.pb.h"
#include "util/proto/warp_config.pb.h"

namespace mldb {

const std::string kServerId = zmq_util::Convert2ZmqId("mldb_server");

// WarpServer is globally unique and talks to WarpClients. Server binds to
// tcp://*:kServerPort.
class WarpServer {
public:
  WarpServer(const WarpServerConfig& config);

  // Send to a client.
  bool Send(int client_id, const std::string& data);

  // Recv internally handles handshake. The rest is handled externally.
  ClientMsg Recv(int* client_id);

  // Get the list of active clients.
  std::vector<int> GetClientIds() const;

private:
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  int num_clients_{0};

  // Map from ith client to zmq id string.
  std::map<int, std::string> client_id2str_;
  std::map<std::string, int> client_str2id_;
};

}  // namespace mldb
