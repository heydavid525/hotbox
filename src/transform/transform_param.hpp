#pragma once

#include <vector>
#include "schema/all.hpp"
#include "transform/proto/transform.pb.h"
#include <glog/logging.h>
#include <string>
#include <map>

namespace hotbox {

// TODO(wdai): Add stat.
class TransformParam {
public:
  // schema must outlive TransformParam, which does not take ownership of
  // schema.
  //
  // TODO(wdai): consider using shared_ptr.
  TransformParam(const Schema& schema,
      const TransformConfig& config) : config_(config) {
    const auto& feature_str = config_.base_config().input_features();
    std::vector<std::string> input_features_str(feature_str.begin(),
        feature_str.end());
    PrepareInputFeatures(schema, input_features_str);
  }

  TransformParam(const TransformParamProto& proto) :
    config_(proto.config()),
    wide_family_offsets_(proto.wide_family_offsets().cbegin(),
        proto.wide_family_offsets().cend()) {
      for (auto it = proto.input_families().cbegin();
          it != proto.input_families().cend(); ++it) {
        const auto& input_features = it->second.input_features();
        input_features_[it->first] = std::vector<Feature>(
            input_features.cbegin(), input_features.cend());

        const auto& input_features_desc = it->second.input_features_desc();
        //input_features_desc_[it->first] = std::vector<std::string>(
        //    input_features_desc.cbegin(), input_features_desc.cend());
      }
    }

  // Return families that are selected by family-wide selection and uses
  // single feature store. Only these families are optimized for store-direct
  // selection.
  std::vector<std::string> GetFamilyWideFamilies() const {
    std::vector<std::string> families;
    for (const auto& p : wide_family_offsets_) {
      families.push_back(p.first);
    }
    return families;
  }

  // Get each family-wide family's store offsets (begin & end).
  const std::map<std::string, StoreTypeAndOffset>&
    GetFamilyWideStoreOffsets() const {
      return wide_family_offsets_;
    }

  // Get all the selected input features.
  std::vector<Feature> GetInputFeatures() const {
    std::vector<Feature> features;
    for (const auto& f : input_features_) {
      features.insert(features.end(), f.second.cbegin(), f.second.cend());
    }
    return features;
  }

  const std::map<std::string, std::vector<Feature>>&
    GetInputFeaturesByFamily() const {
      return input_features_;
  }

  const std::map<std::string, std::vector<std::string>>&
    GetInputFeaturesDescByFamily() const {
      return input_features_desc_;
  }

  std::vector<std::string> GetInputFeaturesDesc() const {
    std::vector<std::string> desc;
    for (const auto& d : input_features_desc_) {
      desc.insert(desc.end(), d.second.cbegin(), d.second.cend());
    }
    return desc;
  }

  const TransformConfig& GetConfig() const {
    return config_;
  }

  TransformParamProto GetProto() const {
    TransformParamProto proto;
    *(proto.mutable_config()) = config_;

    // Instantiate TransformParamProto::input_families.
    for (const auto& p : input_features_) {
      TransformFamilyFeatureProto family_feature;
      const std::vector<Feature>& features = p.second;
      const std::vector<std::string>& features_desc =
        input_features_desc_.at(p.first);
      family_feature.mutable_input_features()->Reserve(features.size());
      family_feature.mutable_input_features_desc()->Reserve(features.size());
      for (int i = 0; i < features.size(); ++i) {
        (*family_feature.add_input_features()) = features[i];
        (*family_feature.add_input_features_desc()) = features_desc[i];
      }
      (*proto.mutable_input_families())[p.first] = family_feature;
    }

    // Instantiate TransformParamProto::wide_family_offsets
    for (const auto& p : wide_family_offsets_) {
      (*proto.mutable_wide_family_offsets())[p.first] = p.second;
    }
    /*
    proto.mutable_input_features()->Reserve(input_features_.size());
    proto.mutable_input_features_desc()->Reserve(input_features_desc_.size());
    for (int i = 0; i < input_features_.size(); ++i) {
      *(proto.add_input_features()) = input_features_[i];
      *(proto.add_input_features_desc()) = input_features_desc_[i];
    }
    */
    return proto;
  }

private:
  // Populate input_features_.
  void PrepareInputFeatures(const Schema& schema,
      const std::vector<std::string>& input_features_str) {
    auto finders = GetInputFeatureFinders(input_features_str);
    for (const auto& finder : finders) {
      // wildcard to select all features in all-family.
      if (finder.family_name == "*") {
        CHECK(finder.all_family) << "Must select all family and all features";
        const std::map<std::string, FeatureFamily>& families =
          schema.GetFamilies();
        for (const auto& p : families) {
          FamilyWideSelection(schema, p.first);
        }
      } else {
        if (finder.all_family) {
          FamilyWideSelection(schema, finder.family_name);
        } else {
          input_features_[finder.family_name].push_back(
              schema.GetFeature(finder));
          input_features_desc_[finder.family_name].push_back(
              finder.family_name + ":" +
              (finder.feature_name.empty() ? std::to_string(finder.family_idx)
              : finder.feature_name));
        }
      }
    }
  }

  // Select all features in family 'family_name', which has to be simple
  // family.
  void FamilyWideSelection(const Schema& schema,
      const std::string& family_name) {
    const auto& input_family = schema.GetFamily(family_name);
    CHECK(input_family.IsSimple());
    // Get the family offsets only for family-wide selection.
    wide_family_offsets_[family_name] =
      input_family.GetStoreTypeAndOffset();
    // Just give empty vector for input_features_desc_ and input_features_.
    input_features_[family_name] = std::vector<Feature>();
    input_features_desc_[family_name] = std::vector<std::string>();
    //  return;
    /*
    const FeatureSeq& feature_seq = input_family.GetFeatures();
    std::vector<Feature> family_features(feature_seq.GetNumFeatures());
    for (int i = 0; i < feature_seq.GetNumFeatures(); ++i) {
      family_features[i] = feature_seq.GetFeature(i);
    }
    input_features_[family_name] = family_features;
    input_features_.append(family_features);

    std::vector<std::string> family_desc(feature_seq.GetNumFeatures());
    for (int i = 0; i < feature_seq.GetNumFeatures(); ++i) {
      family_desc.push_back(input_family.GetFamilyName() + ":"
          + std::to_string(i));
    }
    input_features_desc_[family_name] = family_desc;
    */
  }

  // FeatureFinder is the parsed results from input_features_str_, before
  // family-wide selection gets populated with all features in a family.
  static const std::vector<FeatureFinder> GetInputFeatureFinders(
      const std::vector<std::string>& input_features_str) {
    std::vector<FeatureFinder> finders;
    for (const auto& str : input_features_str) {
      const auto f = ParseFeatureDesc(str);
      finders.insert(finders.end(), f.begin(), f.end());
    }
    return finders;
  }

private:
  TransformConfig config_;

  // input_features_ only include features that are not part of family-wide
  // selection. For simple family-wide selection we elide the feature.
  std::map<std::string, std::vector<Feature>> input_features_;

  // family:feature_name or family:idx depending on how user specifies it. For
  // simple family-wide selection it's empty vector (we elide feature name for
  // efficiency).
  std::map<std::string, std::vector<std::string>> input_features_desc_;

  // Each wide-family using single store will be in this map.
  std::map<std::string, StoreTypeAndOffset> wide_family_offsets_;
};

}  // namespace hotbox
