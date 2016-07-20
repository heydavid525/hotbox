#pragma once

#include <cmath>
#include <string>
#include <sstream>
#include "util.hpp"

namespace petuum {
namespace fm {

// Classification loss.
struct ClassLoss {
  int32_t num_data{0};
  int32_t num_error{0};
  // Used for calibration error, defined as \sum_i p(i) / num_positive.
  // This quantity should get close to 1 when converge, as weights predicted
  // to be positive should be close to the true positive samples.
  int32_t num_positive{0};
  float pred_positive{0.0f};   // \sum_i p(i).
  float sum_log_loss{0.0f};  // sum of log loss.

  void Add(float prob, int y) {
    CHECK_LE(prob, 1);
    CHECK_GE(prob, 0);
    CHECK(y == 0 || y == 1);
    ++num_data;
    num_error += ((prob > 0.5 && y == 0) || (prob <= 0.5 && y == 1)) ? 1 : 0;
    num_positive += y == 1 ? 1 : 0;
    pred_positive += prob;
    sum_log_loss += y == 1 ? -SafeLog(prob) : -SafeLog(1 - prob);
  }

  void AddLoss(const ClassLoss& loss) {
    num_data += loss.num_data;
    num_error += loss.num_error;
    num_positive += loss.num_positive;
    pred_positive += loss.pred_positive;
    sum_log_loss += loss.sum_log_loss;
  }
};

std::ostream& operator<<(std::ostream& stream, const ClassLoss& loss) {
  float true_p = static_cast<float>(loss.num_positive) / loss.num_data;
  // base_log_loss is used to compute normalized entropy (ne). ne < 1 means
  // we are doing better than random guess; ne >= 1 means we didn't learn
  // anything. See 'Practical Lessons from Predicting Clicks on Ads at
  // Facebook (2014)'.
  float base_log_loss =
      -true_p * SafeLog(true_p) - (1 - true_p) * SafeLog(1 - true_p);
  stream << "log loss: " << loss.sum_log_loss / loss.num_data <<
      ", 0-1 loss: " << static_cast<float>(loss.num_error) / loss.num_data
      << ", calibration: " << loss.pred_positive / loss.num_positive <<
      // Normalized entropy.
      ", ne: " << loss.sum_log_loss / loss.num_data / base_log_loss <<
      ", num data: " << loss.num_data;
  return stream;
}

}   // namespace fm
}   // namespace petuum
