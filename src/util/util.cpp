#include "util/util.hpp"
#include "util/proto/util.pb.h"
#include <string>
#include <algorithm>

namespace mldb {

std::string SizeToReadableString(size_t size) {
  double s = static_cast<double>(size);
  const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

  int i = 0;
  while (s > 1024) {
    s /= 1024;
    i++;
  }

  char buf[100];
  sprintf(buf, "%.*f %s", std::max(i, 2), s, units[i]);
  return std::string(buf);
}

std::string SerializeProto(const google::protobuf::Message& msg) {
  std::string data;
  msg.SerializeToString(&data);
  return data;
}

}  // namespace mldb
