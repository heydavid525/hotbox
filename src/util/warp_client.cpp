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

WarpClient::WarpClient(bool connect_proxy) {
  zmq_ctx_.reset(zmq_util::CreateZmqContext());
  sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

  LOG(INFO) << "WarpClient: reset socket. Init phase.";

  // accept only routable messages on ROUTER sockets
  int sock_mandatory = 1;
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
      sizeof(sock_mandatory));
  auto& global_config = GlobalConfig::GetInstance();
  std::string dst_addr;
  if (connect_proxy) {
    dst_addr = kProxyServerAddr;
  } else {
    dst_addr = "tcp://" + global_config.Get<std::string>("server_ip")
      + ":" + global_config.Get<std::string>("server_port");
  }
  LOG(INFO) << "Connect dst_addr: " << dst_addr;
  zmq_util::ZMQConnect(sock_.get(), dst_addr);

  // Wait for connection to establish.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool WarpClient::Send(const std::string& data) {
  if (client_id_ < 0) {
    HandshakeWithServer();
  }
  return zmq_util::ZMQSend(sock_.get(), kServerId, data);
}

// Comment(wdai): Sender thread also perform snappy compression and
// decompression, which could be expensive.
bool WarpClient::Send(const ClientMsg& msg) {
  std::string data = StreamSerialize(msg);
  return Send(data);
}

ServerMsg WarpClient::Recv() {
  if (client_id_ < 0) {
    HandshakeWithServer();
  }
  auto recv = zmq_util::ZMQRecv(sock_.get());
  auto recv_str = std::string(reinterpret_cast<const char*>(recv.data()),
      recv.size());
  ServerMsg server_msg = StreamDeserialize<ServerMsg>(recv_str);
  CHECK(!server_msg.has_handshake_msg());
  return server_msg;
}

ServerMsg WarpClient::SendRecv(const ClientMsg& msg) {
  CHECK(Send(msg));
  return Recv();
}

void WarpClient::HandshakeWithServer() {
  LOG(INFO) << "Client initiate handshake";
  ClientMsg client_msg;
  // Don't need to set anything in the returned handshake_msg.
  client_msg.mutable_handshake_msg();
  std::string data = StreamSerialize(client_msg);
  bool success = false;
  do {
    success = zmq_util::ZMQSend(sock_.get(), kServerId, data);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while(!success);

  auto rep = zmq_util::ZMQRecv(sock_.get(), &server_id_);
  auto rep_str = std::string(reinterpret_cast<const char*>(rep.data()),
      rep.size());
  ServerMsg server_msg = StreamDeserialize<ServerMsg>(rep_str);
  CHECK(server_msg.has_handshake_msg());
  client_id_ = server_msg.handshake_msg().client_id();
  LOG(INFO) << "Client " << client_id_ << " finished handshake with server."
    "server_id: " << server_id_;
}

}  // namespace hotbox
