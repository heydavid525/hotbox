#include "util/compressor/snappy_compressor.hpp"
#include "util/mldb_exceptions.hpp"
#include <snappy.h>

namespace mldb {

std::string SnappyCompressor::Compress(const std::string& in)
  const noexcept {
  std::string compressed;
  snappy::Compress(in.c_str(), in.size(), &compressed);
  return compressed;
}

std::string SnappyCompressor::Uncompress(const std::string& in) const {
  std::string uncompressed;
  if (!snappy::Uncompress(in.c_str(), in.size(), &uncompressed)) {
    throw FailedToUncompressException("Snappy uncompress failed.");
  }
  return uncompressed;
}

}  // namespace mldb
