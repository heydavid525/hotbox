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
MTTransformer works as described below:

                 num_io_threads                                 num_tf_threads
 io task queue --> 2.IoTaskLoop --> transform task queue --> 3.TransformTaskLoop
       |                                                           |
 1.Translate data range                  4.NextBatch     <--    batch queue
*/

/** Some explaination:
 *  tf. tf is short for transform
 *  bt. bt is short for batch
 *  Batch. In a atom file, there are many individual blocks that can be 
 *    decompressed and transformed individually. I call the transformed data
 *    from a block "batch". If some datums in a block if out of the requested
 *    data range, such datums will be abandoned before transforming.
 *  
 */

class MTTransformer {
 public:
  MTTransformer(const SessionProto &session_proto,
                std::vector<std::function<void(TransDatum *)>> transforms,
                BigInt data_begin, BigInt data_end,
                int num_io_threads,
                int num_transform_threads,
                int transform_task_limit,
                int batch_limit);

  ~MTTransformer();

  // check if there is any transformed or untransformed data batch
  bool HasNextBatch() const;
  // NextBatch will take a batch from bt_queue_.
  // It will be blocked only if no batch is available
  // It will transfer the returned pointer's ownership to caller.
  // It The calller is responsible to release it
  // It will return nullptr if HasNextBatch returns false
  std::vector<FlexiDatum> *NextBatch();

  void Start();

 private:
  // A IoTask contains a group of continuous atom blocks that may cross two atom
  // files if the last block cross two atom files.
  // IoTask is to optimize I/O because sequentially reading a file is faster
  // than randomly reading each part of a file.
  // Data range [data_begin_, data_end_) will be translated into IoTasks.
  struct IoTask {
    std::size_t file_begin;
    std::size_t file_end;
    // first block index of global_bytes_offsets within this IoTask
    std::size_t global_bytes_offsets_begin;
    // last block index of global_bytes_offsets within this IoTask
    std::size_t global_bytes_offsets_end;
  };
  // A IoTask may generate many TfTasks by IoTaskLoop()
  struct TfTask {
    BigInt idx;  // index of datum_ids
    std::shared_ptr<std::string> shared_buf;  // shared buffer
    std::size_t offset;  // offset within shared_buf
    std::size_t length;  // buffer length
  };

  // It will translate data range into io tasks and push them to io_queue_.
  void Translate(BigInt data_begin, BigInt data_end);

  // notify all workers to stop and delete unused batches
  void Destory();

  // IoTaskLoop will take io task from io_queue_ and load file into buffer, then
  // generate many transform tasks sharing the buffer and push them to
  // tf_queue_. It will do such thing until io_queue_ is empty.
  // each io_worker will run this function.
  void IoTaskLoop();

  // TransformTaskLoop will take transform task from tf_queue_, then decompress,
  // deserialize it into a batch. Then it will do transforms on the batch and
  // push the batch to bt_queue_.
  // each transform worker will run this function.
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
  std::mutex tf_mtx_;  // transform queue mutex
  std::mutex bt_mtx_;  // batch queue mutex

  // if tf_queue_ is empty, TransformTaskLoop will wait on this
  // if IoTaskLoop pushes tasks to tf_queue_, it will call notify_all to wake up
  // all TransformTaskLoop.
  std::condition_variable tf_cv_;  // transform cv

  // if tf_queue_ is empty, NextBatch will wait on this
  // when TransformTaskLoop push a batch to bt_queue_, it will call noity once
  // so that NextBatch will wake up and return a batch taken from bt_queue.
  std::condition_variable bt_cv_;  // batch cv

  // used for io speed limit, simulate Semaphore
  // IoTaskLoop will use it for io speep limit
  // if tf_size_ >= tf_limit_, then it will wait until be notified by Destory()
  // or TransformTaskLoop()
  // TransformTaskLoop() will call notify_one if it finds bf_size_ < bf_limit
  std::mutex io_wait_mtx_;
  std::condition_variable io_wait_cv_;

  // used for transform speed limit, simulate Semaphore
  // TransformTaskLoop will use it for transform speed limit
  // if bt_size_ >= bf_limit_, then it will wait until be notified by Destory()
  // or NextBatch()
  // NextBatch() will call notify_one if it finds bt_size_ < bt_limit
  std::mutex tf_wait_mtx_;
  std::condition_variable tf_wait_cv_;

  // IoTaskLoop and TransformTaskLoop will check stop_flag_
  // whether it should stop or not
  std::atomic_bool stop_flag_{false};

  // total_batches is in the view of HasNextBatch(). It's used to see if there
  // is any batch in the batch queue or to be generated by TransformTaskLoop()
  //
  // if I don't use it. when I wants to check if there is any batch, I have
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
  // set by Translate(), equals to # of total blocks within data range
  // decrease 1 by NextBatch()
  std::atomic_int total_batches_;  // protected by bt_mtx_

  // total_tf_tasks_ is similar with total_batches_. it's used by
  // TransformTaskLoop to see if there is any TfTasks in tf_queue or to be
  // generated.
  //
  // set by Translate(), equals to # of total blocks within data range
  // decrease 1 when TransformTaskLoop pick up a transform task from tf_queue_
  std::atomic_int total_tf_tasks_;  // protected by tf_mtx_

  // current size of transform task queue, always equals tf_queue_.size()
  // increase when pushing tasks to tf_queue_
  // decrease when poping tasks from tf_queue_
  std::atomic_int tf_size_;

  // current size of transform task queue, always equals tf_queue_.size()
  // increase when pushing tasks to bt_queue_
  // decrease when poping tasks from bt_queue_
  std::atomic_int bt_size_;

  const int num_io_workers_;
  const int num_tf_workers_;

  const int tf_limit_;  // transform task queue size limit, used for io speed control
  const int bt_limit_;  // batch queue size limit, used for transform speed control

  const BigInt data_begin_;
  const BigInt data_end_;
  std::vector<BigInt> datum_ids_;
  std::vector<std::size_t> global_bytes_offsets_;
};

}  // namespace hotbox

