#include <limits>
#include <glog/logging.h>
#include "util/timer.hpp"

namespace hotbox {

Timer::Timer() : total_time_(0) {
  restart();
}

void Timer::restart() {
  int status = clock_gettime(CLOCK_MONOTONIC, &start_time_);
  CHECK_NE(-1, status) << "Couldn't initialize start_time_";
}

double Timer::elapsed() const {
  struct timespec now;
  int status = clock_gettime(CLOCK_MONOTONIC, &now);
  CHECK_NE(-1, status) << "Couldn't get current time.";

  double ret_sec = double(now.tv_sec - start_time_.tv_sec);
  double ret_nsec = double(now.tv_nsec - start_time_.tv_nsec);

  while (ret_nsec < 0) {
    ret_sec -= 1.0;
    ret_nsec += 1e9;
  }
  double ret = ret_sec + ret_nsec / 1e9;
  return total_time_ + ret;
}

double Timer::elapsed_max() const {
  return double((std::numeric_limits<double>::max)());
}

double Timer::elapsed_min() const {
  return 0.0;
}

}   // namespace hotbox
