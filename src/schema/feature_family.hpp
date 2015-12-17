#pragma once

#include "util/hotbox_exceptions.hpp"
#include "schema/proto/schema.pb.h"
#include "schema/constants.hpp"
#include <string>
#include <map>
#include <utility>

namespace hotbox {

class Schema;

// Lightweight accessor to get a sequence of features. Used to get all features
// in a family.
class FeatureSeq {
public:
  FeatureSeq(std::shared_ptr<std::vector<Feature>> features,
      const std::vector<BigInt>& idx) : features_(features),
  idx_(idx) { }

  BigInt GetNumFeatures() const {
    return idx_.size();
  }

  // Get the seq_id-th item in the sequence.
  const Feature& GetFeature(BigInt seq_id) const {
    return (*features_)[idx_[seq_id]];
  }

private:
  std::shared_ptr<std::vector<Feature>> features_;

  std::vector<BigInt> idx_;
};

class FeatureFamily {
public:
  FeatureFamily();

  // A simple_family is a family that uses only one feature store.
  FeatureFamily(const std::string& family_name,
      std::shared_ptr<std::vector<Feature>> features,
      bool simple_family = false);

  FeatureFamily(const FeatureFamilyProto& proto,
      std::shared_ptr<std::vector<Feature>> features,
      bool simple_family = false);

  FeatureFamily(const SelfContainedFeatureFamilyProto& proto);

  bool HasFeature(BigInt family_idx) const;

  //void DeleteFeature(BigInt family_idx);

  const Feature& GetFeature(const std::string& feature_name) const;
  Feature& GetMutableFeature(const std::string& feature_name);

  // Returns feature and whether it is found. If it's not found, then Feature
  // is invalid.
  std::pair<Feature, bool> GetFeatureNoExcept(BigInt family_idx) const;
  const Feature& GetFeature(BigInt family_idx) const;
  Feature& GetMutableFeature(BigInt family_idx);

  // Return both initialized and uninitialized features.
  FeatureSeq GetFeatures() const;

  // Only count the initialized features.
  BigInt GetNumFeatures() const;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures iff
  // there's no uninitialized features.
  BigInt GetMaxFeatureId() const;

  FeatureFamilyProto GetProto() const;

  // Include Feature of this family. Use this to serialize single family
  // without Schema (e.g., internal family).
  SelfContainedFeatureFamilyProto GetSelfContainedProto() const;

  std::string GetFamilyName() const;

  // Set the begin and end of offset.
  inline void SetOffset(const DatumProtoStoreOffset& offset) {
    offset_begin_ = offset;
    offset_end_ = offset;
  }

  inline DatumProtoStoreOffset GetOffsetBegin() const {
    return offset_begin_;
  }

  inline DatumProtoStoreOffset GetOffsetEnd() const {
    return offset_end_;
  }

  // True if this family only uses one FeatureStoreType.
  bool IsSimple() const {
    return simple_family_;
  }

  StoreTypeAndOffset GetStoreTypeAndOffset() const;

private:
  // Allow Schema to access AddFeature.
  //friend void Schema::AddFeature(const std::string& family_name,
  //    const Feature& new_feature, BigInt family_idx);
  friend class Schema;

  // Add new feature as the family_idx-th feature in the family. family_idx =
  // -1 to append as last feature.
  void AddFeature(const Feature& new_feature, BigInt family_idx = -1);

  // Throws FeatureNotFoundException if not found.
  void CheckFeatureExist(BigInt family_idx) const;

  void UpdateOffsets(const Feature& new_feature);

private:
  std::string family_name_;

  // All schema Features across FeatureFamily are here. Indexed by global_idx.
  std::shared_ptr<std::vector<Feature>> features_;

  // feature name --> family_idx.
  std::map<std::string, BigInt> name_to_family_idx_;

  // Maps from family_idx to global idx. Needed in feature lookup.
  // Uninitialized features will have global_idx_ = -1
  std::vector<BigInt> global_idx_;

  // Each FeatureFamily must have contiguous offsets in each store:
  // [offset_begin_, offset_end).
  DatumProtoStoreOffset offset_begin_;
  DatumProtoStoreOffset offset_end_;

  bool simple_family_;
};

}  // namespace hotbox
