#include <glog/logging.h>
#include <string>
#include "db/db_server.hpp"
#include "util/all.hpp"
#include "util/rocksdb_util.hpp"


namespace mldb {

namespace {

// DBfile relative to db_dir_. Each DB is a line in the file.
const std::string kDBRootFile = "/DB_root_file";

}  // anonymous namespace

DBServer::DBServer(const DBServerConfig& config) : db_dir_(config.db_dir()) { }

void DBServer::Start() {
  Init();
  LOG(INFO) << "DBServer running. DB path is " << db_dir_;

  while (true) {
    int client_id;
    ClientMsg client_msg = server_.Recv(&client_id);

    // Comment(wdai): I didn't use switch(client_msg.msg_cast()) because it
    // has weird capitalization issue, like kCreateDbReq instead of
    // kCreateDBReq. But I have to admit that switch format is more readable.
    if (client_msg.has_create_db_req()) {
      LOG(INFO) << "Received create_db_req";
      CreateDBReqHandler(client_id, client_msg.create_db_req());
    } else if (client_msg.has_read_file_req()) {
      LOG(INFO) << "Received read_file_req";
      ReadFileReqHandler(client_id, client_msg.read_file_req());
    } else if (client_msg.has_db_server_shutdown_req()) {
      LOG(INFO) << "Received db_server_shutdown_req";
      return;
    } else if (client_msg.has_create_session_req()) {
      LOG(INFO) << "Received create_session_req";
      CreateSessionHandler(client_id, client_msg.create_session_req());
    } else {
      LOG(ERROR) << "Unrecognized client msg case: "
        << client_msg.msg_case();
    }
  }
}

void DBServer::Init() {
  CreateDirectory(db_dir_);
  RegisterParsers();
  RegisterCompressors();
  InitFromDBRootFile();
}

void DBServer::InitFromDBRootFile() {
  auto db_root_file_path = db_dir_ + kDBRootFile;

  if (!Exists(db_root_file_path)) {
    LOG(INFO) << "DB File doesn't exist yet. This must be a new DB";
    return;
  }
  //auto db_root_file = ReadCompressedFile(db_root_file_path,
  //    Compressor::NO_COMPRESS);
  // DBRootFile db_root;
  // db_root.ParseFromString(db_root_file);
  // for (int i = 0; i < db_root.db_names_size(); ++i) {
  //   std::string db_name = db_root.db_names(i);
  //   std::string db_path = db_dir_ + "/" + db_name;
  //   dbs_[db_name] = make_unique<DB>(db_path);
  // }

  // **** RocksDB Persistency. **********
  std::unique_ptr<rocksdb::DB> db(OpenRocksDB(db_root_file_path));
  std::string db_root_file;
  rocksdb::Status s = db->Get(rocksdb::ReadOptions(), kDBRootFile, &db_root_file);
  assert(s.ok());
  DBRootFile db_root;
  db_root.ParseFromString(db_root_file);
  for (int i = 0; i < db_root.db_names_size(); ++i) {
    std::string db_name = db_root.db_names(i);
    std::string db_path = db_dir_ + "/" + db_name;
    dbs_[db_name] = make_unique<DB>(db_path);
  }
}

void DBServer::CommitToDBRootFile() const {
  auto db_root_file_path = db_dir_ + kDBRootFile;
  DBRootFile db_root;
  for (const auto& p : dbs_) {
    db_root.add_db_names(p.first);
  }
  // WriteCompressedFile(db_root_file_path, SerializeProto(db_root),
  //    Compressor::NO_COMPRESS);

  // **** RocksDB Persistency. **********
  std::unique_ptr<rocksdb::DB> db(OpenRocksDB(db_root_file_path));
  // Put key(kDBRootFile)-value(SerializeProto(db_root)).
  rocksdb::Status s = db->Put(rocksdb::WriteOptions(), kDBRootFile, SerializeProto(db_root));
  assert(s.ok());
}

void DBServer::CreateDirectory(const std::string& dir) {
  // Create directory if necessary. as we would need to a separate store 
  // for different users/sessions.
  if (Exists(dir)) {
    CHECK(Is_Directory(dir));
  } else {
    CHECK(Create_Directory(dir) == 0);
  }
}

void DBServer::SendGenericReply(int client_id, const std::string& msg) {
  ServerMsg reply_msg;
  reply_msg.mutable_generic_reply()->set_msg(msg);
  server_.Send(client_id, reply_msg);
}

void DBServer::CreateDBReqHandler(int client_id, const CreateDBReq& req) {
  // Append db name to db_dir_.
  auto db_config = req.db_config();
  auto db_path = db_dir_ + "/" + db_config.db_name();
  CreateDirectory(db_path);
  db_config.set_db_dir(db_path);
  dbs_[db_config.db_name()] = make_unique<DB>(db_config);
  SendGenericReply(client_id, "Done creating DB");
  CommitToDBRootFile();
}

void DBServer::ReadFileReqHandler(int client_id, const ReadFileReq& req) {
  const auto& it = dbs_.find(req.db_name());
  if (it == dbs_.cend()) {
    SendGenericReply(client_id, "DB " + req.db_name() + " not found.");
  }
  std::string reply_msg = it->second->ReadFile(req);
  SendGenericReply(client_id, reply_msg);
}

void DBServer::CreateSessionHandler(int client_id, const CreateSessionReq& req) {
  auto session_options = req.session_options();
  ServerMsg reply_msg;
  auto create_session_reply = reply_msg.mutable_create_session_reply();

  // Check DB exists.
  auto it = dbs_.find(session_options.db_name());
  if (it == dbs_.cend()) {
    create_session_reply->set_status_code(StatusCode::DB_NOT_FOUND);
    server_.Send(client_id, reply_msg);
    LOG(INFO) << "CreateSession: DB " << session_options.db_name()
      << " not found.";
  }

  // Check if session_id exists.
  auto p = curr_sessions_.insert(session_options.session_id());
  if (p.second) {
    // This is a new session.
    it->second->CreateSession(session_options);
  } else {
    LOG(INFO) << "Session already exists. Disregard CreateSessionRequest";
    create_session_reply->set_status_code(StatusCode::SESSION_ALREADY_EXISTS);
    server_.Send(client_id, reply_msg);
  }

  // TODO(wdai)
}

}  // namespace mldb
