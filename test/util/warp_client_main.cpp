#include "util/warp_client.hpp"
#include "util/proto/warp_msg.pb.h"
#include <gflags/gflags.h>
#include <glog/logging.h>

DEFINE_string(server_ip, "", "server ip.");

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  mldb::WarpClient client;
  mldb::ClientMsg req_msg;
  auto generic_req = req_msg.mutable_generic_req();
  generic_req->set_req("hello world");
  std::string data;
  req_msg.SerializeToString(&data);
  client.Send(data);
  LOG(INFO) << "client done sending generic request.";
  auto generic_reply = client.Recv();
  LOG(INFO) << "Client got server's generic reply: "
    << generic_reply.generic_reply().reply();
  return 0;
};
