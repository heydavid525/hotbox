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
  std::queue<TfTask> tf_queue_;  // buffer queue
  std::queue<std::vector<FlexiDatum> *> bt_queue_;  // batch queue

  // mutex
  std::mutex io_mtx_;  // io queue mutex
  std::mutex tf_mtx_;  // buffer queue mutex
  std::mutex bt_mtx_;  // batch queue mutex

  std::condition_variable tf_cv_;
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

  // total_batches is in the view of HasNextBatch(). It's used to see if there
  // is any batch in the batch queue or to be generated by TransformTaskLoop()
  //
  // if we don't use it. when we wants to check if there is any batch, we have
  // to check the batch queue,  tf task queue, and io queue. if all the three
  // queues are empty, it doesn't mean there is no batch. for example, there
  // is only one transform task in buffer_queue and no batches in batch queue.
  // Then it's taken by TransformTaskLoop. Now both queues are empty while there
  // is one batch to be generated by TransformTaskLoop. see below, one way is to
  // add more variable to count how many tasks hold by TransformTaskLoop, but
  // it's too complex.
  // io_queue     --> empty
  //      |
  // io_task_loop --> holds nothing
  //      |
  // tf_queue     --> empty
  //      |
  // TransformTaskLoop --> hold one transform task
  //      |
  // bt_queue     --> empty
  //
  std::atomic_int total_batches_;  // protected by bt_mtx_

  // total_tf_tasks_ is similar with total_batches_. it's used by
  // TransformTaskLoop to see if there is any TfTasks in tf_queue or to be
  // generated. When TransformTaskLoop pick up a transform task from tf_queue_,
  // it will decrease total_tf_tasks by 1
  std::atomic_int total_tf_tasks_;  // protected by tf_mtx_

  // current size of transform task queue
  std::atomic_int tf_size_;

  // current size of batche queue
  std::atomic_int bt_size_;

  BigInt num_io_workers_;
  BigInt num_tf_workers_;

  BigInt tf_limit_;  // transform task queue size limit
  BigInt bt_limit_;  // batch queue size limit

  BigInt data_begin_;
  BigInt data_end_;
  std::vector<BigInt> datum_ids_;
  std::vector<BigInt> global_bytes_offsets_;
};

}  // namespace hotbox

