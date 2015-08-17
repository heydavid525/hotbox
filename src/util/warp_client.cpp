#include "warp_client.hpp"
#include <glog/logging.h>
#include <chrono>
#include <thread>
#include <utility>

namespace mldb {

WarpClient::WarpClient(const WarpClientConfig& config) {
  zmq_ctx_.reset(zmq_util::CreateZmqContext());
  sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

  // accept only routable messages on ROUTER sockets
  int sock_mandatory = 1;
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
      sizeof(sock_mandatory));

  auto dst_addr = "tcp://" + config.server_ip + ":" +
    std::to_string(kServerPort);
  LOG(INFO) << "Connect dst_addr: " << dst_addr;
  zmq_util::ZMQConnect(sock_.get(), dst_addr);  
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool WarpClient::Send(const std::string& data) {
  if (client_id_ < 0) {
    HandshakeWithServer();
  }
  return zmq_util::ZMQSend(sock_.get(), kServerId, data);
}

zmq::message_t WarpClient::Recv() {
  if (client_id_ < 0) {
    HandshakeWithServer();
  }
  return zmq_util::ZMQRecv(sock_.get());
}

void WarpClient::HandshakeWithServer() {
  ClientMsg client_msg;
  // Don't need to set anything in the returned handshake_msg.
  client_msg.mutable_handshake_msg();
  std::string data;
  client_msg.SerializeToString(&data);
  bool success = false;
  LOG(INFO) << "server id: " << kServerId;
  do {
    success = zmq_util::ZMQSend(sock_.get(), kServerId, data);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while(!success);

  auto rep = zmq_util::ZMQRecv(sock_.get(), &server_id_);
  auto rep_str = std::string(reinterpret_cast<const char*>(rep.data()),
      rep.size());
  ServerMsg server_msg;
  server_msg.ParseFromString(rep_str);
  CHECK(server_msg.has_handshake_msg());
  client_id_ = server_msg.handshake_msg().client_id();
  LOG(INFO) << "Client " << client_id_ << " finished handshake with server."
    "server_id: " << server_id_;
}

}  // namespace mldb
