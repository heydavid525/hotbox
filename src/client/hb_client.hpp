#pragma once

#include "util/all.hpp"
#include "util/proto/warp_msg.pb.h"
#include "schema/all.hpp"
#include "db/proto/db.pb.h"
#include "util/file_util.hpp"
#include "client/status.hpp"
#include "client/data_iterator.hpp"
#include "client/session.hpp"
#include "client/session_options.hpp"

DECLARE_string(hb_config_path);

namespace hotbox {

// hotbox::kDataEnd signal reading to the last data.
extern const int kDataEnd;

struct HBClientConfig {
  bool connect_proxy = false;
  int num_proxy_servers = 1;

  HBClientConfig() { }

  HBClientConfig(bool connect_proxy, int num_proxy_servers = 1) :
  connect_proxy(connect_proxy), num_proxy_servers(num_proxy_servers) { }
};

// A read client that performs transform using a pool of threads.
//
// TODO(wdai): Close connection in d'tor.
class HBClient {
public:
  explicit HBClient(const HBClientConfig& config);

  // Create a session. HBClient must outlive the created Session.
  // Optionally return server_msg (used by ProxyServer).
  Session CreateSession(const SessionOptions& session_options,
      ServerMsg* server_msg = nullptr) noexcept;

  Session* CreateSessionPtr(const SessionOptions& session_options,
      ServerMsg* server_msg = nullptr) noexcept;

private:
  bool connect_proxy_;
  WarpClient warp_client_;
  int num_proxy_servers_;
};

}  // namespace hotbox
