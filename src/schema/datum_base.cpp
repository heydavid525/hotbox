#include <sstream>
#include <string>
#include <cstdint>
#include <glog/logging.h>
#include "schema/constants.hpp"
#include "schema/datum_base.hpp"
#include "schema/schema_util.hpp"

namespace hotbox {

DatumBase::DatumBase(DatumProto* proto,
    StatCollector* stat_collector) : proto_(proto),
  stat_collector_(stat_collector) {
    CheckInOrder();
  }

DatumBase::DatumBase(const DatumBase& other) :
  proto_(new DatumProto(*other.proto_)),
  stat_collector_(other.stat_collector_) { }

DatumProto* DatumBase::Release() {
  return proto_.release();
}

float DatumBase::GetLabel(const FeatureFamilyIf& internal_family) const {
  return GetFeatureVal(internal_family.GetFeature(kLabelFamilyIdx));
}

float DatumBase::GetWeight(const FeatureFamilyIf& internal_family) const {
  float weight = GetFeatureVal(internal_family.GetFeature(kWeightFamilyIdx));
  return weight == 0 ? 1 : weight;
}

void DatumBase::ExtendDenseCatStore(BigInt size) {
  if (proto_->dense_cat_store_size() < size) {
    proto_->mutable_dense_cat_store()->Resize(size, 0);
  }
}

void DatumBase::ExtendDenseNumStore(BigInt size) {
  if (proto_->dense_num_store_size() < size) {
    proto_->mutable_dense_num_store()->Resize(size, 0.);
  }
}

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
      {
        /*
        const auto& it = proto_->sparse_cat_store().find(store_offset);
        if (it != proto_->sparse_cat_store().cend()) {
          return static_cast<float>(it->second);
        }
        return 0.;
        */
        // Binary search to find store_offset in idx.
        const auto& idxs = proto_->sparse_cat_store_idxs();
        const auto low = std::lower_bound(idxs.cbegin(),
            idxs.cend(), store_offset);
        auto found = low - idxs.cbegin();
        if (low == idxs.cend() ||
            proto_->sparse_cat_store_idxs(found) != store_offset) {
          // Not found.
          return 0;
        }
        return proto_->sparse_cat_store_vals(found);
      }
    case FeatureStoreType::SPARSE_NUM:
      {
        const auto& idxs = proto_->sparse_num_store_idxs();
        const auto low = std::lower_bound(idxs.cbegin(),
            idxs.cend(), store_offset);
        auto found = low - idxs.cbegin();
        if (low == idxs.cend() ||
            proto_->sparse_num_store_idxs(found) != store_offset) {
          // Not found.
          return 0;
        }
        return proto_->sparse_num_store_vals(found);
      }
    default:
      LOG(FATAL) << "Unrecognized store_type: " << f.store_type();
  }
  return 0.;
}

void DatumBase::SetFeatureVal(const Feature& f, float val) {
  CHECK_NOTNULL(proto_.get());
  CHECK(IsNumber(f));
  BigInt store_offset = f.store_offset();
  if (stat_collector_ != nullptr) {
    stat_collector_->UpdateStat(f, val);
  }
  switch (f.store_type()) {
    case FeatureStoreType::SPARSE_NUM:
      SetSparseNumFeatureVal(store_offset, val);
      return;
    case FeatureStoreType::DENSE_CAT:
      SetDenseCatFeatureVal(store_offset, static_cast<int32_t>(val));
      return;
    case FeatureStoreType::DENSE_NUM:
      SetDenseNumFeatureVal(store_offset, val);
      return;
    case FeatureStoreType::SPARSE_CAT:
      SetSparseCatFeatureVal(store_offset, static_cast<int32_t>(val));
      return;
    default:
      LOG(FATAL) << "Unrecognized store_type: " << f.store_type();
  }
}

void DatumBase::SetDenseCatFeatureVal(BigInt store_offset, int val) {
  CHECK_LT(store_offset, proto_->dense_cat_store_size());
  proto_->set_dense_cat_store(store_offset, val);
}

// Directly set in sparse_cat_store()
void DatumBase::SetSparseCatFeatureVal(BigInt store_offset, int val) {
  proto_->add_sparse_cat_store_idxs(store_offset);
  proto_->add_sparse_cat_store_vals(val);
  //(*(proto_->mutable_sparse_cat_store()))[store_offset] = val;
}

// Directly set in dense_num_store()
void DatumBase::SetDenseNumFeatureVal(BigInt store_offset, float val) {
  CHECK_LT(store_offset, proto_->dense_num_store_size());
  proto_->set_dense_num_store(store_offset, val);
}

void DatumBase::SetDenseNumFeatureVal(BigInt offset, const std::vector<float>& vals) {
  CHECK_LE(offset + vals.size(), proto_->dense_num_store_size());
  auto len = vals.size(); 
  auto src = vals.data(); 
  auto dst = proto_->mutable_dense_num_store()->mutable_data();
  memcpy(dst+offset, src, sizeof(float) * len);
}

