#include <glog/logging.h>
#include "schema/schema_util.hpp"
#include "db/feature_stat.hpp"
#include "transform/one_hot_transform_engine.hpp"
#include "transform/proto/transform_configs.pb.h"

namespace mldb {

OneHotTransformEngine::OneHotTransformEngine(
    const OneHotTransformConfig& config) : config_(config) { }

TransformOp OneHotTransformEngine::TransformSchema(Schema* schema) {
  auto finders = ParseFeatureDesc(config_.input_features());
  FeatureFamily output_family(config_.output_feature_family());
  int output_family_idx = 0;
  TransformOp op;
  OneHotTransformOp* one_hot_op = op.mutable_one_hot_op();
  for (const auto& finder : finders) {
    if (finder.all_family) {
      // Family-wide selection.
      const auto& input_family = schema->GetFamily(finder.family_name);
      const auto& input_features = input_family.GetFeatures();
      for (int i = 0; i < input_features.size(); ++i) {
        // TODO(wdai): How to index stats?
        TransformOneFeature(input_features[i], schema, &output_family_idx,
            one_hot_op);
      }
    } else {
      const Feature& input_feature = schema->GetFeature(finder);
      TransformOneFeature(input_feature, schema, &output_family_idx,
          one_hot_op);
    }
  }
  return op;
}

void OneHotTransformEngine::TransformOneFeature(
    const Feature& input_feature, Schema* schema, int* output_family_idx,
    OneHotTransformOp* one_hot_op) {
  CHECK(IsNumeral(input_feature));
  int num_buckets = 0;
  const auto& input_feature_loc = input_feature.loc();

  // Construct transform op.
  OneHotTransformOpUnit* unit = one_hot_op->add_units();
  *(unit->mutable_input_loc()) = input_feature.loc();
  if (config_.buckets_size() == 0) {
    // Use input_feature (need to be categorical) to decide buckets.
    CHECK(IsCategorical(input_feature))
      << "Must specify bucket for NUMERIC feature type.";
    // [min, max].
    //int max = static_cast<int>(stat.GetMax());
    //int min = static_cast<int>(stat.GetMin());
    // TODO(wdai): use stat
    int min = 0;
    int max = 10;
    num_buckets = max - min + 1;
    unit->set_min(min);
    unit->set_max(max);
  } else {
    num_buckets = config_.buckets_size() + 1;
    unit->mutable_buckets()->CopyFrom(config_.buckets());
  }
  int output_offset_start = 0;
  for (int i = 0; i < num_buckets; ++i) {
    Feature new_feature;
    FeatureLocator* loc = new_feature.mutable_loc();
    loc->set_type(FeatureType::CATEGORICAL);
    loc->set_store_type(FeatureStoreType::SPARSE);
    new_feature.set_name(input_feature.name() + "-one-hot-bucket-"
        + std::to_string(i));
    new_feature.set_in_final(config_.in_final());
    schema->AddFeature(config_.output_feature_family(), (*output_family_idx)++,
        &new_feature);
    // now new_feature has the offset.
    if (i == 0) {
      output_offset_start = new_feature.loc().offset();
    }
  }
  unit->set_output_offset_start(output_offset_start);
}

}  // namespace mldb
