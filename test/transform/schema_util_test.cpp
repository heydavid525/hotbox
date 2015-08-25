#include <gtest/gtest.h>
#include "transform/schema_util.hpp"
#include <glog/logging.h>

namespace mldb {
namespace schema_util {

TEST(SchemaUtilTest, SmokeTest) {
  auto pairs = ParseFeatureDesc("feat1, feat2,fam1:feat3, feat4,:feat5");
  EXPECT_EQ(kDefaultFamily, pairs[0].first);
  EXPECT_EQ(kDefaultFamily, pairs[1].first);
  EXPECT_EQ("fam1", pairs[2].first);
  EXPECT_EQ("fam1", pairs[3].first);
  EXPECT_EQ(kDefaultFamily, pairs[4].first);

  EXPECT_EQ("feat1", pairs[0].second);
  EXPECT_EQ("feat2", pairs[1].second);
  EXPECT_EQ("feat3", pairs[2].second);
  EXPECT_EQ("feat4", pairs[3].second);
  EXPECT_EQ("feat5", pairs[4].second);
}

TEST(SchemaUtilTest, ErrorTest) {
  try {
    auto pairs = ParseFeatureDesc(",");
  } catch (MLDBException& e) {
    LOG(INFO) << e.what();
  }
}

}  // namespace schema_util
}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
