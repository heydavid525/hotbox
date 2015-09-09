#pragma once

#include <string>
#include "schema/proto/schema.pb.h"
#include "schema/feature_finder.hpp"

namespace mldb {

Feature CreateFeature(const FeatureType& type, const FeatureStoreType& store_type,
    const std::string& name = "");

// Parse feature descriptor (e.g., "mobile:ctr,num_views"). Return finders
// ordered by appearance in feature descriptor.
//
// Some valid feature descriptors:
// "feat1,feat2,fam1:feat3,:feat4" --> [(default, feat1), (default, feat2), (fam1,
// feat3), (default, feat4)]
// "feat5, feat6, fam2:feat7+feat8, feat9" --> [(default, feat5), (default, feat6),
// (fam2, feat7), (fam2, feat8), (default, feat9)]
// "fam3:" --> [(fam3, "")]. Empty string means family-wide selection.
//
// TODO(wdai): Beef up the error checking and messages. E.g, check for
// duplicated selection.
std::vector<FeatureFinder> ParseFeatureDesc(const std::string& feature_desc);

// Categorical and Numerical features are considered numeral and can be
// transformed.
bool IsNumeral(const Feature& f);
bool IsNumeral(const FeatureLocator& loc);
bool IsCategorical(const Feature& f);
bool IsNumerical(const Feature& f);
bool IsDense(const Feature& f);
bool IsSparse(const Feature& f);

//DatumProtoOffset operator+(const DatumProtoOffset& o1,
//    const DatumProtoOffset& o2);

}  // namespace mldb
