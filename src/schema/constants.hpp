#pragma once

#include <string>
#include <cstdint>
#include "schema/proto/schema.pb.h"

namespace hotbox {

const std::string kDefaultFamily = "default";
const std::string kInternalFamily = "_";
const int64_t kLabelFamilyIdx = 0;
const int64_t kWeightFamilyIdx = 1;
const std::string kLabelFeatureName = "label";
const std::string kWeightFeatureName = "weight";

// By default we use int32_t. Note that if a data base is stored as int64_t
// then a 32bit version will throw runtime failure.
#ifndef USE_INT64_INDEX
typedef int64_t BigInt;
const FeatureIndexType kFeatureIndexType = FeatureIndexType::INT64;
#else
typedef int32_t BigInt;
const FeatureIndexType kFeatureIndexType = FeatureIndexType::INT32;
#endif

}  // namespace hotbox
