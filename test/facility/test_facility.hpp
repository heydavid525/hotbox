// This file contains facilities for testing.
#include <string>
#include "util/file_util.hpp"

namespace hotbox {

// Path to hotbox/db_testbed.
std::string GetTestBedDir() {
	return io::ParentPath(io::ParentPath(
  	io::Path(__FILE__))).append("/db_testbed");
}

// Path to hotbox/test.
std::string GetTestDir() {
	return io::ParentPath(io::ParentPath(
      io::Path(__FILE__))).append("/test");
}

std::string GetResourceDir() {
	return io::ParentPath(io::ParentPath(
		io::Path(__FILE__))).append("/test/resource");
}

}  // namespace hotbox
