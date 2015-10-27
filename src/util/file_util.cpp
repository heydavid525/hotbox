#include "util/file_util.hpp"
#include "util/hotbox_exceptions.hpp"
#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include <glog/logging.h>
#include <sstream>
#include <memory>

namespace hotbox {
  namespace io {

dmlc::SeekStream* OpenFileStream(const std::string& file_path){
  dmlc::io::URI path(file_path.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  // We do own the file system pointer.
  return fs->OpenForRead(path);
}

std::string ReadCompressedFile(const std::string& file_path,
    Compressor compressor) {
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
  if (nread != size) {
    throw FailedFileOperationException("Failed to read file: " + file_path
        + "\n");
  }
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
        + " " + e.what());
  }
  // Should never get here.
  return "";
}


size_t WriteCompressedFile(const std::string& file_path,
    const std::string& data, Compressor compressor) {
  // We do own this pointer.
  std::unique_ptr<dmlc::Stream> os(dmlc::Stream::Create(file_path.c_str(), "w"));
  if (!os) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for write.");
  }
  if (compressor != Compressor::NO_COMPRESS) {
    LOG(INFO) << "Writing to " << file_path << " using compressor "
      << compressor;
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

std::string ReadFile(const std::string& file_path) {
  return ReadCompressedFile(file_path, Compressor::NO_COMPRESS);
}

bool Exists(const std::string& path) {
  return dmlc::io::FileSystem::exist(path);
}

bool IsDirectory(const std::string& file_path) {
  return dmlc::io::FileSystem::is_directory(file_path);
}

int CreateDirectory(const std::string& file_path) {
  dmlc::io::URI path(file_path.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  return fs->CreateDirectory(path);
}

// Return the path of a file or directory
std::string Path(const std::string& file_path) {
  return dmlc::io::FileSystem::path(file_path);
}

// Return the parent path of a given file or directory.
std::string ParentPath(const std::string& file_path) {
  return dmlc::io::FileSystem::parent_path(file_path);
}

}  // namaspace hotbox::io
}  // namespace hotbox
