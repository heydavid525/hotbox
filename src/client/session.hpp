#pragma once
#include <functional>
#include <vector>
#include "client/status.hpp"
#include "client/data_iterator.hpp"
#include "client/mt_transformer.hpp"
#include "client/data_iterator_if.hpp"
#include "db/proto/db.pb.h"
#include "schema/all.hpp"
#include "util/all.hpp"

namespace hotbox {

extern const int kDataEnd;

// A client-side session.
class Session {
public:
  // session_id is only used if use_proxy is true.
  Session(WarpClient& warp_client, Status status,
      const SessionProto& session_proto, bool use_proxy);

  // Close the connection in destructor.
  ~Session();

  // Get Output Schema (OSchema).
  OSchema GetOSchema() const;

  // Get number of data in this session (determined by # of data in DB and
  // SessionOption).
  int64_t GetNumData() const;

  // Create an iterator that sequentially returns data [data_begin, data_end).
  // Default to include all data.
  std::unique_ptr<DataIteratorIf> NewDataIterator(int64_t data_begin = 0,
        int64_t data_end = kDataEnd,
        int32_t num_transform_threads = 4,
      int32_t num_io_threads = 1, size_t buffer_limit = 16,
      size_t batch_limit = 16);

  Status GetStatus() const;

private:
  bool use_proxy_;
  int proxy_iter_id_{0};
  WarpClient& warp_client_;

  Status status_;
  SessionProto session_proto_;
  std::vector<std::function<void(std::vector<TransDatum*>*)>> transforms_;
};

}  // namespace hotbox
