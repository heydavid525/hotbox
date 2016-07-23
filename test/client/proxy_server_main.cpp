#include "client/proxy_server.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  hotbox::ProxyServer proxy_server;
  proxy_server.Start();

  return 0;
};
