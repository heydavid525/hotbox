#pragma once
#include "db/proto/db.pb.h"
#include "schema/all.hpp"
#include "metrics/metrics.hpp"

namespace hotbox {

// Iterate over the data returned by a range query on Session.
// DataIterator can only be created by Session.
class DataIteratorIf {
public:
  virtual bool HasNext() const = 0;

  virtual void Restart() = 0;

  virtual FlexiDatum GetDatum() = 0;

  virtual std::unique_ptr<TransStats> GetMetrics() = 0;

  virtual ~DataIteratorIf() {}

protected:
  // Disable public constructor.
  DataIteratorIf() { }
};

} // namespace hotbox
