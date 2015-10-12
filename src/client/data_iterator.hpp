#pragma once
#include "db/proto/db.pb.h"
#include "schema/all.hpp"

namespace hotbox {

// Iterate over the data returned by a range query on Session. DataIterator
// can only be created by Session.
class DataIterator {
public:
  inline bool HasNext() const {
    return next_ < data_end_;
  }

  inline void Next() {
    next_++;
  }

  inline void Restart() {
    next_ = data_begin_;
    chunk_begin_ = data_begin_;
    chunk_end_ = data_begin_;
  }

  FlexiDatum&& GetDatum();

private:
  // Can only be created by Session, and the parent Session needs to outlive
  // DataIterator.
  friend class Session;

  DataIterator(const SessionProto& session_proto,
      std::vector<std::function<void(TransDatum*)>> transforms,
      BigInt data_begin, BigInt data_end);

  // Read an atom file and perform transform.
  void ReadAtomAndTransform(int atom_id);

private:
  const SessionProto& session_proto_;
  std::vector<std::function<void(TransDatum*)>> transforms_;
  BigInt data_begin_;
  BigInt data_end_;

  BigInt next_;

  BigInt chunk_begin_;
  BigInt chunk_end_;

  std::vector<FlexiDatum> data_buffer_;

  std::vector<BigInt> datum_ids_;
};

}  // namespace hotbox
