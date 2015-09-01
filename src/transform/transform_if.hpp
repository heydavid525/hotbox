#pragma once

#include "transform/proto/schema.pb.h"
#include "transform/proto/transforms.pb.h"
#include "transform/datum_base.hpp"

namespace mldb {

// Transform interfaace (if).
class TransformIf {
public:
  // TODO(wdai): Try to enforce that all subclass cannot be created with
  // default constructor, but with protobuf configuration

  virtual ~TransformIf() { }

  // Modify the schema and record the necessary information to transform
  // RawDatum conforming to the input schema. This must be called before
  // Transform().
  virtual void TransformSchema(Schema* schema) = 0;

  // Transform a datum.
  virtual void Transform(DatumBase* datum) const = 0;

protected:
  bool recv_schema_{false};
};

}  // namespace mldb