// Directly set in sparse_num_store()
void DatumBase::SetSparseNumFeatureVal(BigInt store_offset, float val) {
  proto_->add_sparse_num_store_idxs(store_offset);
  proto_->add_sparse_num_store_vals(val);
  //(*(proto_->mutable_sparse_num_store()))[store_offset] = val;
}

void DatumBase::SetSparseNumFeatureVal(BigInt offset, const std::vector<float>& vals) {
  int sparse_idx = proto_->sparse_num_store_idxs_size();
  if (sparse_idx > 0) {
    auto last_offset = proto_->sparse_num_store_idxs(sparse_idx-1);
    CHECK_LT(last_offset, offset)
      << "relative_offset needs to be added in ascending order. Last "
      "offset: " << last_offset << ", curr_offset: " << offset;
  }
  for (int i = 0; i < vals.size(); ++i) {
    proto_->add_sparse_num_store_idxs(offset + i);
    proto_->add_sparse_num_store_vals(vals[i]);
  }
}

std::string DatumBase::ToString() const {
  CHECK_NOTNULL(proto_.get());
  std::stringstream ss;
  ss << "dense_cat:";
  for (int i = 0; i < proto_->dense_cat_store_size(); ++i) {
    ss << " " << i << ":" << proto_->dense_cat_store(i);
  }
  ss << " | sparse_cat:";
  for (int i = 0; i < proto_->sparse_cat_store_idxs_size(); ++i) {
    ss << " " << proto_->sparse_cat_store_idxs(i) << ":" <<
      proto_->sparse_cat_store_vals(i);
  }
  //for (const auto& pair : proto_->sparse_cat_store()) {
  //  ss << " " << pair.first << ":" << pair.second;
  //}
  ss << " | dense_num:";
  for (int i = 0; i < proto_->dense_num_store_size(); ++i) {
    ss << " " << i << ":" << proto_->dense_num_store(i);
  }
  ss << " | sparse_num:";
  //for (const auto& pair : proto_->sparse_num_store()) {
  //  ss << " " << pair.first << ":" << pair.second;
  //}
  for (int i = 0; i < proto_->sparse_num_store_idxs_size(); ++i) {
    ss << " " << proto_->sparse_num_store_idxs(i) << ":" <<
      proto_->sparse_num_store_vals(i);
  }
  ss << " | dense_bytes:";
  for (int i = 0; i < proto_->dense_bytes_store_size(); ++i) {
    ss << " " << i << ":" << proto_->dense_bytes_store(i);
  }
  ss << " | sparse_bytes:";
  //for (const auto& pair : proto_->sparse_bytes_store()) {
  //  ss << " " << pair.first << ":" << pair.second;
  //}
  for (int i = 0; i < proto_->sparse_bytes_store_idxs_size(); ++i) {
    ss << " " << proto_->sparse_bytes_store_idxs(i) << ":" <<
      proto_->sparse_bytes_store_vals(i);
  }
  return ss.str();
}

// The implementation is rather inefficient, so shouldn't be called often.
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
    LOG(FATAL) << "Not implemented yet.";
    /*
    if (pair.second.IsSimple()) {
      LOG(INFO) << "Not implemented yet.";
    } else {
      const auto& feature_seq = pair.second.GetFeatures();
      for (int i = 0; i < feature_seq.GetNumFeatures(); ++i) {
        const auto& f = feature_seq.GetFeature(i);
        std::string feature_name = (f.name().empty() ?
            std::to_string(i) : f.name());
        ss << " " << feature_name << ":" << GetFeatureVal(f);
      }
    }
    */
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
  }
  return ss.str();
}

DatumProto* DatumBase::ReleaseProto() {
  return proto_.release();
}

std::string DatumBase::Serialize() const {
  std::string serialized;
  proto_->SerializeToString(&serialized);
  return serialized;
}

void DatumBase::CheckInOrder() const {
  CHECK_NOTNULL(proto_.get());
  BigInt prev_idx = -1;
  for (int i = 0; i < proto_->sparse_cat_store_idxs_size(); ++i) {
    BigInt curr_idx = proto_->sparse_cat_store_idxs(i);
    CHECK_GT(curr_idx, prev_idx);
    prev_idx = curr_idx;
  }
  prev_idx = -1;
  for (int i = 0; i < proto_->sparse_num_store_idxs_size(); ++i) {
    BigInt curr_idx = proto_->sparse_num_store_idxs(i);
    CHECK_GT(curr_idx, prev_idx) << "sparse_num_store: "
      << proto_->DebugString();
    prev_idx = curr_idx;
  }
  prev_idx = -1;
  for (int i = 0; i < proto_->sparse_bytes_store_idxs_size(); ++i) {
    BigInt curr_idx = proto_->sparse_bytes_store_idxs(i);
    CHECK_GT(curr_idx, prev_idx);
    prev_idx = curr_idx;
  }
}

}  // namespace hotbox
