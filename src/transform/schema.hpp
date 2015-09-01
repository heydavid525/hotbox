#pragma once

#include "transform/proto/schema.pb.h"
#include "transform/schema_util.hpp"
#include "transform/constants.hpp"
#include <glog/logging.h>
#include <cstdint>
#include <string>
#include <sstream>
#include <map>

namespace mldb {

// FeatureFamily has identical fields as FeatureFamilyProto, but it copies them
// out of proto for easier management (and avoid multiple copying when calling
// GetFeatures()).
class FeatureFamily {
public:
  FeatureFamily() { }

  // FeatureFamily takes the ownership of 'proto'
  FeatureFamily(const FeatureFamilyProto& proto) {
    Deserialize(proto);
  }

  // Add new features with specific family_idxes.
  void AddFeatures(const std::vector<Feature>& new_features,
      const std::vector<int> family_idxes) {
    CHECK_EQ(family_idxes.size(), new_features.size());
    int max_family_idx = new_features.size();
    for (const auto& idx : family_idxes) {
      max_family_idx = std::max(idx, max_family_idx);
    }

    features_.resize(max_family_idx + 1);
    for (int i = 0; i < new_features.size(); ++i) {
      int family_idx = family_idxes[i];
      features_[family_idx] = new_features[i];
      if (!new_features[i].name().empty()) {
        name_to_family_idx_[new_features[i].name()] = family_idx;
      }
    }
  }

  // Add new features. features[i] will have family_idx i.
  void AddFeatures(const std::vector<Feature>& new_features) {
    int new_size = std::max(new_features.size(), new_features.size());
    features_.resize(new_size);
    for (int i = 0; i < new_features.size(); ++i) {
      features_[i] = new_features[i];
      if (!new_features[i].name().empty()) {
        name_to_family_idx_[new_features[i].name()] = i;
      }
    }
  }

  // Get feature from name.
  const Feature& GetFeature(const std::string& name) const {
    const auto& it = name_to_family_idx_.find(name);
    CHECK(it != name_to_family_idx_.end()) << "Feature " << name
      << " not found.";
    return GetFeature(it->second);
  }

  // Get feature from family_idx.
  const Feature& GetFeature(uint32_t family_idx) const {
    CHECK_LT(family_idx, features_.size());
    return features_[family_idx];
  }

  const std::vector<Feature>& GetFeatures() const {
    return features_;
  }

  void Deserialize(const FeatureFamilyProto& proto) {
    name_to_family_idx_ = std::map<std::string, uint32_t>(
        proto.name_to_family_idx().begin(), proto.name_to_family_idx().end());
    int num_features = proto.features_size();
    features_.resize(num_features);
    for (int i = 0; num_features; ++i) {
      features_[i] = proto.features(i);
    }
  }

  FeatureFamilyProto Serialize() const {
    LOG(FATAL) << "Not implemented yet";
  }

private:
  // Some features have names.
  std::map<std::string, uint32_t> name_to_family_idx_;

  // All features.
  std::vector<Feature> features_;
};

// Since protobuf msg isn't easy to work with, Schema is a wrapper around
// SchemaProto.
class Schema {
public:
  Schema(const SchemaConfig& config) {
    // Add internal family.
    std::vector<Feature> intern_features;

    // Add label.
    Feature label;
    label.set_name(kLabelFeatureName);
    FeatureLocator* loc = label.mutable_loc();
    loc->set_type(config.int_label() ? FeatureType::CATEGORICAL :
        FeatureType::NUMERICAL);
    loc->set_store_type(FeatureStoreType::DENSE);
    loc->set_offset(0);
    intern_features.push_back(label);
    DatumProtoOffset offset_inc;
    if (config.int_label()) {
      offset_inc.set_dense_cat_store(1);
    } else {
      offset_inc.set_dense_num_store(1);
    }

    // Add weight
    Feature weight;
    weight.set_name(kWeightFeatureName);
    loc = weight.mutable_loc();
    loc->set_type(FeatureType::NUMERICAL);
    loc->set_store_type(config.use_dense_weight() ? FeatureStoreType::DENSE
        : FeatureStoreType::SPARSE);
    intern_features.push_back(weight);
    if (config.use_dense_weight()) {
      offset_inc.set_dense_num_store(offset_inc.dense_num_store() + 1);
    }
    FeatureFamily intern_family;
    intern_family.AddFeatures(intern_features);

    AddFamily(kInternalFamily, intern_family, offset_inc);
  }

  Schema(const SchemaProto& schema_proto) {
    Deserialize(schema_proto);
  }

  const Feature& GetFeature(const FeatureFinder& finder) {
    const auto& family = GetFamily(finder.family_name);
    return finder.feature_name.empty() ? family.GetFeature(finder.family_idx) :
      family.GetFeature(finder.feature_name);
  }

  // Family is immutable once added to Schema.
  //
  // TODO(wdai): Allow mutable features but not adding new features, which
  // mess up offsets.
  void AddFamily(const std::string& name, const FeatureFamily& family,
      const DatumProtoOffset& offset_inc) {
    families_.insert(std::pair<std::string, const FeatureFamily>(name, family));
    offset_ = offset_ + offset_inc;
  }

  const DatumProtoOffset& GetDatumProtoOffset() const {
    return offset_;
  }

  const std::map<std::string, const FeatureFamily> GetFamilies() const {
    return families_;
  }

  const FeatureFamily& GetFamily(const std::string& family) const {
    auto it = families_.find(family);
    CHECK(it != families_.cend()) << "Family " << family << " not found.";
    return it->second;
  }

  void Deserialize(const SchemaProto& schema_proto) {
    families_ = std::map<std::string, const FeatureFamily>(
        schema_proto.families().begin(), schema_proto.families().end());
    offset_ = schema_proto.offset();
  }

  // Print " |family feature1:cat:sp ..." where 'cat' = categorical, sp =
  // sparse.
  std::string ToString() const {
    std::stringstream ss;
    for (const auto& pair : families_) {
      ss << " |" << pair.first;
      const std::vector<Feature>& features = pair.second.GetFeatures();
      for (int i = 0; i < features.size(); ++i) {
        const auto& feature = features[i];
        std::string feature_name = (feature.name().empty() ?
            std::to_string(i) : feature.name());
        std::string type = IsCategorical(feature) ? "cat" :
            (IsNumerical(feature) ? "num" : "bytes");
        std::string store_type = IsSparse(feature) ? "sp" : "ds";
        ss << " " << feature_name << ":" << type << ":" << store_type;
      }
    }
    return ss.str();
  }

  SchemaProto Serialize() const {
    LOG(FATAL) << "Not implemented yet";
  }

private:
  std::map<std::string, const FeatureFamily> families_;

  DatumProtoOffset offset_;
};

}  // namespace mldb
