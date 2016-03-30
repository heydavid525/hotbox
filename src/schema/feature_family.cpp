#include <glog/logging.h>
#include <cstdint>
#include <map>
#include "schema/feature_family.hpp"
#include "schema/constants.hpp"
#include <algorithm>
#include <sstream>

namespace hotbox {

// ============ FeatureFamilyIf ==============

FeatureFamilyProto FeatureFamilyIf::GetProtoIntern() const {
  FeatureFamilyProto proto;
  proto.set_family_name(family_name_);
  (*proto.mutable_offset_begin()) = offset_begin_;
  (*proto.mutable_offset_end()) = offset_end_;
  proto.set_simple_family(IsSimple());
  proto.set_global_offset_begin(global_offset_begin_);
  return proto;
}

// ============ SimpleFeatureFamily ==============

SimpleFeatureFamily::SimpleFeatureFamily(const std::string& family_name,
    const DatumProtoStoreOffset& store_offset,
    FeatureStoreType store_type,
    BigInt global_offset_begin) : 
  FeatureFamilyIf(family_name, store_offset, global_offset_begin),
  store_type_(store_type) { }

bool SimpleFeatureFamily::HasFeature(BigInt family_idx) const {
  return family_idx < (offset_end_.offsets(store_type_) -
      offset_begin_.offsets(store_type_));
}

const Feature& SimpleFeatureFamily::GetFeature(const std::string& feature_name)
  const {
    LOG(FATAL) << "SimpleFeaturefamily does't support accessing feature "
      "through feature_name.";
  }

Feature SimpleFeatureFamily::CreateFeature(BigInt family_idx) const {
  Feature f;
  auto store_offset = GetStoreTypeAndOffset();
  f.set_store_type(store_offset.store_type());
  f.set_store_offset(store_offset.offset_begin() + family_idx);
  f.set_global_offset(global_offset_begin_ + family_idx);
  return f;
}

std::pair<Feature, bool> SimpleFeatureFamily::GetFeatureNoExcept(BigInt family_idx)
  const noexcept {
  if (!this->HasFeature(family_idx)) {
    return std::make_pair(Feature(), false);
  }
  return std::make_pair(CreateFeature(family_idx), true);
}

Feature SimpleFeatureFamily::GetFeature(BigInt family_idx) const {
  if (!this->HasFeature(family_idx)) {
    LOG(FATAL) << GetFamilyName() << ":" << std::to_string(family_idx)
      << " not found";
  }
  return CreateFeature(family_idx);
}

void SimpleFeatureFamily::AddFeature(Feature* new_feature,
    BigInt family_idx) {
  if (family_idx == -1) {
    family_idx = GetMaxFeatureId() + 1;
  }
  // Simple family's feature can be added out of order, but the global
  // offset should be the same order as family_idx.
  new_feature->set_global_offset(global_offset_begin_ + family_idx);
  CHECK_EQ(store_type_, new_feature->store_type());
  AddFeatureRange(family_idx);
}

void SimpleFeatureFamily::AddFeatureRange(BigInt family_idx) {
  auto curr_offset_begin = offset_begin_.offsets(store_type_);
  auto curr_offset_end = offset_end_.offsets(store_type_);
  offset_end_.set_offsets(store_type_, std::max(curr_offset_end,
        curr_offset_begin + family_idx + 1));
}

StoreTypeAndOffset SimpleFeatureFamily::GetStoreTypeAndOffset() const {
  StoreTypeAndOffset store_type_offset;
  store_type_offset.set_store_type(store_type_);
  store_type_offset.set_offset_begin(offset_begin_.offsets(store_type_));
  store_type_offset.set_offset_end(offset_end_.offsets(store_type_));
  return store_type_offset;
}

BigInt SimpleFeatureFamily::GetNumFeatures() const {
  return offset_end_.offsets(store_type_) - offset_begin_.offsets(store_type_);
}

BigInt SimpleFeatureFamily::GetMaxFeatureId() const {
  return offset_end_.offsets(store_type_) - offset_begin_.offsets(store_type_) - 1;
}

FeatureFamilyProto SimpleFeatureFamily::GetProto() const {
  FeatureFamilyProto proto = FeatureFamilyIf::GetProtoIntern();
  proto.set_store_type(store_type_);
  return proto;
}


// ============ FeatureFamily ==============

FeatureFamily::FeatureFamily(const std::string& family_name,
    const DatumProtoStoreOffset& store_offset,
    BigInt global_offset_begin) :
  FeatureFamilyIf(family_name, store_offset, global_offset_begin) { }

FeatureFamily::FeatureFamily(const FeatureFamilyProto& proto) :
  FeatureFamilyIf(proto),
  features_(proto.features().cbegin(), proto.features().cend()),
  initialized_(proto.initialized().cbegin(), proto.initialized().cend()),
  name_to_family_idx_(proto.name_to_family_idx().cbegin(),
      proto.name_to_family_idx().cend()) {
    LOG(INFO) << "FeatureFamily " << family_name_
      << " initialized. # features: " << GetNumFeatures()
      << " features";
  }

bool FeatureFamily::HasFeature(BigInt family_idx) const {
  return family_idx < initialized_.size() && initialized_[family_idx];
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
    return features_[family_idx];
  }

std::pair<Feature, bool> FeatureFamily::GetFeatureNoExcept(BigInt family_idx)
  const noexcept {
  if (!this->HasFeature(family_idx)) {
    return std::make_pair(Feature(), false);
  }
  return std::make_pair(features_[family_idx], true);
}

Feature FeatureFamily::GetFeature(BigInt family_idx) const {
  CheckFeatureExist(family_idx);
  return features_[family_idx];
}

FeatureSeq FeatureFamily::GetFeatures() const {
  // Remove uninitialized features (which have global_idx_ = -1).
  std::vector<BigInt> compact_idx;
  for (int i = 0; i < features_.size(); ++i) {
    if (initialized_[i]) {
      compact_idx.push_back(i);
    }
  }
  return FeatureSeq(features_, compact_idx);
}

BigInt FeatureFamily::GetNumFeatures() const {
  BigInt num_features = 0;
  for (const auto& i : initialized_) {
    if (i) {
      num_features++;
    }
  }
  return num_features;
}

BigInt FeatureFamily::GetMaxFeatureId() const {
  for (BigInt i = initialized_.size() - 1; i >= 0; --i) {
    if (initialized_[i]) {
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


void FeatureFamily::AddFeature(Feature* new_feature, BigInt family_idx) {
  if (family_idx == -1) {
    family_idx = GetMaxFeatureId() + 1;
  }
  new_feature->set_global_offset(global_offset_begin_ + family_idx);
  CHECK(!HasFeature(family_idx)) << "Family idx "
    << family_idx << " in " << family_name_ << " is already initialized in "
    << GetFamilyName();
  if (family_idx >= features_.size()) {
    features_.resize(family_idx + 1);
    initialized_.resize(family_idx + 1, -1);
    initialized_[family_idx] = true;
  }
  UpdateOffsets(*new_feature);
  features_[family_idx] = *new_feature;
  const auto& feature_name = new_feature->name();
  if (!feature_name.empty()) {
    const auto& r =
      name_to_family_idx_.emplace(std::make_pair(feature_name, family_idx));
    CHECK(r.second) << "Feature name " << feature_name
      << " already existed in family " << family_name_;
  }
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
  FeatureFamilyProto proto = FeatureFamilyIf::GetProtoIntern();
  auto idx = proto.mutable_name_to_family_idx();
  for (const auto& p : name_to_family_idx_) {
    (*idx)[p.first] = p.second;
  }
  proto.mutable_initialized()->Resize(initialized_.size(), false);
  for (int i = 0; i < initialized_.size(); ++i) {
    proto.set_initialized(i, initialized_[i]);
  }
  proto.mutable_features()->Reserve(features_.size());
  for (int i = 0; i < features_.size(); ++i) {
    *proto.add_features() = features_[i];
  }
  return proto;
}



}  // namespace hotbox
