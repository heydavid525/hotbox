#pragma once

#include <vector>
#include "schema/all.hpp"
#include "transform/proto/transform.pb.h"
#include <glog/logging.h>
#include <string>

namespace mldb {

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
    LOG(INFO) << "feature_str.size(): " << feature_str.size();
    PrepareInputFeatures(schema, input_features_str);
  }

  TransformParam(const TransformParamProto& proto) :
    config_(proto.config()) {
      for (int i = 0; i < proto.input_features_size(); ++i) {
        input_features_.push_back(proto.input_features(i));
        input_features_desc_.push_back(proto.input_features_desc(i));
      }
    }

  // Get all the selected input features.
  const std::vector<Feature>& GetInputFeatures() const {
    return input_features_;
  }

  const std::vector<std::string>& GetInputFeaturesDesc() const {
    return input_features_desc_;
  }

  const TransformConfig& GetConfig() const {
    return config_;
  }

  TransformParamProto GetProto() const {
    TransformParamProto proto;
    *(proto.mutable_config()) = config_;
    proto.mutable_input_features()->Reserve(input_features_.size());
    proto.mutable_input_features_desc()->Reserve(input_features_desc_.size());
    for (int i = 0; i < input_features_.size(); ++i) {
      *(proto.add_input_features()) = input_features_[i];
      *(proto.add_input_features_desc()) = input_features_desc_[i];
    }
    return proto;
  }

private:
  // Populate input_features_.
  void PrepareInputFeatures(const Schema& schema,
      const std::vector<std::string>& input_features_str) {
    auto finders = GetInputFeatureFinders(input_features_str);
    for (const auto& finder : finders) {
      if (finder.all_family) {
        // Family-wide selection.
        const auto& input_family = schema.GetFamily(finder.family_name);
        const auto& features = input_family.GetFeatures();
        input_features_.insert(input_features_.end(), features.begin(),
            features.end());
        for (int i = 0; i < features.size(); ++i) {
          input_features_desc_.push_back(input_family.GetFamilyName() + ":"
              + std::to_string(i));
        }
      } else {
        input_features_.push_back(schema.GetFeature(finder));
        input_features_desc_.push_back(finder.family_name + ":" +
            (finder.feature_name.empty() ? std::to_string(finder.family_idx)
            : finder.feature_name));
      }
    }
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
  std::vector<Feature> input_features_;

  // family:feature_name or family:idx depending on how user specifies it. For
  // family-wide selection it's always family:idx.
  std::vector<std::string> input_features_desc_;
};

}  // namespace mldb
