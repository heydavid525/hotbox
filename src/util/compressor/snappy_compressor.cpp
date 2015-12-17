#include "util/compressor/snappy_compressor.hpp"
#include "util/hotbox_exceptions.hpp"
#include <snappy.h>

namespace hotbox {

std::string SnappyCompressor::Compress(const std::string& in)
  const noexcept {
  std::string compressed;
  snappy::Compress(in.c_str(), in.size(), &compressed);
  return compressed;
}

std::string SnappyCompressor::Compress(const char* data, size_t len)
  const noexcept {
  	std::string compressed;
  	snappy::Compress(data, len, &compressed);
  	return compressed;
  }

std::string SnappyCompressor::Uncompress(const std::string& in) const {
  std::string uncompressed;
  if (!snappy::Uncompress(in.c_str(), in.size(), &uncompressed)) {
    throw FailedToUncompressException("Snappy uncompress failed.");
  }
  return uncompressed;
}

std::string SnappyCompressor::Uncompress(const char* data, size_t len) const {
	std::string uncompressed;
	if(!snappy::Uncompress((const char*)data, len, &uncompressed)) {
		throw FailedToUncompressException("Snappy uncompress failed.");
	}
	return uncompressed;
}

}  // namespace hotbox
