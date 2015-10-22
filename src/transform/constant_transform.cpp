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
  writer->AddFeature("constant" + ToString(config.constant()));
}

std::function<void(TransDatum*)> ConstantTransform::GenerateTransform(
    const TransformParam& param) const {
  const ConstantTransformConfig& config = 
    param.GetConfig().constant_transform();
  auto constant = config.constant();
  return [constant] (TransDatum *datum) {
      // only one feature is added and idx always starts from 0
      datum->SetFeatureValRelativeOffset(0, constant);
    };
}

} // namespace hotbox

