#include <gtest/gtest.h>
#include <glog/logging.h>
#include "transform/schema_util.hpp"

namespace mldb {

TEST(SchemaUtilTest, SmokeTest) {
  auto finders = ParseFeatureDesc("feat1, feat2,fam1:feat3, feat4,:feat5");
  EXPECT_EQ(5, finders.size());
  EXPECT_EQ(kDefaultFamily, finders[0].family_name);
  EXPECT_EQ(kDefaultFamily, finders[1].family_name);
  EXPECT_EQ("fam1", finders[2].family_name);
  EXPECT_EQ(kDefaultFamily, finders[3].family_name);
  EXPECT_EQ(kDefaultFamily, finders[4].family_name);

  EXPECT_EQ("feat1", finders[0].feature_name);
  EXPECT_EQ("feat2", finders[1].feature_name);
  EXPECT_EQ("feat3", finders[2].feature_name);
  EXPECT_EQ("feat4", finders[3].feature_name);
  EXPECT_EQ("feat5", finders[4].feature_name);

  finders = ParseFeatureDesc(" , fam2:, fam3:feat1 + feat2, fam4:1+2");
  EXPECT_EQ(5, finders.size());
  EXPECT_EQ("fam2", finders[0].family_name);
  EXPECT_EQ("fam3", finders[1].family_name);
  EXPECT_EQ("fam3", finders[2].family_name);
  EXPECT_EQ("fam4", finders[3].family_name);
  EXPECT_EQ("fam4", finders[4].family_name);

  EXPECT_EQ("", finders[0].feature_name);
  EXPECT_TRUE(finders[0].all_family);
  EXPECT_EQ("feat1", finders[1].feature_name);
  EXPECT_EQ("feat2", finders[2].feature_name);
  EXPECT_EQ("", finders[3].feature_name);
  EXPECT_EQ(1, finders[3].family_idx);
  EXPECT_EQ(2, finders[4].family_idx);
}

TEST(SchemaUtilTest, ErrorTest) {
  try {
    auto finders = ParseFeatureDesc(",");
  } catch (MLDBException& e) {
    LOG(INFO) << e.what();
  }
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
