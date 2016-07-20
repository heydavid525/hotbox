#pragma once

#include <iterparallel/iterparallel.hpp>
#include <string>
#include <unordered_set>
#include <vector>
#include "sgd_driver.hpp"
#include "adarev_table.hpp"
#include "sparse_vec.hpp"
#include "common.hpp"
#include <cmath>
#include <memory>
#include "util.hpp"
#include "data.hpp"

namespace petuum {
namespace fm {

class LossIf {
public:
  virtual float Loss(float y_hat, float y) const = 0;

  // Derivative of loss (the part that's independent of derivative of y_hat).
  virtual float DLoss(float y_hat, float y) const = 0;
};

// y \in {0, 1}
class LogisticLoss : public LossIf {
public:
  float Loss(float y_hat, float y) const override {
    return y == 0 ? -SafeLog(1 - Sigmoid(y_hat)) : -SafeLog(Sigmoid(y_hat));
  }

  float DLoss(float y_hat, float y) const override {
    float sig_y_hat = Sigmoid(y_hat);
    return y == 0 ? sig_y_hat : sig_y_hat - 1;
  }
};

class SquaredLoss : public LossIf {
public:
  float Loss(float y_hat, float y) const override {
    return (y_hat - y) * (y_hat - y);
  }

  float DLoss(float y_hat, float y) const override {
    return 2 * (y_hat - y);
  }
};

struct FMWorkPartitionConfig {
  DataConfig data_config;
  DataConfig test_data_config;
  int batch_size;
  int row_size;
  int num_factors;
  int64_t feature_dim;
  std::string loss;
  bool use_v;
};

class FMWorkPartition: public iterparallel::WorkPartition {
public:
  // Takes ownership of loss.
  FMWorkPartition(const FMWorkPartitionConfig& config)
    : data_(config.data_config), test_data_(config.test_data_config),
    batch_size_(config.batch_size),
    row_size_(config.row_size), x_dot_v_(config.num_factors),
    feature_dim_(config.feature_dim),
    num_factors_(config.num_factors), use_v_(config.use_v) {
      if (config.loss == "logistic") {
        loss_.reset(new LogisticLoss);
      } else {
        loss_.reset(new SquaredLoss);
      }
      num_clocks_per_epoch_ = data_.GetNumData() / batch_size_;
      CHECK_GT(num_clocks_per_epoch_, 0) << "sample size: "
        << data_.GetNumData() << " batch_size: " << batch_size_;

      // convert set to vector w_keys_.
      num_w_rows_ = std::ceil(static_cast<float>(feature_dim_)
          / row_size_);
    }

  void HandleDispatch(
      int32_t dispatch_type, const char* str, size_t sz) override {
    const SspDispatchArgs& dispatch_args =
      *reinterpret_cast<const SspDispatchArgs*>(str);
    bool eval_test = dispatch_args.eval_test;
    if (eval_test) {
      return HandleTest(dispatch_args);
    } else {
      return HandleTrain(dispatch_args);
    }
  }

  // 'b' is batch_id.
  void HandleTrain(const SspDispatchArgs& dispatch_args) {
    int32_t b = dispatch_args.clock % num_clocks_per_epoch_;
    size_t num_data_per_clock = data_.GetNumData()
      / num_clocks_per_epoch_;

    // Process batch.
    SspInformArgs inform_args;
    inform_args.clock = dispatch_args.clock;
    float batch_loss = 0.;
    if (b == 0) {   // a new epoch.
      data_.Restart();
    }
    //for (size_t j = begin; j < end; ++j) {
    for (size_t i = 0; i < num_data_per_clock; ++i) {
      auto datum = data_.GetNext();
      auto p = ProcessSample(*(datum.first), datum.second);
      batch_loss += p.second;
      inform_args.loss.Add(p.first, datum.second);
    }
    // Inform the driver.
    DriverInform(reinterpret_cast<char*>(&inform_args),
        sizeof(inform_args));
  }

  // Evaluate test error. 'b' is batch_id.
  void HandleTest(const SspDispatchArgs& dispatch_args) {
    int32_t b = dispatch_args.clock % num_clocks_per_epoch_;
    size_t num_data = test_data_.GetNumData();
    // Process batch.
    SspInformArgs inform_args;
    // Pass the train result along.
    inform_args.train_result = dispatch_args.train_result;
    inform_args.clock = dispatch_args.clock;
    inform_args.eval_test = true;
    test_data_.Restart(); // Always restart to sweep the full test set.
    while (test_data_.HasNext()) {
      auto datum = test_data_.GetNext();
      bool update_model = false;
      auto p = ProcessSample(*(datum.first), datum.second, update_model);
      inform_args.loss.Add(p.first, datum.second);
    }
    // Inform the driver.
    DriverInform(reinterpret_cast<char*>(&inform_args),
        sizeof(inform_args));
  }

  float GetW0() {
    return reinterpret_cast<AdarevValue*>(TableGet(kW0TableId, 0))->w_vec[0];
  }

