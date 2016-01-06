#pragma once

#include "schema/datum_base.hpp"
#include "schema/schema.hpp"
#include "schema/schema_util.hpp"
#include "schema/trans_datum.hpp"
#include "transform/proto/transform.pb.h"
#include "transform/transform_param.hpp"
#include "transform/transform_writer.hpp"

namespace hotbox {

// TransformIf is an interface (If) to implement transforms on schema and
// data.
class TransformIf {
public:
  virtual ~TransformIf() { }

  // Change the schema (e.g., add a feature family) based on TransformParams,
  // which includes transform-specific configuration, input features, input
  // feature stat. This is run on the server and does not see any real data
  // yet.
  virtual void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const = 0;

  // Generate transform to be applied to each datum (TransDatum). The
  // generated transform is performed on each query datum on each client.
  virtual std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const = 0;

  // By default return false
  void UpdateTransformWriterConfig(const TransformConfig& config,
      TransformWriterConfig* writer_config) const {
    // Configure TransWriter.
    writer_config->set_store_type(config.base_config().output_store_type());
    SetTransformWriterConfig(config, writer_config);
  }

protected:
  // Configure TransformWriter based on config and specific transform impl.
  virtual void SetTransformWriterConfig(const TransformConfig& config,
      TransformWriterConfig* writer_config) const {
    LOG(INFO) << "TransformIf::SetTransformWriterConfig";
    // Default does nothing.
  }
};

}  // namespace hotbox
