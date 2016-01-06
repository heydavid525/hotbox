#pragma once

//#include "util/proto/util.pb.h"
#include <string>
#include "db/proto/db.pb.h"
#include "io/filesys.hpp"

// Use this to define file block size.
const int32_t kAtomSizeInBytes = 64*1024*1024;

namespace hotbox {
namespace io {

// Comment(wdai): We need to use a default compression algorithm for DBFile so
// that we know what compression we use for each DB's atom file.

// Read part/whole file and uncompress to string. Throws
// FailedToReadFileException. Implementation using zero_copy_stream_impl of
// protobuf. Designate read_offset position & length of data to read. By
// default start from beginning of file & read through.
std::string ReadCompressedFile(const std::string& file_path,
	Compressor compressor = Compressor::SNAPPY,
	int32_t read_offset = 0, size_t len = 0);

// Read whole data file directly and return the data string.
std::string ReadFile(const std::string& file_path);

// Open a file and Return a smart pointer to a std::istream compatible stream.
std::unique_ptr<dmlc::SeekStream> OpenFileStream(
    const std::string& file_path);

// Compress and write data to file_path. Return compressed bytes.
size_t WriteCompressedFile(const std::string& file_path,
    const std::string& data, Compressor compressor = Compressor::SNAPPY);

// This method writes 'data' to ATOM.curr_atom_id without compression. It
// assumes data.size() <= kAtomSizeInBytes (64MB). Return the new current
// curr_atom_id, which can be curr_atom_id + 1 if current atom file cannot
// contain 'data' entirely.
int WriteAtomFiles(const std::string& file_dir, int curr_atom_id,
    const std::string& data);

// Append data directly to the end of file 'file_path'.
size_t AppendFile(const std::string& file_path, const std::string& data);


// Common File Operations.
// Get the size of the specified file.
size_t GetFileSize(const std::string& file_path);

// Return the path of a file or directory
std::string Path(const std::string& file_path);

// Return the parent path of a given file or directory.
std::string ParentPath(const std::string& file_path);

bool Exists(const std::string& path);
bool IsDirectory(const std::string &path);
int  CreateDirectory(const std::string &path);

}
}  // namespace hotbox
