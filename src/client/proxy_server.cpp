#include "client/proxy_server.hpp"
#include <glog/logging.h>
#include "util/all.hpp"
#include <string>

namespace hotbox {

namespace {

std::string MakeIterName(const std::string& session_id, int iter_id) {
  return session_id + "_" + std::to_string(iter_id);
}

}  // anonymous namespace

void ProxyServer::Start() {
  LOG(INFO) << "ProxyServer started";
  while (true) {
    int client_id;
    ClientMsg client_msg = server_.Recv(&client_id);

    if (client_msg.has_create_session_req()) {
      LOG(INFO) << "Received create_session_req from client "
        << client_id;
      CreateSessionHandler(client_id,
          client_msg.create_session_req());
    } else if (client_msg.has_proxy_destroy_iter_req()) {
      LOG(INFO) << "Received proxy_destroy_iter_req from client "
        << client_id;
      auto& req = client_msg.proxy_destroy_iter_req();
      ProxyDestroyIterHandler(client_id, req);
    } else if (client_msg.has_close_session_req()) {
      LOG(INFO) << "Received close_session_req from client "
        << client_id;
      auto session_id = client_msg.close_session_req().session_id();
      sessions_.erase(session_id);
      SendGenericReply(client_id, "Proxy server closed session");
    } else if (client_msg.has_proxy_create_iter_req()) {
      LOG(INFO) << "Received proxy_create_iter_req from client "
        << client_id;
      auto& req = client_msg.proxy_create_iter_req();
      ProxyCreateIterHandler(client_id, req);
    } else if (client_msg.has_proxy_get_batch_req()) {
      LOG_EVERY_N(INFO, 1000) << "Received proxy_get_batch_req from client "
        << client_id;
      auto& req = client_msg.proxy_get_batch_req();
      ProxyGetBatchHandler(client_id, req);
    } else {
      LOG(ERROR) << "Unrecognized client msg case: "
        << client_msg.msg_case();
    }
    // TODO(wdai): Close proxy server message.
  }
}

void ProxyServer::CreateSessionHandler(int client_id,
    const CreateSessionReq& req) {
  ServerMsg server_reply;
  Session* session = hb_client_.CreateSessionPtr(
      SessionOptions(req.session_options()), &server_reply);
  std::string session_id =
    server_reply.create_session_reply().session_proto().session_id();
  if (sessions_.find(session_id) != sessions_.cend()) {
    LOG(ERROR) << "Overwriting an existing session: " << session_id;
    sessions_.erase(session_id);
  }
  sessions_.emplace(session_id,
      std::unique_ptr<Session>(session));
  // Forward the DBServer's reply to proxy client.
  server_.Send(client_id, server_reply);
}

void ProxyServer::ProxyCreateIterHandler(int client_id,
    const ProxyCreateIterReq& req) {
  auto session_id = req.session_id();
  auto iter_id = req.iter_id();
  std::unique_ptr<DataIteratorIf> s =
    sessions_[session_id]->NewDataIterator(
      req.data_begin(), req.data_end(), req.num_transform_threads(),
      req.num_io_threads(), req.buffer_limit(),
      req.batch_limit());
  auto iter_name = MakeIterName(session_id, iter_id);
  if (iters_.find(iter_name) != iters_.cend()) {
    LOG(ERROR) << "Overwriting an existing iter: " << iter_name;
    iters_.erase(iter_name);
  }
  iters_.emplace(iter_name, std::move(s));
  SendGenericReply(client_id, "Proxy server created iterator.");
}

void ProxyServer::ProxyDestroyIterHandler(int client_id,
    const ProxyDestroyIterReq& req) {
  // collect stats and send to master
  auto session_id = req.session_id();
  auto iter_id = req.iter_id();
  auto iter_name = MakeIterName(session_id, iter_id);
  if (iters_.find(iter_name) == iters_.cend()) {
    LOG(ERROR) << "the iter name doesn't exist: " << iter_name;
  }
  iters_.erase(iter_name);
  SendGenericReply(client_id, "Proxy server deleted iterator.");
}

void ProxyServer::PrepareBatch(const std::string& iter_name,
    int batch_size) {
  // Clear the existing msg.
  next_batch_[iter_name] = ServerMsg();
  ServerMsg& reply_msg = next_batch_[iter_name];
  auto rep = reply_msg.mutable_proxy_get_batch_reply();
  auto it = iters_.find(iter_name);
  CHECK(it != iters_.cend());
  int i = 0;
  for (auto& iter = it->second; iter->HasNext() &&
      i < batch_size; ++i) {
    FlexiDatum datum = iter->GetDatum();
    *rep->add_data() = datum.Serialize();
  }
}

void ProxyServer::ProxyGetBatchHandler(int client_id,
    const ProxyGetBatchReq& req) {
  int batch_size = req.batch_size();
  std::string iter_name = MakeIterName(req.session_id(),
      req.iter_id());
  auto it = next_batch_.find(iter_name);
  if (it == next_batch_.cend()) {
    PrepareBatch(iter_name, batch_size);
    it = next_batch_.find(iter_name);
  }
  bool compress = false;
  server_.Send(client_id, it->second, compress);
  PrepareBatch(iter_name, batch_size);
}

void ProxyServer::SendGenericReply(int client_id,
    const std::string& msg) {
  ServerMsg reply_msg;
  reply_msg.mutable_generic_reply()->set_msg(msg);
  server_.Send(client_id, reply_msg);
}

}  // namespace hotbox
