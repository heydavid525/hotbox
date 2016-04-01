#include <glog/logging.h>
#include <cstdint>
#include <sstream>
#include "util/all.hpp"
#include "schema/schema.hpp"
#include "schema/constants.hpp"
#include "schema/schema_util.hpp"
#include <google/protobuf/text_format.h>
#include <fstream>
#include <algorithm>

namespace hotbox {

namespace {

const std::string kSchemaPrefix = "schema_";

}  // anonymous namespace

Schema::Schema(const SchemaConfig& config) :
  config_(config) {
  append_store_offset_.mutable_offsets()->Resize(
      FeatureStoreType::NUM_STORE_TYPES, 0);
  internal_family_ = FeatureFamily(kInternalFamily, 
      append_store_offset_, curr_global_offset_);
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
  Init(proto);
}

Schema::Schema(const Schema& other) : output_families_(other.output_families_),
  curr_global_offset_(other.curr_global_offset_),
  internal_family_(other.internal_family_),
  append_store_offset_(other.append_store_offset_), config_(other.config_) {
  // Deep copy of families_.
  for (auto& p : other.families_) {
    if (p.second->IsSimple()) {
      families_[p.first] = make_unique<SimpleFeatureFamily>(
          dynamic_cast<const SimpleFeatureFamily&>(*p.second));
    } else {
      families_[p.first] = make_unique<FeatureFamily>(
          dynamic_cast<const FeatureFamily&>(*p.second));
    }
  }
}

void Schema::Init(const SchemaProto& proto) {
  for (const auto& p : proto.families()) {
    std::string family_name = p.first;
    if (family_name == kInternalFamily) {
      internal_family_ = FeatureFamily(p.second);
    } else {
      if (p.second.simple_family()) {
        families_[family_name] = make_unique<SimpleFeatureFamily>(p.second);
      } else {
        families_[family_name] = make_unique<FeatureFamily>(p.second);
      }
    }
  }
  std::copy(proto.output_families().cbegin(), proto.output_families().cend(),
      std::back_inserter(output_families_));
  append_store_offset_ = proto.append_store_offset();
}

Schema::Schema(RocksDB* db) {
  std::string schema_proto_str = db->Get(kSchemaPrefix);
  SchemaProto proto = StreamDeserialize<SchemaProto>(schema_proto_str);
  Init(proto);
}

void Schema::AddFeature(const std::string& family_name,
    Feature* new_feature, BigInt family_idx) {
  auto& family = GetMutableFamily(family_name);
  AddFeature(&family, new_feature, family_idx);
}

void Schema::AddFeature(FeatureFamilyIf* family, Feature* new_feature,
    BigInt family_idx) {
  CHECK_NOTNULL(family);
  CHECK_NOTNULL(new_feature);
  if (family->IsSimple()) {
    BigInt family_offset_begin = family->GetOffsetBegin().offsets(
        new_feature->store_type());
    UpdateStoreOffset(new_feature, family_offset_begin + family_idx);
  } else {
    new_feature->set_global_offset(curr_global_offset_++);
    UpdateStoreOffset(new_feature);
  }
  family->AddFeature(new_feature, family_idx);

  // new_feature->global_offset() is set in AddFeature call.
  curr_global_offset_ = std::max(curr_global_offset_,
      static_cast<BigInt>(new_feature->global_offset()));
}

void Schema::AddFeatures(FeatureFamilyIf* family, BigInt num_features) {
  CHECK_NOTNULL(family);
  CHECK(family->IsSimple());
  SimpleFeatureFamily* s_family = dynamic_cast<SimpleFeatureFamily*>(family);
  s_family->AddFeatures(num_features);
  StoreTypeAndOffset type_offset = s_family->GetStoreTypeAndOffset();
  append_store_offset_.set_offsets(type_offset.store_type(), type_offset.offset_end());
  curr_global_offset_ += num_features;
}

Feature Schema::GetFeature(const std::string& family_name,
    BigInt family_idx) const {
  return GetFamily(family_name).GetFeature(family_idx);
}

const DatumProtoStoreOffset& Schema::GetAppendOffset() const {
  return append_store_offset_;
}

Feature Schema::GetFeature(const FeatureFinder& finder) const {
  const auto& family = GetFamily(finder.family_name);
  return finder.feature_name.empty() ? family.GetFeature(finder.family_idx) :
    family.GetFeature(finder.feature_name);
}

const FeatureFamilyIf& Schema::GetFamily(const std::string& family_name) const {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    throw FamilyNotFoundException(family_name);
  }
  return *(it->second);
}

