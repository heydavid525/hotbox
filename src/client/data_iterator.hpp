#pragma once
#include <vector>
#include "db/proto/db.pb.h"
#include "schema/all.hpp"
#include "client/mt_transformer.hpp"

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
    if (use_multi_threads_) {
      if (mtt_engine_) {
        delete mtt_engine_;
      }
      mtt_engine_ = new MTTransformer(session_proto_, transforms_,
      data_begin_, data_end_, num_io_threads_, num_transform_threads_,
      buffer_limit_, batch_limit_);
    }
  }

  FlexiDatum&& GetDatum();

  ~DataIterator() {
    if (use_multi_threads_ && mtt_engine_) {
      delete mtt_engine_;
    }
  }

  DataIterator(DataIterator &&other);

private:
  // Can only be created by Session, and the parent Session needs to outlive
  // DataIterator.
  friend class Session;
  /*
  DataIterator(const SessionProto& session_proto,
      std::vector<std::function<void(TransDatum*)>> transforms,
      BigInt data_begin, BigInt data_end);
  */

  /**
   * If use_multi_threads is true, parameters after it will be used to create
   * MTTransformer to load and transform data in a multi-threaded way.
   * Otherwise, parameters after it will be ignored and DataIterator will load
   * and transform data in a single thread.
   */
  DataIterator(const SessionProto& session_proto,
    std::vector<std::function<void(TransDatum*)>> transforms,
    BigInt data_begin, BigInt data_end, bool use_multi_threads,
    BigInt num_io_threads, BigInt num_transform_threads,
    BigInt buffer_limit, BigInt batch_limit);

  // Read an atom file and perform transform.
  void ReadAtomAndTransform(int atom_id);
  void ReadSizeLimitedAtomAndTransform(BigInt file_begin, BigInt file_end);

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

  bool use_multi_threads_;

  MTTransformer *mtt_engine_;

  BigInt num_io_threads_;
  BigInt num_transform_threads_;
  BigInt buffer_limit_;
  BigInt batch_limit_;
};

}  // namespace hotbox
