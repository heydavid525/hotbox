// This file contains facilities for testing.
#include <boost/filesystem.hpp>
#include <string>

namespace mldb {

std::string GetTestDir() {
  return boost::filesystem::path(__FILE__)
    .parent_path().parent_path().parent_path().append("db_testbed").string();
}

}  // namespace mldb
