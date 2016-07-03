#pragma once

#include "schema/proto/schema.pb.h"
#include "schema/constants.hpp"

namespace hotbox {

enum SelectMode {
  kSingleFeature = 0  // select only 1 feature (fam:feat)
    , kRangeSelect = 1       // select feature range (fam:1-10 or fam:*)
};

// Select [family_idx_begin, family_idx_end) within
// FeatureFinder::family_name. family_idx_end == -1 indicates selecting all.
struct RangeSelector {
  BigInt family_idx_begin = 0;
  BigInt family_idx_end = 0;
};

// Identify a feature (or a family) in the schema.
struct FeatureFinder {
  std::string family_name;

  // if true, ignore feature_name and family_idx.
  SelectMode mode = kSingleFeature;

  // Ignored if mode != kSingleFeature
  // At most one of the following should be set.
  std::string feature_name;
  BigInt family_idx = -1;

  // Ignored if mode != kRangeSelect
  RangeSelector range_selector;
};

struct TypedFeatureFinder : public FeatureFinder {
  TypedFeatureFinder(const FeatureFinder& finder, const FeatureType& t);

  FeatureType type;
};

}  // namespace hotbox
