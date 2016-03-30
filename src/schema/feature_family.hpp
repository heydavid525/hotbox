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
  FeatureSeq(const std::vector<Feature> features,
      const std::vector<BigInt>& idx) : features_(features),
  idx_(idx) { }

  BigInt GetNumFeatures() const {
    return idx_.size();
  }

  // Get the seq_id-th item in the sequence.
  const Feature& GetFeature(BigInt seq_id) const {
    return features_[idx_[seq_id]];
  }

private:
  const std::vector<Feature>& features_;
  std::vector<BigInt> idx_;
};

// ============ FeatureFamilyIf ==============

class FeatureFamilyIf {
public:
  // Default constructor doesn't construct a valid FeatureFamily.
  // TODO(wdai): Do we need default constructor?
  FeatureFamilyIf() { }

  FeatureFamilyIf(const std::string& family_name,
      const DatumProtoStoreOffset& offset,
      BigInt global_offset_begin) :
    family_name_(family_name), offset_begin_(offset),
    offset_end_(offset),
    global_offset_begin_(global_offset_begin) {
      offset_begin_.mutable_offsets()->Resize(
          FeatureStoreType::NUM_STORE_TYPES, 0);
      offset_end_.mutable_offsets()->Resize(
          FeatureStoreType::NUM_STORE_TYPES, 0);
    }

  FeatureFamilyIf(const FeatureFamilyProto& proto) :
    family_name_(proto.family_name()),
    offset_begin_(proto.offset_begin()), offset_end_(proto.offset_end()),
    global_offset_begin_(proto.global_offset_begin()) { }

  virtual const Feature& GetFeature(const std::string& feature_name) const = 0;

  // Access feature by family_idx. NoExcept version does not throw exception
  // when feature is not found.
  // Comment(wdai): GetFeature has to return value instead of reference because
  // SimpleFeatureFamily does not materialize any Feature.
  virtual Feature GetFeature(BigInt family_idx) const = 0;
  virtual std::pair<Feature, bool> GetFeatureNoExcept(BigInt family_idx)
    const noexcept = 0;

  // Return initialized/valid features only.
  //virtual FeatureSeq GetFeatures() const = 0;

  // Only count the initialized features.
  virtual BigInt GetNumFeatures() const = 0;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures() iff
  // there's no uninitialized features (since feature id is 0-based).
  virtual BigInt GetMaxFeatureId() const = 0;

  std::string GetFamilyName() const {
    return family_name_;
  }

  inline DatumProtoStoreOffset GetOffsetBegin() const {
    return offset_begin_;
  }

  inline DatumProtoStoreOffset GetOffsetEnd() const {
    return offset_end_;
  }

  virtual FeatureFamilyProto GetProto() const = 0;

  virtual bool IsSimple() const = 0;

protected:
  // Allow Schema to access AddFeature.
  friend class Schema;

  // Add new feature as the family_idx-th feature in the family.
  // family_idx = -1 to append as last feature.
  // new_feature->global_offset() must be set properly after the call
  // returns.
  virtual void AddFeature(Feature* new_feature, BigInt family_idx = -1) = 0;

  FeatureFamilyProto GetProtoIntern() const;

protected:
  std::string family_name_;

  // Each FeatureFamily must have contiguous offsets in each store:
  // [offset_begin_, offset_end).
  DatumProtoStoreOffset offset_begin_;
  DatumProtoStoreOffset offset_end_;

  // Each family is added once for all, and thus features in a family have
  // consecutive global offsets starting from global_offset_begin_.
  BigInt global_offset_begin_;
};

// ============ SimpleFeatureFamily ==============

// In SimpleFeatureFamily all features are ananymous and use the same
// store.  Consequently no feature is actually materialized. Note
// that all output families have to be simple.
class SimpleFeatureFamily : public FeatureFamilyIf {
public:
  // A simple_family is a family that uses only one feature store.
  SimpleFeatureFamily(const std::string& family_name,
      const DatumProtoStoreOffset& store_offset,
      FeatureStoreType store_type,
      BigInt global_offset_begin);

