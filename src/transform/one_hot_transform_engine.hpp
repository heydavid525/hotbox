#pragma once

#include <vector>
#include <functional>
#include "db/feature_stat.hpp"
#include "schema/proto/schema.pb.h"
#include "schema/schema.hpp"
#include "schema/datum_base.hpp"
#include "transform/transform_engine_if.hpp"
#include "transform/proto/transform_configs.pb.h"

namespace mldb {

class OneHotTransformEngine : public TransformEngineIf {
public:
  OneHotTransformEngine(const OneHotTransformConfig& config);

  //TransformOp TransformSchema(Schema* schema,
  //    const std::vector<Featurestats>& stats) override;
  // TODO(wdai): Take in stat.
  TransformOp TransformSchema(Schema* schema) override;

private:
  // TODO(wdai): take in stat
  void TransformOneFeature(const Feature& input_feature,
      Schema* schema, int* output_family_idx, OneHotTransformOp* op);

private:
  OneHotTransformConfig config_;
};

}  // namespace mldb
