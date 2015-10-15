#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/constant_transform.hpp"
#include <sstream>
#include "util/util.hpp"

namespace hotbox {

void ConstantTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const ConstantTransformConfig& config = 
    param.GetConfig().constant_transform();
  const auto& input_features = param.GetInputFeatures();
  auto feature_name = "constant" + ToString(input_features.size());
  writer->AddFeature(feature_name);
}

std::function<void(TransDatum*)> ConstantTransform::GenerateTransform(
    const TransformParam& param) const {

  const ConstantTransformConfig& config = 
    param.GetConfig().constant_transform();
  auto constant = config.constant();
  BigInt offset = 0; //only one feature is added and idx always starts from 0
  return [offset, constant] (TransDatum *datum) {
      datum->SetFeatureValRelativeOffset(offset, constant);
    };
}

} // namespace hotbox

