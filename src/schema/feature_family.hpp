#pragma once

#include "util/mldb_exceptions.hpp"
#include "schema/proto/schema.pb.h"
#include <string>
#include <map>

namespace mldb {

class Schema;

class FeatureFamily {
public:
  FeatureFamily(const std::string& family_name);

  FeatureFamily(const FeatureFamilyProto& proto);

  bool HasFeature(int32_t family_idx) const;

  void DeleteFeature(int32_t family_idx);

  const Feature& GetFeature(const std::string& feature_name) const;
  Feature& GetMutableFeature(const std::string& feature_name);

  const Feature& GetFeature(int32_t family_idx) const;
  Feature& GetMutableFeature(int32_t family_idx);

  // Return both initialized and uninitialized features.
  const std::vector<Feature>& GetFeatures() const;

  // Only count the initialized features.
  int GetNumFeatures() const;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures iff
  // there are uninitialized features.
  int GetMaxFeatureId() const;

  FeatureFamilyProto GetProto() const;

private:
  // Allow Schema to access AddFeature.
  //friend void Schema::AddFeature(const std::string& family_name,
  //    const Feature& new_feature, int32_t family_idx);
  friend class Schema;

  // new_feature needs to have offset set.
  void AddFeature(const Feature& new_feature, int32_t family_idx);

  // Throws FeatureNotFoundException if not found.
  void CheckFeatureExist(int family_idx) const;

private:
  std::string family_name_;

  // feature name --> index on Schema::features_.
  std::map<std::string, int32_t> name_to_family_idx_;

  std::vector<Feature> features_;
};

}  // namespace mldb
