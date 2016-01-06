#pragma once
#include <functional>
#include <vector>
#include "client/status.hpp"
#include "client/data_iterator.hpp"
#include "client/mt_transformer.hpp"
#include "db/proto/db.pb.h"
#include "schema/all.hpp"
#include "util/all.hpp"

namespace hotbox {

// A client-side session.
class Session {
public:
  Session(WarpClient& warp_client, Status status,
      const SessionProto& session_proto);

  // Close the connection in destructor.
  ~Session();

  // Get Output Schema (OSchema).
  OSchema GetOSchema() const;

  // Get number of data in this session (determined by # of data in DB and
  // SessionOption).
  BigInt GetNumData() const;

  // Create an iterator that sequentially returns data [data_begin, data_end).
  // Default to include all data.
  /*
  DataIterator NewDataIterator(BigInt data_begin = 0,
      BigInt data_end = -1) const;
  */

  DataIterator NewDataIterator(BigInt data_begin = 0,
        BigInt data_end = -1, bool use_multi_threads = true,
      BigInt num_io_threads = 1, BigInt num_transform_threads = 4,
      BigInt buffer_limit = 16, BigInt batch_limit = 16) const;

  Status GetStatus() const;

private:
  WarpClient& warp_client_;

  Status status_;
  SessionProto session_proto_;
  std::vector<std::function<void(TransDatum*)>> transforms_;
};

}  // namespace hotbox
