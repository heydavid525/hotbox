#pragma once

#include "db/db.hpp"
#include "db/proto/db.pb.h"
#include "db/util.hpp"
#include "util/warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
#include <boost/filesystem.hpp>
#include <string>
#include <map>

namespace mldb {

class DBServer {
public:
  DBServer(const DBServerConfig& config);

  void Start();

private:
  void CreateDirectory(const boost::filesystem::path& dir);

  // Send a string reply.
  void SendGenericReply(int client_id, const std::string& msg);

  void CreateDBReqHandler(int client_id, const CreateDBReq& req);

  void ReadFileReqHandler(int client_id, const ReadFileReq& req);

private:
  boost::filesystem::path db_dir_;

  WarpServer server_;

  std::map<std::string, std::unique_ptr<DB>> dbs_;
};

}  // namespace
