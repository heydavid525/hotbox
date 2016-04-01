#pragma once

#include "transform/transform_api.hpp"

namespace hotbox {

// By default we do not generate the last bucket.
class NgramTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const override;

  std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const override;
};

}  // namespace hotbox
