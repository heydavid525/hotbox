#include "client/hb_client.hpp"
#include "test/facility/test_facility.hpp"
#include "util/timer.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <cstdint>
#include <fstream>

// Hotbox internal
//#include "db/proto/db.pb.h"
//#include "schema/proto/schema.pb.h"

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
DEFINE_bool(read_stats, false, "True to read stats from kStatsOutputPath."
  "False will generate the stats.");

const std::string kStatsOutputPath = "/users/wdai/tmp/stats.dat";

namespace {

void ReadStat(int64_t dim, std::vector<float>* mean, std::vector<float>* var) {
  LOG(INFO) << "Reading back stats";
  std::ifstream in(kStatsOutputPath);
  CHECK(in);
  for (int64_t i = 0; i < dim; ++i) {
    in >> (*mean)[i] >> (*var)[i];
  }
  LOG(INFO) << "Read stats from " << kStatsOutputPath;
}

}  // anonymous namespace

// Compute mean and variance of the data in one-pass using
// https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
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
  int64_t dim = o_schema.GetDimension();
  std::vector<float> means(dim);

  // Computing var uses 'm2', and reading it uses var
  std::vector<float> m2(dim);
  std::vector<float> var(dim);
  if (FLAGS_read_stats) {
    ReadStat(dim, &means, &var);
  }
  int64_t n = 0;
  //LOG(INFO) << "OSchema: " << o_schema.ToString();
  //auto p = o_schema.GetName(4);
  //LOG(INFO) << "o_schema(4): family: " << p.first << " feature_name: "
  //                         << p.second;
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
    std::vector<int64_t> idx = datum.MoveSparseIdx();
    std::vector<float> vals = datum.MoveSparseVals();
    n++;
    if (FLAGS_read_stats) {
      // Apply standardization.
      for (int j = 0; j < idx.size(); ++j) {
        int64_t dim_j = idx[j];
        vals[j] -= means[dim_j];
        vals[j] /= var[dim_j];
      }
    } else {
      // Update means and m2 (online calculation for mean/variance).
      for (int j = 0; j < idx.size(); ++j) {
        int64_t dim_j = idx[j];
        float delta = vals[j] - means[dim_j];
        means[dim_j] += delta / n;
        m2[dim_j] += delta * (vals[j] - means[dim_j]);
      }
    }
  }

  if (!FLAGS_read_stats) {
    // Save stats to disk
    std::ofstream out(kStatsOutputPath);
    CHECK(out);
    for (int64_t i = 0; i < dim; ++i) {
      float var = m2[i]/(n-1);
      if (var == 0) {
        var = 1;
      }
      out << means[i] << " " << var << "\n";
    }
    LOG(INFO) << "Stats output to " << kStatsOutputPath;
  }

  // Save the stats to proto.
  /*
  hotbox::StatProto proto;
  proto.mutable_stats()->Reserve(dim);
  for (int64_t i = 0; i < dim; ++i) {
    proto.add_stats();
  }
  */
  LOG(INFO) << "Read " << n << " data. Time: " << timer.elapsed();
  return 0;
};
