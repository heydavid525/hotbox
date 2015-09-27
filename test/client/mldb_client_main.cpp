#include "client/mldb_client.hpp"
#include "test/facility/test_facility.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  mldb::MLDBClient mldb;
  mldb::SessionOptions session_options;
  session_options.db_name = "test_db";
  session_options.session_id = "test_session";
  session_options.transform_config_path = mldb::GetTestDir() +
    "/resource/test_transform1.conf";
  mldb::Status status = mldb.CreateSession(session_options);
  CHECK(status.ok());
  return 0;
};
