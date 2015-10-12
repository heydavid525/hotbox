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

  // new_feature needs to have FeatureStoreType set, but not offset.
  // 'new_feature' will have offset set correctly after the call. This call
  // invokes map-lookup for family based on family_name. Use another
  // AddFeature to avoid map-lookup. 'family_idx' = -1 to append feature at
  // the end of the family.
  void AddFeature(const std::string& family_name, Feature* new_feature,
      BigInt family_idx = -1);

  // Similar to AddFeature() above, but avoids map lookup for FeatureFamily.
  // 'family' must be a family obtained from this Schema.
  void AddFeature(FeatureFamily* family, Feature* new_feature,
      BigInt family_idx = -1);

  const Feature& GetFeature(const std::string& family_name, BigInt family_idx)
    const;
  Feature& GetMutableFeature(const std::string& family_name,
      BigInt family_idx);

  const Feature& GetFeature(const FeatureFinder& finder) const;
  Feature& GetMutableFeature(const FeatureFinder& finder);

  // Can throws FamilyNotFoundException.
  const FeatureFamily& GetFamily(const std::string& family_name) const;

  // Try to get a family. If it doesn't exist, create it. 'output_family' ==
  // true if the family is stored in FeatureStoreType::OUTPUT. This helps
  // generate a OSchema to send to client.
  const FeatureFamily& GetOrCreateFamily(const std::string& family_name,
      bool output_family = false) const;
  FeatureFamily& GetOrCreateMutableFamily(const std::string& family_name,
      bool output_family = false);

  // Return append_offset_.
  const DatumProtoOffset& GetAppendOffset() const;

  // Not including kInternalFamily.
  const std::map<std::string, FeatureFamily>& GetFamilies() const;

  // Get # of features (not including label/weight or anything in the
  // kInternalFamily) in this schema.
  BigInt GetNumFeatures() const;

  // Generate OSchema (output schema) based on just output_families_.
  OSchemaProto GetOSchemaProto() const;

  SchemaProto GetProto() const;

  std::string Serialize() const;

private:
  // Increment the appropriate append_offset_ and assign the offset to
  // new_feature.
  void UpdateOffset(Feature* new_feature);

private:
  // Comment(wdai): Needs to make it mutable so we can add family while
  // accessing Schema object as const.
  mutable std::map<std::string, FeatureFamily> families_;

  // Keep an ordered list of output families to construct OSchema to
  // send to client.
  // Comment(wdai): Make it mutable for the same reason as families_.
  mutable std::vector<std::string> output_families_;

  // Internal family stores label, weight, and is treated specially so that
  // when returning families_ for DatumBase to iterate over we don't show
  // internal_family_.
  FeatureFamily internal_family_;

  // Tracks the insert point of the next feature.
  DatumProtoOffset append_offset_;
};

}  // namespace mldb
