#include "util/warp_server.hpp"
#include "util/proto/warp_msg.pb.h"
#include <gflags/gflags.h>
#include <glog/logging.h>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  mldb::WarpServer server;
  int client_id;
  mldb::ClientMsg req_msg = server.Recv(&client_id);
  CHECK(req_msg.has_dummy_req());
  std::string req_str = req_msg.dummy_req().req();
  LOG(INFO) << "Got dummy request from client " << client_id << " request: "
    << req_str;
  return 0;
};
