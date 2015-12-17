#pragma once
#include <string>

namespace hotbox {

// Interface for compressors.
class CompressorIf {
public:
  virtual ~CompressorIf() { }

  // Return compressed string.
  virtual std::string Compress(const std::string& in) const noexcept = 0;
  virtual std::string Compress(const char* data, size_t len) const noexcept = 0;

  // Return uncompressed string. (Should always succeed)
  virtual std::string Uncompress(const std::string& in) const = 0;
  virtual std::string Uncompress(const char* data, size_t len) const = 0;
};

}  // namespace hotbox
