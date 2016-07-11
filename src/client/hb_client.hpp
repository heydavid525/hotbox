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

// A read client that performs transform using a pool of threads.
//
// TODO(wdai): Close connection in d'tor.
class HBClient {
public:
  HBClient();

  // Create a session. HBClient must outlive the created Session.
  Session CreateSession(const SessionOptions& session_options) noexcept;

  Session* CreateSessionPtr(const SessionOptions& session_options) noexcept;

private:
  WarpClient warp_client_;
};

}  // namespace hotbox
