#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/one_hot_transform.hpp"

namespace mldb {

void OneHotTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const OneHotTransformConfig& config =
    param.GetConfig().one_hot_transform();
  LOG(INFO) << "one hot config: \n" << config.DebugString();
  const auto& input_features = param.GetInputFeatures();
  LOG(INFO) << "input_features.size(): " << input_features.size();
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    // Use input_feature (need to be categorical) to decide buckets.
    CHECK(IsCategorical(input_feature)) << "Feature " << input_feature.name()
      << " isn't categorical or factor.";
    // [min, max].
    //int max = static_cast<int>(param.GetStat().GetMax());
    //int min = static_cast<int>(param.GetStat().GetMin());
    // TODO(wdai): use stat
    int min = 0;
    int max = 10;
    int num_buckets = max - min + 1;
    for (int i = 0; i < num_buckets; ++i) {
      auto feature_name = input_feature.name() + "_" + std::to_string(min + i);
      writer->AddFeature(feature_name);
    }
  }
}

std::function<void(TransDatum*)> OneHotTransform::GenerateTransform(
    const TransformParam& param) const {
  const OneHotTransformConfig& config =
    param.GetConfig().one_hot_transform();
  std::vector<std::function<void(TransDatum*)>> transforms;
  const auto& input_features = param.GetInputFeatures();
  BigInt offset = 0;
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    // Categorical feature into natural binning.
    int min = 0;
    int max = 10;
    // bin into num_buckets in sparse_cat_store starting with
    // 'output_offset_begin'.
    transforms.push_back(
      [input_feature, min, max, offset]
      (TransDatum* datum) {
        int val = static_cast<int>(datum->GetFeatureVal(input_feature));
        CHECK_LE(val, max);
        CHECK_LE(min, val);
        int bin_id = val - min;
        datum->SetFeatureValRelativeOffset(offset + bin_id, 1);
      });
    // Advance offset to point at bins associated with the next feature.
    offset += max - min + 1;
  }
  return [transforms] (TransDatum* datum) {
    for (const auto& transform : transforms) {
      transform(datum);
    }};
}

}  // namespace mldb
