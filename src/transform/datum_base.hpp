#pragma once

#include <glog/logging.h>
#include <sstream>
#include <string>
#include <cstdint>
#include "transform/proto/schema.pb.h"
#include "transform/schema_util.hpp"
#include "transform/schema.hpp"
#include "transform/constants.hpp"

namespace mldb {

// A zero-copy wrapper class around DatumProto. Not copyable.
class DatumBase {
public:
  // DatumBase takes the ownership of proto.
  DatumBase(DatumProto* proto) : proto_(proto) { }

  DatumBase(const DatumBase& other) : proto_(new DatumProto(*other.proto_)) { }

  DatumProto* Release() {
    return proto_.release();
  }

  float GetLabel(const Schema& schema) const {
    const auto& family = schema.GetFamily(kInternalFamily);
    return GetFeatureVal(family.GetFeature(kLabelFamilyIdx).loc());
  }

  // Weight cannot be 0.
  float GetWeight(const Schema& schema) const {
    const auto& family = schema.GetFamily(kInternalFamily);
    float weight = GetFeatureVal(family.GetFeature(kWeightFamilyIdx).loc());
    return weight == 0 ? 1 : weight;
  }

  // feature_desc (feature descriptor) can only result in 1 feature.
  float GetFeatureVal(const Schema& schema,
      const std::string& feature_desc) const {
    auto finders = ParseFeatureDesc(feature_desc);
    CHECK_EQ(1, finders.size());
    const auto& finder = finders[0];
    const auto& family = schema.GetFamily(finder.family_name);
    CHECK(!finder.all_family);
    FeatureLocator loc;
    if (!finder.feature_name.empty()) {
      loc = family.GetFeature(finder.feature_name).loc();
    } else {
      loc = family.GetFeature(finder.family_idx).loc();
    }
    return GetFeatureVal(loc);
    }

  // Get numeral feature value (CATEGORICAL or NUMERIC). Error otherwise.
  // TODO(wdai): Return flexitype in the future.
  float GetFeatureVal(const FeatureLocator& loc) const {
    CHECK_NOTNULL(proto_.get());
    CHECK(IsNumeral(loc));
    bool is_cat = loc.type() == FeatureType::CATEGORICAL;
    bool is_dense = loc.store_type() == FeatureStoreType::DENSE;
    int32_t offset = loc.offset();
    if (is_cat && is_dense) {
      return static_cast<float>(proto_->dense_cat_store(offset));
    } else if (!is_cat && is_dense) {
      return proto_->dense_num_store(offset);
    } else if (is_cat && !is_dense) {
      const auto& it = proto_->sparse_cat_store().find(offset);
      if (it != proto_->sparse_cat_store().cend()) {
        return static_cast<float>(it->second);
      }
      return 0.;
    } else {
      const auto& it = proto_->sparse_num_store().find(offset);
      if (it != proto_->sparse_num_store().cend()) {
        return it->second;
      }
      return 0.;
    }
  }

  // Assumes the dense feature stores are resized already.
  void SetFeatureVal(const FeatureLocator& loc, float val) {
    CHECK_NOTNULL(proto_.get());
    CHECK(IsNumeral(loc));
    bool is_cat = loc.type() == FeatureType::CATEGORICAL;
    bool is_dense = loc.store_type() == FeatureStoreType::DENSE;
    int32_t offset = loc.offset();
    if (is_cat && is_dense) {
      // Check that the dense store is pre-allocated (before any transform).
      CHECK_LT(offset, proto_->dense_cat_store_size());
      proto_->set_dense_cat_store(offset, static_cast<int32_t>(val));
    } else if (!is_cat && is_dense) {
      CHECK_LT(offset, proto_->dense_num_store_size());
      proto_->set_dense_num_store(offset, val);
    } else if (is_cat && !is_dense) {
      (*(proto_->mutable_sparse_cat_store()))[offset] =
        static_cast<int32_t>(val);
    } else {
      (*(proto_->mutable_sparse_num_store()))[offset] = val;
    }
  }

  std::string ToString() const {
    CHECK_NOTNULL(proto_.get());
    std::stringstream ss;
    ss << "dense_cat:";
    for (int i = 0; i < proto_->dense_cat_store_size(); ++i) {
      ss << " " << i << ":" << proto_->dense_cat_store(i);
    }
    ss << " | sparse_cat:";
    for (const auto& pair : proto_->sparse_cat_store()) {
      ss << " " << pair.first << ":" << pair.second;
    }
    ss << " | dense_num:";
    for (int i = 0; i < proto_->dense_num_store_size(); ++i) {
      ss << " " << i << ":" << proto_->dense_num_store(i);
    }
    ss << " | sparse_num:";
    for (const auto& pair : proto_->sparse_num_store()) {
      ss << " " << pair.first << ":" << pair.second;
    }
    ss << " | dense_bytes:";
    for (int i = 0; i < proto_->dense_bytes_store_size(); ++i) {
      ss << " " << i << ":" << proto_->dense_bytes_store(i);
    }
    ss << " | sparse_bytes:";
    for (const auto& pair : proto_->sparse_bytes_store()) {
      ss << " " << pair.first << ":" << pair.second;
    }
    return ss.str();
  }

  // Print with schema info. Only print numeral features (no bytes).
  std::string ToString(const Schema& schema) const {
    CHECK_NOTNULL(proto_.get());
    std::stringstream ss;
    const auto& families = schema.GetFamilies();
    for (const auto& pair : families) {
      const std::string& family_name = pair.first;
      ss << " |" << family_name;
      const std::vector<Feature>& features = pair.second.GetFeatures();
      for (int i = 0; i < features.size(); ++i) {
        std::string feature_name = (features[i].name().empty() ?
            std::to_string(i) : features[i].name());
        ss << " " << feature_name << ":" << GetFeatureVal(features[i].loc());
      }
    }
    return ss.str();
  }

private:
  std::unique_ptr<DatumProto> proto_;
};
}  // namespace mldb
