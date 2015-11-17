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
  // Get Datum from Size Limited Files.
  CHECK_LT(next_, data_end_);
  if (next_ == chunk_end_) {
    // Read the next chunk.
    auto high = std::upper_bound(datum_ids_.cbegin(), datum_ids_.cend(), next_);
    auto data_idx = high - datum_ids_.cbegin() - 1;
    int32_t file_begin, file_end;
    if(next_ == 0) {
      file_begin = 0;
      file_end = session_proto_.file_map().global_bytes_offset(data_idx);
    } else {
      file_begin = session_proto_.file_map().global_bytes_offset(data_idx - 1);
      file_end = session_proto_.file_map().global_bytes_offset(data_idx);
    }
    LOG(INFO) << "data_idx: " << data_idx << ". "
              << "file_begin: " << file_begin << ". "
              << "file_end: " << file_end << ". "
              << "Length: " << file_end - file_begin << ". "
              << "next_: " << next_ << ". ";
    CHECK_GE(data_idx, 0) << "Couldn't find atom file containing datum " << next_;
    CHECK_LT(data_idx, datum_ids_.size());
    CHECK_LT(file_begin, file_end);
    ReadSizeLimitedAtomAndTransform(file_begin, file_end);
    chunk_begin_ = chunk_end_;
    chunk_end_ = chunk_begin_ + data_buffer_.size();
    LOG(INFO) << "Chunk Info: " << "[" << chunk_begin_ << " - " << chunk_end_ << ")";
    LOG(INFO) << "-------------------------------------";
  }
  return std::move(data_buffer_[next_ - chunk_begin_]);
}

void DataIterator::ReadSizeLimitedAtomAndTransform(BigInt file_begin,
    BigInt file_end) {
  int32_t size_limit = kATOM_SIZE_MB;
  int32_t atom_idx_begin = file_begin / size_limit;
  int32_t atom_idx_end = file_end / size_limit;
  LOG(INFO) << "Which Atom: [" << atom_idx_begin << " - " << atom_idx_end << "]." ;
  
  std::stringstream ss;
  for(int i=0; i < (atom_idx_end + 1 - atom_idx_begin) ; i++) {
    LOG(INFO) << "Reading atom file " << atom_idx_begin + i;
    if(i == 0) {
      int32_t read_len = (atom_idx_end == atom_idx_begin)
                ? file_end - file_begin // total read len
                : (size_limit - file_begin % size_limit); // file_len
      ss << io::ReadCompressedFile(
        session_proto_.file_map().atom_path() + std::to_string(atom_idx_begin + i),
          Compressor::NO_COMPRESS,
            file_begin % size_limit, read_len);
    }
    else if (i < (atom_idx_end - atom_idx_begin)) {
      ss << io::ReadCompressedFile(
        session_proto_.file_map().atom_path() + std::to_string(atom_idx_begin + i),
          Compressor::NO_COMPRESS);
    }
    else {
      ss << io::ReadCompressedFile(
        session_proto_.file_map().atom_path() + std::to_string(atom_idx_begin + i),
          Compressor::NO_COMPRESS, 0, file_end % size_limit);
    }
/*
    std::string content = io::ReadCompressedFile(
        session_proto_.file_map().atom_path() + std::to_string(atom_idx_begin + i),
        Compressor::NO_COMPRESS);
    if(i == 0) {
      ss << content.substr(file_begin % size_limit, file_end - file_begin);
    }
    else if (i < (atom_idx_end - atom_idx_begin)) {
      ss << content;
    }
    else {
      ss << content.substr(0, file_end % size_limit);
    }
*/
  }
  std::string data = ReadCompressedString(ss.str(), session_proto_.compressor());
  LOG(INFO) << "File Read: " << ss.str().size();

  DBAtom atom_proto;
  atom_proto.ParseFromString(data);
  data_buffer_.resize(atom_proto.datum_protos_size());
  FeatureFamily internal_family(session_proto_.internal_family_proto());
  auto output_store_type = session_proto_.output_store_type();
  auto output_dim = session_proto_.output_dim();

  LOG(INFO) << "atom_proto.datum_protos_size(): " << atom_proto.datum_protos_size();
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
