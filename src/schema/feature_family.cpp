#include <glog/logging.h>
#include <cstdint>
#include <map>
#include "schema/feature_family.hpp"

namespace mldb {

FeatureFamily::FeatureFamily(const std::string& family_name) :
  family_name_(family_name) { }

bool FeatureFamily::HasFeature(int32_t family_idx) const {
  return initialized_[family_idx];
}

// TODO(wdai): compact the offsets in DatumProto.
void FeatureFamily::DeleteFeature(int32_t family_idx) {
  initialized_[family_idx] = false;
}

const Feature& FeatureFamily::GetFeature(const std::string& feature_name)
  const {
    const auto& it = name_to_family_idx_.find(feature_name);
    if (it == name_to_family_idx_.cend()) {
      FeatureFinder not_found_feature;
      not_found_feature.family_name = family_name_;
      not_found_feature.feature_name = feature_name;
      throw FeatureNotFoundException(not_found_feature);
    }
    return GetFeature(it->second);
  }

Feature& FeatureFamily::GetMutableFeature(const std::string& feature_name) {
  const auto& it = name_to_family_idx_.find(feature_name);
  if (it == name_to_family_idx_.cend()) {
    FeatureFinder not_found_feature;
    not_found_feature.family_name = family_name_;
    not_found_feature.feature_name = feature_name;
    throw FeatureNotFoundException(not_found_feature);
  }
  return GetMutableFeature(it->second);
}

const Feature& FeatureFamily::GetFeature(int32_t family_idx) const {
  if (!initialized_[family_idx]) {
    FeatureFinder not_found_feature;
    not_found_feature.family_name = family_name_;
    not_found_feature.family_idx = family_idx;
    throw FeatureNotFoundException(not_found_feature);
  }
  return features_[family_idx];
}

Feature& FeatureFamily::GetMutableFeature(int32_t family_idx) {
  if (!initialized_[family_idx]) {
    FeatureFinder not_found_feature;
    not_found_feature.family_name = family_name_;
    not_found_feature.family_idx = family_idx;
    throw FeatureNotFoundException(not_found_feature);
  }
  return features_[family_idx];
}

// TODO(wdai): Do smarter thing about initialized_.
const std::vector<Feature>& FeatureFamily::GetFeatures() const {
  for (int i = 0; i < initialized_.size(); ++i) {
    CHECK(initialized_[i]) << "Feature " << i << " of family " << family_name_
      << " uninitialized";
  }
  return features_;
}

void FeatureFamily::AddFeature(const Feature& new_feature,
    int32_t family_idx) {
  if (family_idx >= features_.size()) {
    features_.resize(family_idx + 1);
    initialized_.resize(family_idx + 1);
  }
  features_[family_idx] = new_feature;
  CHECK(!initialized_[family_idx]) << "Family idx "
    << family_idx << " in " << family_name_ << " is already initialized.";
  initialized_[family_idx] = true;
  const auto& feature_name = new_feature.name();
  if (!feature_name.empty()) {
    const auto& r =
      name_to_family_idx_.emplace(std::make_pair(feature_name, family_idx));
    CHECK(r.second) << "Feature name " << feature_name
      << " already existed in family " << family_name_;
  }
}

}  // namespace mldb
