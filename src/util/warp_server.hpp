#pragma once

#include "zmq_util.hpp"
#include "util/proto/warp_msg.pb.h"
#include <zmq.hpp>
#include <utility>
#include <memory>
#include <string>
#include <unordered_map>

#include <chrono>
#include <thread>


namespace mldb {

const int kServerPort = 19856;
const int kClientPort = 19857;
const std::string kServerId = "mldb_server";
const std::string kClientId = "mldb_client";

// WarpServer is globally unique and talks to WarpClients. Server binds to
// tcp://*:kServerPort.
class WarpServer {
public:
  WarpServer();

  //  Set up the ROUTER socket.
  void      SetUpRouterSocket();
  ClientMsg ReadClientMsg(std::string &client_id);

  //  Handshake Msg Processing
  int       RegisterClient(std::string client_id);
  void      RespondClientID(int client_id);

  // Send to a client.
  bool Send(int client_id, const std::string& data);

  // Recv internally handles handshake. The rest is handled externally.
  ClientMsg Recv(int* client_id);

  // Get the list of active clients.
  std::vector<int> GetClientIds() const;
  int              GetClientId(std::string client_id_str);

  //  The main msg distribution workloop of the Server.
  void Workloop();
  void ServerSleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }

private:
  //  Communication Bus
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  int num_clients_{0};

  // Map from ith client to zmq id string.
  std::unordered_map<int, std::string> client_id2str_;
  std::unordered_map<std::string, int> client_str2id_;
};

}  // namespace mldb