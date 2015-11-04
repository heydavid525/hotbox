#include <sstream>
#include <string>
#include <cstdint>
#include <glog/logging.h>
#include "schema/constants.hpp"
#include "schema/datum_base.hpp"
#include "schema/schema_util.hpp"

namespace hotbox {
<<<<<<< HEAD
  
  DatumBase::DatumBase(DatumProto* proto) : proto_(proto) { }
  
  DatumBase::DatumBase(const DatumBase& other) :
  proto_(new DatumProto(*other.proto_)) { }
  
  DatumProto* DatumBase::Release() {
    return proto_.release();
  }
  
  float DatumBase::GetLabel(const FeatureFamily& internal_family) const {
    return GetFeatureVal(internal_family.GetFeature(kLabelFamilyIdx));
  }
  
  float DatumBase::GetWeight(const FeatureFamily& internal_family) const {
    float weight = GetFeatureVal(internal_family.GetFeature(kWeightFamilyIdx));
    return weight == 0 ? 1 : weight;
  }
  
  /*
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
   */
  
  float DatumBase::GetFeatureVal(const Feature& f) const {
    CHECK_NOTNULL(proto_.get());
    CHECK(IsNumber(f));
    BigInt offset = f.offset();
    switch (f.store_type()) {
      case FeatureStoreType::DENSE_CAT:
      return static_cast<float>(proto_->dense_cat_store(offset));
      case FeatureStoreType::DENSE_NUM:
      return proto_->dense_num_store(offset);
      case FeatureStoreType::SPARSE_CAT:
=======

DatumBase::DatumBase(DatumProto* proto,
    StatCollector* stat_collector) : proto_(proto),
  stat_collector_(stat_collector) { }

DatumBase::DatumBase(const DatumBase& other) :
  proto_(new DatumProto(*other.proto_)),
  stat_collector_(other.stat_collector_) { }

DatumProto* DatumBase::Release() {
  return proto_.release();
}

float DatumBase::GetLabel(const FeatureFamily& internal_family) const {
  return GetFeatureVal(internal_family.GetFeature(kLabelFamilyIdx));
}

float DatumBase::GetWeight(const FeatureFamily& internal_family) const {
  float weight = GetFeatureVal(internal_family.GetFeature(kWeightFamilyIdx));
  return weight == 0 ? 1 : weight;
}

/*
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
*/

float DatumBase::GetFeatureVal(const Feature& f) const {
  CHECK_NOTNULL(proto_.get());
  CHECK(IsNumber(f));
  BigInt store_offset = f.store_offset();
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_CAT:
      return static_cast<float>(proto_->dense_cat_store(store_offset));
    case FeatureStoreType::DENSE_NUM:
      return proto_->dense_num_store(store_offset);
    case FeatureStoreType::SPARSE_CAT:
>>>>>>> 8045cdd041bffe1a6b57ad691d8cf08ca3807140
      {
        const auto& it = proto_->sparse_cat_store().find(store_offset);
        if (it != proto_->sparse_cat_store().cend()) {
          return static_cast<float>(it->second);
        }
        return 0.;
      }
      case FeatureStoreType::SPARSE_NUM:
      {
        const auto& it = proto_->sparse_num_store().find(store_offset);
        if (it != proto_->sparse_num_store().cend()) {
          return it->second;
        }
        return 0.;
      }
      default:
      LOG(FATAL) << "Unrecognized store_type: " << f.store_type();
    }
  }
<<<<<<< HEAD
  
  
  void DatumBase::SetFeatureVal(const Feature& f, float val) {
    CHECK_NOTNULL(proto_.get());
    CHECK(IsNumber(f));
    BigInt offset = f.offset();
    switch (f.store_type()) {
      case FeatureStoreType::DENSE_CAT:
      SetDenseCatFeatureVal(offset, static_cast<int32_t>(val));
      return;
      case FeatureStoreType::DENSE_NUM:
      SetDenseNumFeatureVal(offset, val);
      return;
      case FeatureStoreType::SPARSE_CAT:
      SetSparseCatFeatureVal(offset, static_cast<int32_t>(val));
      return;
      case FeatureStoreType::SPARSE_NUM:
      SetSparseNumFeatureVal(offset, val);
=======
}

void DatumBase::SetFeatureVal(const Feature& f, float val) {
  CHECK_NOTNULL(proto_.get());
  CHECK(IsNumber(f));
  BigInt store_offset = f.store_offset();
  if (stat_collector_ != nullptr) {
    stat_collector_->UpdateStat(f, val);
  }
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_CAT:
      SetDenseCatFeatureVal(store_offset, static_cast<int32_t>(val));
      return;
    case FeatureStoreType::DENSE_NUM:
      SetDenseNumFeatureVal(store_offset, val);
      return;
    case FeatureStoreType::SPARSE_CAT:
      SetSparseCatFeatureVal(store_offset, static_cast<int32_t>(val));
      return;
    case FeatureStoreType::SPARSE_NUM:
      SetSparseNumFeatureVal(store_offset, val);
>>>>>>> 8045cdd041bffe1a6b57ad691d8cf08ca3807140
      return;

      default:
      LOG(FATAL) << "Unrecognized store_type: " << f.store_type();
    }
  }
<<<<<<< HEAD
  
  //Oct 20  dense and sparse string set value
  void DatumBase::SetFeatureValString(const Feature& f,const  char* str, int length){
    BigInt offset = f.offset();
    switch (f.store_type()) {
      case FeatureStoreType::DENSE_BYTES:
      SetDenseStrFeatureVal(offset, str, length);
      return;
      case FeatureStoreType::SPARSE_BYTES:
      SetSparseStrFeatureVal(offset, str, length);
      return;
      
      default:
      LOG(FATAL) << "Unrecognized store_type: " << f.store_type();
    }
  }
  
  //Oct 20 set sparse string val
  void DatumBase::SetDenseStrFeatureVal(BigInt offset,const char* str_val,int length){
    CHECK_LT(offset, proto_->dense_bytes_store_size());
    for(int i = 0 ;i < length ; i++){
      proto_->set_dense_bytes_store(offset+i, &(str_val[i]));
    }
  }
  
  //Oct 20 set sparse string val
  void DatumBase::SetSparseStrFeatureVal(BigInt offset,const char* str_val,int length){
    for(int i = 0 ;i < length ; i++){
      (*(proto_->mutable_sparse_bytes_store()))[offset+i] = str_val[i];
    }
  }
  
  void DatumBase::SetDenseCatFeatureVal(BigInt offset, int val) {
    CHECK_LT(offset, proto_->dense_cat_store_size());
    proto_->set_dense_cat_store(offset, val);
  }
  
  // Directly set in sparse_cat_store()
  void DatumBase::SetSparseCatFeatureVal(BigInt offset, int val) {
    (*(proto_->mutable_sparse_cat_store()))[offset] = val;
  }
  
  // Directly set in dense_num_store()
  void DatumBase::SetDenseNumFeatureVal(BigInt offset, float val) {
    CHECK_LT(offset, proto_->dense_num_store_size());
    proto_->set_dense_num_store(offset, val);
  }
  
  // Directly set in sparse_num_store()
  void DatumBase::SetSparseNumFeatureVal(BigInt offset, float val) {
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
    ss << GetLabel(schema.GetFamily(kInternalFamily));
    float weight = GetWeight(schema.GetFamily(kInternalFamily));
    if (weight != 1) {
      ss << " " << weight;
    }
    for (const auto& pair : families) {
      const std::string& family_name = pair.first;
      ss << " |" << family_name;
      const std::vector<Feature>& features = pair.second.GetFeatures();
      for (int i = 0; i < features.size(); ++i) {
        if (!features[i].initialized()) {
          continue;
        }
        std::string feature_name = (features[i].name().empty() ?
                                    std::to_string(i) : features[i].name());
        ss << " " << feature_name << ":" << GetFeatureVal(features[i]);
      }
    }
    return ss.str();
=======
}

void DatumBase::SetDenseCatFeatureVal(BigInt store_offset, int val) {
  CHECK_LT(store_offset, proto_->dense_cat_store_size());
  proto_->set_dense_cat_store(store_offset, val);
}

// Directly set in sparse_cat_store()
void DatumBase::SetSparseCatFeatureVal(BigInt store_offset, int val) {
  (*(proto_->mutable_sparse_cat_store()))[store_offset] = val;
}

// Directly set in dense_num_store()
void DatumBase::SetDenseNumFeatureVal(BigInt store_offset, float val) {
  CHECK_LT(store_offset, proto_->dense_num_store_size());
  proto_->set_dense_num_store(store_offset, val);
}

// Directly set in sparse_num_store()
void DatumBase::SetSparseNumFeatureVal(BigInt store_offset, float val) {
  (*(proto_->mutable_sparse_num_store()))[store_offset] = val;
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
  ss << GetLabel(schema.GetFamily(kInternalFamily));
  float weight = GetWeight(schema.GetFamily(kInternalFamily));
  if (weight != 1) {
    ss << " " << weight;
  }
  for (const auto& pair : families) {
    const std::string& family_name = pair.first;
    ss << " |" << family_name;
    const auto& feature_seq = pair.second.GetFeatures();
    for (int i = 0; i < feature_seq.GetNumFeatures(); ++i) {
      const auto& f = feature_seq.GetFeature(i);
      std::string feature_name = (f.name().empty() ?
          std::to_string(i) : f.name());
      ss << " " << feature_name << ":" << GetFeatureVal(f);
    }
    /*
    const std::vector<Feature>& features = pair.second.GetFeatures();
    for (int i = 0; i < features.size(); ++i) {
      //if (!features[i].initialized()) {
      //  continue;
      //}
      std::string feature_name = (features[i].name().empty() ?
          std::to_string(i) : features[i].name());
      ss << " " << feature_name << ":" << GetFeatureVal(features[i]);
    }
    */
>>>>>>> 8045cdd041bffe1a6b57ad691d8cf08ca3807140
  }
  
  DatumProto* DatumBase::ReleaseProto() {
    return proto_.release();
  }
  
  std::string DatumBase::Serialize() const {
    std::string serialized;
    proto_->SerializeToString(&serialized);
    return serialized;
  }
  
}  // namespace hotbox
