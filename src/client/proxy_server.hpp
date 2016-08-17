#pragma once
#include "util/all.hpp"
#include "client/hb_client.hpp"
#include "client/session.hpp"
#include <string>
#include <unordered_map>

namespace hotbox {

// A proxy server that forwards requests to DBServer and perform
// transformation locally.
class ProxyServer {
public:
  ProxyServer(int server_id) :
    hb_client_(HBClientConfig(false)),
    server_(WarpServerConfig(true, server_id)),
    server_id_(server_id) { }

  void Start();

private:
  void CreateSessionHandler(int client_id, const CreateSessionReq& req);
  void ProxyCreateIterHandler(int client_id,
      const ProxyCreateIterReq& req);
  void ProxyDestroyIterHandler(int client_id,
      const ProxyDestroyIterReq& req);
  void ProxyGetBatchHandler(int client_id,
      const ProxyGetBatchReq& req);
  void PrepareBatch(const std::string& iter_name, int batch_size);
  void SendGenericReply(int client_id, const std::string& msg);

private:
  // hb_client_ communicates with DBServer.
  HBClient hb_client_;

  // warp_client_ communicates with the proxy client.
  WarpServer server_;
  int server_id_;

  // Each session is identified by a session_id.
  std::unordered_map<std::string, std::unique_ptr<Session>> sessions_;
  std::unordered_map<std::string, std::unique_ptr<DataIteratorIf>>
    iters_;
  // Read next batch ahead of time.
  std::unordered_map<std::string, ServerMsg> next_batch_;
  int session_counter_{0};
};

}  // namespace hotbox
