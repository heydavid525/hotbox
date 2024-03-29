#include "util/file_util.hpp"
#include "util/hotbox_exceptions.hpp"
#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include <glog/logging.h>
#include <sstream>
#include <memory>
#include <cmath>

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <io/general_fstream.hpp>

namespace hotbox {
namespace io {

std::unique_ptr<dmlc::SeekStream> OpenFileStream(
    const std::string& file_path) {
  std::unique_ptr<dmlc::SeekStream> sk
        (dmlc::SeekStream::CreateForRead(file_path.c_str())); 
  if (!sk) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for read.");
  }
  return sk;
}

// Implementation using zero_copy_stream_impl of protobuf.
std::string ReadCompressedFile(const std::string& file_path,
  Compressor compressor, int32_t read_offset, size_t len) {
  {
  // sk is a smart pointer. We do own the SeekStream pointer.
  // auto sk = OpenFileStream(file_path.c_str()); 
  // dmlc::istream does not own the pointer.
  // dmlc::istream in(sk.get());
  } 
  petuum::io::ifstream in(file_path, std::ifstream::binary);
  CHECK(in) << "Failed to open " << file_path;
  // By default, read the whole file.
  if (len == 0) {
    len = GetFileSize(file_path);
  }
  auto fp = make_unique<google::protobuf::io::IstreamInputStream>
      (dynamic_cast<std::basic_istream<char>*>(&in), len);

  const char* buffer;
  int size;
  fp->Skip(read_offset); // offset to start reading.
  // Read a bulk of size len, into buffer, length returned in size
  bool b_succeed = fp->Next(reinterpret_cast<const void**>(&buffer), &size);
  if (!b_succeed || (len != size)) {
    throw FailedFileOperationException("Failed to read file: " + file_path
        + "\n");
  }
  return DecompressString(buffer, size, compressor);
}

int WriteAtomFiles(const std::string& file_dir, int curr_atom_id,
    const std::string& data) {
  int32_t size_written = 0;
  std::string curr_file_path = file_dir +
    std::to_string(curr_atom_id);
  size_t curr_atom_size = GetFileSize(curr_file_path);
  int32_t data_offset = kAtomSizeInBytes - curr_atom_size;
  // Assume that an atom obj will never excceed kAtomSizeInBytes, 
  // and thus span at most 2 files.
  size_written += AppendFile(curr_file_path,
      data.substr(0, data_offset));
  if (data_offset < data.size()) {
    curr_file_path = file_dir + std::to_string(++curr_atom_id);
    size_written += AppendFile(curr_file_path,
        data.substr(data_offset, kAtomSizeInBytes));
  }
  return curr_atom_id;
}

size_t WriteCompressedFile(const std::string& file_path,
    const std::string& data, Compressor compressor) {
  // We do own this pointer.
  std::unique_ptr<dmlc::Stream> os(dmlc::Stream::Create(
        file_path.c_str(), "w"));
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

size_t AppendFile(const std::string& file_path,
    const std::string& data) {
  // We do own this pointer.
  std::unique_ptr<dmlc::Stream> os(dmlc::Stream::Create(file_path.c_str(),
        "a"));
  if (!os) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for write.");
  }
  os->Write(data.c_str(), data.size());
  return data.size();
}

std::string ReadFile(const std::string& file_path) {
  return ReadCompressedFile(file_path, Compressor::NO_COMPRESS,0,0);
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

size_t GetFileSize(const std::string& file_path) {
  dmlc::io::URI path(file_path.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  dmlc::io::FileInfo info = fs->GetPathInfo(path);
  return info.size;
}

}  // namaspace hotbox::io
}  // namespace hotbox
