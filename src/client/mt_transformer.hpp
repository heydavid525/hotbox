#pragma once

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include "db/proto/db.pb.h"
#include "schema/all.hpp"

namespace hotbox {

/*
 MTTransformer works like this
 1. new files will be added to file_queue_
 2. io_workers will pick up file from file_queue_, load it into buffer and then
    put the buffer into buffer_queue_
 3. transform workers will pick up buffer from buffer_queue, decompress,
    deserialize and transform it. Then transform worker will put the transformed
    data to datum_batch_queue_
 3. NextBatch() gets transformed data batch
*/

class MTTransformer {
 public:
  // @files will be added to io_queue
  // deprecated since atom files are not independent from each other for now
  MTTransformer(const SessionProto &session_proto,
                const std::vector<std::string> &files,
                std::vector<std::function<void(TransDatum *)>> transforms,
                int num_io_threads,
                int num_transform_threads,
                int buffer_limit,
                int batch_limit) = delete;

  //
  MTTransformer(const SessionProto &session_proto,
                std::vector<std::function<void(TransDatum *)>> transforms,
                BigInt data_begin, BigInt data_end,
                int num_io_threads,
                int num_transform_threads,
                int buffer_limit,
                int batch_limit);

  ~MTTransformer();

  // check if there is any transformed or untransformed data batch
  bool HasNextBatch() const;

  // will be blocked only if no batch is available
  // will transfer the returned pointer's ownership to caller.
  // The calller is responsible to release it
  // will return nullptr if HasNextBatch returns false
  std::vector<FlexiDatum> *NextBatch();

  void Start();

 private:
  struct IoTask {
    // global_bytes_offset ranges within a atom file (maybe two)
    std::size_t file_begin;
    std::size_t file_end;
    std::size_t global_bytes_offsets_begin;
    std::size_t global_bytes_offsets_end;
  };
  // a IoTask may generate many TfTasks
  struct TfTask {
    BigInt idx;
    std::shared_ptr<std::string> shared_buf;  // shared buffer
    std::size_t offset;  // offset within shared_buf
    std::size_t length;  // buffer length
  };

  void
  Translate(BigInt data_begin, BigInt data_end);

  void Destory();

  void IoTaskLoop();

  void TransformTaskLoop();


  const SessionProto &session_proto_;

  std::vector<std::thread> io_workers_;
  std::vector<std::thread> tf_workers_;
  std::vector<std::function<void(TransDatum *)>> transforms_;
  std::queue<IoTask> io_queue_;  // io files queue
  std::queue<TfTask> bf_queue_;  // buffer queue
  std::queue<std::vector<FlexiDatum> *> bt_queue_;  // batch queue

  // mutex
  std::mutex io_mtx_;  // io queue mutex
  std::mutex bf_mtx_;  // buffer queue mutex
  std::mutex bt_mtx_;  // batch queue mutex

  std::condition_variable bf_cv_;
  std::condition_variable bt_cv_;

  // used for io limit, simulate Semaphore
  std::mutex io_wait_mtx_;
  std::condition_variable io_wait_cv_;

  // used for transform limit, simulate Semaphore
  std::mutex tf_wait_mtx_;
  std::condition_variable tf_wait_cv_;

  // IoTaskLoop and TransformTaskLoop will check stop_flag_
  // whether it should stop or not
  std::atomic_bool stop_flag_{false};

  std::atomic_int total_batches_;  //
  std::atomic_int total_buffers_;  //
  std::atomic_int bf_size_;
  std::atomic_int bt_size_;

  BigInt io_workers_count_;
  BigInt tf_workers_count_;

  BigInt bf_limit_;
  BigInt bt_limit_;
  //
  BigInt data_begin_;
  BigInt data_end_;
  std::vector<BigInt> datum_ids_;
  std::vector<BigInt> global_bytes_offsets_;
};

}  // namespace hotbox

