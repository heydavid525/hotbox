#include <algorithm>
#include <string>
#include <functional>
#include <cctype>
#include <locale>

namespace mldb {

// Example: SplitString("a,bc,d", ',') --> ["a", "bc", "d"].
std::vector<std::string> SplitString(const std::string& in, char delim) {
  std::stringstream ss(in);
  std::vector<std::string> segments;
  std::string segment;
  while (std::getline(ss, segment, delim)) {
    segments.push_back(segment);
  }
  return segments;
}

// trim new line, whitespace, tab from both ends, and other optional
// characters.
inline std::string Trim(const std::string& s, const std::string& targets = "") {
  auto s_trim = s;
  // trim from end
  s_trim.erase(s_trim.find_last_not_of(" \n\r\t" + targets) + 1);

  // trim from start
  s_trim.erase(0, s_trim.find_first_not_of(" \n\r\t" + targets));
  return s_trim;
}
}  // namespace mldb
