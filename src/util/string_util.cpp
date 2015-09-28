#include "util/string_util.hpp"
#include <vector>
#include <sstream>
#include <string>

namespace mldb {

std::vector<std::string> SplitString(const std::string& in, char delim) {
  std::stringstream ss(in);
  std::vector<std::string> segments;
  std::string segment;
  while (std::getline(ss, segment, delim)) {
    segments.push_back(segment);
  }
  return segments;
}

}  // namespace mldb
