#include "io/fstream.hpp"
#include "util/file_util.hpp"
#include "util/mldb_exceptions.hpp"
#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include <glog/logging.h>
#include <sstream>
#include <memory>
#include "io.dmlc/filesys.h"

namespace mldb {

std::string ReadCompressedFile(const std::string& file_path,
    Compressor compressor) {
  /*
  // Comment(wdai): This doesn't work because size becomes very large due to
  // some corruption.
  dmlc::istream is(dmlc::Stream::Create(file_path.c_str(), "r"));
  is.seekg(0, std::ios::end);
  size_t size = is.tellg();
  std::string buffer(size, ' ');
  is.seekg(0);
  is.read(&buffer[0], size);
  return buffer;
  */

  // Comment(wdai): There's a lot of copying. Optimize it! Check out 
  // zero_copy_stream.h
  // (https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.io.zero_copy_stream?hl=en)

  // Read  
  dmlc::io::URI path(file_path.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  dmlc::io::FileInfo info = fs->GetPathInfo(path);
  // We do own the file system pointer.
  std::unique_ptr<dmlc::SeekStream> fp(fs->OpenForRead(path));
  if (!fp) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for read.");
  }
  size_t size = info.size;
  std::string buffer(size, ' ');
  size_t nread = fp->Read(&buffer[0], size);
/*
  io::ifstream is(file_path);
  if (!is) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for read.");
  }
  std::stringstream buffer;
  buffer << is.rdbuf();
*/
  // Uncompress
  if (compressor == Compressor::NO_COMPRESS) {
    return buffer;
  }
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  std::unique_ptr<CompressorIf> compressor_if =
    registry.CreateObject(compressor);
  try {
    return compressor_if->Uncompress(buffer);
  } catch (const FailedToUncompressException& e) {
    throw FailedFileOperationException("Failed to uncompress " + file_path
        + "\n" + e.what());
  }
  // Should never get here.
  return "";
}

size_t WriteCompressedFile(const std::string& file_path,
    const std::string& data, Compressor compressor) {
/*  
  io::ofstream out(file_path, std::ios::out | std::ios::binary);
  if (!out) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for write.");
  }
*/  
  // We do own this pointer.
  std::unique_ptr<dmlc::Stream> os(dmlc::Stream::Create(file_path.c_str(), "w"));
  if (!os) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for write.");
  }
  if (compressor != Compressor::NO_COMPRESS) {
    // Compress always succeed.
    auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
    std::unique_ptr<CompressorIf> compressor_if =
      registry.CreateObject(compressor);
    std::string compressed = compressor_if->Compress(data);
    os->Write(compressed.c_str(), compressed.size());
    return compressed.size();
  }
  os->Write(data.c_str(), data.size());
  return data.size();
}

}  // namespace mldb
