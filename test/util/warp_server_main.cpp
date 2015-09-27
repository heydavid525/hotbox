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
  CHECK(req_msg.has_generic_req());
  std::string req_str = req_msg.generic_req().req();
  LOG(INFO) << "Got generic request from client " << client_id << " request: "
    << req_str;
  mldb::ServerMsg reply_msg;
  auto generic_reply = reply_msg.mutable_generic_reply();
  generic_reply->set_msg("got it");
  server.Send(client_id, reply_msg);
  LOG(INFO) << "Server gave generic reply";
  return 0;
};
