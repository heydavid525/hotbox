#include "warp_server.hpp"
#include <glog/logging.h>

namespace mldb {

WarpServer::WarpServer() {
  zmq_ctx_.reset(zmq_util::CreateZmqContext());
  SetUpRouterSocket();

  Workloop();
}

void WarpServer::SetUpRouterSocket() {
  sock_.reset(new zmq::socket_t(*zmq_ctx_, ZMQ_ROUTER));
  // Set a globally unique id.
  std::string ServerZmdId = zmq_util::Convert2ZmqId(kServerId);
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_IDENTITY, ServerZmdId.c_str(),
      ServerZmdId.size());
  LOG(INFO) << "Server ZMQ ID: " << ServerZmdId;

  // accept only routable messages on ROUTER sockets
  // This option means the unroutable message would be sent anyway.
  // If it doesn't work, it would through error.
  int sock_mandatory = 1;
  zmq_util::ZMQSetSockOpt(sock_.get(), ZMQ_ROUTER_MANDATORY, &(sock_mandatory),
      sizeof(sock_mandatory));

  std::string bind_addr = "tcp://*:" + std::to_string(kServerPort);
  zmq_util::ZMQBind(sock_.get(), bind_addr);
  ServerSleep(20);
  LOG(INFO) << "Server binds to " << bind_addr;

  //  Socket Connect.
  //std::string conn_addr = "tcp://*:" + std::to_string(kClientPort);
  //zmq_util::ZMQConnect(sock_.get(), conn_addr);
}


ClientMsg WarpServer::ReadClientMsg(std::string &client_id) {
  ClientMsg client_msg;
  LOG(INFO) << "Reading ClientMsg" ;
  auto recv = zmq_util::ZMQRecv(sock_.get(), &client_id);
  LOG(INFO) << "Parsing ClientMsg" << "size: " << recv.size();
  auto recv_str = std::string(reinterpret_cast<const char*>(recv.data()), recv.size());
  if (!client_msg.ParseFromString(recv_str))
    LOG(FATAL) << "Msg Parsing Failed."
               << "From Client " << client_str2id_[client_id]
               << ", ZMQ_ID: " << client_id;
  
  return client_msg;
}

int WarpServer::RegisterClient(std::string client_id_str) {
  int client_id = num_clients_++;
  client_id2str_[client_id] = client_id_str;
  client_str2id_[client_id_str] = client_id;
  LOG(INFO) << "Server registered client: " << client_id
    << "; client id string: " << client_id_str;

  return client_id;
}

void WarpServer::RespondClientID(int client_id) {
  //  Send ClientID to the client.
  ServerMsg server_msg;
  std::string server_msg_data;
  auto handshake_msg = server_msg.mutable_handshake_msg();
  handshake_msg->set_client_id(client_id);
  server_msg.SerializeToString(&server_msg_data);

  Send(client_id, server_msg_data);
}

void WarpServer::Workloop() { 
  zmq::pollitem_t items [] = {
    { *sock_.get(), 0 , ZMQ_POLLIN, 0}
  };

  while (true) {    
    zmq::poll (&items[0], 1, -1);
    ClientMsg client_msg;
    std::string client_id_str;

    if (items [0].revents & ZMQ_POLLIN) {
      client_msg = ReadClientMsg(client_id_str);
      LOG(INFO) << "ClientMsg Read" ;

      if (client_msg.has_handshake_msg()) {
        LOG(INFO) << "HandShake Message" ;
        RespondClientID(RegisterClient(client_id_str));
        ServerSleep(100); // Pretend to do some work.
      } else

      if (client_msg.has_dummy_req()) {
        LOG(INFO) << "Dummy Message" ;
        std::string req_str = client_msg.dummy_req().req();
        LOG(INFO) << "Got dummy request from client " << GetClientId(client_id_str) 
                  << " request: " << req_str;
      }

      /* // Why does this need two sockets rather than one?
      while (1) {
        //  Process all parts of the message
        frontend.recv(&message);
        size_t more_size = sizeof (more);
        frontend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        backend.send(message, more? ZMQ_SNDMORE: 0);
        
        if (!more)
            break;      //  Last message part
      }
      */
    }
  }
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

/*
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
*/

std::vector<int> WarpServer::GetClientIds() const {
  std::vector<int> client_ids(client_id2str_.size());
  int i = 0;
  for (auto& pair : client_id2str_) {
    client_ids[i++] = pair.first;
  }
  return client_ids;
}

int WarpServer::GetClientId(std::string id_str) {
  auto it = client_str2id_.find(id_str);
  if (it == client_str2id_.cend()) {
    LOG(FATAL) << "ZMQ client: " << id_str << " is not registered with server "
      "yet.";
    return -1;
  }
  return it->second;
}

}  // namespace mldb
