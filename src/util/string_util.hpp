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

// trim from both ends
inline std::string Trim(const std::string& s) {
  auto s_trim = s;
  // trim from start
  s_trim.erase(s_trim.begin(), std::find_if(s_trim.begin(), s_trim.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace))));

  // trim from end
  s_trim.erase(std::find_if(s_trim.rbegin(), s_trim.rend(),
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s_trim.end());
  return s_trim;
}

}  // namespace mldb
