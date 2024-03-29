#include <glog/logging.h>
#include "db/db_server.hpp"
#include "util/all.hpp"
#include "util/file_util.hpp"
#include "util/global_config.hpp"
#include <string>
#include <algorithm>

namespace hotbox {

namespace {

// DBfile relative to db_dir_. Each DB is a line in the file.
const std::string kDBRootFile = "/DB_root_file";

}  // anonymous namespace

DBServer::DBServer(const DBServerConfig& config) :
  db_dir_(config.db_dir()),
  db_dir_meta_(config.db_dir_meta().empty() ?
      db_dir_ : config.db_dir_meta()),
server_(WarpServerConfig()) { }

void DBServer::Start() {
  Init();
  LOG(INFO) << "DBServer running. DB path is " << db_dir_ << " meta dir: "
    << db_dir_meta_;

  while (true) {
    int client_id;
    ClientMsg client_msg = server_.Recv(&client_id);

    // Comment(wdai): I didn't use switch(client_msg.msg_cast()) because it
    // has weird capitalization issue, like kCreateDbReq instead of
    // kCreateDBReq. But I have to admit that switch format is more readable.
    if (client_msg.has_create_db_req()) {
      LOG(INFO) << "Received create_db_req from client " << client_id;
      CreateDBReqHandler(client_id, client_msg.create_db_req());
    } else if (client_msg.has_read_file_req()) {
      LOG(INFO) << "Received read_file_req from client " << client_id;
      ReadFileReqHandler(client_id, client_msg.read_file_req());
    } else if (client_msg.has_db_server_shutdown_req()) {
      LOG(INFO) << "Received db_server_shutdown_req from client " << client_id;
      return;
    } else if (client_msg.has_create_session_req()) {
      LOG(INFO) << "Received create_session_req from client " << client_id;
      CreateSessionHandler(client_id, client_msg.create_session_req());
    } else if (client_msg.has_close_session_req()) {
      LOG(INFO) << "Received close_session_req from client " << client_id;
      CloseSessionHandler(client_id, client_msg.close_session_req());
    } else {
      LOG(ERROR) << "Unrecognized client msg case: "
        << client_msg.msg_case();
    }
  }
}

void DBServer::Init() {
  CreateDirectory(db_dir_);
  CreateDirectory(db_dir_meta_);
  RegisterAll();
  InitFromDBRootFile();
}

void DBServer::InitFromDBRootFile() {
  auto db_root_file_path = db_dir_ + kDBRootFile;

  if (!io::Exists(db_root_file_path)) {
    LOG(INFO) << "DB File (" << db_root_file_path << ") doesn't exist yet. "
      "This must be a new DB";
    return;
  }
  auto db_root_file_str = io::ReadFile(db_root_file_path);
  DBRootFile db_root = StreamDeserialize<DBRootFile>(db_root_file_str);
  //CHECK(db_root.ParseFromString(db_root_file));
  std::stringstream ss;
  for (int i = 0; i < db_root.db_names_size(); ++i) {
    std::string db_name = db_root.db_names(i);
    //std::string db_path = db_dir_ + "/" + db_name;
    std::string db_path_meta = db_dir_meta_ + "/" + db_name;
    LOG(INFO) << "Load DB MetaFile (" << db_path_meta << ")";
    dbs_[db_name] = make_unique<DB>(db_path_meta);
    ss << db_name << std::endl;
  }
  LOG(INFO) << "DBServer initialized from " << db_root_file_path
    << ". List of DB:\n" << ss.str();
}

void DBServer::CommitToDBRootFile() const {
  auto db_root_file_path = db_dir_ + kDBRootFile;

  DBRootFile db_root;
  for (const auto& p : dbs_) {
    db_root.add_db_names(p.first);
  }
  LOG(INFO) << "Write to " << db_root_file_path << " with compressor "
    << Compressor::NO_COMPRESS;

  io::WriteCompressedFile(db_root_file_path, StreamSerialize(db_root),
      Compressor::NO_COMPRESS);
}

void DBServer::CreateDirectory(const std::string& dir) {
  // Create directory if necessary. as we would need to a separate store
  // for different users/sessions.
  CHECK(io::CreateDirectory(dir) == 0);
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
  auto db_path_meta = db_dir_meta_ + "/" + db_config.db_name();
  CreateDirectory(db_path);
  CreateDirectory(db_path_meta);

  db_config.set_db_dir(db_path);
  db_config.set_db_dir_meta(db_path_meta);
  const auto it = dbs_.find(db_config.db_name());
  std::string msg;
  if (it == dbs_.cend()) {
    dbs_[db_config.db_name()] = make_unique<DB>(db_config);
    msg = "Done creating DB " + db_config.db_name();
    SendGenericReply(client_id, msg);
    CommitToDBRootFile();
  } else {
    msg = "DB " + db_config.db_name() + " already exists";
    SendGenericReply(client_id, msg);
  }
  LOG(INFO) << msg;
}

void DBServer::ReadFileReqHandler(int client_id, const ReadFileReq& req) {
  const auto& it = dbs_.find(req.db_name());
  if (it == dbs_.cend()) {
    SendGenericReply(client_id, "DB " + req.db_name() + " not found.");
  }
  auto& global_config = GlobalConfig::GetInstance();
  int num_io = global_config.Get<int>("num_io_ingest");
  std::string reply_msg;
  /*
  if (num_io == 1) {
    reply_msg = it->second->ReadFile(req);
  } else {
    reply_msg = it->second->ReadFileMT(req);
  }
  */
  reply_msg = it->second->ReadFileMT(req);
  SendGenericReply(client_id, reply_msg);
}

void DBServer::CreateSessionHandler(int client_id, const CreateSessionReq& req) {
  auto session_options = req.session_options();
  ServerMsg reply_msg;
  auto create_session_reply = reply_msg.mutable_create_session_reply();

  // Check DB exists.
  auto db_it = dbs_.find(session_options.db_name());
  if (db_it == dbs_.cend()) {
    create_session_reply->set_status_code(StatusCode::DB_NOT_FOUND);
    std::string msg = "CreateSession: DB " + session_options.db_name()
      + " not found.";
    create_session_reply->set_msg(msg);
    LOG(INFO) << msg;
    server_.Send(client_id, reply_msg);
    return;
  }

  std::string session_id = session_options.session_id();

  // Check if session_id exists.
  auto it = curr_sessions_.find(session_id);
  if (it != curr_sessions_.cend()) {
    session_clients_[session_id].insert(client_id);
    std::string msg = "Session already exists. Joining existing session.";
    LOG(INFO) << msg;
    create_session_reply->set_msg(msg);
    create_session_reply->set_status_code(StatusCode::OK);
    *(create_session_reply->mutable_session_proto()) = it->second;
    server_.Send(client_id, reply_msg);
    return;
  }

  // This is a new session.
  SessionProto session = db_it->second->CreateSession(session_options);
  std::string msg = "Session created.";
  create_session_reply->set_status_code(StatusCode::OK);
  create_session_reply->set_msg(msg);
  LOG(INFO) << "Client " << client_id << " created session " << session_id;
  *(create_session_reply->mutable_session_proto()) = session;
  curr_sessions_.insert(std::make_pair(session_id, session));
  session_clients_[session_id].insert(client_id);
  server_.Send(client_id, reply_msg);
}

void DBServer::CloseSessionHandler(int client_id, const CloseSessionReq& req) {
  auto session_id = req.session_id();
  const auto& clients = session_clients_[session_id];
  const auto& find_it = clients.find(client_id);
  auto& mutable_clients = session_clients_[session_id];
  if (find_it != clients.cend()) {
    mutable_clients.erase(client_id);
    if (mutable_clients.empty()) {
      LOG(INFO) << "Session " << session_id << " ends";
      curr_sessions_.erase(session_id);
      session_clients_.erase(session_id);
    }
    SendGenericReply(client_id, "Removed from session: " + session_id);
    LOG(INFO) << "client " << client_id << " removed from session "
      << session_id;
  } else {
    SendGenericReply(client_id, "You were not in session: " + session_id);
    LOG(INFO) << "CloseSessionHandler: client " << client_id
      << " weren't in session.";
  }
}

}  // namespace hotbox
