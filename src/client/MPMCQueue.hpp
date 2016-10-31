#pragma once

#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>

namespace hotbox {

  // allows specifying busy wait read:
  // ioloop busy retrying to read until queue is empty because we know it will
  // always have tasks, wait on bound limit
  // tfloop wait on empty queue, or on bound limit
  enum QueueType { IO, TF, NORMAL }; 
  // only used with small int/pointer, not implemeting move semantic
  template <QueueType Q, typename T>
    class MPMCQueue;

  template <typename T>
    class MPMCQueue<QueueType::NORMAL, T> {
      public:
        MPMCQueue(int limit) : limit_(limit) {};
        void blockingRead(T& elem) {
          {
            std::lock_guard<std::mutex> lock(mtx_);
            if (!queue_.empty()) {
              elem = queue_.front();
              queue_.pop();
              write_cv_.notify_one();
              return;
            }
          }
          {
            std::unique_lock<std::mutex> lock(mtx_);
            read_cv_.wait(lock);
            elem = queue_.front();
            queue_.pop();
            write_cv_.notify_one();
            return;
          }
        };

        void blockingWrite(T elem) {
          int csize = 0;
          {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(elem);
            read_cv_.notify_one();
            csize = queue_.size();
          }
          if (csize >= limit_) {
            std::unique_lock<std::mutex> lock(wait_mtx_);
            write_cv_.wait(lock);
          }
          return;
        };

        int size() {
          std::lock_guard<std::mutex> lock(mtx_);
          return queue_.size();
        };

        bool isEmpty() {
          std::lock_guard<std::mutex> lock(mtx_);
          return queue_.empty();
        }

      private:
        std::queue<T> queue_;
        std::mutex mtx_, wait_mtx_;
        std::condition_variable read_cv_, write_cv_;
        const int limit_;
    };
}
