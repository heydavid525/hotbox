#pragma once
#include "util/warp_client.hpp"
#include "util/global_config.hpp"
#include "client/proto/client.pb.h"
#include "util/proto/warp_msg.pb.h"
#include "io.dmlc/util.hpp"
#include <google/protobuf/text_format.h>
#include <glog/logging.h>
#include <string>
#include <cstdint>
#include <sstream>
#include "io/fstream.hpp"

namespace mldb {

// Status uses Status enum from util/proto/warp_msg.proto.
class Status {
public:
  Status() : status_code_(StatusCode::OK) { }

  Status(StatusCode status) : status_code_(status) { }

  bool ok() const {
    return status_code_ == StatusCode::OK;
  }

  // TODO(wdai): Come up with a way to define string next to StatusCode in
  // the proto definition.
  std::string ToString() const {
    switch (status_code_) {
      case StatusCode::OK:
        return "OK";
      case StatusCode::DB_NOT_FOUND:
        return "DB not found";
      default:
        LOG(ERROR) << "Unrecognized status code: " << status_code_;
        return "";
    }
  }

private:
  StatusCode status_code_;
};

class SessionOptions {
public:
  std::string db_name;
  std::string session_id;
  std::string transform_config_path;

  // TODO(wdai): Suppor these.
  //int64_t max_examples;
  //double subsample_rate;

  // Create and validate proto.
  SessionOptionsProto GetProto() const {
    SessionOptionsProto proto;
    CHECK_NE("", db_name);
    proto.set_db_name(db_name);
    proto.set_session_id(session_id);

    // Validate transform file.
    //std::string config_str = dmlc_util::ReadFile(transform_config_path);
    io::ifstream is(transform_config_path);
    std::stringstream buffer;
    buffer << is.rdbuf();
    std::string config_str = buffer.str();
    TransformConfigList configs;
    CHECK(google::protobuf::TextFormat::ParseFromString(config_str, &configs))
      << "Error in parsing " << transform_config_path;
    auto mutable_configs = proto.mutable_transform_config_list();
    *mutable_configs = configs;
    return proto;
  }
};

// A read client that performs transform using a pool of threads.
class MLDBClient {
public:
  // Create a sesion. session_info has session_id, transform info etc.
  Status CreateSession(const SessionOptions& session_options) noexcept {
    LOG(INFO) << "Create session";
    ClientMsg client_msg;
    auto mutable_req = client_msg.mutable_create_session_req();
    *mutable_req->mutable_session_options() = session_options.GetProto();
    ServerMsg server_reply = client_.SendRecv(client_msg);
    CHECK(server_reply.has_create_session_reply());
    auto session_reply = server_reply.create_session_reply();
    LOG(INFO) << session_reply.msg();
    return Status(session_reply.status_code());
  }

private:
  WarpClient client_;
};

}  // namespace mldb
