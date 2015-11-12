#include "util/file_util.hpp"
#include "util/hotbox_exceptions.hpp"
#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include <glog/logging.h>
#include <sstream>
#include <memory>
#include <cmath>

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

size_t WriteSizeLimitedFiles(const std::string& file_dir, int32_t& file_idx,
    const std::string& data) {
  // Configurations.
  // int32_t size_limit = _ATOM_SIZE_MB;
  int32_t curr_atom_idx = file_idx;
  int32_t size_written = 0;
  int32_t data_offset = 0;

  // Get current loop count according to current atom size.
  std::string curr_file_path = file_dir + std::to_string(curr_atom_idx);
  dmlc::io::URI path(curr_file_path.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  dmlc::io::FileInfo info = fs->GetPathInfo(path);
  size_t curr_atom_size = info.size;
  //float curr_X = (float)(data.size() + curr_atom_size)) / (float)_ATOM_SIZE_MB;
  int32_t atom_size_mb = _ATOM_SIZE_MB;
  int32_t curr_X = data.size() + curr_atom_size + atom_size_mb;
  int32_t loop_size = curr_X / atom_size_mb; // MACRO can't be put in demoninator?
  LOG(INFO) << "WriteSizeLimitedFiles: "
            << "Size Limit: " << atom_size_mb << ". "
            << "Current Atom File: " << curr_atom_idx << ". "
            << "Current X: " << curr_X << ". "
            << "Data Size: " << data.size() << ". "
            << "curr_atom_size " << curr_atom_size << ". "
            << "Loop Size: " << loop_size << ". ";
  
  //TODO(weiren): find a non-copy method for compressing data.
  for(int i=0; i < loop_size; i++, curr_atom_idx++) {
    // Write to the current atom file.
    if(i == 0) {
      data_offset = atom_size_mb - curr_atom_size;
      size_written += AppendFile(curr_file_path, data.substr(0, data_offset));
      LOG(INFO) << "Initial Offset for Length: " << data_offset;
    }
    // Write whole size_limit files.
    else if(i < loop_size - 1) {
      curr_file_path = file_dir + std::to_string(curr_atom_idx);
      size_written += AppendFile(curr_file_path, data.substr(data_offset, atom_size_mb));
      data_offset += atom_size_mb;
      LOG(INFO) << "Intermediate Offset for seeking: " << data_offset;
    }
    // Write the left data to a new file.
    else {
      curr_file_path = file_dir + std::to_string(curr_atom_idx);
      //int32_t len = data.size() - data_offset;
      //size_written += AppendFile(curr_file_path, data.substr(data_offset, len));
      size_written += AppendFile(curr_file_path, data.substr(data_offset, atom_size_mb));
      LOG(INFO) << "Final Offset for Seeking. ";// << data_offset;
    }
  }
  file_idx = --curr_atom_idx;
  LOG(INFO) << "file_idx: " << file_idx;
  return size_written;
}

size_t WriteAtomFiles(const std::string& file_dir, int32_t& file_idx,
    const std::string& data, Compressor compressor) {
  // Fisrt Compress then write to separate files.
  if (compressor != Compressor::NO_COMPRESS) {
    LOG(INFO) << "Compressing Atom Data.";
    auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
    std::unique_ptr<CompressorIf> compressor_if = 
                          registry.CreateObject(compressor);
    std::string compressed = compressor_if->Compress(data);
    LOG(INFO) << "Compressed String Len: " << compressed.size(); 
    return WriteSizeLimitedFiles(file_dir, file_idx, compressed);
  } else {
    LOG(INFO) << "Writing UnCompressed Atom Data.";
    return WriteSizeLimitedFiles(file_dir, file_idx, data);
  }
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

size_t AppendFile(const std::string& file_path,
    const std::string& data) {
  // We do own this pointer.
  std::unique_ptr<dmlc::Stream> os(dmlc::Stream::Create(file_path.c_str(), "a"));
  if (!os) {
    throw FailedFileOperationException("Failed to open " + file_path
        + " for write.");
  }
  LOG(INFO) << "Writing to " << file_path;
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
