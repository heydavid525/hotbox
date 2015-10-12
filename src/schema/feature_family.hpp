#pragma once

#include "util/hotbox_exceptions.hpp"
#include "schema/proto/schema.pb.h"
#include "schema/constants.hpp"
#include <string>
#include <map>

namespace hotbox {

class Schema;

class FeatureFamily {
public:
  FeatureFamily(const std::string& family_name);

  FeatureFamily(const FeatureFamilyProto& proto);

  bool HasFeature(int64_t family_idx) const;

  void DeleteFeature(int64_t family_idx);

  const Feature& GetFeature(const std::string& feature_name) const;
  Feature& GetMutableFeature(const std::string& feature_name);

  const Feature& GetFeature(int64_t family_idx) const;
  Feature& GetMutableFeature(int64_t family_idx);

  // Return both initialized and uninitialized features.
  const std::vector<Feature>& GetFeatures() const;

  // Only count the initialized features.
  BigInt GetNumFeatures() const;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures iff
  // there are uninitialized features.
  BigInt GetMaxFeatureId() const;

  FeatureFamilyProto GetProto() const;

  std::string GetFamilyName() const;

private:
  // Allow Schema to access AddFeature.
  //friend void Schema::AddFeature(const std::string& family_name,
  //    const Feature& new_feature, int64_t family_idx);
  friend class Schema;

  // Add new feature as the family_idx-th feature in the family. family_idx =
  // -1 to append as last feature.
  void AddFeature(const Feature& new_feature, int64_t family_idx = -1);

  // Throws FeatureNotFoundException if not found.
  void CheckFeatureExist(BigInt family_idx) const;

private:
  std::string family_name_;

  // feature name --> index on Schema::features_.
  std::map<std::string, int64_t> name_to_family_idx_;

  std::vector<Feature> features_;
};

}  // namespace hotbox
