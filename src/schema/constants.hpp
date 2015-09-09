#pragma once

#include <string>

namespace mldb {

const std::string kDefaultFamily = "default";
const std::string kInternalFamily = "_";
const int kLabelFamilyIdx = 0;
const int kWeightFamilyIdx = 1;
const std::string kLabelFeatureName = "label";
const std::string kWeightFeatureName = "weight";

}  // namespace mldb
