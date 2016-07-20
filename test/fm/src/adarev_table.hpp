#pragma once

#include <glog/logging.h>
#include <cmath>
#include <iterparallel/iterparallel.hpp>

namespace petuum {
namespace fm {

// AdarevClientUpdate are aggregated to AdarevServerUpdate, which is then sent
// to server and applied (per-entry level).
struct AdarevServerUpdate {
  AdarevServerUpdate() : idx(-1), g(0) { }

  int32_t idx;
  float g;
  float base_w;
  float base_gsum;
};

// Adarev row for the client include parameter 'w' and gradient sum
// 'gsum'
struct AdarevValue {
  AdarevValue(const char* str, size_t sz)
      : w_vec(reinterpret_cast<const float*>(str),
              reinterpret_cast<const float*>(str + sz / 2)),
        gsum_vec(reinterpret_cast<const float*>(str + sz / 2),
                 reinterpret_cast<const float*>(str + sz)) { }

  std::vector<float> w_vec;
  std::vector<float> gsum_vec;
};

// Update on a single entry, issued by the client.
struct AdarevClientUpdate {
  int32_t idx;  // column id
  float g;      // gradient
  AdarevValue* base_value;
};

namespace {

// # of value types stored in server table.
const int kNumValTypes = 4;

}  // anonymous namespace

// Each entry of Adarevision server table contains: w (parameters), gsum, z,
// sqrt_zmax. Support L2 regularization (lambda2).
struct AdarevServerValue {
  explicit AdarevServerValue(size_t num_elems)
      : w_vec(num_elems), gsum_vec(num_elems), z_vec(num_elems, 1.),
      sqrt_zmax_vec(num_elems, 1.) { }

  AdarevServerValue(const char* str, size_t sz) {
    const float* ptr = reinterpret_cast<const float*>(str);
    CHECK_EQ(0, (sz / sizeof(float)) % kNumValTypes);
    int num_elems = sz / kNumValTypes;
    w_vec = std::vector<float>(ptr, ptr + num_elems);
    gsum_vec = std::vector<float>(ptr + num_elems, ptr + num_elems * 2);
    z_vec = std::vector<float>(ptr + num_elems * 2, ptr + num_elems * 3);
    sqrt_zmax_vec = std::vector<float>(ptr + num_elems * 3, ptr + num_elems * 4);
  }

