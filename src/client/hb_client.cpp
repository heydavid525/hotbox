#include "client/hb_client.hpp"
#include "util/all.hpp"
#include <glog/logging.h>

namespace hotbox {

HBClient::HBClient() {
  RegisterAll();
}

Session HBClient::CreateSession(const SessionOptions& session_options) noexcept {
  ClientMsg client_msg;
  auto mutable_req = client_msg.mutable_create_session_req();
  *mutable_req->mutable_session_options() = session_options.GetProto();
  ServerMsg server_reply = warp_client_.SendRecv(client_msg);
  CHECK(server_reply.has_create_session_reply());
  auto session_reply = server_reply.create_session_reply();
  LOG(INFO) << session_reply.msg();
  return Session(warp_client_, session_reply.status_code(),
      session_reply.session_proto());
}

}  // namespace hotbox
