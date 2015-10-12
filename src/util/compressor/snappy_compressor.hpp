#pragma once
#include "util/compressor/compressor_if.hpp"

namespace hotbox {

// Use Google's snappy to compress.
class SnappyCompressor : public CompressorIf {
public:
  std::string Compress(const std::string& in) const noexcept override;
  std::string Uncompress(const std::string& in) const override;
};

}  // namespace hotbox
