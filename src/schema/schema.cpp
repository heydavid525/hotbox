#include <glog/logging.h>
#include <cstdint>
#include <sstream>
#include "schema/schema.hpp"
#include "schema/constants.hpp"
#include "schema/schema_util.hpp"

namespace mldb {

Schema::Schema(const SchemaConfig& config) : internal_family_(kInternalFamily) {
  // Add label.
  auto type = config.int_label() ? FeatureType::CATEGORICAL :
    FeatureType::NUMERICAL;
  auto store_type = FeatureStoreType::DENSE;
  Feature label = CreateFeature(type, store_type, kLabelFeatureName);
  int internal_family_idx = 0;
  AddFeature(kInternalFamily, internal_family_idx++, &label);

  // Add weight
  type = FeatureType::NUMERICAL;
  store_type = config.use_dense_weight() ? FeatureStoreType::DENSE
    : FeatureStoreType::SPARSE;
  Feature weight = CreateFeature(type, store_type, kWeightFeatureName);
  AddFeature(kInternalFamily, internal_family_idx, &weight);
}

void Schema::AddFeature(const std::string& family_name, int32_t family_idx,
    Feature* new_feature) {
  UpdateOffset(new_feature);
  GetOrCreateFamily(family_name).AddFeature(*new_feature, family_idx);
}

const Feature& Schema::GetFeature(const std::string& family_name,
    int32_t family_idx) const {
  return GetOrCreateFamily(family_name).GetFeature(family_idx);
}

Feature& Schema::GetMutableFeature(const std::string& family_name,
    int32_t family_idx) {
  return GetOrCreateFamily(family_name).GetMutableFeature(family_idx);
}

const Feature& Schema::GetFeature(const FeatureFinder& finder) const {
  const auto& family = GetOrCreateFamily(finder.family_name);
  return finder.feature_name.empty() ? family.GetFeature(finder.family_idx) :
    family.GetFeature(finder.feature_name);
}

Feature& Schema::GetMutableFeature(const FeatureFinder& finder) {
  auto& family = GetOrCreateFamily(finder.family_name);
  return finder.feature_name.empty() ?
    family.GetMutableFeature(finder.family_idx) :
    family.GetMutableFeature(finder.feature_name);
}

const FeatureFamily& Schema::GetFamily(const std::string& family_name) const {
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    throw FamilyNotFoundException(family_name);
  }
  return it->second;
}

FeatureFamily& Schema::GetOrCreateFamily(const std::string& family_name)
  const {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    auto inserted = families_.emplace(
        std::make_pair(family_name, FeatureFamily(family_name)));
    it = inserted.first;
  }
  return it->second;
}

const DatumProtoOffset& Schema::GetDatumProtoOffset() const {
  return append_offset_;
}

const std::map<std::string, FeatureFamily>& Schema::GetFamilies() const {
  return families_;
}

int Schema::GetNumFeatures() const {
  int num_features = 0;
  for (const auto& p : families_) {
    num_features += p.second.GetNumFeatures();
  }
  return num_features;
}

void Schema::UpdateOffset(Feature* new_feature) {
  int32_t offset;
  if (IsCategorical(*new_feature)) {
    if (IsDense(*new_feature)) {
      offset = append_offset_.dense_cat_store();
      append_offset_.set_dense_cat_store(offset + 1);
    } else {
      offset = append_offset_.sparse_cat_store();
      append_offset_.set_sparse_cat_store(offset + 1);
    }
  } else if (IsNumerical(*new_feature)) {
    if (IsDense(*new_feature)) {
      offset = append_offset_.dense_num_store();
      append_offset_.set_dense_num_store(offset + 1);
    } else {
      offset = append_offset_.sparse_num_store();
      append_offset_.set_sparse_num_store(offset + 1);
    }
  } else {
    if (IsDense(*new_feature)) {
      offset = append_offset_.dense_bytes_store();
      append_offset_.set_dense_bytes_store(offset + 1);
    } else {
      offset = append_offset_.sparse_bytes_store();
      append_offset_.set_sparse_bytes_store(offset + 1);
    }
  }
  new_feature->mutable_loc()->set_offset(offset);
}

SchemaProto Schema::GetProto() const {
  SchemaProto proto;
  auto families = proto.mutable_families();
  for (const auto& p : families_) {
    (*families)[p.first] = p.second.GetProto();
  }
  (*families)[kInternalFamily] = internal_family_.GetProto();
  *(proto.mutable_append_offset()) = append_offset_;
  return proto;
}

Schema::Schema(const SchemaProto& proto) :
internal_family_(proto.families().at(kInternalFamily)) {
  for (const auto& p : proto.families()) {
    if (p.first == kInternalFamily) {
      internal_family_ = p.second;
    }
  }
  append_offset_ = proto.append_offset();
}

std::string Schema::Serialize() const {
  auto proto = GetProto();
  std::string data;
  proto.SerializeToString(&data);
  return data;
}

}  // namespace mldb
