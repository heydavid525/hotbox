#include "client/mt_transformer.hpp"
#include <glog/logging.h>
#include <algorithm>
#include <string>
#include <vector>
#include "util/all.hpp"

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
  Start();
}


MTTransformer::~MTTransformer() {
  Destory();
}

void MTTransformer::IoTaskLoop() {
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " Starts...";
  while (true) {
    Task task;
    // get atom_id from io_queue
    {
      std::lock_guard<std::mutex> lock(io_mtx_);
      if (io_queue_.empty() || stop_flag_) {
        break;
      }
      task = std::move(io_queue_.front());
      io_queue_.pop();
    }
    std::string path = session_proto_.file_map().atom_path()
                              + std::to_string(task.atom_id);
    task.buffer = std::move(io::ReadCompressedFile(
                              path, session_proto_.compressor()));

    {
      std::lock_guard<std::mutex> lock(tf_mtx_);
      tf_queue_.push(std::move(task));
      tf_size_++;
    }
    tf_cv_.notify_all();
    {
      // speed limit
      std::unique_lock<std::mutex> lock(io_wait_mtx_);
      io_wait_cv_.wait(lock, [this]() {
        return stop_flag_ || (tf_size_ < tf_limit_);
      });
      if (stop_flag_)
        break;
    }
  }
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " ends...";
}

void MTTransformer::TransformTaskLoop() {
  // LOG(INFO) << "TFTaskLoop " << std::this_thread::get_id() << " Starts...";
  while (true) {
    Task task;
    // get content buffer from bf_queue
    {
      std::unique_lock<std::mutex> lock(tf_mtx_);
      if (stop_flag_ || total_tf_tasks_ <= 0)
        break;
      tf_cv_.wait(lock, [this]() {
        return stop_flag_ || tf_queue_.size();
      });
      if (stop_flag_)
        break;
      task = std::move(tf_queue_.front());
      tf_queue_.pop();
      tf_size_--;
      total_tf_tasks_--;
      lock.unlock();
    }
    if (tf_size_ < tf_limit_)
      io_wait_cv_.notify_one();
    DBAtom atom_proto = StreamDeserialize<DBAtom>(task.buffer);
    auto output_store_type = session_proto_.output_store_type();
    auto output_dim = session_proto_.output_dim();
    std::vector<FlexiDatum> *vec = new std::vector<FlexiDatum>(
      task.datum_end - task.datum_begin);

    // do transform
    for (int i = atom_proto.datum_protos_size() - 1; i >= task.datum_begin;
         --i) {
      if (i >= task.datum_end) {
        delete atom_proto.mutable_datum_protos()->ReleaseLast();
        continue;
      }
      DatumBase* datum_base = new DatumBase(
        atom_proto.mutable_datum_protos()->ReleaseLast());
      TransDatum trans_datum(datum_base, session_proto_.label(),
                      session_proto_.weight(), output_store_type, output_dim);

      for (int t = 0; t < transforms_.size(); ++t) {
        trans_datum.ReadyTransform(session_proto_.transform_output_ranges(t));
        transforms_[t](&trans_datum);
      }
      (*vec)[i - task.datum_begin] = std::move(trans_datum.GetFlexiDatum());
    }

    // push vec to bt_queue
    {
      std::lock_guard<std::mutex> lock(bt_mtx_);
      bt_queue_.push(vec);
      bt_size_++;
    }
    bt_cv_.notify_one();

    // speed limit
    {
      std::unique_lock<std::mutex> lock(tf_wait_mtx_);
      tf_wait_cv_.wait(lock, [this]() {
        return stop_flag_ || (bt_size_ < bt_limit_);
      });
      lock.unlock();
      if (stop_flag_)
        break;
    }
  }
  // LOG(INFO) << "TFTaskLoop " << std::this_thread::get_id() << " ends...";
}

void MTTransformer::Destory() {
  // set stop flag
  stop_flag_ = true;

  // notify all workers to check stop flag and exit
  tf_cv_.notify_all();
  io_wait_cv_.notify_all();
  bt_cv_.notify_all();
  tf_wait_cv_.notify_all();
  for (auto &worker : io_workers_) {
    if (worker.joinable())
      worker.join();
  }

  for (auto &worker : tf_workers_) {
    if (worker.joinable())
      worker.join();
  }

  // delete any held pointers
  {
    std::lock_guard<std::mutex> lock{bt_mtx_};
    while (bt_queue_.size()) {
      auto batch = bt_queue_.front();
      bt_queue_.pop();
      delete batch;
    }
  }
  LOG(INFO) << "MTTransform finished. Bye!";
}

std::vector<FlexiDatum> *MTTransformer::NextBatch() {
  if (total_batches_ <= 0) {
    return nullptr;
  }
  std::vector<FlexiDatum> *vec = nullptr;
  std::unique_lock<std::mutex> lock(bt_mtx_);
  bt_cv_.wait(lock, [this]() {
    return bt_size_ > 0;
  });
  vec = bt_queue_.front();
  bt_queue_.pop();
  ///
  // for (int i = 0; i < vec->size(); ++i) {
  //   LOG(INFO) << (*vec)[i].ToString();
  // }
  total_batches_--;
  bt_size_--;
  lock.unlock();
  tf_wait_cv_.notify_one();
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
    io_queue_.push(std::move(task));
  }

  total_tf_tasks_ = io_queue_.size();
  total_batches_ = io_queue_.size();
  LOG(INFO) << "Total Batches :" << total_batches_;
}


void MTTransformer::Start() {
  // translate data range into io tasks
  Translate(data_begin_, data_end_);

  bt_size_ = 0;
  tf_size_ = 0;
  for (int i = 0; i < num_io_workers_; i++) {
    io_workers_.push_back(std::thread([this]() {
      this->IoTaskLoop();
    }));
  }

  for (int i = 0; i < num_tf_workers_; i++) {
    tf_workers_.push_back(std::thread([this]() {
      this->TransformTaskLoop();
    }));
  }
}
bool MTTransformer::HasNextBatch() const {
  return total_batches_;
}
}  // namespace hotbox
