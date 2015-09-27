#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <iostream>
#include <dmlc/io.h>
#include "test/facility/test_facility.hpp"
#include <sstream>

namespace mldb {

namespace {

const std::string kContent{"Hello World!"};
std::string kTestPath = GetTestBedDir() + "/stream_test_file";

}  // anonymous namespace

TEST(StreamTest, SmokeTest) {
  // We do not own fs.
  {
    dmlc::ostream os(dmlc::Stream::Create(kTestPath.c_str(), "w"));
    os.write(kContent.c_str(), kContent.size());
  }
  {
    dmlc::istream is(dmlc::Stream::Create(kTestPath.c_str(), "r"));
    //std::stringstream buffer;
    // buffer << is.rdbuf();
    // TODO(Weiren): Maybe consider fixing it by implementing better stream
    // (rdbuf).
    // EXPECT_EQ(kContent, buffer.str());
  }
  {
    // Use this method to read the full file.
    dmlc::istream is(dmlc::Stream::Create(kTestPath.c_str(), "r"));
    is.seekg(0, std::ios::end);
    size_t size = is.tellg();
    LOG(INFO) << "size: " << size;
    std::string buffer(size, ' ');
    is.seekg(0);
    is.read(&buffer[0], size);
    EXPECT_EQ(kContent, buffer);
  }
  LOG(INFO) << "stream test passed";
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
