#include <glog/logging.h>
#include <cstdint>
#include <sstream>
#include "util/all.hpp"
#include "schema/schema.hpp"
#include "schema/constants.hpp"
#include "schema/schema_util.hpp"

namespace hotbox {

Schema::Schema(const SchemaConfig& config) :
  features_(new std::vector<Feature>),
  internal_family_(kInternalFamily, features_),
  config_(config) {
  append_store_offset_.mutable_offsets()->Resize(
      FeatureStoreType::NUM_STORE_TYPES, 0);
  // Add label.
  auto store_type = config.int_label() ? FeatureStoreType::DENSE_CAT :
    FeatureStoreType::DENSE_NUM;
  Feature label = CreateFeature(store_type, kLabelFeatureName);
  AddFeature(kInternalFamily, &label, kLabelFamilyIdx);

  // Add weight
  store_type = config.use_dense_weight() ? FeatureStoreType::DENSE_NUM
    : FeatureStoreType::SPARSE_NUM;
  Feature weight = CreateFeature(store_type, kWeightFeatureName);
  AddFeature(kInternalFamily, &weight, kWeightFamilyIdx);
}

Schema::Schema(const SchemaProto& proto) {
  features_.reset(new std::vector<Feature>(proto.features().cbegin(),
        proto.features().cend()));

  for (const auto& p : proto.families()) {
    LOG(INFO) << "Initializing schema, family: " << p.first;
    if (p.first == kInternalFamily) {
      internal_family_ = FeatureFamily(p.second, features_);
    } else {
      families_.emplace(std::make_pair(p.first,
            FeatureFamily(p.second, features_)));
    }
  }
  output_families_.resize(proto.output_families_size());
  for (int i = 0; i < proto.output_families_size(); ++i) {
    output_families_[i] = proto.output_families(i);
  }
  append_store_offset_ = proto.append_store_offset();
}

void Schema::AddFeature(const std::string& family_name,
    Feature* new_feature, BigInt family_idx) {
  UpdateStoreOffset(new_feature);
  new_feature->set_global_offset(features_->size());
  features_->emplace_back(*new_feature);
  //LOG(INFO) << "Adding feature " << new_feature->name() << " to family "
  //  << family_name << " global offset: " << new_feature->global_offset();
  GetOrCreateMutableFamily(family_name).AddFeature(*new_feature, family_idx);
}

void Schema::AddFeature(FeatureFamily* family, Feature* new_feature,
    BigInt family_idx) {
  UpdateStoreOffset(new_feature);
  new_feature->set_global_offset(features_->size());
  features_->emplace_back(*new_feature);
  //LOG(INFO) << "Adding feature " << new_feature->name() << " to family "
  //  << family->GetFamilyName() << " global offset: "
  //  << new_feature->global_offset();
  family->AddFeature(*new_feature, family_idx);
}

const Feature& Schema::GetFeature(const std::string& family_name,
    BigInt family_idx) const {
  return GetOrCreateFamily(family_name).GetFeature(family_idx);
}

const DatumProtoStoreOffset& Schema::GetAppendOffset() const {
  return append_store_offset_;
}

Feature& Schema::GetMutableFeature(const std::string& family_name,
    BigInt family_idx) {
  return GetOrCreateMutableFamily(family_name).GetMutableFeature(family_idx);
}

const Feature& Schema::GetFeature(const FeatureFinder& finder) const {
  const auto& family = GetOrCreateFamily(finder.family_name);
  return finder.feature_name.empty() ? family.GetFeature(finder.family_idx) :
    family.GetFeature(finder.feature_name);
}

Feature& Schema::GetMutableFeature(const FeatureFinder& finder) {
  auto& family = GetOrCreateMutableFamily(finder.family_name);
  return finder.feature_name.empty() ?
    family.GetMutableFeature(finder.family_idx) :
    family.GetMutableFeature(finder.feature_name);
}

Feature& Schema::GetMutableFeature(const Feature& feature) {
  return (*features_)[feature.global_offset()];
}

const FeatureFamily& Schema::GetFamily(const std::string& family_name) const {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    throw FamilyNotFoundException(family_name);
  }
  return it->second;
}

