#pragma once

#include <vector>
#include "schema/all.hpp"
#include "transform/proto/transform.pb.h"
#include <glog/logging.h>
#include <string>
#include <map>
#include <utility>

namespace hotbox {

struct WideFamilySelector {
  StoreTypeAndOffset offset;
  RangeSelector range_selector;
};

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
    config_(proto.config()) {
      for (auto it = proto.wide_family_selectors().cbegin();
          it != proto.wide_family_selectors().cend(); ++it) {
        WideFamilySelector w_selector;
        w_selector.offset = it->offset();
        w_selector.range_selector.family_idx_begin = it->family_idx_begin();
        w_selector.range_selector.family_idx_end = it->family_idx_end();
        wide_family_selectors_.insert(
            std::make_pair(it->family_name(), w_selector));
      }
      for (auto it = proto.input_families().cbegin();
          it != proto.input_families().cend(); ++it) {
        const auto& input_features = it->second.input_features();
        ns_input_features_[it->first] = std::vector<Feature>(
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
    for (const auto& p : wide_family_selectors_) {
      families.push_back(p.first);
    }
    return families;
  }

  // Get each family-wide family's store offsets (begin & end).
  const std::multimap<std::string, WideFamilySelector>&
    GetWideFamilySelectors() const {
      return wide_family_selectors_;
    }

  // Get all the selected input features.
  std::vector<Feature> GetInputFeatures() const {
    std::vector<Feature> features;
    for (const auto& f : ns_input_features_) {
      features.insert(features.end(), f.second.cbegin(), f.second.cend());
    }
    return features;
  }

  // Return family --> [feature1, feature2,...] including only
  // non-family-wide features.
  const std::map<std::string, std::vector<Feature>>&
    GetInputFeaturesByFamily() const {
      return ns_input_features_;
  }

  //const std::map<std::string, std::vector<std::string>>&
  //  GetInputFeaturesDescByFamily() const {
  //    return input_features_desc_;
  //}

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
    for (const auto& p : ns_input_features_) {
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

    // Instantiate TransformParamProto::wide_family_selectors
    for (const auto& p : wide_family_selectors_) {
      auto selector = proto.add_wide_family_selectors();
      selector->set_family_name(p.first);
      *(selector->mutable_offset()) = p.second.offset;
      selector->set_family_idx_begin(p.second.range_selector.family_idx_begin);
      selector->set_family_idx_end(p.second.range_selector.family_idx_end);
    }
    /*
    proto.mutable_input_features()->Reserve(ns_input_features_.size());
    proto.mutable_input_features_desc()->Reserve(input_features_desc_.size());
    for (int i = 0; i < ns_input_features_.size(); ++i) {
      *(proto.add_input_features()) = ns_input_features_[i];
      *(proto.add_input_features_desc()) = input_features_desc_[i];
    }
    */
    return proto;
  }

private:
  void PrepareInputFeatures(const Schema& schema,
      const std::vector<std::string>& input_features_str) {
    auto finders = GetInputFeatureFinders(input_features_str);
    for (const auto& finder : finders) {
      // wildcard to select all features in all-family.
      if (finder.family_name == "*") {
        CHECK_EQ(kRangeSelect, finder.mode)
          << "Must select all family and all features";
        CHECK_EQ(finder.range_selector.family_idx_begin,
            finder.range_selector.family_idx_end)
          << "Must select all family and all features";
        const std::map<std::string,
              std::unique_ptr<FeatureFamilyIf>>& families =
          schema.GetFamilies();
        for (const auto& p : families) {
          FamilyWideSelection(schema, p.first, finder.range_selector);
        }
      } else {
        if (finder.mode == kRangeSelect) {
          FamilyWideSelection(schema, finder.family_name,
              finder.range_selector);
        } else {
          ns_input_features_[finder.family_name].push_back(
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
      const std::string& family_name, const RangeSelector& selector) {
    const auto& input_family = schema.GetFamily(family_name);
    CHECK(input_family.IsSimple());
    // Get the family offsets only for family-wide selection.
    WideFamilySelector w_selector;
    w_selector.offset = dynamic_cast<const SimpleFeatureFamily&>(
            input_family).GetStoreTypeAndOffset();
    w_selector.range_selector = selector;
    wide_family_selectors_.insert(std::make_pair(family_name, w_selector));
    // Just give empty vector for input_features_desc_ and ns_input_features_.
    ns_input_features_[family_name] = std::vector<Feature>();
    input_features_desc_[family_name] = std::vector<std::string>();
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

  // ns_input_features_ (non-simple input_features) only include features that
  // are not part of family-wide selection. For simple family-wide selection we
  // elide the feature.
  std::map<std::string, std::vector<Feature>> ns_input_features_;

  // family:feature_name or family:idx depending on how user specifies it. For
  // simple family-wide selection it's empty vector (we elide feature name for
  // efficiency).
  std::map<std::string, std::vector<std::string>> input_features_desc_;

  // Each wide-family using single store will be in this map.
  std::multimap<std::string, WideFamilySelector> wide_family_selectors_;
};

}  // namespace hotbox
