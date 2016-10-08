#include "client/data_iterator.hpp"
#include <glog/logging.h>
#include <algorithm>
#include <vector>
#include <string>
#include "util/all.hpp"

namespace hotbox {

DataIterator::DataIterator(const SessionProto& session_proto,
    std::vector<std::function<void(TransDatum*)>> transforms,
    size_t data_begin, size_t data_end, bool use_multi_threads,
    int32_t num_io_threads, int32_t num_transform_threads,
    size_t buffer_limit, size_t batch_limit)
  : session_proto_(session_proto), transforms_(transforms),
  data_begin_(data_begin), data_end_(data_end), next_(data_begin),
  datum_ids_(session_proto_.file_map().datum_ids().cbegin(),
      session_proto_.file_map().datum_ids().cend()),
      use_multi_threads_(use_multi_threads), mtt_engine_(nullptr),
      num_io_threads_(num_io_threads),
      num_transform_threads_(num_transform_threads),
      buffer_limit_(buffer_limit),
      batch_limit_(batch_limit) {
  Restart();
}

FlexiDatum DataIterator::GetDatum() {
  // Get Datum from Size Limited Files.
  CHECK_LT(next_, data_end_);
  if (next_ == chunk_end_) {
    if (use_multi_threads_) {
      auto *vec = mtt_engine_->NextBatch();
      data_buffer_ = std::move(*vec);
      delete vec;
    } else {
      // Read the next chunk.
      auto high = std::upper_bound(datum_ids_.cbegin(), datum_ids_.cend(),
                                  next_);
      auto atom_id = high - datum_ids_.cbegin() - 1;
      CHECK_GE(atom_id, 0) << "Couldn't find atom file containing datum "
                            << next_;
      CHECK_LT(atom_id, datum_ids_.size());
      ReadAtomAndTransform(atom_id);
    }
    chunk_begin_ = chunk_end_;
    chunk_end_ = chunk_begin_ + data_buffer_.size();
    //LOG(INFO) << "Loaded data range: " << "[" << chunk_begin_
    //          << ", " << chunk_end_ << ")";
  }
  // Use move to avoid copying.
  FlexiDatum datum = std::move(data_buffer_[next_++ - chunk_begin_]);
  return datum;
}

void DataIterator::ReadAtomAndTransform(int atom_id) {
  std::string content = io::ReadCompressedFile(
      session_proto_.file_map().atom_path() + std::to_string(atom_id),
      session_proto_.compressor());
  DBAtom atom_proto = StreamDeserialize<DBAtom>(content);
  data_buffer_.resize(atom_proto.datum_protos_size());
  auto output_store_type = session_proto_.output_store_type();
  auto output_dim = session_proto_.output_dim();

  // Collect transform ranges to std::vector
  std::vector<TransformOutputRange> ranges(transforms_.size());
  for (int i = 0; i < transforms_.size(); ++i) {
    ranges[i] = session_proto_.transform_output_ranges(i);
  }
  // Start from the last datum because protobuf only has
  // ReleaseLast().
  for (int i = atom_proto.datum_protos_size() - 1; i >= 0; --i) {
    DatumBase* datum_base = new DatumBase(
        atom_proto.mutable_datum_protos()->ReleaseLast());
    TransDatum trans_datum(datum_base, session_proto_.label(),
        session_proto_.weight(), output_store_type, output_dim, ranges);
    for (int t = 0; t < transforms_.size(); ++t) {
      trans_datum.ReadyTransform(
          session_proto_.transform_output_ranges(t));
      transforms_[t](&trans_datum);
    }
    data_buffer_[i] = std::move(trans_datum.GetFlexiDatum());
  }
}

DataIterator::DataIterator(DataIterator &&other)
  : session_proto_(other.session_proto_),
  transforms_(std::move(other.transforms_)),
  data_begin_(other.data_begin_), data_end_(other.data_end_),
  next_(other.next_), chunk_begin_(other.chunk_begin_),
  chunk_end_(other.chunk_end_),
  data_buffer_(std::move(other.data_buffer_)),
  datum_ids_(std::move(other.datum_ids_)),
  use_multi_threads_(other.use_multi_threads_),
  mtt_engine_(other.mtt_engine_),
  num_io_threads_(other.num_io_threads_),
  num_transform_threads_(other.num_transform_threads_),
  buffer_limit_(other.buffer_limit_),
  batch_limit_(other.batch_limit_) {
    other.mtt_engine_ = nullptr;
  }

std::unique_ptr<TransStats> DataIterator::GetMetrics() {
  return mtt_engine_->GetMetrics();
}

}  // namespace hotbox
