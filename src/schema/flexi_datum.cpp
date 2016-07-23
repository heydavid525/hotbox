#include "schema/flexi_datum.hpp"
#include <glog/logging.h>
#include <sstream>

namespace hotbox {

FlexiDatum::FlexiDatum() :
  store_type_(OutputStoreType::SPARSE) { }

FlexiDatum::FlexiDatum(std::vector<BigInt>&& feature_ids,
    std::vector<float>&& vals, BigInt feature_dim,
    float label, float weight) :
  store_type_(OutputStoreType::SPARSE), feature_dim_(feature_dim),
  label_(label), weight_(weight), sparse_idx_(feature_ids),
  sparse_vals_(vals) { }

FlexiDatum::FlexiDatum(std::vector<float>&& vals, float label, float weight) :
  store_type_(OutputStoreType::DENSE), feature_dim_(vals.size()),
  label_(label), weight_(weight), dense_vals_(vals) { }

FlexiDatum::FlexiDatum(FlexiDatum&& other) noexcept :
  store_type_(other.store_type_), feature_dim_(other.feature_dim_)
  , label_(other.label_), weight_(other.weight_),
  sparse_idx_(std::move(other.sparse_idx_)),
  sparse_vals_(std::move(other.sparse_vals_)),
  dense_vals_(std::move(other.dense_vals_)) { }

FlexiDatum& FlexiDatum::operator=(FlexiDatum&& other) {
  store_type_ = other.store_type_;
  feature_dim_ = other.feature_dim_;
  label_ = other.label_;
  weight_ = other.weight_;
  sparse_idx_ = std::move(other.sparse_idx_);
  sparse_vals_ = std::move(other.sparse_vals_);
  dense_vals_ = std::move(other.dense_vals_);
  return *this;
}

FlexiDatum& FlexiDatum::operator=(const FlexiDatum&& other) {
  LOG(INFO) << "Copy constructor. Please try not to use me.";
  store_type_ = other.store_type_;
  feature_dim_ = other.feature_dim_;
  label_ = other.label_;
  weight_ = other.weight_;
  sparse_idx_ = std::move(other.sparse_idx_);
  sparse_vals_ = std::move(other.sparse_vals_);
  dense_vals_ = std::move(other.dense_vals_);
  return *this;
}

const std::vector<float>& FlexiDatum::GetDenseStore() const {
  return dense_vals_;
}

const std::vector<BigInt>& FlexiDatum::GetSparseIdx() const {
  return sparse_idx_;
}

const std::vector<float>& FlexiDatum::GetSparseVals() const {
  return sparse_vals_;
}

std::vector<float>&& FlexiDatum::MoveDenseStore() {
  CHECK_EQ(OutputStoreType::DENSE, store_type_);
  return std::move(dense_vals_);
}

std::vector<BigInt>&& FlexiDatum::MoveSparseIdx() {
  CHECK_EQ(OutputStoreType::SPARSE, store_type_);
  return std::move(sparse_idx_);
}

std::vector<float>&& FlexiDatum::MoveSparseVals() {
  CHECK_EQ(OutputStoreType::SPARSE, store_type_);
  return std::move(sparse_vals_);
}

std::string FlexiDatum::ToString(bool libsvm_string) const {
  if (libsvm_string) {
    return ToLibsvmString();
  }
  return ToFullString();
}

std::string FlexiDatum::ToFullString() const {
  std::stringstream ss;
  ss << (store_type_ == OutputStoreType::SPARSE ?  "sparse" : "dense")
    << " dim: " << feature_dim_
    << " label: " << label_
    << " weight: " << weight_ << " |";
  if (store_type_ == OutputStoreType::SPARSE) {
    for (int i = 0; i < sparse_idx_.size(); ++i) {
      ss << " " << sparse_idx_[i] << ":" << sparse_vals_[i];
    }
  } else {
    for (int i = 0; i < dense_vals_.size(); ++i) {
      ss << " " << i << ":" << dense_vals_[i];
    }
  }
  return ss.str();
}

std::string FlexiDatum::ToLibsvmString() const {
  std::stringstream ss;
  ss << label_;
  if (store_type_ == OutputStoreType::SPARSE) {
    for (int i = 0; i < sparse_idx_.size(); ++i) {
      ss << " " << sparse_idx_[i] << ":" << sparse_vals_[i];
    }
  } else {
    for (int i = 0; i < dense_vals_.size(); ++i) {
      ss << " " << i << ":" << dense_vals_[i];
    }
  }
  return ss.str();
}

FlexiDatumProto FlexiDatum::GetFlexiDatumProto() const {
  FlexiDatumProto proto;
  proto.set_feature_dim(feature_dim_);
  proto.set_label(label_);
  proto.set_weight(weight_);
  proto.set_store_type(store_type_);
  if (store_type_ == OutputStoreType::SPARSE) {
    proto.mutable_sparse_idx()->Resize(sparse_idx_.size(), 0);
    proto.mutable_sparse_vals()->Resize(sparse_vals_.size(), 0);
    for (int i = 0; i < sparse_idx_.size(); ++i) {
      proto.set_sparse_idx(i, sparse_idx_[i]);
      proto.set_sparse_vals(i, sparse_vals_[i]);
    }
  } else {
    proto.mutable_dense_vals()->Resize(dense_vals_.size(), 0);
    for (int i = 0; i < dense_vals_.size(); ++i) {
      proto.set_dense_vals(i, dense_vals_[i]);
    }
  }
  return proto;
}

FlexiDatum::FlexiDatum(const FlexiDatumProto& proto) :
  store_type_(proto.store_type()), feature_dim_(proto.feature_dim()),
  label_(proto.label()), weight_(proto.weight()) {
  if (store_type_ == OutputStoreType::SPARSE) {
    sparse_idx_.resize(proto.sparse_idx_size());
    sparse_vals_.resize(proto.sparse_vals_size());
    for (int i = 0; i < proto.sparse_idx_size(); ++i) {
      sparse_idx_[i] = proto.sparse_idx(i);
      sparse_vals_[i] = proto.sparse_vals(i);
    }
  } else {
    dense_vals_.resize(proto.dense_vals_size());
    for (int i = 0; i < proto.dense_vals_size(); ++i) {
      dense_vals_[i] = proto.dense_vals(i);
    }
  }
}

}  // namespace hotbox
