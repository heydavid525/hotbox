#pragma once

#include <glog/logging.h>
#include <cmath>
#include <iterparallel/iterparallel.hpp>
#include "adarev_table.hpp"

namespace petuum {
namespace fm {

// Each entry of FTRL-adarevision server table contains: gsum, z,
// n, sqrt_nmax. Primal parameter 'w' are computed lazily upon read (see
// SerializeAsValue). We use beta = 1 always.
struct FtrlServerValue {
  explicit FtrlServerValue(size_t num_elems, float alpha,
      float lambda1, float lambda2)
      : gsum_vec(num_elems), z_vec(num_elems), n_vec(num_elems),
      sqrt_nmax_vec(num_elems), alpha_(alpha),
      lambda1_(lambda1), lambda2_(lambda2) {
      }

  // TODO(wdai): Deserialize alpha_, lambda1_, lambda2_.
  FtrlServerValue(const char* str, size_t sz, size_t num_elems, float alpha,
      float lambda1, float lambda2) : alpha_(alpha),
      lambda1_(lambda1), lambda2_(lambda2) {
    const float* ptr = reinterpret_cast<const float*>(str);
    CHECK_EQ(0, sz / sizeof(float) % 4);
    gsum_vec = std::vector<float>(ptr, ptr + num_elems);
    z_vec = std::vector<float>(ptr + num_elems, ptr + num_elems * 2);
    n_vec = std::vector<float>(ptr + num_elems * 2, ptr + num_elems * 3);
    sqrt_nmax_vec = std::vector<float>(ptr + num_elems * 3,
        ptr + num_elems * 4);
  }

