#pragma once

#include <chrono>

namespace petuum {
namespace fm {

class Timer {
  public:
    Timer();

    void restart();

    // return elapsed time (including previous restart-->pause time) in seconds.
    double elapsed() const;

    // return estimated maximum value for elapsed()
    double elapsed_max() const;

    // return minimum value for elapsed()
    double elapsed_min() const;

  private:
    double total_time_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
    struct timespec start_time_;
};

}  // namespace fm
}  // namespace petuum
