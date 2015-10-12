#pragma once

#include "schema/datum_base.hpp"
#include "schema/schema.hpp"
#include "schema/schema_util.hpp"
#include "schema/trans_datum.hpp"
#include "transform/proto/transform.pb.h"
#include "transform/transform_param.hpp"
#include "transform/transform_writer.hpp"

namespace mldb {

// TransformIf is an interface (If) for two static functions (thus it has no
// constructor).
class TransformIf {
public:
  //TransformIf() = delete;

  virtual ~TransformIf() { }

  // Change the schema (e.g., add a feature family, make some feature
  // in_final) based on TransformParams, which includes transform-specific
  // configuration, input features, input feature stat. This is run on the
  // server.
  virtual void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const = 0;

  // Generate transform to be applied to each DatumBase. The generated
  // transform is performed on each client.
  virtual std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const = 0;
};

}  // namespace mldb
