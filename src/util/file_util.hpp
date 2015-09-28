#pragma once

//#include "util/proto/util.pb.h"
#include <string>
#include "db/proto/db.pb.h"

namespace mldb {

// Read full file and uncompress to string. Throws FailedToReadFileException.
//
// Comment(wdai): We need to use a default compression algorithm for DBFile so
// that we know what compression we use for each DB's atom file.
std::string ReadCompressedFile(const std::string& file_path,
    Compressor compressor = Compressor::SNAPPY);

// Compress and write data to file_path. Return compressed bytes.
size_t WriteCompressedFile(const std::string& file_path,
    const std::string& data, Compressor compressor = Compressor::SNAPPY);

}  // namespace mldb
