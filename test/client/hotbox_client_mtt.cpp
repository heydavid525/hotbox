#include <client/mt_transformer.hpp>
#include "client/hb_client.hpp"
#include "test/facility/test_facility.hpp"
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
  session_options.db_name = "url_combined_db";
  session_options.session_id = "url_combined_db_session#213234";
  session_options.transform_config_path = hotbox::GetTestDir() +
    "/resource/" + FLAGS_transform_config;
  session_options.output_store_type = hotbox::OutputStoreType::SPARSE;
  hotbox::Session session = hb_client.CreateSession(session_options);
  CHECK(session.GetStatus().IsOk());
  hotbox::OSchema o_schema = session.GetOSchema();
  LOG(INFO) << "OSchema: " << o_schema.ToString();
  auto p = o_schema.GetName(4);
  LOG(INFO) << "o_schema(4): family: " << p.first << " feature_name: "
                           << p.second;
  int batch = 0, data = 0;
  hotbox::MTTransformer *mtt = nullptr;
  for(mtt = session.NewMTTransformer(0, -1, 1, 4); mtt->HasNextBatch();){
    auto *vec = mtt->NextBatch();
    batch++;
	data+= vec->size();
    // do something with vec

    delete vec;
  }
  delete mtt;
  LOG(INFO) <<"read "<< batch << " batches, " << data << " data";
  return 0;
};

