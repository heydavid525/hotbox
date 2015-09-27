#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/one_hot_transform.hpp"

namespace mldb {

void OneHotTransform::TransformSchema(const TransformParams& params,
    TransformWriter* writer) const {
  const OneHotTransformConfig& config =
    params.GetConfig().one_hot_transform();
  const auto& input_features = params.GetInputFeatures();
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    CHECK(IsNumeral(input_feature));
    if (config.buckets_size() == 0) {
      // Use input_feature (need to be categorical) to decide buckets.
      CHECK(IsCategorical(input_feature))
        << "Must specify bucket for NUMERIC feature type.";
      // [min, max].
      //int max = static_cast<int>(params.GetStat().GetMax());
      //int min = static_cast<int>(params.GetStat().GetMin());
      // TODO(wdai): use stat
      int min = 0;
      int max = 10;
      int num_buckets = max - min + 1;
      for (int i = 0; i < num_buckets; ++i) {
        auto feature_name = input_feature.name() + "-one-hot-bucket-"
          + std::to_string(min + i);
        writer->AddSparseCatFeature(config.output_feature_family(), feature_name,
            config.not_in_final());
      }
    } else {
      const auto& buckets = config.buckets();
      int num_buckets = config.buckets_size() + 1;
      for (int i = 0; i < num_buckets; ++i) {
        std::string range = (i == 0) ? "<" + std::to_string(buckets.Get(0)) :
          (i == num_buckets - 1) ? ">" + std::to_string(buckets.Get(i)) :
          "[" + std::to_string(buckets.Get(i-1)) + ", " +
          std::to_string(buckets.Get(i)) + ")";
        auto feature_name = input_feature.name() + "-one-hot-bucket-" + range;
        writer->AddSparseCatFeature(config.output_feature_family(), feature_name,
            config.not_in_final());
      }
    }
  }
}

std::function<void(DatumBase*)> OneHotTransform::GenerateTransform(
    const TransformParams& params) const {
  const OneHotTransformConfig& config =
    params.GetConfig().one_hot_transform();
  std::vector<std::function<void(DatumBase*)>> transforms;
  const auto& input_features = params.GetInputFeatures();
  int offset = 0;
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    if (config.buckets_size() == 0) {
      // Categorical feature into natural binning.
      int min = 0;
      int max = 10;
      // bin into num_buckets in sparse_cat_store starting with
      // 'output_offset_start'.
      transforms.push_back(
        [input_feature, min, max, offset]
        (DatumBase* datum) {
          int val = static_cast<int>(datum->GetFeatureVal(input_feature));
          CHECK_LE(val, max);
          CHECK_LE(min, val);
          int bin_id = val - min;
          datum->SetSparseCatFeatureVal(offset + bin_id, 1);
        });
      // Advance offset to point at bins associated with the next feature.
      offset += max - min + 1;
    } else {
      const auto& buckets = config.buckets();
      transforms.push_back(
        [input_feature, buckets, offset]
        (DatumBase* datum) {
          float val = datum->GetFeatureVal(input_feature);
          if (val < buckets.Get(0)) {
            // First bucket
            datum->SetSparseCatFeatureVal(offset, 1);
          } else {
            for (int i = 0; i < buckets.size(); ++i) {
              if (val >= buckets.Get(i)) {
                datum->SetSparseCatFeatureVal(offset + i + 1, 1);
                break;
              }
            }
          }
        });
      offset += config.buckets_size() + 1;
    }
  }
  return [transforms] (DatumBase* datum) {
    for (const auto& transform : transforms) {
      transform(datum);
    }};
}

}  // namespace mldb
