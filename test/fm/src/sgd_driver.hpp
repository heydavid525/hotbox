#pragma once

#include <iterparallel/iterparallel.hpp>
#include "class_loss.hpp"
#include "timer.hpp"

namespace petuum {
namespace fm {

struct TrainResult {
  int epoch;
  ClassLoss loss;
  double train_time_elapsed;
};

struct SspDispatchArgs {
  int32_t clock;
  // True to perform test evaluation.
  bool eval_test = false;
  // train_result is only used by eval_test tasks to print training results
  // after test eval.
  TrainResult train_result;
};

struct SspInformArgs {
  int32_t clock;
  ClassLoss loss;
  bool eval_test = false;
  // train_result is used only by eval_test tasks.
  TrainResult train_result;
};


struct SGDDriverConfig {
  int num_epochs{0};
  int num_clocks_per_epoch{0};
  int staleness{0};
  int num_work_partitions{0};
  int num_batches_per_print{0};
  // True to evaluate test error.
  bool eval_test{false};
};

class SGDDriver: public iterparallel::Driver {
 public:
  SGDDriver(const SGDDriverConfig& config)
      : num_clocks_(config.num_epochs * config.num_clocks_per_epoch),
      num_epochs_(config.num_epochs),
      num_clocks_per_epoch_(config.num_clocks_per_epoch),
      num_batches_per_print_(config.num_batches_per_print),
        num_work_partitions_(config.num_work_partitions),
        eval_test_(config.eval_test),
        staleness_(config.staleness) { }

  iterparallel::Driver* GetCopy() const override {
    return new SGDDriver(*this);
  }

  void HandleInform(int32_t work_partition_id, int32_t dispatch_type_id,
                            const char* args, size_t size) override {
    const SspInformArgs* inform_args =
        reinterpret_cast<const SspInformArgs*>(args);
    int32_t clock = inform_args->clock;
    if (inform_args->eval_test) {
      test_loss_[clock].AddLoss(inform_args->loss);
      ++num_finished_test_[clock];
      if (num_finished_test_[clock] == num_work_partitions_) {
        TrainResult result = inform_args->train_result;
        int batch = clock % num_clocks_per_epoch_ + 1;
        LOG(INFO) << "Epoch " << result.epoch << "/" << num_epochs_
          << " batch " << batch << "/" << num_clocks_per_epoch_
          << " (clock: " << clock << ") TrainTime: "
          << result.train_time_elapsed << " TestTime: " << timer_.elapsed()
          << "\nTrainLoss: " << result.loss
          << "\nTestLoss: " << test_loss_[clock];
      }
      return;
    }
    // Inform from training task.
    loss_[clock].AddLoss(inform_args->loss);
    ++num_finished_[clock];
    if (num_finished_[clock] == num_work_partitions_) {
      if (clock % num_batches_per_print_ == 0) {
        ClassLoss loss;
        for (int32_t b = clock; b > clock - num_clocks_per_epoch_; --b) {
          if (b >= 0) {
            loss.AddLoss(loss_.at(b));
          }
        }
        int epoch = std::ceil(static_cast<float>(clock) /
            num_clocks_per_epoch_);
        if (clock % num_clocks_per_epoch_ == 0) {
          epoch++;
        }
        int batch = clock % num_clocks_per_epoch_ + 1;

        if (eval_test_) {
          TrainResult train_result;
          train_result.loss = loss;
          train_result.train_time_elapsed = timer_.elapsed();
          train_result.epoch = epoch;
          // Dispatch test tasks.
          SspDispatchArgs dispatch_args;
          // evaluation clock.
          dispatch_args.clock = inform_args->clock;
          dispatch_args.eval_test = true;
          dispatch_args.train_result = train_result;
          for (int32_t p = 0; p < num_work_partitions_; ++p) {
            WorkDispatch(p, 0, reinterpret_cast<char*>(&dispatch_args),
                         sizeof(dispatch_args));
          }
        } else {
          LOG(INFO) << "Epoch " << epoch << "/" << num_epochs_
            << " batch " << batch << "/" << num_clocks_per_epoch_
            << " (clock: " << clock << ") Time: " << timer_.elapsed()
            << " " << loss;
        }
      }
      if (inform_args->clock + staleness_ + 1 < num_clocks_) {
        for (int32_t p = 0; p < num_work_partitions_; ++p) {
          SspDispatchArgs dispatch_args;
          dispatch_args.clock = inform_args->clock + staleness_ + 1;
          WorkDispatch(p, 0, reinterpret_cast<char*>(&dispatch_args),
                       sizeof(dispatch_args));
        }
      }
    }
  }

  void Initialize() override {
    for (int32_t c = 0; c < staleness_ + 1; ++c) {
      if (c < num_clocks_) {
        for (int32_t p = 0; p < num_work_partitions_; ++p) {
          SspDispatchArgs dispatch_args;
          dispatch_args.clock = c;
          WorkDispatch(p, 0, reinterpret_cast<char*>(&dispatch_args),
                       sizeof(dispatch_args));
        }
      }
    }
    timer_.restart();
  }

 private:
  // num_finished_[clock] is the number of finished tasks for clock 'clock'.
  std::unordered_map<int32_t, int32_t> num_finished_;
  std::unordered_map<int32_t, int32_t> num_finished_test_;
  const int32_t num_clocks_;
  const int num_epochs_;
  const int num_clocks_per_epoch_;
  const int num_batches_per_print_;
  const int32_t num_work_partitions_;
  const bool eval_test_;
  std::unordered_map<int32_t, ClassLoss> loss_;
  std::unordered_map<int32_t, ClassLoss> test_loss_;
  const int32_t staleness_;
  // Record time elapsed since Initialize().
  Timer timer_;
};

}
}
