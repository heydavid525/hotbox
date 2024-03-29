#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/bucketize_transform.hpp"
#include <sstream>
#include "util/util.hpp"

namespace hotbox {

void BucketizeTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const BucketizeTransformConfig& config =
    param.GetConfig().bucketize_transform();
  const std::vector<Feature>& input_features = param.GetInputFeatures();
  const std::vector<std::string>& input_features_desc =
    param.GetInputFeaturesDesc();
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    CHECK(IsNumber(input_feature));
    const auto& buckets = config.buckets();
    int num_buckets = config.buckets_size() - 1;
    for (int j = 0; j < num_buckets; ++j) {
      std::string range_str = "[" + ToString(buckets.Get(j)) + "," +
        ToString(buckets.Get(j+1)) + ")";
      auto feature_name = input_features_desc[i] + range_str;
      writer->AddFeature(feature_name);
    }
  }
}

std::function<void(TransDatum*)> BucketizeTransform::GenerateTransform(
    const TransformParam& param) const {
  const BucketizeTransformConfig& config =
    param.GetConfig().bucketize_transform();
  std::vector<std::function<void(TransDatum*)>> transforms;
  const auto& input_features = param.GetInputFeatures();
  BigInt offset = 0;
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    const auto& buckets = config.buckets();
    transforms.push_back(
      [input_feature, buckets, offset]
      (TransDatum* datum) {
        float val = datum->GetFeatureVal(input_feature);
        for (int j = 0; j < buckets.size() - 1; ++j) {
          if (val >= buckets.Get(j) && val < buckets.Get(j+1)) {
            datum->SetFeatureValRelativeOffset(offset + j, 1);
            break;
          }
        }
      });
    // The buckets include both ends of the bucket boundaries.
    offset += config.buckets_size() - 1;
  }
  return [transforms] (TransDatum* datum) {
    for (const auto& transform : transforms) {
      transform(datum);
    }};
}

}  // namespace hotbox
