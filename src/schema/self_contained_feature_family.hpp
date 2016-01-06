#pragma once

#include "schema/feature_family.hpp"
#include <algorithm>

namespace hotbox {

class SelfContainedFeatureFamily : public FeatureFamily {
public:
  SelfContainedFeatureFamily(const SelfContainedFeatureFamilyProto& proto) : 
    this->family_name_(proto.family_name()),
    features_(proto.features().cbegin(),
        proto.features().cend()),
    this->name_to_family_idx_(proto.name_to_family_idx().cbegin(),
        proto.name_to_family_idx().cend()) { }

  bool HasFeature(BigInt family_idx) const override {
    return family_idx < features_.size();
  }

  /*
  const Feature& GetFeature(const std::string& feature_name) const;
  Feature& GetMutableFeature(const std::string& feature_name);
  */

  const Feature& GetFeature(BigInt family_idx) const override {
    CheckFeatureExist(family_idx);
    return features_[family_idx];
  }

  Feature& GetMutableFeature(BigInt family_idx) override {
    CheckFeatureExist(family_idx);
    return features_[family_idx];
  }

  // Return both initialized and uninitialized features.
  FeatureSeq GetFeatures() const override {
    std::vector<BigInt> idx(features_.size());
    std::iota(idx.begin(), idx.end(), 0);
    return FeatureSeq(std::make_shared(new std::vector<Feature>(features_)),
        idx);
  }

  // Only count the initialized features.
  BigInt GetNumFeatures() const;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures iff
  // there's no uninitialized features.
  BigInt GetMaxFeatureId() const;

  FeatureFamilyProto GetProto() const;

  // Include Feature of this family. Use this to serialize single family
  // without Schema (e.g., internal family).
  SelfContainedFeatureFamilyProto GetProto() const;

private:
  // features_[family_idx] to access the feature.
  std::vector<Feature> features_;
}

}  // namespace hotbox
