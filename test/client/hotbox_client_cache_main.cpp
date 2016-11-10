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
#include <unistd.h>
#include <sstream>

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
// because tf is delegated to tf and internally parallelized
// so usually run with few worker thread
// when with cache, we want many threads to work on transformation
DEFINE_string(transform_cached, "", "comma separated list of transformations cached");
DEFINE_string(transform_tocache, "", "comma separated list of transformations to cache");

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

std::string printVector(const std::vector<int>& s) {
  std::stringstream ss;
  for (auto &i : s) {
    ss<<i<<",";
  }
  ss<<"\n";
  return ss.str();
}

int64_t data_begin;
int64_t data_end;
// helper function to create a new data iterator and run
// return execution time
hotbox::TransStats execute(hotbox::Session& session, std::vector<int>& tocache,
    std::vector<int>& cached, bool printMetrics) {
  LOG(INFO)<<"transforms to cache: "<< printVector(tocache);
  LOG(INFO)<<"transforms cached: "<< printVector(cached);
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
  auto elapsed = timer.elapsed();
  auto metrics = it->GetMetrics();
  metrics.print();
  LOG(INFO) << "Read " << i << " data. Time: " << elapsed;
  return metrics;
}

void parseTransList(const std::string& s, std::vector<int>& v) {
  std::stringstream ss(s);
  int t;
  char c;
  while(ss>>t) {
    ss>>c;
    v.push_back(t);
  }
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
  //data_begin = num_data_per_worker * FLAGS_worker_id;
  //data_end = FLAGS_worker_id == FLAGS_num_workers - 1 ?
        //num_data : data_begin + num_data_per_worker;
  //LOG(INFO) << "Old Range: " << data_begin << " -> " << data_end;
  // align by atom boundary so no slicing will happen
  // because caching is supported at atom level
  std::tie(data_begin, data_end) = session.GetRange(FLAGS_worker_id, FLAGS_num_workers);
  LOG(INFO) << "New Range: " << data_begin << " -> " << data_end;

  /* test
  for (int i = 0; i < FLAGS_num_workers; ++i) {
    int a,b,c,d,e,f,g,h;
    std::tie(a, b) = session.GetRange(i, FLAGS_num_workers);
    std::tie(c, d) = session.GetAtomRange(a, b);
    e = num_data_per_worker * i;
    f = i == FLAGS_num_workers - 1 ? num_data : e + num_data_per_worker;
    std::tie(g, h) = session.GetAtomRange(e, f);
    LOG(INFO) << "Worker " << i << " Old Range: " << e << " -> " << f << " Atom: " << g << " -> " << h << " total " << h - g << " atoms, " << f-e << " datums.";
    LOG(INFO) << "Worker " << i << " New Range: " << a << " -> " << b << " Atom: " << c << " -> " << d << " total " << d - c << " atoms, " << b-a << " datums.";
  }
  return 0;
  */

  std::vector<int> tocache, cached; 
  parseTransList(FLAGS_transform_cached, cached);
  parseTransList(FLAGS_transform_tocache, tocache);

  hotbox::TransStats metrics;
  metrics = execute(session, tocache, cached, true);
  // decision from cost model
  // note the generated value if stored in dense, doesn't affect the actual
  // storage
  tocache.clear();
  cached.clear();
  for (int t = 0; t < session.GetNumTrans(); ++t) {
    if (metrics.decision(t)) {
      tocache.push_back(t);
    }
  }
  std::cout<<"Decision: " << printVector(tocache);
  return 0;
};
