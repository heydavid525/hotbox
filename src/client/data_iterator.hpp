#pragma once
#include <vector>
#include "client/data_iterator_if.hpp"
#include "db/proto/db.pb.h"
#include "schema/all.hpp"
#include "client/mt_transformer.hpp"

namespace hotbox {

// Iterate over the data returned by a range query on Session.
// DataIterator can only be created by Session.
class DataIterator : public DataIteratorIf {
public:
  inline bool HasNext() const override {
    return next_ < data_end_;
  }

  inline void Restart() override {
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

  // This advances the iterator as it returns a r-reference.
  FlexiDatum GetDatum() override;

  ~DataIterator() {
    if (use_multi_threads_ && mtt_engine_) {
      delete mtt_engine_;
    }
  }

  DataIterator(DataIterator&& other);

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
    std::vector<std::function<void(std::vector<TransDatum*>*)>> transforms,
    size_t data_begin, size_t data_end, bool use_multi_threads,
    int32_t num_io_threads, int32_t num_transform_threads,
    size_t buffer_limit, size_t batch_limit);

  // Read an atom file and perform transform.
  void ReadAtomAndTransform(int atom_id);

private:
  const SessionProto& session_proto_;
  std::vector<std::function<void(std::vector<TransDatum*>*)>> transforms_;
  size_t data_begin_;
  size_t data_end_;

  size_t next_;

  size_t chunk_begin_;
  size_t chunk_end_;

  std::vector<FlexiDatum> data_buffer_;

  std::vector<int64_t> datum_ids_;

  bool use_multi_threads_;

  MTTransformer *mtt_engine_;

  int32_t num_io_threads_;
  int32_t num_transform_threads_;
  size_t buffer_limit_;
  size_t batch_limit_;
};

}  // namespace hotbox
