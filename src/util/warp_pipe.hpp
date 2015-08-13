#pragma once

#include "zmq_util.hpp"
#include "sole.hpp"
#include <zmq.hpp>
#include <utility>
#include <string>

namespace mldb {

const int kServerPort = 19856;


// WarpServer is globally unique and talks to WarpClients. Server binds to
// tcp://*:kServerPort.
class WarpServer {
public:
  WarpServer() {
    zmq_ctx_.reset(ZMQUtil::CreateZmqContext());
    sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

    // Set a globally unique id.
    sole::uuid uuid = sole::uuid1();
    my_id_ = ZMQUtil::Convert2ZmqId(uuid.str());
    ZMQUtil::ZMQSetSockOpt(sock_.get(), ZMQ_IDENTITY, &my_id_, my_id_.size());

    // accept only routable messages on ROUTER sockets
    int sock_mandatory = 1;
    ZMQUtil::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
        sizeof(sock_mandatory));

    std::string bind_addr = "tcp://*:" + std::to_string(kServerPort);
    ZMQUtil::ZMQBind(sock_.get(), bind_addr);
  }

private:
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  std::string my_id_;
  int num_clients_{0};

  // Map from ith client to zmq id string.
  std::unordered_map<int, std::string> client_ids_;
};




struct WarpClientConfig {
  // E.g. "50.1.1.2". 
  std::string server_ip;
};

class WarpClient {
public:
  // Server binds to a socket, while clients connect to it. The order of bind
  // and connect doesn't matter (but for inproc, which we don't use, order
  // matters).
  explicit WarpClient(const WarpClientConfig& config) {
    zmq_ctx_.reset(ZMQUtil::CreateZmqContext());
    sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

    // Set a globally unique id.
    sole::uuid uuid = sole::uuid1();
    my_id_ = ZMQUtil::Convert2ZmqId(uuid.str());
    ZMQUtil::ZMQSetSockOpt(sock_.get(), ZMQ_IDENTITY, &my_id_, my_id_.size());

    // accept only routable messages on ROUTER sockets
    int sock_mandatory = 1;
    ZMQUtil::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
        sizeof(sock_mandatory));

    auto dst_addr = config.server_ip + ":" + std::to_string(kServerPort);
    ZMQUtil::ZMQConnect(sock_.get(), dst_addr);
  }

  // Return # bytes sent, or 0 if failed. Always async.
  size_t Send(const void* data, size_t len) {
    if (my_rank_ < 0) {
      ServerHandShake();
    }
  }

  // Receive from server (blocking)
  zmq::message_t Recv() {
    if (my_rank_ < 0) {
      ServerHandShake();
    }
  }

private:
  // Handshake with the server and get the client's client rank. The first
  // connect and receive.
  void ServerHandShake() {
    ZMQUtil::ZMQSend(sock_.get(), dst_zmq_id, data, len);
  }

private:
  std::unique_ptr<zmq::context_t> zmq_ctx_;
  std::unique_ptr<zmq::socket_t> sock_;
  std::string my_id_;
  std::string server_id_;

  // client rank among all clients. It only increase monotonically, so missing
  // rank means dead client.
  int my_rank_{-1};
};

}  // namespace mldb
