#include <glog/logging.h>
#include <cstdint>
#include <map>
#include "schema/feature_family.hpp"
#include "schema/constants.hpp"
#include <algorithm>

namespace hotbox {

FeatureFamily::FeatureFamily(const std::string& family_name,
    std::shared_ptr<std::vector<Feature>> features, bool simple_family) :
  family_name_(family_name), features_(features),
  simple_family_(simple_family) {
    offset_begin_.mutable_offsets()->Resize(
        FeatureStoreType::NUM_STORE_TYPES, 0);
    offset_end_.mutable_offsets()->Resize(
        FeatureStoreType::NUM_STORE_TYPES, 0);
  }

FeatureFamily::FeatureFamily() { }

bool FeatureFamily::HasFeature(BigInt family_idx) const {
  return global_idx_.size() > family_idx &&
    global_idx_[family_idx] >= 0;
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
    auto family_idx = it->second;
    CheckFeatureExist(family_idx);
    return (*features_)[global_idx_[family_idx]];
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

std::pair<Feature, bool> FeatureFamily::GetFeatureNoExcept(BigInt family_idx)
  const {
  if (!this->HasFeature(family_idx)) {
    return std::make_pair(Feature(), false);
  }
  return std::make_pair((*features_)[global_idx_[family_idx]], true);
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
  CHECK(!simple_family_);
  /*
  if (simple_family_) {
    auto store_type = GetStoreTypeAndOffset().store_type();
    BigInt num_features_all = offset_end_.offsets(store_type) -
      offset_begin_.offsets(store_type);
    // fill_in_features includes uninstantiated features, if any.
    std::shared_ptr<std::vector<Feature>> fill_in_features =
      std::make_shared<std::vector<Feature>>(num_features_all);
    std::vector<BigInt> idx(num_features_all);
    std::iota(idx.begin(), idx.end(), 0);
    for (int i = 0; i < global_idx_.size(); ++i) {
      if (global_idx_[i] != -1) {
        (*fill_in_features)[i] = (*features_)[global_idx_[i]];
      } else {
        (*fill_in_features)[i].set_store_type(store_type);
      }
    }
    return FeatureSeq(fill_in_features, idx);
  }
  */

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

void FeatureFamily::UpdateOffsets(const Feature& new_feature) {
  FeatureStoreType store_type = new_feature.store_type();
  auto curr_offset_end = offset_end_.offsets(store_type);
  offset_end_.set_offsets(store_type,
      std::max(new_feature.store_offset() + 1, curr_offset_end));
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
  UpdateOffsets(new_feature);
  global_idx_[family_idx] = new_feature.global_offset();
  //LOG(INFO) << "Added feature id " << family_idx << " family name: " << family_name_ << " output offset_end: " << offset_end_.offsets(FeatureStoreType::OUTPUT);
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
  (*proto.mutable_offset_begin()) = offset_begin_;
  (*proto.mutable_offset_end()) = offset_end_;
  proto.set_simple_family(simple_family_);
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
  (*proto.mutable_offset_begin()) = offset_begin_;
  (*proto.mutable_offset_end()) = offset_end_;
  proto.set_simple_family(simple_family_);
  return proto;
}

FeatureFamily::FeatureFamily(const FeatureFamilyProto& proto,
    std::shared_ptr<std::vector<Feature>> features, bool simple_family) :
  family_name_(proto.family_name()), features_(features),
  name_to_family_idx_(proto.name_to_family_idx().cbegin(),
      proto.name_to_family_idx().cend()),
  global_idx_(proto.global_idx().cbegin(), proto.global_idx().cend()),
  offset_begin_(proto.offset_begin()), offset_end_(proto.offset_end()),
  simple_family_(proto.simple_family()) {
  }

FeatureFamily::FeatureFamily(const SelfContainedFeatureFamilyProto& proto) :
  family_name_(proto.family_name()),
  features_(new std::vector<Feature>(proto.features().cbegin(),
        proto.features().cend())),
  name_to_family_idx_(proto.name_to_family_idx().cbegin(),
      proto.name_to_family_idx().cend()),
  global_idx_(features_->size()) {
    // Fill in [0, 1, ..., global_idx_.size() -1]
    std::iota(global_idx_.begin(), global_idx_.end(), 0);
  }

StoreTypeAndOffset FeatureFamily::GetStoreTypeAndOffset() const {
  CHECK(IsSimple());
  StoreTypeAndOffset store_type_offset;
  for (int i = 0; i < FeatureStoreType::NUM_STORE_TYPES; ++i) {
    if (offset_begin_.offsets(i) != offset_end_.offsets(i)) {
      store_type_offset.set_store_type(static_cast<FeatureStoreType>(i));
      store_type_offset.set_offset_begin(offset_begin_.offsets(i));
      store_type_offset.set_offset_end(offset_end_.offsets(i));
      return store_type_offset;
    }
  }
  LOG(FATAL) << "Should not get here. Family name: " << family_name_
    << " offset_begin_: " << offset_begin_.DebugString()
    << " offset_end_: " << offset_end_.DebugString();
  return store_type_offset;
}

}  // namespace hotbox
