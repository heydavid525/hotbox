#pragma once

#include <vector>
#include "transform/transform_api.hpp"

namespace hotbox {

// Get values as pairs of idx:val (sparse).
std::vector<std::pair<BigInt, float>> GetSparseVals(const TransDatum& datum,
    const WideFamilySelector& selector);

// Get values as std::vector<float> (dense)
std::vector<float> GetDenseVals(const TransDatum& datum,
  const WideFamilySelector& selector);

}  // namespace hotbox
