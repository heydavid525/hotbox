#include "client/hb_client.hpp"
#include "util/all.hpp"
#include <glog/logging.h>

namespace hotbox {
const int kDataEnd = -1;

HBClient::HBClient(const HBClientConfig& config) :
  connect_proxy_(config.connect_proxy),
  warp_client_(WarpClientConfig(config.connect_proxy,
        config.num_proxy_servers)),
  num_proxy_servers_(config.num_proxy_servers) {
  RegisterAll();
}

Session HBClient::CreateSession(
    const SessionOptions& session_options, ServerMsg* server_msg)
  noexcept {
  ClientMsg client_msg;
  auto mutable_req = client_msg.mutable_create_session_req();
  *mutable_req->mutable_session_options() =
    session_options.GetProto();
  CreateSessionReply session_reply;
  for (int s = 0; s < num_proxy_servers_; ++s) {
    ServerMsg server_reply = warp_client_.SendRecv(client_msg, true,
        s);
    if (server_msg != nullptr) {
      *server_msg = server_reply;
    }
    CHECK(server_reply.has_create_session_reply());
    session_reply = server_reply.create_session_reply();
  }
  LOG(INFO) << session_reply.msg();
  return Session(warp_client_, session_reply.status_code(),
      session_reply.session_proto(), connect_proxy_);
}

Session* HBClient::CreateSessionPtr(
    const SessionOptions& session_options, ServerMsg* server_msg) 
  noexcept {
  ClientMsg client_msg;
  auto mutable_req = client_msg.mutable_create_session_req();
  *mutable_req->mutable_session_options() =
    session_options.GetProto();
  ServerMsg server_reply = warp_client_.SendRecv(client_msg);
  if (server_msg != nullptr) {
    *server_msg = server_reply;
  }
  CHECK(server_reply.has_create_session_reply());
  auto session_reply = server_reply.create_session_reply();
  LOG(INFO) << session_reply.msg();
  return new Session(warp_client_, session_reply.status_code(),
      session_reply.session_proto(), connect_proxy_);
}

}  // namespace hotbox
