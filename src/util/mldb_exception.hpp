#pragma once
#include <exception>
#include <string>

namespace mldb {

// A custom exception with error message.
class MLDBException: public std::exception {
public:
  MLDBException(const std::string& msg);
  virtual const char* what() const throw();

private:
  std::string msg_;
};

}  // namespace mldb
