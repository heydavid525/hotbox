#include <util/rocksdb_util.hpp>
#include "test/facility/test_facility.hpp"

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <iostream>
#include <memory>

namespace hotbox {

namespace {

const std::string kContent{"Hello World!\n"};
std::string kTestPath = GetTestDir() + "/rocksdb_if_test";

}  // anonymous namespace

TEST(StreamTest, SmokeTest) {

  { // Use this method to read the full file, once.
    std::unique_ptr<rocksdb::DB> db(io::OpenRocksMetaDB(kTestPath));
    io::Put(db.get(), "Hello", kContent);
    std::string value;
    io::Get(db.get(), "Hello", &value);
    EXPECT_EQ(kContent, value);
  }
  LOG(INFO) << "rocksdb_util interface test passed";
}

}  // namespace hotbox

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
