#pragma once
#include <functional>
#include "client/status.hpp"
#include "client/data_iterator.hpp"
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
  DataIterator NewDataIterator(BigInt data_begin = 0,
      BigInt data_end = -1) const;

  Status GetStatus() const;

private:
  WarpClient& warp_client_;

  Status status_;
  SessionProto session_proto_;
  std::vector<std::function<void(TransDatum*)>> transforms_;
};

}  // namespace hotbox
