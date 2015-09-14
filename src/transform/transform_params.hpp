#pragma once

#include <vector>

namespace mldb {

// TODO(wdai): Add stat.
class TransformParams {
public:
  // schema must outlive TransformParams, which does not take ownership of
  // schema.
  //
  // TODO(wdai): consider using shared_ptr.
  TransformParams(Schema* schema) : schema_(schema) { }

  // Get all the selected input features.
  std::vector<Feature> GetInputFeatures() const {
    auto finders = GetInputFeatureFinders();
    std::vector<Feature> input_features;
    for (const auto& finder : finders) {
      if (finder.all_family) {
        // Family-wide selection.
        const auto& input_family = schema_->GetFamily(finder.family_name);
        const auto& features = input_family.GetFeatures();
        input_features.insert(input_features.end(), features.begin(),
            features.end());
      } else {
        input_features.push_back(schema_->GetFeature(finder));
      }
    }
    return input_features;
  }

  // FeatureFinder is the parsed results from input_features_str_, before
  // family-wide selection gets populated with all features in a family.
  const std::vector<FeatureFinder> GetInputFeatureFinders() const {
    std::vector<FeatureFinder> finders;
    for (const auto& str : input_features_str_) {
      const auto f = ParseFeatureDesc(str);
      finders.insert(finders.end(), f.begin(), f.end());
    }
    return finders;
  }

  void SetConfig(const TransformConfig& config) {
    config_ = config;
  }

  const TransformConfig& GetConfig() const {
    return config_;
  }

private:
  Schema* schema_;
  TransformConfig config_;
  std::vector<std::string> input_features_str_;
};

}  // namespace mldb
