#include <glog/logging.h>
#include <cstdint>
#include <map>
#include "schema/feature_family.hpp"
#include "schema/constants.hpp"
#include <algorithm>

namespace hotbox {

FeatureFamily::FeatureFamily(const std::string& family_name,
    std::shared_ptr<std::vector<Feature>> features) :
  family_name_(family_name), features_(features) { }

FeatureFamily::FeatureFamily() { }

bool FeatureFamily::HasFeature(BigInt family_idx) const {
  return global_idx_.size() > family_idx &&
    global_idx_[family_idx] >= 0;
}

/*
// TODO(wdai): compact the offsets in DatumProto (can be expensive).
void FeatureFamily::DeleteFeature(BigInt family_idx) {
  (*features_)[global_idx_[family_idx]].set_initialized(false);
}
*/

const Feature& FeatureFamily::GetFeature(const std::string& feature_name)
  const {
    const auto& it = name_to_family_idx_.find(feature_name);
    if (it == name_to_family_idx_.cend()) {
      FeatureFinder not_found_feature;
      not_found_feature.family_name = family_name_;
      not_found_feature.feature_name = feature_name;
      throw FeatureNotFoundException(not_found_feature);
    }
    return this->GetFeature(it->second);
  }

Feature& FeatureFamily::GetMutableFeature(const std::string& feature_name) {
  const auto& it = name_to_family_idx_.find(feature_name);
  if (it == name_to_family_idx_.cend()) {
    FeatureFinder not_found_feature;
    not_found_feature.family_name = family_name_;
    not_found_feature.feature_name = feature_name;
    throw FeatureNotFoundException(not_found_feature);
  }
  return this->GetMutableFeature(it->second);
}

const Feature& FeatureFamily::GetFeature(BigInt family_idx) const {
  CheckFeatureExist(family_idx);
  return (*features_)[global_idx_[family_idx]];
}

Feature& FeatureFamily::GetMutableFeature(BigInt family_idx) {
  CheckFeatureExist(family_idx);
  return (*features_)[global_idx_[family_idx]];
}

FeatureSeq FeatureFamily::GetFeatures() const {
  // Remove uninitialized features (which have global_idx_ = -1).
  std::vector<BigInt> compact_idx;
  for (const auto& i : global_idx_) {
    if (i != -1) {
      compact_idx.push_back(i);
    }
  }
  return FeatureSeq(features_, compact_idx);
}

BigInt FeatureFamily::GetNumFeatures() const {
  BigInt num_features = 0;
  for (const auto& i : global_idx_) {
    if (i >= 0) {
      num_features++;
    }
  }
  return num_features;
}

BigInt FeatureFamily::GetMaxFeatureId() const {
  for (BigInt i = global_idx_.size() - 1; i >= 0; --i) {
    if (global_idx_[i] >= 0) {
      return i;
    }
  }
  return -1;
}

void FeatureFamily::AddFeature(const Feature& new_feature, BigInt family_idx) {
  if (family_idx == -1) {
    family_idx = GetMaxFeatureId() + 1;
  }
  if (family_idx >= global_idx_.size()) {
    global_idx_.resize(family_idx + 1, -1);
  }
  CHECK(!HasFeature(family_idx)) << "Family idx "
    << family_idx << " in " << family_name_ << " is already initialized.";
  global_idx_[family_idx] = new_feature.global_offset();
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
  if (!this->HasFeature(family_idx)) {
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
  proto.mutable_global_idx()->Resize(global_idx_.size(), 0);
  for (int i = 0; i < global_idx_.size(); ++i) {
    proto.set_global_idx(i, global_idx_[i]);
  }
  return proto;
}

SelfContainedFeatureFamilyProto FeatureFamily::GetSelfContainedProto() const {
  SelfContainedFeatureFamilyProto proto;
  proto.set_family_name(family_name_);
  auto idx = proto.mutable_name_to_family_idx();
  for (const auto& p : name_to_family_idx_) {
    (*idx)[p.first] = p.second;
  }
  proto.mutable_features()->Reserve(global_idx_.size());
  for (const auto& i : global_idx_) {
    (*proto.add_features()) = (*features_)[i];
  }
  return proto;
}

FeatureFamily::FeatureFamily(const FeatureFamilyProto& proto,
    std::shared_ptr<std::vector<Feature>> features) :
  family_name_(proto.family_name()), features_(features),
  name_to_family_idx_(proto.name_to_family_idx().cbegin(),
      proto.name_to_family_idx().cend()),
  global_idx_(proto.global_idx().cbegin(), proto.global_idx().cend()) {
}

FeatureFamily::FeatureFamily(const SelfContainedFeatureFamilyProto& proto) :
  family_name_(proto.family_name()),
  features_(new std::vector<Feature>(proto.features().cbegin(),
        proto.features().cend())),
  name_to_family_idx_(proto.name_to_family_idx().cbegin(),
      proto.name_to_family_idx().cend()),
  global_idx_(features_->size()) {
    std::iota(global_idx_.begin(), global_idx_.end(), 0);
}

}  // namespace hotbox
