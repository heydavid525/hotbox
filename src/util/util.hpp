#pragma once

#include <memory>
#include <string>
#include <google/protobuf/message.h>
#include "db/proto/db.pb.h"
#include <sstream>
#include <iomanip>
#include "io/compressed_streams.hpp"
#include "util/compressor/all.hpp"
#include <glog/logging.h>
#include <limits>
#include <memory>

namespace hotbox {

const size_t kProtoSizeLimitInBytes = 64 * 1024 * 1024;

namespace {

const int buffer_limit = std::numeric_limits<int>::max();

}  // anonymous namespace

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Convert size_t to human-readable (e.g., size = 1024 --> "1KB").
std::string SizeToReadableString(size_t size);

// Serialize proto. Will check that the serialized string does
// not exceeds kProtoSizeLimitInBytes.
std::string SerializeProto(const google::protobuf::Message& msg);

// Similar to SerializeProto, but add snappy compression.
std::string SerializeAndCompressProto(const google::protobuf::Message& msg);

// Uncompress proto_str and deserialize.
template<typename PROTO>
PROTO DeserializeAndUncompressProto(const std::string& proto_str) {
  PROTO proto;
  SnappyCompressor compressor;
  std::string uncompressed = compressor.Uncompress(proto_str);
  CHECK(proto.ParseFromString(uncompressed));
  return proto;
}

// Serialize and compress.
// std::string SerializeProtoAndCompress(const google::protobuf::Message& msg,
//     Compressor compressor = Compressor::SNAPPY);

std::string DecompressString(const std::string& input,
    Compressor compressor = Compressor::SNAPPY);

std::string DecompressString(const void* data, const int size,
    Compressor compressor = Compressor::SNAPPY);

size_t WriteCompressedString(std::string& input,
    Compressor compressor = Compressor::SNAPPY);

// Deserialize stream up to std::numeric_limits<int>::max()
// bytes to PROTO (a proto message). No streaming decompression.
template<typename PROTO>
PROTO StreamDeserialize(const std::string& proto_str) {
  // Read back to another FloatContainer
  SnappyCompressor compressor;
  std::string uncompressed = compressor.Uncompress(proto_str);
  google::protobuf::io::ArrayInputStream istream_arr(
      uncompressed.data(), uncompressed.size());
  google::protobuf::io::CodedInputStream istream_coded(
      &istream_arr);
  istream_coded.SetTotalBytesLimit(buffer_limit, buffer_limit);
  // LOG(INFO) << "StreamDeserialize to size: " << proto_str.size();

  PROTO proto;
  CHECK(proto.ParseFromCodedStream(&istream_coded));
  return proto;
}

// Serialize and snappy compress proto using stream. No streaming
// compression. Optionally return the size of serialized proto (before
// snappy compress).
template<typename PROTO>
std::string StreamSerialize(const PROTO& proto,
    size_t* serialized_size = nullptr) {
  std::string buffer;
  {
    // Write to buffer
    google::protobuf::io::StringOutputStream ostream_str(&buffer);
    google::protobuf::io::CodedOutputStream ostream_coded(
        &ostream_str);
    CHECK(proto.SerializeToCodedStream(&ostream_coded));
  }
  if (serialized_size != nullptr) {
    *serialized_size = buffer.size();
  }
  SnappyCompressor compressor;
  std::string compressed = compressor.Compress(buffer);
  return compressed;
}

/*
// Disable because SnappyCompressStream is buggy.
// 
// Deserialize and decompress stream up to std::numeric_limits<int>::max()
// bytes to PROTO (a proto message).
template<typename PROTO>
PROTO CompressedStreamDeserialize(const std::string& proto_str) {
  // Read back to another FloatContainer
  google::protobuf::io::ArrayInputStream istream_arr(
      proto_str.data(), proto_str.size());
  std::unique_ptr<SnappyInputStream> istream_snappy(
      new SnappyInputStream(&istream_arr));
  google::protobuf::io::CodedInputStream istream_coded(
      istream_snappy.get());
  istream_coded.SetTotalBytesLimit(buffer_limit, buffer_limit);
  LOG(INFO) << "StreamDeserialize to size: " << proto_str.size();

  PROTO proto;
  CHECK(proto.ParseFromCodedStream(&istream_coded));
  return proto;
}

// Serialize and snappy compress proto using stream.
template<typename PROTO>
std::string CompressedStreamSerialize(const PROTO& proto) {
  std::string buffer;
  {
    // Write to buffer
    google::protobuf::io::StringOutputStream ostream_str(&buffer);
    std::unique_ptr<SnappyOutputStream> ostream_snappy(
        new SnappyOutputStream(&ostream_str));
    google::protobuf::io::CodedOutputStream ostream_coded(
        ostream_snappy.get());
    CHECK(proto.SerializeToCodedStream(&ostream_coded));
    ostream_snappy->Flush();
  }
  LOG(INFO) << "StreamSerialize to size: " << buffer.size();
  return buffer;
}
*/

// Convert float/double to limited precision string.
// Comment(wdai): The impl isn't very efficient.
template <typename T>
std::string ToString(T val, int num_decimals = 2) {
  std::ostringstream out;
  out << std::setprecision(num_decimals) << val;
  return out.str();
}

}   // namespace hotbox
