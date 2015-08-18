#include "util/warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
#include <gflags/gflags.h>
#include <glog/logging.h>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  mldb::WarpServer server;
  return 0;
};
