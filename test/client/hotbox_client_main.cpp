#include "client/hb_client.hpp"
#include "test/facility/test_facility.hpp"
#include "util/timer.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <cstdint>

DEFINE_string(db_name, "", "Database name");
DEFINE_string(session_id, "test_session", "session identifier");
DEFINE_string(transform_config, "", "Transform config filename under "
    "hotbox/test/resource/");
DEFINE_bool(use_proxy, false, "true to use proxy server");
DEFINE_int32(num_proxy_servers, 1, "number of proxy server.");
DEFINE_int32(num_workers, 1, "num workers in total");
DEFINE_int32(worker_id, 0, "worker rank.");
DEFINE_int32(num_threads, 1, "num transform threads");
DEFINE_int32(num_io_threads, 1, "num IO threads");
DEFINE_int32(buffer_limit, 1, "");
DEFINE_int32(batch_limit, 1, "");

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  hotbox::HBClientConfig config;
  config.connect_proxy = FLAGS_use_proxy;
  config.num_proxy_servers = FLAGS_num_proxy_servers;
  hotbox::HBClient hb_client(config);
  LOG(INFO) << "HBClient Initialized";
  hotbox::SessionOptions session_options;
  session_options.db_name = FLAGS_db_name;
  session_options.session_id = FLAGS_session_id;
  session_options.transform_config_path = hotbox::GetTestDir() +
    "/resource/" + FLAGS_transform_config;
  session_options.output_store_type = hotbox::OutputStoreType::SPARSE;
  hotbox::Session session = hb_client.CreateSession(session_options);
  CHECK(session.GetStatus().IsOk());
  hotbox::OSchema o_schema = session.GetOSchema();
  LOG(INFO) << "output dim: " << o_schema.GetDimension();
  //LOG(INFO) << "OSchema: " << o_schema.ToString();
  auto p = o_schema.GetName(4);
  LOG(INFO) << "o_schema(4): family: " << p.first << " feature_name: "
                           << p.second;
  int i = 0;
  hotbox::Timer timer;
  int64_t num_data = session.GetNumData();
  int64_t num_data_per_worker = num_data / FLAGS_num_workers;
  int64_t data_begin = num_data_per_worker * FLAGS_worker_id;
  int64_t data_end = FLAGS_worker_id == FLAGS_num_workers - 1 ?
      num_data : data_begin + num_data_per_worker;
  std::unique_ptr<hotbox::DataIteratorIf> it =
    session.NewDataIterator(data_begin, data_end, FLAGS_num_threads,
    FLAGS_num_io_threads, FLAGS_buffer_limit, FLAGS_batch_limit);
  for (; it->HasNext();) {
    hotbox::FlexiDatum datum = it->GetDatum();
    // LOG_IF(INFO, i < 2) << datum.ToString();
    i++;
  }
  LOG(INFO) << "Read " << i << " data. Time: " << timer.elapsed();
  return 0;
};
