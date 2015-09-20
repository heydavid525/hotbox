#include "util/warp_server.hpp"
#include "util/global_config.hpp"
#include <glog/logging.h>

namespace mldb {

WarpServer::WarpServer(const WarpServerConfig& config) {
  zmq_ctx_.reset(zmq_util::CreateZmqContext());
  sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

  // Set a globally unique id.
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_IDENTITY, kServerId.c_str(),
      kServerId.size());
  //zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_IDENTITY, &kServerId,
  //    sizeof(kServerId));
  LOG(INFO) << "Server ID: " << kServerId;

  // accept only routable messages on ROUTER sockets
  int sock_mandatory = 1;
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
      sizeof(sock_mandatory));
  int port = config.port() == 0 ?
    GlobalConfig::GetInstance().Get<int>("default_server_port") :
    config.port();
  std::string bind_addr = "tcp://*:" + std::to_string(port);
  LOG(INFO) << "Server binds to " << bind_addr;
  zmq_util::ZMQBind(sock_.get(), bind_addr);
}

bool WarpServer::Send(int client_id, const std::string& data) {
  auto it = client_id2str_.find(client_id);
  if (it == client_id2str_.cend()) {
    LOG(FATAL) << "client " << client_id << " is not registered with server "
      "yet.";
    return false;
  }
  return zmq_util::ZMQSend(sock_.get(), it->second, data);
}

ClientMsg WarpServer::Recv(int* client_id) {
  ClientMsg client_msg;
  std::string client_id_str;
  do {
    auto recv = zmq_util::ZMQRecv(sock_.get(), &client_id_str);
    auto recv_str = std::string(reinterpret_cast<const char*>(recv.data()),
        recv.size());
    CHECK(client_msg.ParseFromString(recv_str)) << "Failed to parse msg from"
      " client " << client_id_str;

    // Handle handshake.
    if (client_msg.has_handshake_msg()) {
      *client_id = num_clients_++;
      client_id2str_[*client_id] = client_id_str;
      client_str2id_[client_id_str] = *client_id;
      LOG(INFO) << "Server registered client " << *client_id
        << "; client id string: " << client_id_str;

      // Respond to the client.
      ServerMsg server_msg;
      auto handshake_msg = server_msg.mutable_handshake_msg();
      handshake_msg->set_client_id(*client_id);
      std::string data;
      server_msg.SerializeToString(&data);
      CHECK(Send(*client_id, data));
    }
  } while (client_msg.has_handshake_msg());
  auto it = client_str2id_.find(client_id_str);
  CHECK(client_str2id_.cend() != it) << "client " << client_id_str
    << " didn't register before sending request.";
  *client_id = it->second;
  return client_msg;
}

std::vector<int> WarpServer::GetClientIds() const {
  std::vector<int> client_ids(client_id2str_.size());
  int i = 0;
  for (auto& pair : client_id2str_) {
    client_ids[i++] = pair.first;
  }
  return client_ids;
}

}  // namespace mldb
