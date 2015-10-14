#pragma once

//#include "util/proto/util.pb.h"
#include <string>
#include "db/proto/db.pb.h"
#include "io/filesys.hpp"

namespace hotbox {
	namespace io {

// Read full file and uncompress to string. Throws FailedToReadFileException.
//
// Comment(wdai): We need to use a default compression algorithm for DBFile so
// that we know what compression we use for each DB's atom file.
std::string ReadCompressedFile(const std::string& file_path,
    Compressor compressor = Compressor::SNAPPY);

// Compress and write data to file_path. Return compressed bytes.
size_t WriteCompressedFile(const std::string& file_path,
    const std::string& data, Compressor compressor = Compressor::SNAPPY);

// Read normal data file and return the string
std::string ReadFile(const std::string& file_path);

// Open a file and Return a std::ifstream compatible stream.
dmlc::SeekStream* OpenFileStream(const std::string& file_path);

// Return the path of a file or directory
std::string Path(const std::string& file_path);

// Return the parent path of a given file or directory.
std::string ParentPath(const std::string& file_path);

bool Exists(const std::string& path);
bool IsDirectory(const std::string &path);
int  CreateDirectory(const std::string &path);

}
}  // namespace hotbox