  // TODO(wdai): Serialize alpha_, lambda1_, lambda2_.
  std::string Serialize() const {
    int num_elems = gsum_vec.size();
    CHECK_EQ(num_elems, z_vec.size());
    CHECK_EQ(num_elems, n_vec.size());
    CHECK_EQ(num_elems, sqrt_nmax_vec.size());
    std::string ret;
    ret.reserve(4 * num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(gsum_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(z_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(n_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(sqrt_nmax_vec.data()),
               num_elems * sizeof(float));
    return ret;
  }

  std::string SerializeAsValue() const {
    int num_elems = z_vec.size();
    std::string ret;
    ret.reserve(2 * num_elems * sizeof(float));
    for (size_t i = 0; i < z_vec.size(); ++i) {
      // Do lazy proximal projection.
      float sign = z_vec[i] >= 0 ? 1.0f : -1.0f;
      float w_i = 0.0f;
      if (sign * z_vec[i] > lambda1_) {
        // Use beta = 1.
        w_i = static_cast<float>(-(z_vec[i] - sign * lambda1_) /
            ((1 + sqrt_nmax_vec[i]) / alpha_ + lambda2_));
      }
      ret.append(reinterpret_cast<const char*>(&w_i), sizeof(w_i));
    }
    CHECK_EQ(num_elems, gsum_vec.size());
    ret.append(reinterpret_cast<const char*>(gsum_vec.data()),
               num_elems * sizeof(float));
    return ret;
  }

  std::vector<float> gsum_vec;
  std::vector<float> z_vec;
  std::vector<float> n_vec;
  std::vector<float> sqrt_nmax_vec;

  float alpha_;
  float lambda1_;
  float lambda2_;
};

struct FtrlTableConfig {
  int row_size;
  float alpha;
  float lambda1;
  float lambda2;
};

// FTRL-adarevision table.
class FtrlTable: public iterparallel::TableCore {
 public:
  FtrlTable(const FtrlTableConfig& config)
      : partition_size_(config.row_size),
        alpha_(config.alpha),
        lambda1_(config.lambda1),
        lambda2_(config.lambda2) { }

  void ApplySerializedUpdateBatch(const char* update_batch, size_t sz,
                                  void* server_value) const override {
    CHECK_EQ(0, sz % sizeof(AdarevServerUpdate));
    int num_updates = sz / sizeof(AdarevServerUpdate);
    FtrlServerValue* server_val =
        reinterpret_cast<FtrlServerValue*>(server_value);
    const AdarevServerUpdate* entries =
        reinterpret_cast<const AdarevServerUpdate*>(update_batch);
    for (size_t i = 0; i < num_updates; ++i) {
      int32_t idx = entries[i].idx;
      float g = entries[i].g;
      float base_w = entries[i].base_w;
      float base_gsum = entries[i].base_gsum;
      float g_bck = server_val->gsum_vec[idx] - base_gsum;
      // beta = 1
      float old_eta = alpha_ / (1. + server_val->sqrt_nmax_vec[idx]);
      float new_n = server_val->n_vec[idx] +
          g * g + 2.0f * g * g_bck;
      float new_sqrt_nmax;
      if (new_n > 0.0f) {
        float new_sqrt_n = std::sqrt(new_n);
        new_sqrt_nmax = std::max(new_sqrt_n, server_val->sqrt_nmax_vec[idx]);
      } else {
        new_sqrt_nmax = server_val->sqrt_nmax_vec[idx];
      }
      // beta = 1
      float new_eta = alpha_ / (1. + new_sqrt_nmax);
      float sigma_i =
          1.0f / alpha_ * (new_sqrt_nmax - server_val->sqrt_nmax_vec[idx]);
      server_val->gsum_vec[idx] += g;
      server_val->z_vec[idx] += g - sigma_i * base_w;
      server_val->n_vec[idx] = new_n;
      server_val->sqrt_nmax_vec[idx] = new_sqrt_nmax;
      // AdaptiveRevision update.
      server_val->z_vec[idx] += (new_eta - old_eta) / new_eta * g_bck;
    }
  }

  void CoalesceUpdate(const void* update, void* update_batch) const override {
    const AdarevClientUpdate* upd =
      reinterpret_cast<const AdarevClientUpdate*>(update);
    AdarevUpdateBatch* upd_batch =
        reinterpret_cast<AdarevUpdateBatch*>(update_batch);
    upd_batch->AddUpdate(*upd);
  }

  void* CreateServerValue() const override {
    return new FtrlServerValue(partition_size_, alpha_, lambda1_,
        lambda2_);
  }

  void* CreateUpdateBatch(const void* update) const override {
    const AdarevClientUpdate* upd =
      reinterpret_cast<const AdarevClientUpdate*>(update);
    AdarevUpdateBatch* upd_batch = new AdarevUpdateBatch();
    upd_batch->AddUpdate(*upd);
    return upd_batch;
  }

  void* DeserializeServerValue(const char* str, size_t sz) const override {
    return new FtrlServerValue(str, sz, partition_size_, alpha_, lambda1_,
        lambda2_);
  }

  void* DeserializeValue(const char* str, size_t sz) const override {
    return new AdarevValue(str, sz);
  }

  void DestroyServerValue(void* server_value) const override {
    delete reinterpret_cast<FtrlServerValue*>(server_value);
  }

  void DestroyUpdateBatch(void* update_batch) const override {
    delete reinterpret_cast<AdarevUpdateBatch*>(update_batch);
  }

  void DestroyValue(void* value) const override {
    delete reinterpret_cast<AdarevValue*>(value);
  }

  std::string SerializeServerValue(const void* server_value) const override {
    const FtrlServerValue* server_val =
        reinterpret_cast<const FtrlServerValue*>(server_value);
    return server_val->Serialize();
  }

  std::string SerializeUpdateBatch(const void* update_batch) const override {
    const AdarevUpdateBatch* upd_batch =
        reinterpret_cast<const AdarevUpdateBatch*>(update_batch);
    return upd_batch->Serialize();
  }

  std::string SerializeValue(const void* server_value) const override {
    const FtrlServerValue* server_val =
        reinterpret_cast<const FtrlServerValue*>(server_value);
    return server_val->SerializeAsValue();
  }

 private:
  size_t partition_size_;
  float alpha_;
  float lambda1_;
  float lambda2_;
};
}
}
