// hotbox client experiment
// with cost model and control logic at client side

#include "client/hb_client.hpp"
#include "test/facility/test_facility.hpp"
#include "util/timer.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <tuple>
#include "metrics/metrics.hpp"

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

namespace {

void WriteLibSVM(const std::string& file_path,
  const std::vector<std::vector<int64_t>>& idx,
  const std::vector<std::vector<float>>& vals,
  const std::vector<int>& labels) {
  std::ofstream out(file_path);
  CHECK(out);
  for (int i = 0; i < idx.size(); ++i) {
    out << labels[i];
    for (int j = 0; j < idx[i].size(); ++j) {
      out << " " << idx[i][j] + 1 << ":" << vals[i][j];
    }
    out << "\n";
  }
  LOG(INFO) << "LibSVM output to " << file_path;
}

void printVector(const std::vector<int>& s) {
  for (auto &i : s) {
    std::cout<<i<<" ";
  }
  std::cout<<"\n";
}

int64_t data_begin;
int64_t data_end;
// helper function to create a new data iterator and run
// return execution time
float execute(hotbox::Session& session, std::vector<int>& tocache,
    std::vector<int>& cached, bool printMetrics) {
  std::cout<<"transforms to cache: "; printVector(tocache);
  std::cout<<"transforms cached: "; printVector(cached);
  session.SetTransformsToCache(tocache);
  session.SetTransformsCached(cached);
  int64_t i = 0;
  hotbox::Timer timer;
  auto it = session.NewDataIterator(data_begin, data_end, FLAGS_num_threads,
      FLAGS_num_io_threads, FLAGS_buffer_limit, FLAGS_batch_limit);
  for (; it->HasNext();) {
    hotbox::FlexiDatum datum = it->GetDatum();
    i++;
  }
  auto metrics = it->GetMetrics();
  metrics.print();
  return timer.elapsed();
}

}  // anonymous namespace

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
  int64_t num_data = session.GetNumData();
  int64_t num_data_per_worker = num_data / FLAGS_num_workers;
  data_begin = num_data_per_worker * FLAGS_worker_id;
  data_end = FLAGS_worker_id == FLAGS_num_workers - 1 ?
        num_data : data_begin + num_data_per_worker;
  LOG(INFO) << "Old Range: " << data_begin << " -> " << data_end;
  // align by atom boundary so no slicing will happen
  // because caching is supported at atom level
  //std::tie(data_begin, data_end) = session.GetRange(FLAGS_worker_id, FLAGS_num_workers);
  //LOG(INFO) << "New Range: " << data_begin << " -> " << data_end;

  // normal execution
  hotbox::Timer timer;
  int64_t i = 0;
  std::unique_ptr<hotbox::DataIteratorIf> it =
    session.NewDataIterator(data_begin, data_end, FLAGS_num_threads,
    FLAGS_num_io_threads, FLAGS_buffer_limit, FLAGS_batch_limit);
  for (; it->HasNext();) {
    hotbox::FlexiDatum datum = it->GetDatum();
    i++;
  }
  LOG(INFO) << "(Normal execution) Read " << i << " data. Time: " << timer.elapsed();

  std::vector<int> tocache, cached; 

  // decision from cost model
  auto metrics = it->GetMetrics();
  metrics.print();
  // note the generated value if stored in dense, doesn't affect the actual
  // storage
  for (int t = 0; t < metrics.t_transform.size(); ++t) {
    // all in seconds
    // time to write out, time to read in, time to stage in, compression,
    // decompression
    float io_const_cost = 0.05;
    float cache_in_cost = io_const_cost + metrics.n_generated_value[t]*4.0/1024/1024/100; // estimate size in mb then 100mb/s
    float cost = cache_in_cost + io_const_cost + metrics.n_generated_value[t]*4.0/1024/1024/1024/10; // mem cpy
    float gain = metrics.t_transform[t];
    LOG(INFO) << "Transform " << t << " generated: " << metrics.n_generated_value[t] << " time: " << metrics.t_transform[t];
    if (gain > cost) {
      LOG(INFO) << "Cost: " << cost << " Gain: " << gain << " (Cache)";
      tocache.push_back(i);
    } else {
      LOG(INFO) << "Cost: " << cost << " Gain: " << gain << " (Not to cache)";
    }
  }

  // cache all
  tocache.clear();
  cached.clear();
  for (int i = 0; i < metrics.ntransforms; ++i) {
    tocache.push_back(i);
  }
  LOG(INFO) << "(Cache all: tocache) Read " << i << " data. Time: " <<
    execute(session, tocache, cached, true);
  tocache.swap(cached);
  LOG(INFO) << "(Cache all: cached) Read " << i << " data. Time: " <<
    execute(session, tocache, cached, true);
  return 0;
};
