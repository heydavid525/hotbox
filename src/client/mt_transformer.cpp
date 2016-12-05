#include "client/mt_transformer.hpp"
#include "util/global_config.hpp"
#include <glog/logging.h>
#include <algorithm>
#include <string>
#include <vector>
#include "util/all.hpp"
#include "util/timer.hpp"

namespace hotbox {

MTTransformer::MTTransformer(const SessionProto &session_proto,
                             std::vector<std::function<
                               void(std::vector<TransDatum*>*)>> transforms,
                             size_t data_begin, size_t data_end,
                             int num_io_threads,
                             int num_transform_threads,
                             int transform_task_limit,
                             int batch_limit) : session_proto_(session_proto),
  transforms_(transforms),
  num_io_workers_(std::min(num_io_threads, transform_task_limit)),
  num_tf_workers_(std::min(num_transform_threads, batch_limit)),
  tf_limit_(transform_task_limit), bt_limit_(batch_limit),
  data_begin_(data_begin), data_end_(data_end),
  datum_ids_(session_proto_.file_map().datum_ids().cbegin(),
             session_proto_.file_map().datum_ids().cend()) {
  // Last atom file range ends with num_data.
  datum_ids_.push_back(session_proto_.file_map().num_data());

  num_cache_in_workers_ = num_io_workers_/2+1;
  num_cache_out_workers_ = num_io_workers_/2+1;
  Start();
}


MTTransformer::~MTTransformer() {
  Destory();
}

void MTTransformer::IoTaskLoop(int tid) {
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " Starts...";
  bool sampling = false;
  if (tid == 0) sampling = true;

  while (true) {
    TaskId taskid;
    // get atom_id from io_queue
    if (io_queue_->isEmpty() || stop_flag_) {
      break;
    }
    io_queue_->blockingRead(taskid);

    auto& task = tasks_[taskid];

    std::string path = session_proto_.file_map().atom_path()
                              + std::to_string(task.atom_id);
    //task.buffer = std::move(io::ReadCompressedFile(
    //                          path, session_proto_.compressor()));
    Timer timer;
    task.buffer = std::move(io::ReadCompressedFile(
                              path, Compressor::NO_COMPRESS));
    if (sampling)
      metrics_.add_input(timer.elapsed());

    if (trans_cached.size())
      cache_read_queue_->blockingWrite(taskid);
    else
      tf_queue_->blockingWrite(taskid);
  }
  // LOG(INFO) << "IoTaskLoop " << std::this_thread::get_id() << " ends...";
}

// read cache and pass to tf
void MTTransformer::CacheReadLoop(int tid) {
  bool sampling = false;
  if (tid == 0) sampling = true;

  while (true) {
    TaskId taskid;
    if (stop_flag_ || total_tf_tasks_ <= 0)
      break;
    cache_read_queue_->blockingRead(taskid);
    if (taskid == -1) break; // sentinel task for termination

    auto& task = tasks_[taskid];

    DLOG(INFO) << "Caching in for atom " << taskid << " start.";

    if (sampling)
      metrics_.add_rcache();
    for (auto& it_tid : trans_cached) {
      // skip if transform for this datum is not cached
      if (!cached_datums[it_tid][taskid]) {
        continue;
      }
      
      DLOG(INFO) << "Caching in for atom " << taskid << " transform " << it_tid << " begin";
      Timer timer;
      task.cache[it_tid] = std::move(io::ReadCompressedFile(
            getCachePath(task.atom_id, it_tid),
            session_proto_.compressor()));
      DLOG(INFO) << "Caching in for atom " << taskid << " transform " << it_tid << " end";
      if (sampling)
        metrics_.add_rcache(it_tid, timer.elapsed());
    }

    DLOG(INFO) << "Caching in for atom " << taskid << " end.";

    tf_queue_->blockingWrite(taskid);
  }
}

// tid: transformer id
void MTTransformer::TransformTaskLoop(int tid) {
  auto& global_config = GlobalConfig::GetInstance();
  int batch_size = global_config.Get<int>("transform_batch_size");

  bool sampling = false;
  if (tid == 0) sampling = true;

  while (true) {
    TaskId taskid;
    // get content buffer from bf_queue
    if (stop_flag_ || total_tf_tasks_ <= 0)
      break;
    tf_queue_->blockingRead(taskid);
    --total_tf_tasks_;

    if (taskid == -1) break; // sentinel task for termination
    DLOG(INFO) << "TransformTaskLoop atom " << taskid << " start.";
    auto& task = tasks_[taskid];

    // caching in and deserialize
    // the following transformation is done on per datum basis
    std::unordered_map<int, DBAtom> caches;
    for (auto &t : trans_cached) {
      caches[t] = StreamDeserialize<DBAtom>(task.cache[t]);
			task.cache.erase(t);
    }
		task.cache.clear();
    
    // deserialze input dataset
    // construct placeholder if input dataset is not needed
    // assume the number of datums should be the same from cache
    DBAtom atom_proto;
    if (skipIO) {
      for (int i = 0; i < caches[0].datum_protos().size(); ++i)
        atom_proto.mutable_datum_protos()->Add();
    } else {
      atom_proto = StreamDeserialize<DBAtom>(task.buffer);
    }
    task.buffer = "";
    task.buffer.shrink_to_fit(); // free unused memory
    auto output_store_type = session_proto_.output_store_type();
    auto output_dim = session_proto_.output_dim();
    std::vector<FlexiDatum> *vec = new std::vector<FlexiDatum>(
      task.datum_end - task.datum_begin);

    // Collect transform ranges to std::vector
    std::vector<TransformOutputRange> ranges(transforms_.size());
    for (int i = 0; i < transforms_.size(); ++i) {
      ranges[i] = session_proto_.transform_output_ranges(i);
    }
    // Trim data for atom files at the boundaries.
    int num_release = atom_proto.datum_protos_size() - task.datum_end;
    for (int j = 0; j < num_release; ++j) {
      delete atom_proto.mutable_datum_protos()->ReleaseLast();
    }

    // do transform by mini-batch
    int atom_size = atom_proto.datum_protos_size() - task.datum_begin;
    int num_batches = atom_size / batch_size;
    if (atom_size % batch_size != 0) {
      num_batches++;
    }
    // caching: prepare space for datum_bases
    task.datum_bases.resize(atom_size);

    if (sampling)
      metrics_.add_transform();

    for (int b = 0; b < num_batches; ++b) {
      int offset = b * batch_size;
      int num_items = (b == num_batches - 1) ? (atom_size - offset)
        : batch_size;
      std::vector<TransDatum*> data_batch(num_items);
      for (int j = 0; j < num_items; ++j) {
        auto datum_base = std::make_shared<DatumBase>(
          atom_proto.mutable_datum_protos()->ReleaseLast());
        data_batch[j] = new TransDatum(datum_base, session_proto_.label(),
                        session_proto_.weight(), output_store_type, output_dim,
                        ranges);
        task.datum_bases[offset+j] = datum_base; // global datum idx for this atom
      }

      BigInt output_counter_old = 0;
      for (int t = 0; t < transforms_.size(); ++t) {
        // skip transformation if constructed from cache
        if (trans_cached.find(t) != trans_cached.end() && cached_datums[t][taskid]) {
          Timer timer;
          for (int i = offset; i < offset + num_items; ++i) {
            DLOG(INFO) << "Rebuilding from cache for atom " << taskid << " datum " << i << " transform " << t;
            auto& cache = caches[t]; // cached atom for the transformation
            auto& range = session_proto_.transform_output_ranges(t);
            auto type = range.store_type();
            auto begin = range.store_offset_begin();
            auto end = range.store_offset_end();
            // note: sharing to get the datum_bases reference
            auto datum_base = task.datum_bases[i];
            // the target store has already reserved space for storage
            switch (type) {
              case FeatureStoreType::DENSE_CAT:
                {
                  auto len = end - begin;
                  auto src =
                    cache.mutable_datum_protos(i)->dense_cat_store().data();
                  auto dst =
                    datum_base->GetMutableDatumProto().mutable_dense_cat_store()->mutable_data();
                  memcpy(dst+begin, src, sizeof(long long) * len);
                  break;
                }
              case FeatureStoreType::DENSE_NUM:
                {
                  auto len = end - begin;
                  auto src =
                    cache.mutable_datum_protos(i)->dense_num_store().data();
                  auto dst =
                    datum_base->GetMutableDatumProto().mutable_dense_num_store()->mutable_data();
                  memcpy(dst+begin, src, sizeof(float) * len);
                  break;
                }
              case FeatureStoreType::SPARSE_CAT:
                {
                  auto len = cache.datum_protos(i).sparse_cat_store_idxs_size();
                  if (len <= 0) LOG(FATAL) << "Caching range doesn't have value";
                  auto src_idx =
                    cache.mutable_datum_protos(i)->mutable_sparse_cat_store_idxs()->mutable_data();
                  for (int i = 0; i < len; ++i) {
                    src_idx[i] += begin;
                  }
                  datum_base->GetMutableDatumProto().MergeFrom(cache.datum_protos(i));
                  break;
                }
              case FeatureStoreType::SPARSE_NUM:
                {
                  auto len = cache.datum_protos(i).sparse_num_store_idxs_size();
                  if (len <= 0) LOG(FATAL) << "Caching range doesn't have value";
                  auto src_idx =
                    cache.mutable_datum_protos(i)->mutable_sparse_num_store_idxs()->mutable_data();
                  for (int i = 0; i < len; ++i) {
                    src_idx[i] += begin;
                  }
                  datum_base->GetMutableDatumProto().MergeFrom(cache.datum_protos(i));
                  break;
                }
            }
          }
          if (sampling) metrics_.add_transform(t, timer.elapsed(), 0);
          continue;
        }
        for (int j = 0; j < num_items; ++j) {
          data_batch[j]->ReadyTransform(
            session_proto_.transform_output_ranges(t));
        }
        // collect time (cput time) + size for the transformation
        Timer timer;
        transforms_[t](&data_batch);

        BigInt output_counter_new = 0;
        for (int j = 0; j < num_items; ++j) {
          output_counter_new += data_batch[j]->GetOutputCounter();
        }

        if (sampling) metrics_.add_transform(t, timer.elapsed(),
            output_counter_new - output_counter_old);
        output_counter_old = output_counter_new;
      }
      for (int j = 0; j < num_items; ++j) {
        int i = task.datum_end - (j + offset) - 1;
        (*vec)[i - task.datum_begin] = std::move(data_batch[j]->GetFlexiDatum());
        delete data_batch[j];
      }
    }

    if (trans_tocache.size()) {
      cache_write_queue_->blockingWrite(taskid);
    } else {
      task.datum_bases.clear();
    }
    bt_queue_->blockingWrite(vec);
  }
  // LOG(INFO) << "TFTaskLoop " << std::this_thread::get_id() << " ends...";
}

// write cache out and pass output to bt_queue_
void MTTransformer::CacheWriteLoop(int tid) {

  bool sampling = false;
  if (tid == 0) sampling = true;

  while (true) {
    TaskId taskid;
    cache_write_queue_->blockingRead(taskid);
    if (taskid == -1) break; // sentinel task for termination

    auto& task = tasks_[taskid];
    DLOG(INFO) << "Caching out for atom " << taskid << " start.";

    if (sampling)
      metrics_.add_wcache();
    for (auto& it_tid : trans_tocache) {
      // skip if transform for this datum is cached
      if (cached_datums[it_tid][taskid]) {
        continue;
      }
      DLOG(INFO) << "Caching out transform " << it_tid;
      auto& range = session_proto_.transform_output_ranges(it_tid);
      auto type = range.store_type();
      auto begin = range.store_offset_begin();
      auto end = range.store_offset_end();

      auto atom = new DBAtom();
      for (auto& it_datum : task.datum_bases) {
        auto datum = new DatumProto();

        switch (type) {
          case FeatureStoreType::DENSE_CAT:
            {
              auto len = end - begin;
              datum->mutable_dense_cat_store()->Resize(len, 0);
              auto src =
                it_datum->GetDatumProto().dense_cat_store().data();
              auto dst = datum->mutable_dense_cat_store()->mutable_data();
              memcpy(dst, src+begin, sizeof(long long) * len);
              break;
            }
          case FeatureStoreType::DENSE_NUM:
            {
              auto len = end - begin;
              datum->mutable_dense_num_store()->Resize(len, 0.);
              auto src =
                it_datum->GetDatumProto().dense_num_store().data();
              auto dst = datum->mutable_dense_num_store()->mutable_data();
              memcpy(dst, src+begin, sizeof(float) * len);
              break;
            }
          case FeatureStoreType::SPARSE_CAT:
            {
              // locate range
              const auto& idxs =
                it_datum->GetDatumProto().sparse_cat_store_idxs();
              const auto low = std::lower_bound(idxs.cbegin(), idxs.cend(),
                  begin) - idxs.cbegin();
              const auto up = std::upper_bound(idxs.cbegin(), idxs.cend(),
                  end) - idxs.cbegin();
              auto len = up - low;
              if (len <= 0) LOG(FATAL) << "Caching range doesn't have value";
              // copy
              datum->mutable_sparse_cat_store_idxs()->Resize(len, 0);
              datum->mutable_sparse_cat_store_vals()->Resize(len, 0);
              auto src_idx =
                it_datum->GetDatumProto().sparse_cat_store_idxs().data();
              auto dst_idx = datum->mutable_sparse_cat_store_idxs()->mutable_data();
              memcpy(dst_idx, src_idx+low, sizeof(int) * len);
              auto src_val =
                it_datum->GetDatumProto().sparse_cat_store_vals().data();
              auto dst_val = datum->mutable_sparse_cat_store_vals()->mutable_data();
              memcpy(dst_val, src_val+low, sizeof(long long) * len);
              // recover the offset
              for (int i = 0; i < len; ++i) {
                dst_idx[i] -= begin;
              }
              break;
            }
          case FeatureStoreType::SPARSE_NUM:
            {
              // locate range
              const auto& idxs =
                it_datum->GetDatumProto().sparse_num_store_idxs();
              const auto low = std::lower_bound(idxs.cbegin(), idxs.cend(),
                  begin) - idxs.cbegin();
              const auto up = std::upper_bound(idxs.cbegin(), idxs.cend(),
                  end) - idxs.cbegin();
              auto len = up - low;
              if (len <= 0) LOG(FATAL) << "Caching range doesn't have value";
              // copy
              datum->mutable_sparse_num_store_idxs()->Resize(len, 0);
              datum->mutable_sparse_num_store_vals()->Resize(len, 0);
              auto src_idx =
                it_datum->GetDatumProto().sparse_num_store_idxs().data();
              auto dst_idx = datum->mutable_sparse_num_store_idxs()->mutable_data();
              memcpy(dst_idx, src_idx+low, sizeof(int) * len);
              auto src_val =
                it_datum->GetDatumProto().sparse_num_store_vals().data();
              auto dst_val = datum->mutable_sparse_num_store_vals()->mutable_data();
              memcpy(dst_val, src_val+low, sizeof(float) * len);
              // recover the offset
              for (int i = 0; i < len; ++i) {
                dst_idx[i] -= begin;
              }
              break;
            }
          default:
            LOG(FATAL) << "Caching out for store type " <<
              type << " is not supported yet.";
        }
        atom->mutable_datum_protos()->AddAllocated(datum);
      }

      Timer timer;
      size_t uncompressed_size = 0;
      std::string compressed_atom = StreamSerialize(*atom, &uncompressed_size);
      io::WriteCompressedFile(getCachePath(task.atom_id, it_tid),
          compressed_atom);
      DLOG(INFO) << "Wrote to atom " << task.atom_id << 
        " for caching transform " << it_tid << " size: " <<
        SizeToReadableString(compressed_atom.size());
      delete atom;
      if (sampling)
        metrics_.add_wcache(it_tid, timer.elapsed());
    }

    task.datum_bases.clear();

    DLOG(INFO) << "Caching out for atom " << taskid << " end.";
  }
}

void MTTransformer::Destory() {
  LOG(INFO) << "Tearing down MTTransformer!";
  // set stop flag
  stop_flag_ = true;

  // notify all workers to check stop flag and exit
  for (int i = 0; i < num_cache_in_workers_; i++) {
    cache_read_queue_->blockingWrite(-1);
  }
  for (int i = 0; i < num_tf_workers_; i++) {
    tf_queue_->blockingWrite(-1);
  }
  for (int i = 0; i < num_cache_out_workers_; i++) {
    cache_write_queue_->blockingWrite(-1);
  }

  for (auto &worker : io_workers_) {
    if (worker.joinable())
      worker.join();
  }

  for (auto &worker : cache_read_workers_) {
    if (worker.joinable())
      worker.join();
  }
  
  for (auto &worker : tf_workers_) {
    if (worker.joinable())
      worker.join();
  }

  for (auto &worker : cache_write_workers_) {
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
  auto low = std::lower_bound(datum_ids_.cbegin(), datum_ids_.cend(),
                              data_begin) - datum_ids_.cbegin();
  auto high = std::lower_bound(datum_ids_.cbegin(), datum_ids_.cend(),
                               data_end) - datum_ids_.cbegin();
  if (high == datum_ids_.size())
    high--;

  DLOG(INFO) << "Translate [Datum:" << data_begin << " -> " << data_end << "] [Atom: " << low << " -> " << high << "]";
  tasks_.reserve(high-low);

  if (skipIO) {
    cache_read_queue_ = make_unique<folly::MPMCQueue<TaskId> >(high-low);
  } else {
    io_queue_ = make_unique<folly::MPMCQueue<TaskId> >(high-low);
    cache_read_queue_ = make_unique<folly::MPMCQueue<TaskId> >(num_io_workers_);
  }
  tf_queue_ = make_unique<folly::MPMCQueue<TaskId> >(tf_limit_);  // buffer queue
  bt_queue_ = make_unique<folly::MPMCQueue<std::vector<FlexiDatum> *> >(bt_limit_);  // batch queue
  cache_write_queue_ = make_unique<folly::MPMCQueue<TaskId> >(tf_limit_);

  // add the cached datums info per transformations from session_proto
  for (int tid = 0; tid < transforms_.size(); ++tid) {
    cached_datums[tid].resize(high-low);
    auto transform_config = session_proto_.trans_params(i).GetConfig();
    for (auto& it = transform_config.cached_datums.cbegin(); it !=
        transform_config.cached_datums.cend(); ++ it) {
      cached_datums[tid][it] = 1;
    }
  }

  // add tasks to queue and start working
  for (int atom_id = low; atom_id < high; atom_id++) {
    tasks_[atom_id].atom_id = atom_id;
    tasks_[atom_id].datum_begin = std::max(data_begin, (size_t)datum_ids_[atom_id])
                      - datum_ids_[atom_id];
    tasks_[atom_id].datum_end = std::min(data_end, (size_t)datum_ids_[atom_id + 1])
                      - datum_ids_[atom_id];

    // skip reading input dataset when all transforms are cached
    if (skipIO)
      cache_read_queue_->blockingWrite(atom_id);
    else
      io_queue_->blockingWrite(atom_id);
  }

  total_tf_tasks_ = high - low;
  total_batches_ = high -low;
}


void MTTransformer::Start() {
  // metrics
  metrics_.init(transforms_.size());

  // cache instruction
  for (auto& t : session_proto_.transforms_tocache())
      trans_tocache.insert(t);
  for (auto& t : session_proto_.transforms_cached())
      trans_cached.insert(t);

  if (trans_cached.size() == transforms_.size()) {
    CHECK_EQ(trans_tocache.size(), 0) << "don't try to cache when all transformations are already cached";
    skipIO = true;
    DLOG(INFO) << "Start SkipIO = true";
  }

  // translate data range into io tasks
  Translate(data_begin_, data_end_);

  for (int i = 0; !skipIO && i < num_io_workers_; i++) {
    io_workers_.push_back(std::thread([this, i]() {
      this->IoTaskLoop(i);
    }));
  }

	// TODO only launch workers when needed
	// can cache in in the same place as IO
	// can launch separate thread to cache out to avoid queue overhead
  if (trans_tocache.empty()) {
    num_cache_out_workers_ = 0;
  }
  if (trans_cached.empty()) {
    num_cache_in_workers_ = 0;
  }
  for (int i = 0; i < num_cache_in_workers_; i++) {
    cache_read_workers_.push_back(std::thread([this, i]() {
      this->CacheReadLoop(i);
    }));
  }
  for (int i = 0; i < num_cache_out_workers_; i++) {
    cache_write_workers_.push_back(std::thread([this, i]() {
      this->CacheWriteLoop(i);
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

TransStats MTTransformer::GetMetrics() {
  return metrics_;
}

std::string MTTransformer::getCachePath(int atomid, int transformid) {
  return session_proto_.file_map().atom_path() + "/cache/" +
    std::to_string(atomid) + "." + session_proto_.trans_params(i).GetConfig().GetHash(); 
}

}  // namespace hotbox
