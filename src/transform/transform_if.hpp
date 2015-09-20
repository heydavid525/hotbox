#pragma once

#include "schema/datum_base.hpp"
#include "schema/schema.hpp"
#include "schema/schema_util.hpp"
#include "transform/proto/transform_config.pb.h"
#include "transform/transform_params.hpp"
#include "transform/transform_writer.hpp"

namespace mldb {

// TransformIf is an interface (If) for two static functions (thus it has no
// constructor).
class TransformIf {
public:
  TransformIf() = delete;

  virtual ~TransformIf() { }

  // Change the schema (e.g., add a feature family, make some feature
  // in_final) based on TransformParams, which includes transform-specific
  // configuration, input features, input feature stat. This is run on the
  // server.
  virtual void TransformSchema(const TransformParams& params,
      TransformWriter* writer) const = 0;

  // Generate transform to be applied to each DatumBase. The generated
  // transform is performed on each client.
  virtual std::function<void(DatumBase*)> GenerateTransform(
      const TransformParams& params) const = 0;
};

}  // namespace mldb
