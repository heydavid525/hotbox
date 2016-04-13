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
             session_proto_.file_map().datum_ids().cend()),
             global_bytes_offsets_(
              session_proto_.file_map().global_bytes_offsets().size() + 1) {
  // datum_ids does not contain num_data, just put it in for convenience
  // global_bytes_offsets does not start from 0, just put 0 in the beginning
  datum_ids_.push_back(session_proto_.file_map().num_data());
  global_bytes_offsets_[0] = (0);
  std::copy(session_proto_.file_map().global_bytes_offsets().cbegin(),
            session_proto_.file_map().global_bytes_offsets().cend(),
            global_bytes_offsets_.begin() + 1);
  Start();
}


MTTransformer::~MTTransformer() {
  Destory();
}

void MTTransformer::IoTaskLoop() {
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " Starts...";
  while (true) {
    IoTask iotask;
    // get file from io_queue
    {
      std::lock_guard<std::mutex> lock(io_mtx_);
      if (io_queue_.empty() || stop_flag_) {
        break;
      }
      iotask = std::move(io_queue_.front());
      io_queue_.pop();
    }
    // read buffer
    auto file_size = kAtomSizeInBytes;
    auto atom_id_begin = iotask.file_begin / file_size;
    auto atom_id_end = iotask.file_end / file_size;
    int offset, length;
    std::string *content = new std::string;  // to make it shared
    // we will read at most two files
    for (auto atom_id = atom_id_begin; atom_id <= atom_id_end; atom_id++) {
      std::string path = session_proto_.file_map().atom_path()
                          + std::to_string(atom_id);
      if (atom_id == atom_id_begin) {
        offset = iotask.file_begin % file_size;
      } else {
        offset = 0;
      }
      if (atom_id == atom_id_end) {
        length = iotask.file_end % file_size - offset;
      } else {
        length = file_size - offset;
      }
      // Comment (Yangyang): too much copy
      if (atom_id  == atom_id_begin) {
        *content = std::move(io::ReadCompressedFile(path,
              Compressor::NO_COMPRESS,
              offset, length));
      } else {
        content->append(io::ReadCompressedFile(path,
              Compressor::NO_COMPRESS,
              offset, length));
      }
    }
    // generate TfTasks sharing shared_buf
    std::shared_ptr<std::string> shared_buf(content);
    {
      std::lock_guard<std::mutex> lock(tf_mtx_);
      for (auto idx = iotask.global_bytes_offsets_begin;
          idx < iotask.global_bytes_offsets_end;
          idx++) {
        TfTask task;
        task.shared_buf = shared_buf;
        task.idx = idx;
        task.offset = global_bytes_offsets_[idx] - iotask.file_begin;
        task.length = global_bytes_offsets_[idx + 1]
          - global_bytes_offsets_[idx];
        tf_queue_.push(task);
        tf_size_++;
      }
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
    TfTask task;
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
    /*
    // decompress buffer
    std::string content = DecompressString(
        task.shared_buf->c_str() + task.offset,
        task.length,
        session_proto_.compressor());
    */
    // deserialize buffer
    DBAtom atom_proto = StreamDeserialize<DBAtom>(
        task.shared_buf.get()->c_str() + task.offset, task.length);
    // atom_proto.ParseFromString(content);
    auto output_store_type = session_proto_.output_store_type();
    auto output_dim = session_proto_.output_dim();

    // do transform
    size_t datum_begin, datum_end, datum_base;
    datum_begin = datum_ids_[task.idx];
    datum_end = datum_ids_[task.idx + 1];
    if (datum_begin < data_begin_)
      datum_begin = data_begin_;
    if (datum_end > data_end_)
      datum_end = data_end_;
    datum_base = datum_begin;
    datum_begin -= datum_base;
    datum_end -= datum_base;
    std::vector<FlexiDatum> *vec = new std::vector<FlexiDatum>(
        datum_end - datum_begin);
    // vec->reserve(atom_proto.datum_protos_size());
    for (int i = atom_proto.datum_protos_size() - 1; i >= datum_begin; --i) {
      // check if datum i is in required
      if (i >= datum_end) {
        atom_proto.mutable_datum_protos()->ReleaseLast();
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
      // vec->push_back(std::move(trans_datum.GetFlexiDatum()));
      (*vec)[i - datum_begin] = std::move(trans_datum.GetFlexiDatum());
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
  // LOG(INFO) << "Destory...";
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
  //for (int i = 0; i < vec->size(); ++i) {
  //  LOG(INFO) << (*vec)[i].ToString();
  //}
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
      data_begin);
  auto high = std::upper_bound(datum_ids_.cbegin(), datum_ids_.cend(),
      data_end);
  auto global_bytes_offsets_begin = low - datum_ids_.cbegin() - 1;
  size_t global_bytes_offsets_end;
  if (high == datum_ids_.cend())
    global_bytes_offsets_end = high - datum_ids_.cbegin() - 1;
  else
    global_bytes_offsets_end = high - datum_ids_.cbegin();
  auto file_begin = global_bytes_offsets_[global_bytes_offsets_begin];
  auto file_end = global_bytes_offsets_[global_bytes_offsets_end];
  auto file_size = kAtomSizeInBytes;
  auto global_bytes_offsets_index = global_bytes_offsets_begin;
  for (auto offset = file_begin; offset < file_end;) {
    IoTask task;
    task.file_begin = offset;
    task.global_bytes_offsets_begin = global_bytes_offsets_index;
    offset += file_size - offset % file_size;
    auto upper = std::upper_bound(global_bytes_offsets_.cbegin(),
        global_bytes_offsets_.cend(), offset);
    if (upper == global_bytes_offsets_.cend())
      upper--;
    global_bytes_offsets_index = upper - global_bytes_offsets_.cbegin();
    task.file_end = *(upper);
    task.global_bytes_offsets_end = global_bytes_offsets_index;
    if (task.file_end > file_end) {
      task.file_end = file_end;
      task.global_bytes_offsets_end = global_bytes_offsets_end;
    }
    offset = task.file_end;
    io_queue_.push(task);
  }

  total_tf_tasks_ = global_bytes_offsets_end - global_bytes_offsets_begin;
  total_batches_ = global_bytes_offsets_end - global_bytes_offsets_begin;

  // LOG(INFO) << "Total Buffers :" << total_tf_tasks_;
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
