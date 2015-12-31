#include <glog/logging.h>
#include <cstdint>
#include <sstream>
#include "util/all.hpp"
#include "schema/schema.hpp"
#include "schema/constants.hpp"
#include "schema/schema_util.hpp"
#include <google/protobuf/text_format.h>
#include <fstream>

namespace hotbox {

namespace {

const std::string kSchemaPrefix = "schema_";

std::string MakeSegmentKey(int seg_id) {
  return kSchemaPrefix + "seg_" + std::to_string(seg_id);
}

}  // anonymous namespace

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
  Init(proto);
}

void Schema::Init(const SchemaProto& proto) {
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

Schema::Schema(RocksDB* db) {
  std::string schema_proto_str = db->Get(kSchemaPrefix);
  SchemaProto proto = StreamDeserialize<SchemaProto>(schema_proto_str);

  // Need to initialize features_ before families_. Fill in features_ from
  // FeatureSegment.
  features_.reset(new std::vector<Feature>(proto.num_features()));
  for (int i = 0; i < proto.num_segments(); ++i) {
    std::string seg_key = MakeSegmentKey(i);
    std::string seg_proto_str = db->Get(seg_key);
    FeatureSegment seg_proto =
      StreamDeserialize<FeatureSegment>(seg_proto_str);
    BigInt id_begin = seg_proto.id_begin();
    for (int j = 0; j < seg_proto.features_size(); ++j) {
      (*features_)[j + id_begin] = seg_proto.features(j);
    }
  }
  LOG(INFO) << "Schema reads " << proto.num_segments() << " segments";

  // Init() after features_ is initialized.
  Init(proto);
}

void Schema::AddFeature(const std::string& family_name,
    Feature* new_feature, BigInt family_idx) {
  auto& family = GetMutableFamily(family_name);
  AddFeature(&family, new_feature, family_idx);
}

void Schema::AddFeature(FeatureFamily* family, Feature* new_feature,
    BigInt family_idx) {
  if (family->IsSimple()) {
    BigInt family_offset_begin = family->GetOffsetBegin().offsets(
        new_feature->store_type());
    UpdateStoreOffset(new_feature, family_offset_begin + family_idx);
  } else {
    UpdateStoreOffset(new_feature);
  }
  new_feature->set_global_offset(features_->size());
  //LOG(INFO) << "Add feature. family: " << family->GetFamilyName() <<
  //  " store_offset: " << new_feature->store_offset() << " global offset: "
  //  << new_feature->global_offset();
  features_->emplace_back(*new_feature);
  family->AddFeature(*new_feature, family_idx);
}

const Feature& Schema::GetFeature(const std::string& family_name,
    BigInt family_idx) const {
  return GetFamily(family_name).GetFeature(family_idx);
}

const DatumProtoStoreOffset& Schema::GetAppendOffset() const {
  return append_store_offset_;
}

Feature& Schema::GetMutableFeature(const std::string& family_name,
    BigInt family_idx) {
  return GetMutableFamily(family_name).GetMutableFeature(family_idx);
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

FeatureFamily& Schema::GetMutableFamily(const std::string& family_name) {
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
    bool output_family, bool simple_family) const {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    auto inserted = families_.emplace(
        std::make_pair(family_name, FeatureFamily(family_name, features_,
            simple_family)));
    it = inserted.first;
    it->second.SetOffset(append_store_offset_);
    if (output_family) {
      output_families_.push_back(family_name);
    }
  }
  return it->second;
}

FeatureFamily& Schema::GetOrCreateMutableFamily(
    const std::string& family_name, bool output_family, bool simple_family) {
  if (family_name == kInternalFamily) {
    return internal_family_;
  }
  auto it = families_.find(family_name);
  if (it == families_.cend()) {
    // LOG(INFO) << "Insert family " << family_name << " to families_";
    auto inserted = families_.emplace(
        std::make_pair(family_name, FeatureFamily(family_name, features_,
            simple_family)));
    it = inserted.first;
    it->second.SetOffset(append_store_offset_);
    if (output_family) {
      // LOG(INFO) << "Adding " << family_name << " to output_families_";
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
    const FeatureFamily& family = GetFamily(output_families_[i]);
    proto.add_family_names(family.GetFamilyName());
    proto.set_feature_name_offsets(i, proto.feature_names_size());
    if (family.IsSimple()) {
      proto.add_is_simple_family(true);
      StoreTypeAndOffset offset = family.GetStoreTypeAndOffset();
      proto.set_family_offsets(i, offset.offset_begin());
      output_dim += offset.offset_end() - offset.offset_begin();
      continue;
    }
    const auto& feature_seq = family.GetFeatures();
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

/*
SchemaProto Schema::GetProto(bool with_features) const {
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
  if (with_features) {
    proto.mutable_features()->Reserve(features_->size());
    for (BigInt i = 0; i < features_->size(); ++i) {
      *(proto.add_features()) = (*features_)[i];
    }
  }
  return proto;
}

std::string Schema::Serialize(bool with_features) const {
  auto proto = GetProto(with_features);
  return SerializeProto(proto);
}
*/

size_t Schema::Commit(RocksDB* db) const {
  // proto contains everything in SchemaProto except features.
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
  int num_segments = std::ceil(static_cast<float>(features_->size())
      / kSeqBatchSize);
  proto.set_num_segments(num_segments);
  proto.set_num_features(features_->size());
  std::string schema_proto_str = StreamSerialize(proto);
  size_t total_size = schema_proto_str.size();
  db->Put(kSchemaPrefix, schema_proto_str);
  /////
  //LOG(INFO) << "Verifying schema StreamSerialize";
  //SchemaProto proto2 = StreamDeserialize<SchemaProto>(schema_proto_str);

  // Chop repeated features to FeatureSegment.
  for (int i = 0; i < num_segments; ++i) {
    BigInt id_begin = kSeqBatchSize * i;
    BigInt id_end = std::min(id_begin + kSeqBatchSize,
        static_cast<BigInt>(features_->size()));
    FeatureSegment seg;
    seg.set_id_begin(id_begin);
    seg.mutable_features()->Reserve(id_end - id_begin);
    for (BigInt j = id_begin; j < id_end; ++j) {
      *(seg.add_features()) = (*features_)[j];
    }
    // LOG(INFO) << "seg has num features: " << seg.features_size();
    std::string output_str;
    std::string seg_key = MakeSegmentKey(i);
    std::string seg_proto_str = StreamSerialize(seg);

    // TODO(wdai): Figure out why StreamDeserialize and StreamDeserialize
    // doesn't work.
    //std::string seg_proto_str = StreamSerialize(seg);
    //FeatureSegment seg2 = StreamDeserialize<FeatureSegment>(seg_proto_str);
    // FeatureSegment seg2 = DeserializeProto<FeatureSegment>(seg_proto_str);
    total_size += seg_proto_str.size();
    db->Put(seg_key, seg_proto_str);
    /*
    LOG(INFO) << "Commit: writing feature key: " << seg_key
      << " feature range: ["
      << id_begin << ", " << id_end << ") proto size: "
      << SizeToReadableString(seg_proto_str.size());
      */
  }
  LOG(INFO) << "Schema commit " << num_segments << " segments. Total size: "
    << SizeToReadableString(total_size);
  return total_size;
}

}  // namespace hotbox
