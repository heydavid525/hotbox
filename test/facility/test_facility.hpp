// This file contains facilities for testing.
//#include <boost/filesystem.hpp>
#include <string>
#include "io.dmlc/filesys.h"

namespace hotbox {

// Path to hotbox/db_testbed.
std::string GetTestBedDir() {
	return dmlc::io::FileSystem::parent_path(
  			dmlc::io::FileSystem::parent_path(
    			dmlc::io::FileSystem::path(__FILE__)))
    				.append("/db_testbed");
}

// Path to hotbox/test.
std::string GetTestDir() {
	return dmlc::io::FileSystem::parent_path(
  			dmlc::io::FileSystem::parent_path(
    			dmlc::io::FileSystem::path(__FILE__)))
    				.append("/test");
}

}  // namespace hotbox
