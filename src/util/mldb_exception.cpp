#include "util/mldb_exception.hpp"

namespace mldb {

MLDBException::MLDBException(const std::string& msg) : msg_(msg) { }

const char* MLDBException::what() const throw() {
  return msg_.c_str();
}

}  // namespace mldb
