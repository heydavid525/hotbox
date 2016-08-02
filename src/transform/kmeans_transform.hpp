#pragma once

#include "transform/transform_api.hpp"

namespace hotbox {

class KmeansTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const override;

  std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const override;
private:
  constexpr static double MAX_VALUE = 999999;
};

} // namespace hotbox

