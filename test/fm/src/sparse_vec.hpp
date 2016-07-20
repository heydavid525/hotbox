#pragma once
#include <glog/logging.h>
#include <utility>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace petuum {
namespace fm {

// Represent sparse vector by storing as vector of (id, val) pairs (member
// 'pairs_'). pairs_ need not be sorted for FTRL implementation.
class SparseVec {
public:
  SparseVec() {}

  // ids needs to be in ascending order. This is not checked.
  SparseVec(const std::vector<int64_t>& ids, const std::vector<float>& vals) :
  ids_(ids), vals_(vals) {
    CHECK_EQ(ids_.size(), vals_.size());
  }

  SparseVec(std::vector<int64_t>&& ids, std::vector<float>&& vals) :
  ids_(ids), vals_(vals) {
    CHECK_EQ(ids_.size(), vals_.size());
  }

  inline const std::vector<int64_t>& GetIds() const {
    return ids_;
  }

  inline const std::vector<float>& GetVals() const {
    return vals_;
  }

  // Dot product with dense model. pairs_ doesn't need to be sorted.
  inline float Dot(const std::vector<float>& w) const {
    float sum = 0.;
    for (int i = 0; i < ids_.size(); ++i) {
      sum += w[ids_[i]] * vals_[i];
    }
    return sum;
  }

  // \sum_i (w_i*x_i)^2
  inline float DotSq(const std::vector<float>& w) const {
    float sum = 0.;
    for (int i = 0; i < ids_.size(); ++i) {
      float a = w[ids_[i]] * vals_[i];
      sum += a * a;
    }
    return sum;
  }

private:
  std::vector<int64_t> ids_;
  std::vector<float> vals_;
};

}  // namespace fm
}  // namespace petuum