FeatureFamilyIf& Schema::GetMutableFamily(const std::string& family_name) {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    throw FamilyNotFoundException(family_name);
  }
  return *(it->second);
}

FeatureFamilyIf& Schema::GetOrCreateFamily(
    const std::string& family_name, bool simple_family,
    FeatureStoreType store_type, BigInt num_features) {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    if (simple_family) {
      families_[family_name] = make_unique<SimpleFeatureFamily>(
              family_name, append_store_offset_, store_type,
              curr_global_offset_);
    } else {
      CHECK_EQ(0, num_features)
        << "num_features must be 0 for non-simple family";
      families_[family_name] = make_unique<FeatureFamily>(family_name,
              append_store_offset_, curr_global_offset_);
    }
    if (store_type == FeatureStoreType::OUTPUT) {
      output_families_.push_back(family_name);
    }
  }
  it = families_.find(family_name);
  curr_global_offset_ += num_features;  // no-op if num_features == 0 (default)
  auto curr_offset = append_store_offset_.offsets(store_type);
  append_store_offset_.set_offsets(store_type, curr_offset + num_features);
  return *it->second;
}

const std::map<std::string, std::unique_ptr<FeatureFamilyIf>>&
Schema::GetFamilies() const {
  return families_;
}

// Comment(wdai): Do not use featuers_->size() which includes internal
// feature family.
BigInt Schema::GetNumFeatures() const {
  BigInt num_features = 0;
  for (const auto& p : families_) {
    num_features += p.second->GetNumFeatures();
  }
  return num_features;
}

void Schema::UpdateStoreOffset(Feature* new_feature, BigInt store_offset) {
  FeatureStoreType store_type = new_feature->store_type();
  if (store_offset == -1) {
    store_offset = append_store_offset_.offsets(store_type);
  }
  new_feature->set_store_offset(store_offset);
  append_store_offset_.set_offsets(store_type, store_offset + 1);
}

OSchemaProto Schema::GetOSchemaProto() const {
  OSchemaProto proto;
  //proto.mutable_feature_names()->Reserve(output_feature_dim);
  proto.mutable_family_names()->Reserve(output_families_.size());
  proto.mutable_family_offsets()->Resize(output_families_.size(), 0);
  proto.mutable_feature_name_offsets()->Resize(output_families_.size(), 0);

  BigInt output_dim = 0;
  for (int i = 0; i < output_families_.size(); ++i) {
    const FeatureFamilyIf& family = GetFamily(output_families_[i]);
    proto.add_family_names(family.GetFamilyName());
    proto.set_feature_name_offsets(i, proto.feature_names_size());
    if (family.IsSimple()) {
      proto.add_is_simple_family(true);
      StoreTypeAndOffset offset =
        dynamic_cast<const SimpleFeatureFamily&>(family).GetStoreTypeAndOffset();
      proto.set_family_offsets(i, offset.offset_begin());
      output_dim += offset.offset_end() - offset.offset_begin();
      continue;
    }
    const auto& feature_seq =
      dynamic_cast<const FeatureFamily&>(family).GetFeatures();
    CHECK_GT(feature_seq.GetNumFeatures(), 0);
    output_dim += feature_seq.GetNumFeatures();

    // We assume output feature family's features are added in ascending
    // order, so the offset of first feature is the family offset.
    BigInt family_offset = feature_seq.GetFeature(0).store_offset();
    proto.add_is_simple_family(false);
    proto.set_family_offsets(i, family_offset);
    for (int j = 0; j < feature_seq.GetNumFeatures(); ++j) {
      const auto& f = feature_seq.GetFeature(j);
      proto.add_feature_names(f.name());
    }
  }
  proto.set_output_dim(output_dim);
  return proto;
}

SchemaConfig Schema::GetConfig() const {
  return config_;
}

size_t Schema::Commit(RocksDB* db) const {
  // proto contains everything in SchemaProto except features.
  SchemaProto proto;
  auto families = proto.mutable_families();
  for (const auto& p : families_) {
    (*families)[p.first] = p.second->GetProto();
  }
  (*families)[kInternalFamily] = internal_family_.GetProto();
  *(proto.mutable_append_store_offset()) = append_store_offset_;
  proto.mutable_output_families()->Reserve(output_families_.size());
  for (int i = 0; i < output_families_.size(); ++i) {
    *(proto.add_output_families()) = output_families_[i];
  }
  std::string schema_proto_str = StreamSerialize(proto);
  size_t total_size = schema_proto_str.size();
  db->Put(kSchemaPrefix, schema_proto_str);
  LOG(INFO) << "Schema commited. Total size: "
    << SizeToReadableString(total_size);
  return total_size;
}

}  // namespace hotbox
