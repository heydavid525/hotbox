#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/one_hot_transform.hpp"
#include "schema/all.hpp"
#include <algorithm>

namespace hotbox {

void OneHotTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const OneHotTransformConfig& config =
    param.GetConfig().one_hot_transform();
  const auto& input_features = param.GetInputFeatures();
  const auto& input_features_desc = param.GetInputFeaturesDesc();
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    std::string f_name = input_feature.name().empty() ? input_feature.name() :
      input_features_desc[i];
    const FeatureStatProto& stat = param.GetStat(input_feature);
    int num_buckets = 0;
    CHECK(IsNumerical(input_feature) || IsCategorical(input_feature));
    bool is_num = IsNumerical(input_feature);
    int num_unique = is_num ? stat.unique_num_values_size() :
      stat.unique_cat_values_size();
    if (num_unique == kNumUniqueMax) {
      LOG(ERROR) << "More unique values than maximum " << kNumUniqueMax;
    }
    num_buckets = num_unique;
    for (int j = 0; j < num_buckets; ++j) {
      float val = is_num ? stat.unique_num_values(j) :
        stat.unique_cat_values(j);
      auto feature_name = f_name + "_" + std::to_string(val);
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
    const FeatureStatProto& stat = param.GetStat(input_feature);
    int num_unique = IsNumerical(input_feature) ?
      stat.unique_num_values_size() : stat.unique_cat_values_size();
    std::vector<float> unique_vals(num_unique);
    for (int j = 0; j < num_unique; ++j) {
      unique_vals[j] = IsNumerical(input_feature) ? stat.unique_num_values(j)
        : stat.unique_cat_values(j);
    }
    // bin into num_buckets in sparse_cat_store starting with
    // 'output_offset_begin'.
    transforms.push_back(
      [input_feature, stat, offset, unique_vals]
      (TransDatum* datum) {
        float val = datum->GetFeatureVal(input_feature);
        auto it = std::find(unique_vals.cbegin(), unique_vals.cend(), val);
        CHECK(it != unique_vals.cend()) << "feature "
        << input_feature.DebugString() << " value " << val
        << " not in unique value";
        int bin_id = it - unique_vals.cbegin();
        datum->SetFeatureValRelativeOffset(offset + bin_id, 1);
      });
    // Advance offset to point at bins associated with the next feature.
    offset += num_unique;
  }
  return [transforms] (TransDatum* datum) {
    for (const auto& transform : transforms) {
      transform(datum);
    }};
}

}  // namespace hotbox
