#pragma once

#include <vector>
#include <functional>
#include "schema/datum_base.hpp"
#include "transform/transform_if.hpp"
#include "transform/proto/transform_configs.pb.h"

namespace mldb {

class OneHotTransform : public TransformIf {
public:
  OneHotTransform(const OneHotTransformOp& op);

  void Transform(DatumBase* datum) const override;

private:
  std::vector<std::function<void(DatumBase*)>> transforms_;
};

}  // namespace mldb
