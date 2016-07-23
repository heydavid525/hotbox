#pragma once
#include "db/proto/db.pb.h"
#include <string>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>

namespace hotbox {

class SessionOptions {
public:
  std::string db_name;
  std::string session_id;
  std::string transform_config_path;
  OutputStoreType output_store_type{OutputStoreType::SPARSE};

  SessionOptions() { }
  explicit SessionOptions(const SessionOptionsProto& proto) : proto_init_(true),
  proto_(proto) { }

  // Create and validate proto.
  SessionOptionsProto GetProto() const {
    if (proto_init_) {
      return proto_;
    }
    SessionOptionsProto proto;
    CHECK_NE("", db_name);
    proto.set_db_name(db_name);
    proto.set_session_id(session_id);
    proto.set_output_store_type(output_store_type);

    // Validate transform file.
    std::string config_str = io::ReadCompressedFile(transform_config_path,
        Compressor::NO_COMPRESS);
    TransformConfigList configs;
    CHECK(google::protobuf::TextFormat::ParseFromString(config_str, &configs))
      << "Error in parsing " << transform_config_path;
    auto mutable_configs = proto.mutable_transform_config_list();
    *mutable_configs = configs;
    return proto;
  }

private:
  bool proto_init_{false};
  SessionOptionsProto proto_;
};

}  // namespace hotbox
