#pragma once

#include "util/mldb_exceptions.hpp"
#include "schema/proto/schema.pb.h"
#include "schema/feature_family.hpp"
#include <cstdint>
#include <string>
#include <map>

namespace mldb {

// Since protobuf msg isn't easy to work with, Schema is a wrapper around
// SchemaProto.
class Schema {
public:
  Schema(const SchemaConfig& config);

  Schema(const SchemaProto& proto);

  // new_feature needs to have loc().store_type and type set, but not offset. 
  // 'new_feature' will have offset set correctly after the call.
  //
  // TODO(wdai): Currently this is the only means to add feature and involves
  // map lookup. Consider optimizing this.
  void AddFeature(const std::string& family_name, int32_t family_idx,
      Feature* new_feature);

  const Feature& GetFeature(const std::string& family_name, int32_t family_idx)
    const;

  Feature& GetMutableFeature(const std::string& family_name,
      int32_t family_idx);

  const Feature& GetFeature(const FeatureFinder& finder) const;

  Feature& GetMutableFeature(const FeatureFinder& finder);

  // Can throws FamilyNotFoundException.
  const FeatureFamily& GetFamily(const std::string& family_name) const;

  // Try to get a family. If it doesn't exist, create it.
  FeatureFamily& GetOrCreateFamily(const std::string& family_name) const;

  const DatumProtoOffset& GetDatumProtoOffset() const;

  // Not including kInternalFamily.
  const std::map<std::string, FeatureFamily>& GetFamilies() const;

  // Get # of features (not including label/weight) in this schema.
  int GetNumFeatures() const;

  SchemaProto GetProto() const;

  std::string Serialize() const;

private:
  // Increment the appropriate append_offset_ and assign the offset to
  // new_feature.
  void UpdateOffset(Feature* new_feature);

private:
  // Comment(wdai): declare it mutable so we can create FeatureFamily in const
  // functions.
  mutable std::map<std::string, FeatureFamily> families_;

  // Internal family is treated specially.
  mutable FeatureFamily internal_family_;

  // Tracks the insert point of the next feature.
  DatumProtoOffset append_offset_;

  // TODO(wdai): Add inverse lookup  from feature to family. Needed for
  // feature deletion.
  // std::vector<std::map<std::string, FeatureFamily>::const_iterator>
  // inverse_lookup;
};

}  // namespace mldb
