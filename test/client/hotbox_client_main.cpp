#include "client/client.hpp"
#include "test/facility/test_facility.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  mldb::Client hotbox_client;
  mldb::SessionOptions session_options;
  session_options.db_name = "test_db";
  session_options.session_id = "test_session";
  session_options.transform_config_path = mldb::GetTestDir() +
    "/resource/test_transform1.conf";
  session_options.output_store_type = mldb::OutputStoreType::DENSE;
  mldb::Session session = hotbox_client.CreateSession(session_options);
  mldb::OSchema o_schema = session.GetOSchema();
  LOG(INFO) << "OSchema: " << o_schema.ToString();
  auto p = o_schema.GetName(4);
  LOG(INFO) << "o_schema(4): family: " << p.first << " feature_name: "
                           << p.second;
  CHECK(session.GetStatus().ok());
  int i = 0;
  for (mldb::DataIterator it = session.NewDataIterator(0, 5); it.HasNext();
      it.Next()) {
    mldb::FlexiDatum datum = it.GetDatum();
    LOG(INFO) << datum.ToString();
    i++;
  }
  LOG(INFO) << "Read " << i << " data";
  return 0;
};
