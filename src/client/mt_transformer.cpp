#include "client/mt_transformer.hpp"
#include "util/all.hpp"
#include <glog/logging.h>
#include <algorithm>

namespace hotbox {
MTTransformer::MTTransformer(const SessionProto &session_proto,
                             const std::vector<std::string> &files,
                             std::vector<std::function<void(TransDatum * )>> transforms,
                             int io_threads,
                             int transform_threads,
                             int buffer_limit,
                             int batch_limit) : session_proto_(session_proto),
  transforms_(transforms),
  io_workers_count_(io_threads), tf_workers_count_(transform_threads),
  bf_limit_(buffer_limit),
  bt_limit_(batch_limit) {
  io_workers_.reserve(io_workers_count_);
  tf_workers_.reserve(tf_workers_count_);
  for(auto &file : files)
    io_queue_.push(file);
}

void MTTransformer::IoTaskLoop() {
  while (true) {
    std::string path;

    // get file from io_queue
    {
      std::lock_guard<std::mutex> lock(io_mtx_);
      if (io_queue_.empty() || stop_flag_) {
        break;
      }
      path = std::move(io_queue_.front());
      io_queue_.pop();
    }

    std::string content = io::ReadCompressedFile(
                            path, session_proto_.compressor());

    {
      std::lock_guard<std::mutex> lock(bf_mtx_);
      bf_queue_.push(std::move(content));
    }
    bf_size_++;
    bf_cv_.notify_one();

    {
      // speed limit
      std::unique_lock<std::mutex> lock(io_wait_mtx_);
      io_wait_cv_.wait(lock, [this]() {
        return stop_flag_ || (bf_size_ < bf_limit_);
      });
      if(stop_flag_)
        break;
    }
  }
}

void MTTransformer::TransformTaskLoop() {
  while(true) {
    std::string content;

    // get content buffer from bf_queue
    {
      std::unique_lock<std::mutex> lock(bf_mtx_);
      if(stop_flag_ || total_buffers_ <= 0)
        break;
      bf_cv_.wait(lock, [this]() {
        return stop_flag_ || bf_queue_.size();
      });
      if(stop_flag_)
        break;
      content = std::move(bf_queue_.front());
      bf_queue_.pop();
      lock.unlock();
    }
    bf_size_--;
    total_buffers_--;
    io_wait_cv_.notify_one();

    // deserialize buffer
    DBAtom atom_proto;
    atom_proto.ParseFromString(content);

    std::vector<FlexiDatum> *vec = new std::vector<FlexiDatum>;
    vec->reserve(atom_proto.datum_protos_size());
    FeatureFamily internal_family(session_proto_.internal_family_proto());
    auto output_store_type = session_proto_.output_store_type();
    auto output_dim = session_proto_.output_dim();

    std::vector<FlexiDatum> &batch = *vec;
    // do transform
    for (int i = atom_proto.datum_protos_size() - 1; i >= 0; --i) {
      DatumBase* datum_base = new DatumBase(
        atom_proto.mutable_datum_protos()->ReleaseLast());
      TransDatum trans_datum(datum_base, internal_family, output_store_type,
                             output_dim);
      for (int t = 0; t < transforms_.size(); ++t) {
        trans_datum.ReadyTransform(session_proto_.transform_output_ranges(t));
        transforms_[t](&trans_datum);
      }
      batch[i] = std::move(trans_datum.GetFlexiDatum());
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
      if(stop_flag_)
        break;
    }
  }
}

void MTTransformer::Destory() {
  // set stop flag
  stop_flag_ = true;

  // notify all workers to check stop flag and exit
  bf_cv_.notify_all();
  io_wait_cv_.notify_all();
  bt_cv_.notify_all();
  tf_wait_cv_.notify_all();
  for(auto &worker : io_workers_) {
    if(worker.joinable())
      worker.join();
  }

  for(auto &worker : tf_workers_) {
    if(worker.joinable())
      worker.join();
  }

  // delete any held pointers

  {
    std::lock_guard<std::mutex> lock{bt_mtx_};
    while(bt_queue_.size()) {
      auto batch = bt_queue_.front();
      bt_queue_.pop();
      delete batch;
    }
  }
}

std::vector<FlexiDatum> *MTTransformer::NextBatch() {
  if (total_batches_ <= 0) {
    return nullptr;
  }
  std::vector<FlexiDatum> *vec = nullptr;
  std::unique_lock<std::mutex> lock(bt_mtx_);
  bt_cv_.wait(lock, [this]() {
    return bt_queue_.size();
  });
  vec = bt_queue_.front();
  bt_queue_.pop();
  total_batches_--;
  bt_size_--;
  lock.unlock();
  tf_wait_cv_.notify_one();
  return vec;
}

void MTTransformer::Start() {
  total_batches_ = this->io_queue_.size();
  total_buffers_ = this->io_queue_.size();
  bt_size_ = 0;
  bf_size_ = 0;
  for (int i = 0; i < io_workers_count_; i++) {
    io_workers_.push_back(std::thread([this]() {
      this->IoTaskLoop();
    }));
  }

  for (int i = 0; i < tf_workers_count_; i++) {
    tf_workers_.push_back(std::thread([this]() {
      this->TransformTaskLoop();
    }));
  }
}

}