  float GetW(int i) {
    CHECK_LT(i, feature_dim_);
    return reinterpret_cast<AdarevValue*>(TableGet(kWTableId, i / row_size_))
        ->w_vec[i % row_size_];
  }

  float GetV(int f, int i) {
    CHECK_LT(i, feature_dim_);
    CHECK_LT(f, num_factors_);
    int row_id = f * num_w_rows_ + i / row_size_;
    return reinterpret_cast<AdarevValue*>(TableGet(kVTableId, row_id))
        ->w_vec[i % row_size_];
  }

  // Compute \hat{y} according to the formula next to Eq.(5) of Rendle 2012
  // Learning recommender systems with adaptive regularization.
  float Predict(const SparseVec& x) {
    // 0-th order w0
    float y_hat = GetW0();

    // 1st order interaction \sum_i w_i x_i
    const auto& ids = x.GetIds();
    const auto& vals = x.GetVals();
    for (int i = 0; i < ids.size(); ++i) {
      y_hat += vals[i] * GetW(ids[i]);
    }

    if (use_v_) {
      // 2nd order
      float order2 = 0.;
      for (int f = 0; f < num_factors_; ++f) {
        // a = \sum_i v_{i,f} x_i
        float a = 0.;
        //for (auto& p : x.GetPairs()) {
        for (int i = 0; i < ids.size(); ++i) {
          a += vals[i] * GetV(f, ids[i]);
        }
        x_dot_v_[f] = a;
        order2 += a*a;

        // a2 = \sum_i (v_{i,f} x_i)^2
        float a2 = 0.;
        //for (auto& p : x.GetPairs()) {
        for (int i = 0; i < ids.size(); ++i) {
          float term = vals[i] * GetV(f, ids[i]);
          a2 += term * term;
        }
        order2 -= a2;
      }
      y_hat += 0.5 * order2;
    }
    return y_hat;
  }

  std::string Serialize() const override {
    return "";
  }

  // Return (probability, loss)
  std::pair<float,float> ProcessSample(const SparseVec& x, int32_t y,
      bool update_model = true) {
    // Note that Predict sets x_dot_v_[f].
    float y_hat = Predict(x);
    float loss = loss_->Loss(y_hat, y);
    float dloss = loss_->DLoss(y_hat, y);

    // Update w0
    AdarevClientUpdate update;
    update.idx = 0;
    update.g = dloss;
    update.base_value = reinterpret_cast<AdarevValue*>(TableGet(kW0TableId, 0));
    TableUpdate(kW0TableId, 0, &update);

    // Update w
    const auto& ids = x.GetIds();
    const auto& vals = x.GetVals();
    for (int i = 0; i < ids.size(); ++i) {
      update.idx = ids[i] % row_size_;
      update.g = vals[i] * dloss;
      int row_id = ids[i] / row_size_;
      update.base_value =
          reinterpret_cast<AdarevValue*>(TableGet(kWTableId, row_id));
      TableUpdate(kWTableId, row_id, &update);
    }

    if (use_v_) {
      // Update v
      for (int f = 0; f < num_factors_; ++f) {
        for (int i = 0; i < ids.size(); ++i) {
          update.idx = ids[i] % row_size_;
          update.g = vals[i] * (x_dot_v_[f] - vals[i] * GetV(f, ids[i]))
            * dloss;
          int row_id = f * num_w_rows_ + ids[i] / row_size_;
          update.base_value =
              reinterpret_cast<AdarevValue*>(TableGet(kVTableId, row_id));
          TableUpdate(kVTableId, row_id, &update);
        }
      }
    }

    float prob = Sigmoid(y_hat);
    return std::make_pair(prob, loss);
  }

private:
  Data data_;
  Data test_data_;
  int num_w_rows_;
  int num_factors_;
  const int batch_size_;
  int num_clocks_per_epoch_;
  const int row_size_;
  // x_dot_v_[f] = \sum_i x_i * v_{i,f}
  std::vector<float> x_dot_v_;
  int feature_dim_;
  std::unique_ptr<LossIf> loss_;

  // true to enable second order interaction.
  bool use_v_;
};

class FMWorkPartitionFactory: public iterparallel::WorkPartitionFactory {
public:
  FMWorkPartitionFactory(const FMWorkPartitionConfig& partition_config)
    :  partition_config_(partition_config) { }

  iterparallel::WorkPartition* Create(
      int32_t work_partition_id) const override {
    FMWorkPartitionConfig config = partition_config_;
    config.data_config.work_partition_id = work_partition_id;
    return new FMWorkPartition(config);
  }

  iterparallel::WorkPartition* Deserialize(
      int32_t work_partition_id, const char* str, size_t sz) const override {
    FMWorkPartitionConfig config = partition_config_;
    config.data_config.work_partition_id = work_partition_id;
    return new FMWorkPartition(config);
  }

private:
  FMWorkPartitionConfig partition_config_;
};

} // namespace fm
} // namespace petuum
