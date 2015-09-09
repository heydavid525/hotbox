#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>
#include "db/proto/db.pb.h"
#include "util/warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
#include "glog/logging.h"
#include <boost/filesystem.hpp>
#include <string>
#include <map>

namespace mldb {

class DBServer {
public:
  DBServer(const DBServerConfig& config) : db_dir_(config.db_dir()),
  server_(config.warp_server_config()) { }
  void Start() {
    CreateDirectory(db_dir_);

    while (true) {
      int client_id;
      ClientMsg client_msg = server_.Recv(&client_id);

      if (client_msg.has_create_db()) {
      switch (client_msg.msg_case()) {
        case kCreateDBReq:
          CreateDBReqHandler(client_id, client_msg.create_db_req());
          break;
        case kIngestFileReq:
          IngestFileReqHandler(client_id, client_msg.ingest_file_req());
          break;
        default:
          LOG(ERROR) << "Unrecognized client msg case: "
            << client_msg.msg_case();
      }
    }
  }
private:
  void CreateDirectory(const boost::filesystem::path& dir) {
    // Create directory if necessary.
    if (boost::filesystem::exist(dir)) {
      CHECK(boost::filesystem::is_directory(dir));
    } else {
      CHECK(boost::filesystem::create_directory(dir));
    }
  }

  // Send a string reply.
  void SendGenericReply(int client_id, const std::string& msg) {
    ServerMsg reply_msg;
    reply_msg.mutable_generic_reply()->set_reply(msg);
    std::string data;
    reply_msg.SerializeToString(&data);
    server_.Send(client_id, data);
  }

  void CreateDBReqHandler(int client_id, const CreateDBReq& req) {
    // Append db name to db_dir_.
    const auto& db_config = req.db_config();
    auto db_path = db_dir_ / db_config.db_name();
    CreateDirectory(db_path);
    dbs_[db_config.db_name()] = make_unique<DB>(db_config);
    SendGenericReply(client_id, "Done creating DB");
  }

   IngestFileReqHandler(const IngestFileReq& req) {
    const auto& it = dbs_.fine(req.db_name());
    if (it == dbs_.cend()) {
      SendGenericReply(client_id, "DB " + req.db_name() + " not found.");
    }
    it->second->IngestFile(req);
  }

private:
  boost::filesystem::path db_dir_;

  WarpServer server_;

  std::map<std::string, <std::unique_ptr<DB>> dbs_;
  };

}  // namespace
