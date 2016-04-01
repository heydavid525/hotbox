#pragma once

#include "util/all.hpp"
#include "schema/proto/schema.pb.h"
#include "schema/feature_family.hpp"
#include <cstdint>
#include <string>
#include <map>
#include <utility>

namespace hotbox {

// Since protobuf msg isn't easy to work with, Schema is a wrapper around
// SchemaProto.
// Comment(wdai): We let each FeatureFamily to each store a vector of Feature
// instead of storing one vector<Feature> in Schema to reduce coupling

class Schema {
public:
  Schema(const SchemaConfig& config);

  Schema(const SchemaProto& proto);

  // Initialize from Schema committed to db (Commit()).
  Schema(RocksDB* db);

  // Deep copy of families_.
  Schema(const Schema& other);

  void Init(const SchemaProto& proto);

  // new_feature needs to have FeatureStoreType set, but not offset
  // (which is set in schema).  'new_feature' will have global_offset
  // set correctly after the call. This call invokes map-lookup for
  // family based on family_name. Use another AddFeature to avoid
  // map-lookup. 'family_idx' = -1 to append feature at the end of
  // the family.
  void AddFeature(const std::string& family_name, Feature* new_feature,
      BigInt family_idx = -1);

  // Similar to AddFeature() above, but avoids map lookup for FeatureFamily.
  // 'family' must be a family obtained from this Schema.
  void AddFeature(FeatureFamilyIf* family, Feature* new_feature,
      BigInt family_idx = -1);

  // Extend a simple family's feature by num_features. family has to be
  // SimpleFeatureFamily.
  void AddFeatures(FeatureFamilyIf* family, BigInt num_features);

  Feature GetFeature(const std::string& family_name, BigInt family_idx) const;

  // Throws FamilyNotFound exception if family doesn't exist.
  Feature GetFeature(const FeatureFinder& finder) const;

  //Feature& GetMutableFeature(const Feature& feature);

  // Throws FamilyNotFoundException.
  const FeatureFamilyIf& GetFamily(const std::string& family_name) const;
  FeatureFamilyIf& GetMutableFamily(const std::string& family_name);

  // Try to get a family. If it doesn't exist, create it. 'simple_family' ==
  // true if the family only uses single store, otherwise 'store_type' is
  // ignored. Note that all output family has to be simple, since they can only
  // store in FeatureStoreType::OUTPUT. If 'simple_family' == false,
  // 'store_type' is ignored. Optional 'num_features' specifies number of
  // features in this family. Without it it'd be determined by
  // AddFeature/AddFeatures.
  FeatureFamilyIf& GetOrCreateFamily(const std::string& family_name,
      bool simple_family = false,
      FeatureStoreType store_type = FeatureStoreType::OUTPUT,
      BigInt num_features = 0);

  // Return append_store_offset_.
  const DatumProtoStoreOffset& GetAppendOffset() const;

  // Not including kInternalFamily.
  const std::map<std::string, std::unique_ptr<FeatureFamilyIf>>& GetFamilies()
    const;

  // Get # of features (not including label/weight or anything in the
  // kInternalFamily) in this schema.
  BigInt GetNumFeatures() const;

  // Generate OSchema (output schema) based on just output_families_.
  OSchemaProto GetOSchemaProto() const;

  SchemaConfig GetConfig() const;

  // Commit Schema to DB, chopping up repeated features. Return number of bytes
  // stored to disk.
  size_t Commit(RocksDB* db) const;

private:
  // if store_offset == -1 (default), increment the appropriate
  // append_store_offset_ and assign the offset to new_feature. Otherwise set
  // new_feature->store_offset and append_store_offset_ according to
  // store_offset.
  void UpdateStoreOffset(Feature* new_feature, BigInt store_offset = -1);

private:
  std::map<std::string, std::unique_ptr<FeatureFamilyIf>> families_;

  // Keep an ordered list of output families to construct OSchema to
  // send to client.
  std::vector<std::string> output_families_;

  // All feature are stored in features_. FeatureFamily maintains the
  // global_index to find feature from here.
  //std::shared_ptr<std::vector<Feature>> features_;

  // Each added feature will increment this.
  BigInt curr_global_offset_ = 0;

  // Internal family stores label, weight, and is treated specially so that
  // when returning families_ for DatumBase to iterate over we don't show
  // internal_family_.
  FeatureFamily internal_family_;

  // Tracks the insert store_offset for the next feature.
  DatumProtoStoreOffset append_store_offset_;

  SchemaConfig config_;
};

}  // namespace hotbox
