#include <glog/logging.h>
#include "db/db_server.hpp"
#include "util/util.hpp"

namespace mldb {

DBServer::DBServer(const DBServerConfig& config) : db_dir_(config.db_dir()) { }

void DBServer::Start() {
  CreateDirectory(db_dir_);
  RegisterParsers();
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

void DBServer::CreateDirectory(const boost::filesystem::path& dir) {
  // Create directory if necessary.
  if (boost::filesystem::exists(dir)) {
    CHECK(boost::filesystem::is_directory(dir));
  } else {
    CHECK(boost::filesystem::create_directory(dir));
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
  auto db_path = db_dir_ / db_config.db_name();
  CreateDirectory(db_path);
  db_config.set_db_dir(db_path.string());
  dbs_[db_config.db_name()] = make_unique<DB>(db_config);
  SendGenericReply(client_id, "Done creating DB");
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