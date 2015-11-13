#include "util/util.hpp"
#include "util/proto/util.pb.h"
#include <string>
#include <algorithm>

#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include "util/file_util.hpp"
#include "util/hotbox_exceptions.hpp"
namespace hotbox {

std::string SizeToReadableString(size_t size) {
  double s = static_cast<double>(size);
  const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

  int i = 0;
  while (s > 1024) {
    s /= 1024;
    i++;
  }

  char buf[100];
  sprintf(buf, "%.*f %s", std::max(i, 2), s, units[i]);
  return std::string(buf);
}

std::string SerializeProto(const google::protobuf::Message& msg) {
  std::string data;
  msg.SerializeToString(&data);
  return data;
}


std::string ReadCompressedString(std::string input,
    Compressor compressor) {
  // Uncompress
  if (compressor == Compressor::NO_COMPRESS) {
    return input;
  }
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  std::unique_ptr<CompressorIf> compressor_if =
    registry.CreateObject(compressor);
  try {
    return compressor_if->Uncompress(input);
  } catch (const FailedToUncompressException& e) {
    throw FailedFileOperationException("Failed to uncompress " + input
        + "\n" + e.what());
  }
  // Should never get here.
  return "";
}


std::string ReadCompressedString(const void* data, const int size,
    Compressor compressor) {
  // Uncompress
  if (compressor == Compressor::NO_COMPRESS) {
    std::string ret((const char *)data, size);
    return ret;
  }
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  std::unique_ptr<CompressorIf> compressor_if =
    registry.CreateObject(compressor);
  try {
    return compressor_if->Uncompress(data, size);
  } catch (const FailedToUncompressException& e) {
    throw FailedFileOperationException(std::string("Failed to uncompress ")
        + "\n" + e.what());
  }
  // Should never get here.
  return "";
}



size_t WriteCompressedString(std::string& input,
    Compressor compressor) {
  if (compressor != Compressor::NO_COMPRESS) {
    // Compress always succeed.
    auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
    std::unique_ptr<CompressorIf> compressor_if =
      registry.CreateObject(compressor);
    input = compressor_if->Compress(input);
  }
  return input.size();
}

}  // namespace hotbox
