#pragma once

#include "util/hotbox_exceptions.hpp"
#include "schema/proto/schema.pb.h"
#include "schema/feature_family.hpp"
#include <cstdint>
#include <string>
#include <map>

namespace hotbox {

// Since protobuf msg isn't easy to work with, Schema is a wrapper around
// SchemaProto.
// Comment(wdai): We let each FeatureFamily to each store a vector of Feature
// instead of storing one vector<Feature> in Schema to reduce coupling

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

  Feature& GetMutableFeature(const Feature& feature);

  // Throws FamilyNotFoundException.
  const FeatureFamily& GetFamily(const std::string& family_name) const;
  FeatureFamily& GetMutableFamily(const std::string& family_name);

  // Try to get a family. If it doesn't exist, create it. 'output_family' ==
  // true if the family is stored in FeatureStoreType::OUTPUT. This helps
  // generate a OSchema to send to client.
  const FeatureFamily& GetOrCreateFamily(const std::string& family_name,
      bool output_family = false, bool simple_family = false) const;
  FeatureFamily& GetOrCreateMutableFamily(const std::string& family_name,
      bool output_family = false, bool simple_family = false);

  // Return append_store_offset_.
  const DatumProtoStoreOffset& GetAppendOffset() const;

  // Not including kInternalFamily.
  const std::map<std::string, FeatureFamily>& GetFamilies() const;

  // Get # of features (not including label/weight or anything in the
  // kInternalFamily) in this schema.
  BigInt GetNumFeatures() const;

  // Generate OSchema (output schema) based on just output_families_.
  OSchemaProto GetOSchemaProto() const;

  SchemaConfig GetConfig() const;

  SchemaProto GetProto() const;

  std::string Serialize() const;

private:
  // if store_offset == -1 (default), increment the appropriate
  // append_store_offset_ and assign the offset to new_feature. Otherwise set
  // new_feature->store_offset and append_store_offset_ according to
  // store_offset.
  void UpdateStoreOffset(Feature* new_feature, BigInt store_offset = -1);

private:
  // Comment(wdai): It's mutable so we can add family while
  // accessing Schema object as const.
  mutable std::map<std::string, FeatureFamily> families_;

  // Keep an ordered list of output families to construct OSchema to
  // send to client.
  // Comment(wdai): It's mutable for the same reason as families_.
  mutable std::vector<std::string> output_families_;

  // All feature are stored in features_. FeatureFamily maintains the
  // global_index to find feature from here.
  std::shared_ptr<std::vector<Feature>> features_;

  // Internal family stores label, weight, and is treated specially so that
  // when returning families_ for DatumBase to iterate over we don't show
  // internal_family_.
  FeatureFamily internal_family_;

  // Tracks the insert store_offset for the next feature.
  DatumProtoStoreOffset append_store_offset_;

  SchemaConfig config_;
};

}  // namespace hotbox
