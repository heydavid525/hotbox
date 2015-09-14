#pragma once

#include "transform/transform_api.hpp"

namespace mldb {

class OneHotTransform : public TransformIf {
public:
  void TransformSchema(const TransformParams& params,
      TransformWriter* writer) const override;

  std::function<void(DatumBase*)> GenerateTransform(
      const TransformParams& params) const override;
};

}  // namespace mldb
