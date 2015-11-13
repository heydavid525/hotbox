#pragma once

#include <memory>
#include <string>
#include <google/protobuf/message.h>
#include "db/proto/db.pb.h"
#include <sstream>
#include <iomanip>

namespace hotbox {

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Convert size_t to human-readable (e.g., size = 1024 --> "1KB").
std::string SizeToReadableString(size_t size);

std::string SerializeProto(const google::protobuf::Message& msg);

std::string ReadCompressedString(std::string input,
    Compressor compressor = Compressor::SNAPPY);

std::string ReadCompressedString(const void* data, const int size,
    Compressor compressor = Compressor::SNAPPY);

size_t WriteCompressedString(std::string& input,
    Compressor compressor = Compressor::SNAPPY);

// Convert float/double to limited precision string.
// Comment(wdai): The impl isn't very efficient.
template <typename T>
std::string ToString(T val, int num_decimals = 2) {
  std::ostringstream out;
  out << std::setprecision(num_decimals) << val;
  return out.str();
}

}   // namespace hotbox
