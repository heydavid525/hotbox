#pragma once

#include "schema/proto/schema.pb.h"
#include "schema/datum_base.hpp"
#include "transform/proto/transform_configs.pb.h"

namespace mldb {

// Each TransformEngineIf has an associated TransformIf (interface) runs on
// client and performs actual transform operations.
class TransformIf {
public:
  virtual ~TransformIf() { }

  // Transform a datum.
  virtual void Transform(DatumBase* datum) const = 0;
};

}  // namespace mldb
