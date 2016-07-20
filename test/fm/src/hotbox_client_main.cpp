#include "client/hb_client.hpp"
#include "test/facility/test_facility.hpp"
#include "util/timer.hpp"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "iterparallel/iterparallel.hpp"

// #include <openmpi/mpi.h>
DEFINE_string(db_name, "", "Database name");
DEFINE_string(session_id, "test_session", "session identifier");
DEFINE_string(transform_config, "", "Transform config filename under "
    "hotbox/test/resource/");
DEFINE_int32(data_begin, 0, "Data begin");
DEFINE_int32(data_end, -1, "Data end");
int main(int argc, char *argv[]) {

//iterparallel::Context context(&argc, &argv, false);

  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
//  MPI_Init(&argc,&argv);
  hotbox::HBClient hb_client;
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
  LOG(INFO) << "# data: " << session.GetNumData();
  auto p = o_schema.GetName(4);
  LOG(INFO) << "o_schema(4): family: " << p.first << " feature_name: "
                           << p.second;
  int i = 0;
  hotbox::Timer timer;
  // Test move constructor of DataIterator.
  int num_transform_threads = 4;
  // int client_id, num_clients;
  int64_t num_data = session.GetNumData();
  // MPI_Comm_size(MPI_COMM_WORLD,&num_clients);
  // MPI_Comm_rank(MPI_COMM_WORLD,&client_id);
//  LOG(INFO) << "data: " << num_data;
//  LOG(INFO) << "client #" << client_id << ": " << num_data / num_clients * client_id;
  LOG(INFO) << "Hotbox Dataend: " << hotbox::kDataEnd;
  hotbox::DataIterator iter = session.NewDataIterator(0, 1000,
      num_transform_threads);
  //iter.Restart();
  hotbox::DataIterator it = std::move(iter);
  for (; it.HasNext();) {
    hotbox::FlexiDatum datum = it.GetDatum();
    LOG(INFO) << "Reading datum: " << i;
    i++;
  }
  
  LOG(INFO) << "Read " << i << " data. Time: " << timer.elapsed();
  // MPI_Finalize();
  return 0;
};
