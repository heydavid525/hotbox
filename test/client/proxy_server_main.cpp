#include "client/proxy_server.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>

DEFINE_int32(server_id, 0, "Server ID, starting from 0");

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  hotbox::ProxyServer proxy_server(FLAGS_server_id);
  proxy_server.Start();

  return 0;
};
