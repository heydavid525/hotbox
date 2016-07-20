#include <limits>
#include <glog/logging.h>
#include "timer.hpp"

namespace petuum {
namespace fm {

Timer::Timer() : total_time_(0) {
  restart();
}

void Timer::restart() {
  start_ = std::chrono::steady_clock::now();
}

double Timer::elapsed() const {
  std::chrono::duration<double> diff =
    std::chrono::steady_clock::now() - start_;
  return total_time_ + diff.count();
}

double Timer::elapsed_max() const {
  return double((std::numeric_limits<double>::max)());
}

double Timer::elapsed_min() const {
  return 0.0;
}

}  // namespace fm
}  // namespace petuum
