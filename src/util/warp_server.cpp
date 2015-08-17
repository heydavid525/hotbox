#include "warp_server.hpp"
#include <glog/logging.h>

namespace mldb {

WarpServer::WarpServer() {
  zmq_ctx_.reset(zmq_util::CreateZmqContext());
  sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));

  // Set a globally unique id.
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_IDENTITY, kServerId.c_str(),
      kServerId.size());
  LOG(INFO) << "kServerId: " << kServerId;

  // accept only routable messages on ROUTER sockets
  // This option means the unroutable message would be sent anyway.
  // If it doesn't work, it would through error.
  int sock_mandatory = 1;
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
      sizeof(sock_mandatory));

  std::string bind_addr = "tcp://*:" + std::to_string(kServerPort);
  LOG(INFO) << "Server binds to " << bind_addr;
  zmq_util::ZMQBind(sock_.get(), bind_addr);

  //std::string conn_addr = "tcp://*:" + std::to_string(kClientPort);
  //zmq_util::ZMQConnect(sock_.get(), conn_addr);
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
    client_msg.ParseFromString(recv_str);
    if (client_msg.has_handshake_msg()) {
      *client_id = num_clients_++;
      client_id2str_[*client_id] = client_id_str;
      client_str2id_[client_id_str] = *client_id;
      LOG(INFO) << "Server registered client " << num_clients_
        << "; client id string: " << client_id_str;

      // Respond to the client.
      ServerMsg server_msg;
      auto handshake_msg = server_msg.mutable_handshake_msg();
      handshake_msg->set_client_id(*client_id);
      std::string data;
      server_msg.SerializeToString(&data);
      CHECK(zmq_util::ZMQSend(sock_.get(), client_id_str, data));
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
