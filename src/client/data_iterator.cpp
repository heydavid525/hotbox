#include "client/data_iterator.hpp"
#include "util/all.hpp"
#include <glog/logging.h>
#include <algorithm>

namespace hotbox {

DataIterator::DataIterator(const SessionProto& session_proto,
    std::vector<std::function<void(TransDatum*)>> transforms,
    BigInt data_begin, BigInt data_end)
  : session_proto_(session_proto), transforms_(transforms),
  data_begin_(data_begin), data_end_(data_end), next_(data_begin),
  datum_ids_(session_proto_.file_map().datum_ids().cbegin(),
      session_proto_.file_map().datum_ids().cend()) {
    Restart();
  }

FlexiDatum&& DataIterator::GetDatum() {
  CHECK_LT(next_, data_end_);
  if (next_ == chunk_end_) {
    // Read the next chunk.
    auto high = std::upper_bound(datum_ids_.cbegin(), datum_ids_.cend(), next_);
    auto atom_id = high - datum_ids_.cbegin() - 1;
    CHECK_GE(atom_id, 0) << "Couldn't find atom file containing datum " << next_;
    CHECK_LT(atom_id, datum_ids_.size());
    ReadAtomAndTransform(atom_id);
    chunk_begin_ = chunk_end_;
    chunk_end_ = chunk_begin_ + data_buffer_.size();
  }
  return std::move(data_buffer_[next_ - chunk_begin_]);
}

void DataIterator::ReadAtomAndTransform(int atom_id) {
  LOG(INFO) << "Reading atom file " << atom_id;
  std::string content = io::ReadCompressedFile(
      session_proto_.file_map().atom_path() + std::to_string(atom_id),
      session_proto_.compressor());
  DBAtom atom_proto;
  atom_proto.ParseFromString(content);
  data_buffer_.resize(atom_proto.datum_protos_size());
  FeatureFamily internal_family(session_proto_.internal_family_proto());
  auto output_store_type = session_proto_.output_store_type();
  auto output_dim = session_proto_.output_dim();

  // Start from the last datum because protobuf only has ReleaseLast().
  for (int i = atom_proto.datum_protos_size() - 1; i >= 0; --i) {
    DatumBase* datum_base = new DatumBase(
        atom_proto.mutable_datum_protos()->ReleaseLast());
    TransDatum trans_datum(datum_base, internal_family, output_store_type,
        output_dim);
    for (int t = 0; t < transforms_.size(); ++t) {
      trans_datum.ReadyTransform(session_proto_.transform_output_ranges(t));
      transforms_[t](&trans_datum);
    }
    data_buffer_[i] = std::move(trans_datum.GetFlexiDatum());
  }
}

}  // namespace hotbox