  SimpleFeatureFamily(const FeatureFamilyProto& proto) :
    FeatureFamilyIf(proto), store_type_(proto.store_type()) {
      LOG(INFO) << "SimpleFeatureFamily " << family_name_
        << " initialized. # features: " << GetNumFeatures()
        << " features";
    }

  const Feature& GetFeature(const std::string& feature_name) const override;

  // Access feature by family_idx. NoExcept version does not throw exception when
  // feature is not found.
  Feature GetFeature(BigInt family_idx) const override;
  std::pair<Feature, bool> GetFeatureNoExcept(BigInt family_idx)
    const noexcept override;

  // Returns the store type and offset.
  StoreTypeAndOffset GetStoreTypeAndOffset() const;

  bool IsSimple() const override {
    return true;
  }

  // Only count the initialized features.
  BigInt GetNumFeatures() const override;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures iff
  // there's no uninitialized features.
  BigInt GetMaxFeatureId() const override;

  FeatureFamilyProto GetProto() const override;

private:
  bool HasFeature(BigInt family_idx) const;

  Feature CreateFeature(BigInt family_idx) const;

  // Add new feature as the family_idx-th feature in the family. family_idx =
  // -1 to append as last feature.
  void AddFeature(Feature* new_feature, BigInt family_idx = -1) override;

  // Extend family idx range to family_idx.
  void AddFeatureRange(BigInt family_idx);

private:
  FeatureStoreType store_type_;
  //BigInt offset_begin_;
  //BigInt offset_end_;
};

// ============ FeatureFamily ==============

// FeatureFamily materializes all the features and support multi-store.
class FeatureFamily : public FeatureFamilyIf {
public:
  // Default constructor does not instantiate a valid FeatureFamily.
  FeatureFamily() { };

  FeatureFamily(const std::string& family_name,
      const DatumProtoStoreOffset& store_offset,
      BigInt global_offset_begin);

  FeatureFamily(const FeatureFamilyProto& proto);

  const Feature& GetFeature(const std::string& feature_name) const override;
  //Feature& GetMutableFeature(const std::string& feature_name);

  Feature GetFeature(BigInt family_idx) const override;
  // Returns feature and whether it is found. If it's not found, then Feature
  // is invalid.
  std::pair<Feature, bool> GetFeatureNoExcept(BigInt family_idx) const
    noexcept override;

  //Feature& GetMutableFeature(BigInt family_idx);

  // Return initialized features, skipping the uninitialized ones.
  FeatureSeq GetFeatures() const;

  // Only count the initialized features.
  BigInt GetNumFeatures() const;

  // Get max feature id in this family. MaxFeatureId + 1 == NumFeatures iff
  // there's no uninitialized features.
  BigInt GetMaxFeatureId() const override;

  FeatureFamilyProto GetProto() const override;

  bool IsSimple() const override {
    return false;
  }

private:
  bool HasFeature(BigInt family_idx) const;

  // Allow Schema to access AddFeature.
  //friend void Schema::AddFeature(const std::string& family_name,
  //    const Feature& new_feature, BigInt family_idx);
  //friend class Schema;

  // Add new feature as the family_idx-th feature in the family. family_idx =
  // -1 to append as last feature.
  void AddFeature(Feature* new_feature, BigInt family_idx = -1) override;

  // Throws FeatureNotFoundException if not found.
  void CheckFeatureExist(BigInt family_idx) const;

  void UpdateOffsets(const Feature& new_feature);

private:
  // All schema Features across FeatureFamily are here. Indexed by global_idx.
  std::vector<Feature> features_;

  // initialized_[family_idx] == true means features_[family_idx] is
  // initialized.
  std::vector<bool> initialized_;

  // feature name --> family_idx.
  std::map<std::string, BigInt> name_to_family_idx_;
};

}  // namespace hotbox
