#include "io.dmlc/util.hpp"
#include <glog/logging.h>

namespace mldb {
namespace dmlc_util {

std::string ReadFile(const std::string& file_path) {
  dmlc::istream is(dmlc::Stream::Create(file_path.c_str(), "r"));
  is.seekg(0, std::ios::end);
  size_t size = is.tellg();
  std::string buffer(size, ' ');
  is.seekg(0);
  is.read(&buffer[0], size);
  return buffer;
}

}  // namespace dmlc_util
}  // namespace mldb