// Comment(wdai): GetOrCreateFamily has identical implementation as
// GetOrCreateMutableFamily.
const FeatureFamily& Schema::GetOrCreateFamily(const std::string& family_name,
    bool output_family) const {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    LOG(INFO) << "Insert family " << family_name << " to families_";
    auto inserted = families_.emplace(
        std::make_pair(family_name, FeatureFamily(family_name, features_)));
    it = inserted.first;
    if (output_family) {
      output_families_.push_back(family_name);
    }
  }
  return it->second;
}

FeatureFamily& Schema::GetOrCreateMutableFamily(const std::string& family_name,
    bool output_family) {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    LOG(INFO) << "Insert family " << family_name << " to families_";
    auto inserted = families_.emplace(
        std::make_pair(family_name, FeatureFamily(family_name, features_)));
    it = inserted.first;
    if (output_family) {
      LOG(INFO) << "Adding " << family_name << " to output_families_";
      output_families_.push_back(family_name);
    }
  }
  return it->second;
}

const std::map<std::string, FeatureFamily>& Schema::GetFamilies() const {
  return families_;
}

// Comment(wdai): Do not use featuers_->size() which includes internal
// feature family.
BigInt Schema::GetNumFeatures() const {
  BigInt num_features = 0;
  for (const auto& p : families_) {
    //LOG(INFO) << "Schema::GetNumFeatures " << p.first <<
    //  " has # features: " << p.second.GetNumFeatures();
    num_features += p.second.GetNumFeatures();
  }
  return num_features;
}

void Schema::UpdateStoreOffset(Feature* new_feature) {
  FeatureStoreType store_type = new_feature->store_type();
  auto curr_store_offset = append_store_offset_.offsets(store_type);
  new_feature->set_store_offset(curr_store_offset);
  append_store_offset_.set_offsets(store_type, curr_store_offset + 1);
}

OSchemaProto Schema::GetOSchemaProto() const {
  BigInt output_feature_dim =
    append_store_offset_.offsets(FeatureStoreType::OUTPUT);

  OSchemaProto proto;
  proto.mutable_feature_names()->Reserve(output_feature_dim);
  proto.mutable_family_names()->Reserve(output_families_.size());
  proto.mutable_family_offsets()->Resize(output_families_.size(), 0);

  BigInt curr_feature_id = 0;
  for (int i = 0; i < output_families_.size(); ++i) {
    const FeatureFamily& family = GetFamily(output_families_[i]);
    const auto& feature_seq = family.GetFeatures();
    CHECK_GT(feature_seq.GetNumFeatures(), 0);

    // We assume output feature family's features are added in ascending
    // order, so the offset of first feature is the family offset.
    BigInt family_offset = feature_seq.GetFeature(0).store_offset();
    proto.add_family_names(family.GetFamilyName());
    proto.set_family_offsets(i, family_offset);
    CHECK_EQ(curr_feature_id, family_offset);
    for (int j = 0; j < feature_seq.GetNumFeatures(); ++j) {
      const auto& f = feature_seq.GetFeature(j);
      proto.add_feature_names(f.name());

      // Verify that feature offset matches the location of the feature's name
      // in OSchemaProto feature_names.  OUTPUT features in each family
      // should be added in exactly this order.
      CHECK_EQ(f.store_offset(), curr_feature_id++) << f.name();
    }
  }
  return proto;
}

SchemaConfig Schema::GetConfig() const {
  return config_;
}

SchemaProto Schema::GetProto() const {
  SchemaProto proto;
  auto families = proto.mutable_families();
  for (const auto& p : families_) {
    (*families)[p.first] = p.second.GetProto();
  }
  (*families)[kInternalFamily] = internal_family_.GetProto();
  *(proto.mutable_append_store_offset()) = append_store_offset_;
  proto.mutable_output_families()->Reserve(output_families_.size());
  for (int i = 0; i < output_families_.size(); ++i) {
    *(proto.add_output_families()) = output_families_[i];
  }
  proto.mutable_features()->Reserve(features_->size());
  for (BigInt i = 0; i < features_->size(); ++i) {
    *(proto.add_features()) = (*features_)[i];
  }
  return proto;
}

std::string Schema::Serialize() const {
  auto proto = GetProto();
  return SerializeProto(proto);
}

}  // namespace hotbox
