#include "client/mt_transformer.hpp"
#include <glog/logging.h>
#include <algorithm>
#include <string>
#include <vector>
#include "util/all.hpp"
#include <chrono>
#include <ctime>

namespace hotbox {

MTTransformer::MTTransformer(const SessionProto &session_proto,
                             std::vector<std::function<void(TransDatum *)>>
                             transforms,
                             size_t data_begin, size_t data_end,
                             int num_io_threads,
                             int num_transform_threads,
                             int transform_task_limit,
                             int batch_limit) : session_proto_(session_proto),
  transforms_(transforms),
  num_io_workers_(num_io_threads), num_tf_workers_(num_transform_threads),
  tf_limit_(transform_task_limit), bt_limit_(batch_limit),
  data_begin_(data_begin), data_end_(data_end),
  datum_ids_(session_proto_.file_map().datum_ids().cbegin(),
             session_proto_.file_map().datum_ids().cend()) {
  // Last atom file range ends with num_data.
  datum_ids_.push_back(session_proto_.file_map().num_data());

  io_queue_ = new folly::MPMCQueue<Task,std::atomic,true>(8);
  tf_queue_ = new folly::MPMCQueue<Task>(tf_limit_);  // buffer queue
  bt_queue_ = new folly::MPMCQueue<std::vector<FlexiDatum> *>(bt_limit_);  // batch queue
  Start();
}


MTTransformer::~MTTransformer() {
  Destory();
  delete io_queue_;
  delete tf_queue_;
  delete bt_queue_;
}

void MTTransformer::IoTaskLoop() {
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " Starts...";
  while (true) {
    Task task;
    // get atom_id from io_queue
    if (io_queue_->isEmpty() || stop_flag_) {
      break;
    }
    io_queue_->blockingRead(task);

    std::string path = session_proto_.file_map().atom_path()
                              + std::to_string(task.atom_id);
    task.buffer = std::move(io::ReadCompressedFile(
                              path, session_proto_.compressor()));

    tf_queue_->blockingWrite(std::move(task));
  }
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " ends...";
}

// tid: transformer id
void MTTransformer::TransformTaskLoop(int tid) {
  // LOG(INFO) << "TFTaskLoop " << std::this_thread::get_id() << " Starts...";
  while (true) {
    Task task;
    // get content buffer from bf_queue
    if (stop_flag_ || total_tf_tasks_ <= 0)
      break;
    tf_queue_->blockingRead(task);
    --total_tf_tasks_;
    if (task.atom_id == -1) break; // sentinel task for termination

    DBAtom atom_proto = StreamDeserialize<DBAtom>(task.buffer);
    auto output_store_type = session_proto_.output_store_type();
    auto output_dim = session_proto_.output_dim();
    std::vector<FlexiDatum> *vec = new std::vector<FlexiDatum>(
      task.datum_end - task.datum_begin);

    // Collect transform ranges to std::vector
    std::vector<TransformOutputRange> ranges(transforms_.size());
    for (int i = 0; i < transforms_.size(); ++i) {
      ranges[i] = session_proto_.transform_output_ranges(i);
    }
    // do transform
    for (int i = atom_proto.datum_protos_size() - 1; i >= task.datum_begin;
         --i) {
      if (i >= task.datum_end) {
        delete atom_proto.mutable_datum_protos()->ReleaseLast();
        continue;
      }
      DatumBase* datum_base = new DatumBase(
        atom_proto.mutable_datum_protos()->ReleaseLast());
      auto& last_range = session_proto_.transform_output_ranges(
          transforms_.size() - 1);
      TransDatum trans_datum(datum_base, session_proto_.label(),
                      session_proto_.weight(), output_store_type, output_dim,
                      ranges);

      BigInt output_counter_old = 0;
      for (int t = 0; t < transforms_.size(); ++t) {
        // collect time (cput time) + size for the transformation
        std::clock_t c_start = std::clock();
        trans_datum.ReadyTransform(session_proto_.transform_output_ranges(t));
        transforms_[t](&trans_datum);
        std::clock_t c_end = std::clock();

        BigInt output_counter_new = trans_datum.GetOutputCounter();
        DLOG(INFO) << "TFTaskLoop " << tid << " finish transform #" << t << " \
          in " << c_end - c_start << " ticks, generating " <<
          output_counter_new - output_counter_old << " values.";

        metrics_[tid][t].set_time(metrics_[tid][t].time() + (c_end-c_start) /
            CLOCKS_PER_SEC);
        metrics_[tid][t].set_space(metrics_[tid][t].space() +
            output_counter_new - output_counter_old);
        output_counter_old = output_counter_new;
      }
      (*vec)[i - task.datum_begin] = std::move(trans_datum.GetFlexiDatum());
    }

    // push vec to bt_queue
    bt_queue_->blockingWrite(vec);
  }
  // LOG(INFO) << "TFTaskLoop " << std::this_thread::get_id() << " ends...";
}

void MTTransformer::Destory() {
  // set stop flag
  stop_flag_ = true;

  // notify all workers to check stop flag and exit
  for (int i = 0; i < num_tf_workers_; i++) {
    Task task;
    task.atom_id = -1;
    tf_queue_->blockingWrite(std::move(task));
  }
  
  for (auto &worker : io_workers_) {
    if (worker.joinable())
      worker.join();
  }

  for (auto &worker : tf_workers_) {
    if (worker.joinable())
      worker.join();
  }

  // delete any held pointers
  while (!bt_queue_->isEmpty()) {
    std::vector<FlexiDatum> * batch;
    bt_queue_->blockingRead(batch);
    delete batch;
  }
  LOG(INFO) << "MTTransform finished. Bye!";
}

std::vector<FlexiDatum> *MTTransformer::NextBatch() {
  if (total_batches_ <= 0) {
    return nullptr;
  }
  std::vector<FlexiDatum> *vec = nullptr;
  bt_queue_->blockingRead(vec);
  ///
  // for (int i = 0; i < vec->size(); ++i) {
  //   LOG(INFO) << (*vec)[i].ToString();
  // }
  total_batches_--;
  return vec;
}

// split data range into subrange group by atom file
void
MTTransformer::Translate(size_t data_begin, size_t data_end) {
  CHECK_LT(data_begin, data_end);
  auto low = std::upper_bound(datum_ids_.cbegin(), datum_ids_.cend(),
                              data_begin) - datum_ids_.cbegin() - 1;
  auto high = std::upper_bound(datum_ids_.cbegin(), datum_ids_.cend(),
                               data_end) - datum_ids_.cbegin();
  if (high == datum_ids_.size())
    high--;
  for (int atom_id = low; atom_id < high; atom_id++) {
    Task task;
    task.atom_id = atom_id;
    task.datum_begin = std::max(data_begin, (size_t)datum_ids_[atom_id])
                      - datum_ids_[atom_id];
    task.datum_end = std::min(data_end, (size_t)datum_ids_[atom_id + 1])
                      - datum_ids_[atom_id];
    io_queue_->blockingWrite(std::move(task));
  }

  total_tf_tasks_ = io_queue_->size();
  total_batches_ = io_queue_->size();
}


void MTTransformer::Start() {
  // translate data range into io tasks
  Translate(data_begin_, data_end_);
  // metrics
  metrics_.resize(num_tf_workers_, TransStats(transforms_.size()));

  for (int i = 0; i < num_io_workers_; i++) {
    io_workers_.push_back(std::thread([this]() {
      this->IoTaskLoop();
    }));
  }

  for (int i = 0; i < num_tf_workers_; i++) {
    tf_workers_.push_back(std::thread([this, i]() {
      this->TransformTaskLoop(i);
    }));
  }
}

bool MTTransformer::HasNextBatch() const {
  return total_batches_;
}

std::unique_ptr<TransStats> MTTransformer::GetMetrics() {
  auto ret = std::unique_ptr<TransStats>(new TransStats(transforms_.size()));
  for (auto it = metrics_.begin(); it != metrics_.end(); ++it) {
    for (int i = 0; i < transforms_.size(); i++) {
      auto transform_stat = (*ret)[i];
      transform_stat.set_time(transform_stat.time() + (*it)[i].time());
      transform_stat.set_space(transform_stat.space() + (*it)[i].space());
    }
  }
  return ret;
}

}  // namespace hotbox
