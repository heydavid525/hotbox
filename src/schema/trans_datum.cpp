#include "schema/trans_datum.hpp"
#include <glog/logging.h>
#include <utility>

namespace hotbox {

TransDatum::TransDatum(DatumBase* base, const FeatureFamily& internal_family,
    OutputStoreType output_store_type, BigInt output_dim) : base_(base),
  internal_family_(internal_family),
  output_store_type_(output_store_type), output_feature_dim_(output_dim) {
  if (output_store_type == OutputStoreType::DENSE) {
    dense_vals_.resize(output_feature_dim_);
  }
}

float TransDatum::GetFeatureVal(const Feature& f) const {
  return base_->GetFeatureVal(f);
}

void TransDatum::ReadyTransform(const TransformOutputRange& output_range) {
  store_type_ = output_range.store_type();
  offset_begin_ = output_range.store_offset_begin();
  offset_end_ = output_range.store_offset_end();
  range_ = offset_end_ - offset_begin_;
}

void TransDatum::SetFeatureValRelativeOffset(BigInt relative_offset,
    float val) {
  CHECK_LT(relative_offset, range_) << "relative_offset " << relative_offset
    << " is bigger than range " << range_;
  BigInt offset = relative_offset + offset_begin_;
  if (store_type_ == FeatureStoreType::OUTPUT) {
    if (output_store_type_ == OutputStoreType::DENSE) {
      dense_vals_[offset] = val;
    } else {
      if (sparse_idx_.size() > 0) {
        auto last_offset = sparse_idx_[sparse_idx_.size()-1];
        CHECK_LT(last_offset, offset)
          << "relative_offset needs to be added in ascending order. Last "
          "offset: " << last_offset << ", curr_offset: " << offset;
      }
      sparse_idx_.push_back(offset);
      sparse_vals_.push_back(val);
    }
    return;
  }
  // Internal stores.
  switch (store_type_) {
    case FeatureStoreType::DENSE_CAT:
      base_->SetDenseCatFeatureVal(offset, static_cast<int32_t>(val));
      return;
    case FeatureStoreType::DENSE_NUM:
      base_->SetDenseNumFeatureVal(offset, val);
      return;
    case FeatureStoreType::SPARSE_CAT:
      base_->SetSparseCatFeatureVal(offset, static_cast<int32_t>(val));
      return;
    case FeatureStoreType::SPARSE_NUM:
      base_->SetSparseNumFeatureVal(offset, val);
      return;
    default:
      LOG(FATAL) << "Unrecognized store_type: " << store_type_;
  }
}

FlexiDatum TransDatum::GetFlexiDatum() {
  float label = base_->GetLabel(internal_family_);
  float weight = base_->GetWeight(internal_family_);
  switch (output_store_type_) {
    case OutputStoreType::SPARSE:
      return FlexiDatum(std::move(sparse_idx_), std::move(sparse_vals_),
          output_feature_dim_, label, weight);
    case OutputStoreType::DENSE:
      return FlexiDatum(std::move(dense_vals_), label, weight);
    default:
      LOG(FATAL) << "Unrecognized output_store_type_" << output_store_type_;
  }
  LOG(FATAL) << "Should not reach here";
  return FlexiDatum(std::move(dense_vals_), label, weight);
}

}  // namespace hotbox
