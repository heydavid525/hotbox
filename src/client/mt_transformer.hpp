#pragma once

#include "db/proto/db.pb.h"
#include "schema/all.hpp"
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <queue>

namespace hotbox {

class MTTransformer {
public:
  MTTransformer(const SessionProto &session_proto, int io_threads,
          int transform_threads,
          const std::vector<std::string> &files);

  inline bool HasNextBatch() const;

  // will be blocked only if no batch is available
  // will transfer the returned pointer's ownership to caller. The calller is responsible to release it
  // will return NULL if all data has been transformed
  std::vector<FlexiDatum> *NextBatch();

  void Start();

private:
  // private functions
  void StartIOThreads();
  
  void StartTransformThreads();

  void IOThreadFunc(std::string &file);

  std::vector<FlexiDatum> *TransformThreadFunc(std::string &content);

private:
  std::vector<std::thread> io_workers_;
  std::vector<std::thread> tf_workers_;
  std::vector<std::function<void(TransDatum * )>> transforms_;
  std::queue<std::string> file_queue_;
  std::queue<std::vector<FlexiDatum> *> datum_batch_queue_;

  // mutex
  std::mutex io_que_mtx_;
  std::mutex tf_que_mtx_;
};

} // namespace hotbox

