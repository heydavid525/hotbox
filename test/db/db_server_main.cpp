#include "db/db_server.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <iostream>
#include "db/proto/db.pb.h"
#include "test/facility/test_facility.hpp"
#include "io/filesys.hpp"
#include <string>
#include "util/all.hpp"

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  hotbox::DBServerConfig server_config;

  // Set db_dir
  bool found = false;
  std::string db_dir =
    hotbox::GlobalConfig::GetInstance().Get<std::string>(
        "db_dir", &found);
  if (!found) {
    db_dir = hotbox::GetTestBedDir() + "/test_db_root";
    LOG(INFO) << "Using default db_dir: " << db_dir;
  } else {
    LOG(INFO) << "db_dir: "  << db_dir;
  }
  server_config.set_db_dir(db_dir);

  // Set db_meta_dir
  std::string db_meta_dir =
    hotbox::GlobalConfig::GetInstance().Get<std::string>(
        "db_meta_dir", &found);
  if (!found) {
    db_meta_dir = hotbox::GetTestBedDir() + "/test_db_root_meta";
    LOG(INFO) << "Using default db_meta_dir: " << db_meta_dir;
  } else {
    LOG(INFO) << "db_meta_dir: "  << db_meta_dir;
  }
  server_config.set_db_dir_meta(db_meta_dir);

  hotbox::DBServer db_server(server_config);
  db_server.Start();

  return 0;
};
