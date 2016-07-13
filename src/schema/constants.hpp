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

// By default we use int64_t. Note that if a data base is stored as int64_t
// then a 32bit version will throw runtime failure.
#ifdef USE_INT32_INDEX
typedef int32_t BigInt;
const FeatureIndexType kFeatureIndexType = FeatureIndexType::INT32;
#else
typedef int64_t BigInt;
const FeatureIndexType kFeatureIndexType = FeatureIndexType::INT64;
#endif

// Batch size to break up StatsProto and Schema::features into
// FeatureStatProtoSeq messages that cannot exceeds proto size limit (2GB).
const int kSeqBatchSize = 1e6;

}  // namespace hotbox
