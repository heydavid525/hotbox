#pragma once

#include "schema/proto/schema.pb.h"
#include "schema/datum_base.hpp"
#include "transform/proto/transform_configs.pb.h"

namespace mldb {

// TransformEngine interfaace (if). TransformEngine runs on the server and
// generates the transform operations, while TransformIf runs on the client
// and perform the actual transform logic.
class TransformEngineIf {
public:
  // TODO(wdai): Try to enforce that all subclass cannot be created with
  // default constructor, but with protobuf configuration

  virtual ~TransformEngineIf() { }

  // Modify the schema and record the necessary information to transform
  // RawDatum conforming to the input schema. This must be called before
  // Transform().
  virtual TransformOp TransformSchema(Schema* schema) = 0;

  /*
protected:
  bool recv_schema_{false};
  */
};

}  // namespace mldb
