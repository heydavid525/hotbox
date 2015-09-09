#include <sstream>
#include <string>
#include <cstdint>
#include <glog/logging.h>
#include "schema/constants.hpp"
#include "schema/datum_base.hpp"
#include "schema/schema_util.hpp"

namespace mldb {

DatumBase::DatumBase(DatumProto* proto) : proto_(proto) { }

DatumBase::DatumBase(const DatumBase& other) :
  proto_(new DatumProto(*other.proto_)) { }

DatumProto* DatumBase::Release() {
  return proto_.release();
}

float DatumBase::GetLabel(const Schema& schema) const {
  const auto& family = schema.GetFamily(kInternalFamily);
  return GetFeatureVal(family.GetFeature(kLabelFamilyIdx).loc());
}

float DatumBase::GetWeight(const Schema& schema) const {
  const auto& family = schema.GetFamily(kInternalFamily);
  float weight = GetFeatureVal(family.GetFeature(kWeightFamilyIdx).loc());
  return weight == 0 ? 1 : weight;
}

float DatumBase::GetFeatureVal(const Schema& schema,
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

float DatumBase::GetFeatureVal(const FeatureLocator& loc) const {
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

void DatumBase::SetFeatureVal(const FeatureLocator& loc, float val) {
  CHECK_NOTNULL(proto_.get());
  CHECK(IsNumeral(loc));
  bool is_cat = loc.type() == FeatureType::CATEGORICAL;
  bool is_dense = loc.store_type() == FeatureStoreType::DENSE;
  int32_t offset = loc.offset();
  if (is_cat && is_dense) {
    // Check that the dense store is pre-allocated (before any transform).
    SetDenseCatFeatureVal(offset, static_cast<int32_t>(val));
  } else if (!is_cat && is_dense) {
    SetDenseNumFeatureVal(offset, val);
  } else if (is_cat && !is_dense) {
    SetSparseCatFeatureVal(offset, static_cast<int32_t>(val));
  } else {
    SetSparseNumFeatureVal(offset, val);
  }
}

void DatumBase::SetDenseCatFeatureVal(int offset, int val) {
  CHECK_LT(offset, proto_->dense_cat_store_size());
  proto_->set_dense_cat_store(offset, val);
}

// Directly set in sparse_cat_store()
void DatumBase::SetSparseCatFeatureVal(int offset, int val) {
  (*(proto_->mutable_sparse_cat_store()))[offset] = val;
}

// Directly set in dense_num_store()
void DatumBase::SetDenseNumFeatureVal(int offset, float val) {
  CHECK_LT(offset, proto_->dense_num_store_size());
  proto_->set_dense_num_store(offset, val);
}

// Directly set in sparse_num_store()
void DatumBase::SetSparseNumFeatureVal(int offset, float val) {
  (*(proto_->mutable_sparse_num_store()))[offset] = val;
}

std::string DatumBase::ToString() const {
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

std::string DatumBase::ToString(const Schema& schema) const {
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

}  // namespace mldb
