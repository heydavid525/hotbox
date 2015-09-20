#include "db/db_server.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "db/proto/db.pb.h"
#include <boost/filesystem.hpp>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  mldb::DBServerConfig server_config;
  std::string db_test_dir = boost::filesystem::path(__FILE__)
    .parent_path().parent_path().parent_path().append("db_testbed").string();
  server_config.set_db_dir(db_test_dir);
  // use default warp server config.
  server_config.mutable_warp_server_config();

  mldb::DBServer db_server(server_config);
  db_server.Start();

  return 0;
};
