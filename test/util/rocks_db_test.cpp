#include "util/rocks_db.hpp"
#include "test/facility/test_facility.hpp"
#include <gtest/gtest.h>
#include <glog/logging.h>

namespace hotbox {

namespace {

std::string kTestDBPath = GetTestDir() + "/rocks_db_test";

}  // anonymous namespace

TEST(RocksDBTest, SmokeTest) {
  RocksDB db(kTestDBPath);
  std::string key = "key1";
  std::string val = "val1";
  db.Put(key, val);
  EXPECT_EQ(val, db.Get(key));
}

}  // namespace hotbox

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
