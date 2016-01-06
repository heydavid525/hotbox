#pragma once
#include "util/all.hpp"

namespace hotbox {

// Status uses Status enum from util/proto/warp_msg.proto.
class Status {
public:
  Status() : status_code_(StatusCode::OK) { }

  Status(StatusCode status) : status_code_(status) { }

  bool IsOk() const {
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

} // namespace hotbox
