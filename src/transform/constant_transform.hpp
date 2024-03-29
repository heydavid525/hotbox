#pragma once

#include "transform/transform_api.hpp"

namespace hotbox {

class ConstantTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const override;

  std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const override;
};

} // namespace hotbox
