#include "util/warp_client.hpp"
#include "util/warp_server.hpp"
#include "util/global_config.hpp"
#include "util/proto/warp_msg.pb.h"
#include "util/util.hpp"
#include <glog/logging.h>
#include <chrono>
#include <thread>
#include <utility>
#include <string>

namespace hotbox {

WarpClient::WarpClient(const WarpClientConfig& config) {
  WarpClientConfig conf = config;
  num_servers_ = conf.num_servers;
  zmq_ctx_.reset(zmq_util::CreateZmqContext());
  sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

  LOG(INFO) << "WarpClient: reset socket. Init phase.";

  // accept only routable messages on ROUTER sockets
  int sock_mandatory = 1;
  zmq_util::ZMQSetSockOpt(sock_.get(),
      ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
      sizeof(sock_mandatory));
  auto& global_config = GlobalConfig::GetInstance();
  if (conf.connect_proxy) {
    for (int i = 0; i < num_servers_; ++i) {
      std::string dst_addr = MakeProxyServerAddr(i);
      LOG(INFO) << "Connect dst_addr: " << dst_addr;
      zmq_util::ZMQConnect(sock_.get(), dst_addr);
    }
  } else {
    std::string dst_addr = "tcp://"
      + global_config.Get<std::string>("server_ip")
      + ":" + global_config.Get<std::string>("server_port");
    LOG(INFO) << "Connect dst_addr: " << dst_addr;
    zmq_util::ZMQConnect(sock_.get(), dst_addr);
  }

  // Wait for connection to establish.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool WarpClient::Send(const std::string& data, int server_id) {
  if (client_id_ < 0) {
    HandshakeWithServers();
  }
  if (server_id != -1) {
    return zmq_util::ZMQSend(sock_.get(), MakeServerId(server_id),
        data);
  }
  bool success = true;
  for (int s = 0; s < num_servers_; ++s) {
    bool succ = zmq_util::ZMQSend(sock_.get(), MakeServerId(s),
        data);
    success = success && succ;
  }
  return success;
}

// Comment(wdai): Sender thread also perform snappy compression and
// decompression, which could be expensive.
bool WarpClient::Send(const ClientMsg& msg, bool compress, int server_id) {
  std::string data = StreamSerialize(msg, nullptr, compress);
  return Send(data, server_id);
}

bool WarpClient::Broadcast(const ClientMsg& msg, bool compress) {
  std::string data = StreamSerialize(msg, nullptr, compress);
  return Send(data, -1);
}

ServerMsg WarpClient::RecvAny(bool decompress, int* server_id) {
  if (client_id_ < 0) {
    HandshakeWithServers();
  }
  std::string recv_server_id;
  auto recv = zmq_util::ZMQRecv(sock_.get(), &recv_server_id);
  auto recv_str = std::string(reinterpret_cast<const char*>(recv.data()),
      recv.size());
  auto it = server_ids_.find(recv_server_id);
  CHECK(it != server_ids_.cend());
  if (server_id != nullptr) {
    *server_id = it->second;
  }
  ServerMsg server_msg = StreamDeserialize<ServerMsg>(
      recv_str, decompress);
  CHECK(!server_msg.has_handshake_msg());
  return server_msg;
}

ServerMsg WarpClient::Recv(bool decompress, int server_id) {
  int server_id_recv = -1;
  ServerMsg msg = RecvAny(decompress, &server_id_recv);
  CHECK_EQ(server_id, server_id_recv);
  return msg;
}

std::vector<ServerMsg> WarpClient::RecvAll(bool decompress) {
  std::vector<ServerMsg> msgs(num_servers_);
  for (int s = 0; s < num_servers_; ++s) {
    msgs[s] = Recv(decompress, s);
  }
  return msgs;
}

ServerMsg WarpClient::SendRecv(const ClientMsg& msg, bool compress,
    int server_id) {
  CHECK(Send(msg, compress, server_id));
  return Recv(compress, server_id);
}

std::vector<ServerMsg> WarpClient::BroadcastRecvAll(
    const ClientMsg& msg, bool compress) {
  CHECK(Broadcast(msg, compress));
  return RecvAll(compress);
}

void WarpClient::HandshakeWithServers() {
  LOG(INFO) << "Client initiate handshake";
  ClientMsg client_msg;
  // Don't need to set anything in the returned handshake_msg.
  client_msg.mutable_handshake_msg();
  std::string data = StreamSerialize(client_msg);
  for (int s = 0; s < num_servers_; ++s) {
    bool success = false;
    std::string server_id = MakeServerId(s);
    do {
      success = zmq_util::ZMQSend(sock_.get(), server_id, data);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (!success);

    std::string recv_server_id;
    auto rep = zmq_util::ZMQRecv(sock_.get(), &recv_server_id);
    server_ids_[recv_server_id] = s;
    auto rep_str = std::string(reinterpret_cast<const char*>(rep.data()),
        rep.size());
    ServerMsg server_msg = StreamDeserialize<ServerMsg>(rep_str);
    CHECK(server_msg.has_handshake_msg());

    // Comment(wdai): the specific value of client_id_
    // isn't important. It merely ensures that the connection with
    // the server is established after the first connect-receive.
    client_id_ = server_msg.handshake_msg().client_id();
    LOG(INFO) << "Client " << client_id_ << " finished handshake with server."
      "server_id: " << recv_server_id;
  }
}

}  // namespace hotbox
