#pragma once
#include "util/warp_client.hpp"
#include "util/global_config.hpp"
#include "client/proto/client.pb.h"
#include "io.dmlc/util.hpp"
#include <google/protobuf/text_format.h>
#include <string>
#include <cstdint>

namespace mldb {

class Status {
public:
  Status() : status_(kOk) { }

  bool ok() const {
    return status_ == kOk;
  }

private:
  enum Code {
    kOk = 0
  };

  Code status_;
};

class SessionOptions {
public:
  std::string db_name;
  std::string session_id;
  std::string transform_config_path;

  // TODO(wdai): Suppor these.
  //int64_t max_examples;
  //double subsample_rate;

  SessionOptionsProto GetProto() const {
    SessionOptionsProto proto;
    proto.set_session_id(session_id);

    // Validate transform file.
    std::string config_str = dmlc_util::ReadFile(transform_config_path);
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
    auto session_options_proto = session_options.GetProto();
    CreateSessionReq req;
    auto mutable_session_opt = req.mutable_session_options();
    *mutable_session_opt = session_options_proto;
    std::string data;
    req.SerializeToString(&data);
    ServerMsg server_reply = client_.SendRecv(data);
    auto session_reply = server_reply.create_session_reply();
    LOG(INFO) << session_reply.msg();
    return Status();
  }

private:
  WarpClient client_;
};

}  // namespace mldb
