#pragma once

#include <dmlc/io.h>
#include <string>

namespace mldb {
namespace dmlc_util {

// DMLC Stream doesn't have full support for stream. Use utility for safety.

// Read full ASCII file to string.
std::string ReadFile(const std::string& file_path);

}  // namespace dmlc_util
}  // namespace mldb
