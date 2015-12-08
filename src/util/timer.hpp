#pragma once

#include <chrono>

namespace hotbox {

// Usage:
// Timer timer;   // start timing at construction.
// ....do some work...
// // Get number of seconds elapsed since timer started.
// std::cout << "Time elapsed: " << timer.elapsed();
//
// timer.restart();  // restart the timer.
class Timer {
  public:
    Timer() { restart(); }

    inline void restart() {
      start_time_ = std::chrono::steady_clock::now();
    }

    // return elapsed time (including previous restart-->pause time) in seconds.
    inline float elapsed() const {
      std::chrono::duration<float> elapsed_seconds =
        std::chrono::steady_clock::now() - start_time_;
      return elapsed_seconds.count();
    }

  private:
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
};

} // namespace hotbox
