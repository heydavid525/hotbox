#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/select_transform.hpp"
#include <sstream>
#include "util/util.hpp"

namespace hotbox {

void SelectTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const SelectTransformConfig& config =
    param.GetConfig().select_transform();
  const std::vector<Feature>& input_features = param.GetInputFeatures();
  const std::vector<std::string>& input_features_desc =
    param.GetInputFeaturesDesc();
  for (int i = 0; i < input_features.size(); ++i) {
    const auto& input_feature = input_features[i];
    CHECK(IsNumber(input_feature));
    writer->AddFeature(input_features_desc[i]);
  }
  LOG(INFO) << "Done transformSchema";
}

std::function<void(TransDatum*)> SelectTransform::GenerateTransform(
    const TransformParam& param) const {
  const auto& input_features = param.GetInputFeatures();
  return [input_features] (TransDatum* datum) {
    for (int i = 0; i < input_features.size(); ++i) {
      const auto& input_feature = input_features[i];
      float val = datum->GetFeatureVal(input_feature);
      if (val != 0) {
        datum->SetFeatureValRelativeOffset(i, val);
      }
    }
  };
}

}  // namespace hotbox
