#include <glog/logging.h>
#include <cstdint>
#include <map>
#include "schema/feature_family.hpp"
#include "schema/constants.hpp"

namespace mldb {

FeatureFamily::FeatureFamily(const std::string& family_name) :
  family_name_(family_name) { }

bool FeatureFamily::HasFeature(int64_t family_idx) const {
  return features_.size() > family_idx && features_[family_idx].initialized();
}

// TODO(wdai): compact the offsets in DatumProto (can be expensive).
void FeatureFamily::DeleteFeature(int64_t family_idx) {
  features_[family_idx].set_initialized(false);
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

const Feature& FeatureFamily::GetFeature(int64_t family_idx) const {
  CheckFeatureExist(family_idx);
  return features_[family_idx];
}

Feature& FeatureFamily::GetMutableFeature(int64_t family_idx) {
  CheckFeatureExist(family_idx);
  return features_[family_idx];
}

const std::vector<Feature>& FeatureFamily::GetFeatures() const {
  return features_;
}

BigInt FeatureFamily::GetNumFeatures() const {
  BigInt num_features = 0;
  for (BigInt i = 0; i < features_.size(); ++i) {
    if (features_[i].initialized()) {
      num_features++;
    }
  }
  return num_features;
}

BigInt FeatureFamily::GetMaxFeatureId() const {
  for (BigInt i = features_.size() - 1; i >= 0; --i) {
    if (features_[i].initialized()) {
      return i;
    }
  }
  return -1;
}

void FeatureFamily::AddFeature(const Feature& new_feature,
    int64_t family_idx) {
  if (family_idx == -1) {
    family_idx = GetMaxFeatureId() + 1;
  }
  if (family_idx >= features_.size()) {
    features_.resize(family_idx + 1);
  }
  CHECK(!features_[family_idx].initialized()) << "Family idx "
    << family_idx << " in " << family_name_ << " is already initialized.";
  features_[family_idx] = new_feature;
  features_[family_idx].set_initialized(true);
  const auto& feature_name = new_feature.name();
  if (!feature_name.empty()) {
    const auto& r =
      name_to_family_idx_.emplace(std::make_pair(feature_name, family_idx));
    CHECK(r.second) << "Feature name " << feature_name
      << " already existed in family " << family_name_;
  }
}

std::string FeatureFamily::GetFamilyName() const {
  return family_name_;
}

void FeatureFamily::CheckFeatureExist(BigInt family_idx) const {
  if (!HasFeature(family_idx)) {
    FeatureFinder not_found_feature;
    not_found_feature.family_name = family_name_;
    not_found_feature.family_idx = family_idx;
    throw FeatureNotFoundException(not_found_feature);
  }
}

FeatureFamilyProto FeatureFamily::GetProto() const {
  FeatureFamilyProto proto;
  proto.set_family_name(family_name_);
  auto idx = proto.mutable_name_to_family_idx();
  for (const auto& p : name_to_family_idx_) {
    (*idx)[p.first] = p.second;
  }
  proto.mutable_features()->Reserve(features_.size());
  for (BigInt i = 0; i < features_.size(); ++i) {
    *(proto.add_features()) = features_[i];
  }
  return proto;
}

FeatureFamily::FeatureFamily(const FeatureFamilyProto& proto) :
family_name_(proto.family_name()),
  name_to_family_idx_(proto.name_to_family_idx().begin(),
      proto.name_to_family_idx().end()) {
    features_.resize(proto.features_size());
    for (BigInt i = 0; i < features_.size(); ++i) {
      features_[i] = proto.features(i);
    }
}

}  // namespace mldb
