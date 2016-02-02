#include "client/hb_client.hpp"
#include "test/facility/test_facility.hpp"
#include "util/timer.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>

DEFINE_string(transform_config, "", "Transform config filename under "
    "hotbox/test/resource/");

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  hotbox::HBClient hb_client;
  LOG(INFO) << "HBClient Initialized";
  hotbox::SessionOptions session_options;
  session_options.db_name = "test_db";
  session_options.session_id = "test_session";
  session_options.transform_config_path = hotbox::GetTestDir() +
    "/resource/" + FLAGS_transform_config;
  session_options.output_store_type = hotbox::OutputStoreType::SPARSE;
  hotbox::Session session = hb_client.CreateSession(session_options);
  CHECK(session.GetStatus().IsOk());
  hotbox::OSchema o_schema = session.GetOSchema();
  LOG(INFO) << "output dim: " << o_schema.GetDimension();
  LOG(INFO) << "OSchema: " << o_schema.ToString();
  auto p = o_schema.GetName(4);
  LOG(INFO) << "o_schema(4): family: " << p.first << " feature_name: "
                           << p.second;
  int i = 0;
  hotbox::Timer timer;
  // Test move constructor of DataIterator.
  hotbox::DataIterator iter = session.NewDataIterator();
  for (hotbox::DataIterator it = std::move(iter); it.HasNext(); it.Next()) {
    hotbox::FlexiDatum datum = it.GetDatum();
    //LOG(INFO) << datum.ToString();
    i++;
  }
  LOG(INFO) << "Read " << i << " data. Time: " << timer.elapsed();
  return 0;
};
