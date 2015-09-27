// This file contains facilities for testing.
#include <boost/filesystem.hpp>
#include <string>

namespace mldb {

// Path to mldb/db_testbed.
std::string GetTestBedDir() {
  return boost::filesystem::path(__FILE__)
    .parent_path().parent_path().parent_path().append("db_testbed").string();
}

// Path to mldb/test.
std::string GetTestDir() {
  return boost::filesystem::path(__FILE__)
    .parent_path().parent_path().parent_path().append("test").string();
}

}  // namespace mldb