  std::string Serialize() const {
    int num_elems = w_vec.size();
    CHECK_EQ(num_elems, gsum_vec.size());
    CHECK_EQ(num_elems, z_vec.size());
    CHECK_EQ(num_elems, sqrt_zmax_vec.size());
    std::string ret;
    ret.reserve(kNumValTypes * num_elems * sizeof(float) + sizeof(float));
    ret.append(reinterpret_cast<const char*>(w_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(gsum_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(z_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(sqrt_zmax_vec.data()),
               num_elems * sizeof(float));
    return ret;
  }

  // Return string representing client-side AdarevValue.
  std::string SerializeAsValue() const {
    size_t num_elems = w_vec.size();
    std::string ret;
    ret.reserve(2 * num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(w_vec.data()),
               num_elems * sizeof(float));
    ret.append(reinterpret_cast<const char*>(gsum_vec.data()),
               num_elems * sizeof(float));
    return ret;
  }

  std::vector<float> w_vec;
  std::vector<float> gsum_vec;
  std::vector<float> z_vec;
  std::vector<float> sqrt_zmax_vec;
};

// Aggregate AdarevClientUpdate and maintain sparsity.
struct AdarevUpdateBatch {
  void AddUpdate(const AdarevClientUpdate& update) {
    const AdarevValue* value = update.base_value;
    AdarevServerUpdate& entry = entries[update.idx];
    if (entry.idx == -1) {
      entry.idx = update.idx;
      entry.base_w = value->w_vec[update.idx];
      entry.base_gsum = value->gsum_vec[update.idx];
    }
    entry.g += update.g;
  }

  std::string Serialize() const {
    size_t nnz = entries.size();
    std::string ret;
    ret.reserve(nnz * sizeof(AdarevServerUpdate));
    for (auto& iter : entries) {
      ret.append(
          reinterpret_cast<const char*>(&iter.second),
          sizeof(AdarevServerUpdate));
    }
    return ret;
  }

 private:
  std::unordered_map<int32_t, AdarevServerUpdate> entries;
};

struct AdarevTableConfig {
  int row_size;
  float alpha;
  float lambda2;
};

// Server table.
// Comment(wdai): We only perform L2-regularization on dimension with non-zero
// gradient.
class AdarevTable: public iterparallel::TableCore {
 public:
  AdarevTable(const AdarevTableConfig& config)
      : partition_size_(config.row_size),
        alpha_(config.alpha), lambda2_(config.lambda2) { }

  // update_batch is the result of AdarevBatch.Serialize().
  void ApplySerializedUpdateBatch(const char* update_batch, size_t sz,
                                  void* server_value) const override {
    CHECK_EQ(0, sz % sizeof(AdarevServerUpdate));
    int num_updates = sz / sizeof(AdarevServerUpdate);
    AdarevServerValue* server_val =
        reinterpret_cast<AdarevServerValue*>(server_value);
    const AdarevServerUpdate* updates =
        reinterpret_cast<const AdarevServerUpdate*>(update_batch);
    for (int i = 0; i < num_updates; ++i) {
      int32_t idx = updates[i].idx;
      float g = updates[i].g + lambda2_ * server_val->w_vec[idx];
      float g_bck = server_val->gsum_vec[idx] - updates[i].base_gsum;
      float eta_old = alpha_ / server_val->sqrt_zmax_vec[idx];
      server_val->z_vec[idx] += g * g + 2 * g * g_bck;
      server_val->sqrt_zmax_vec[idx] = std::max(
          server_val->sqrt_zmax_vec[idx],
          std::sqrt(server_val->z_vec[idx]));
      float eta = alpha_ / server_val->sqrt_zmax_vec[idx];
      server_val->w_vec[idx] += -eta * g + (eta_old - eta) * g_bck;
      server_val->gsum_vec[idx] += g;
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
    return new AdarevServerValue(partition_size_);
  }

  void* CreateUpdateBatch(const void* update) const override {
    const AdarevClientUpdate* upd =
      reinterpret_cast<const AdarevClientUpdate*>(update);
    AdarevUpdateBatch* upd_batch = new AdarevUpdateBatch();
    upd_batch->AddUpdate(*upd);
    return upd_batch;
  }

  void* DeserializeServerValue(const char* str, size_t sz) const override {
    return new AdarevServerValue(str, sz);
  }

  void* DeserializeValue(const char* str, size_t sz) const override {
    return new AdarevValue(str, sz);
  }

  void DestroyServerValue(void* server_value) const override {
    delete reinterpret_cast<AdarevServerValue*>(server_value);
  }

  void DestroyUpdateBatch(void* update_batch) const override {
    delete reinterpret_cast<AdarevUpdateBatch*>(update_batch);
  }

  void DestroyValue(void* value) const override {
    delete reinterpret_cast<AdarevValue*>(value);
  }

  std::string SerializeServerValue(const void* server_value) const override {
    const AdarevServerValue* server_val =
        reinterpret_cast<const AdarevServerValue*>(server_value);
    return server_val->Serialize();
  }

  std::string SerializeUpdateBatch(const void* update_batch) const override {
    const AdarevUpdateBatch* upd_batch =
        reinterpret_cast<const AdarevUpdateBatch*>(update_batch);
    return upd_batch->Serialize();
  }

  std::string SerializeValue(const void* server_value) const override {
    const AdarevServerValue* server_val =
        reinterpret_cast<const AdarevServerValue*>(server_value);
    return server_val->SerializeAsValue();
  }

 private:
  size_t partition_size_;  // # of columns
  float alpha_;
  float lambda2_;
};

}   // namespace fm
}   // namespace petuum
