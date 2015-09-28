#pragma once

#include <memory>
#include <string>

namespace mldb {

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Convert size_t to human-readable (e.g., size = 1024 --> "1KB").
std::string SizeToReadableString(size_t size);

}   // namespace mldb
