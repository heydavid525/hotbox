#pragma once

#include "transform/transform_api.hpp"

namespace hotbox {

// By default we do not generate the last bucket.
class SelectTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const override;

  std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const override;

protected:
  void SetTransformWriterConfig(const TransformConfig& config,
      TransformWriterConfig* writer_config) const override;
};

}  // namespace hotbox
