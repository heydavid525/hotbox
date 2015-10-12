#pragma once
//#include <boost/filesystem.hpp>
#include <string>
#include <set>
#include <map>

#include "db/db.hpp"
#include "db/proto/db.pb.h"
#include "util/warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
//#include "io.dmlc/filesys.h"



namespace mldb {

// Comment(wdai): Currently list of DB are stored as a DB file under db_dir_,
// and each DB is stored under a folder (e.g., db_dir_/test_db for DB named
// 'test_db'.) Each DB folder contains /schema and /atom. DBServer reads DB
// file and initialize all the DB schemas to memory.
//
// TODO(weiren): We shouldn't need to read everything into memory (though
// chances are we can keep schema in memory.) Use RocksDB to make it better.
//
// TODO(weiren): Replace boost::filesystem with dmlc IO.
class DBServer {
public:
  DBServer(const DBServerConfig& config);

  void Start();

private:
  void Init();

  // Get DB stored under db_dir_.
  void InitFromDBRootFile();

  // Write DB info to disk.
  void CommitToDBRootFile() const;

  void CreateDirectory(const std::string& dir);

  // Send a string reply.
  void SendGenericReply(int client_id, const std::string& msg);

  void CreateDBReqHandler(int client_id, const CreateDBReq& req);

  void ReadFileReqHandler(int client_id, const ReadFileReq& req);

  void CreateSessionHandler(int client_id, const CreateSessionReq& req);

  void CloseSessionHandler(int client_id, const CloseSessionReq& req);

private:
  //boost::filesystem::path db_dir_;
  std::string db_dir_;

  WarpServer server_;

  std::map<std::string, std::unique_ptr<DB>> dbs_;

  std::map<std::string, SessionProto> curr_sessions_;

  // Maintain a list of client_id for each session.
  std::map<std::string, std::vector<int>> session_clients_;
};

}  // namespace
