#pragma once

#include "transform/transform_api.hpp"

namespace hotbox{
class TreeTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param, TransformWriter* writer) const override;
  
  //c++ 11 
  std::function<void<TransDatum*>> GenerateTransform(const TransformParam& param) const override;

};

}//namespace hotbox


