#include <gtest/gtest.h>
#include <glog/logging.h>
#include "transform/datum_util.hpp"
#include "transform/schema.hpp"
#include "transform/proto/schema.pb.h"
#include "test/transform/test_util.hpp"

namespace mldb {

const int kNumCatFeatures = 5;
const int kNumNumFeatures = 5;

TEST(DatumUtilTest, SmokeTest) {
  SchemaConfig schema_config;
  schema_config.set_int_label(true);
  schema_config.set_use_dense_weight(false);
  Schema schema(schema_config);
  AddCatNumFamily("fam1", kNumCatFeatures, kNumNumFeatures, &schema);
  AddCatNumFamily("fam2", kNumCatFeatures, kNumNumFeatures, &schema);
  LOG(INFO) << "Schema: " << schema.ToString();

  std::string datum1_str = "1 3.2| fam2 | fam1 0:1 2:4 7:1.5";
  DatumBase datum1 = CreateDatumFromFamilyString(schema, datum1_str);
  LOG(INFO) << "datum1: " << datum1.ToString(schema);
  LOG(INFO) << "datum1: " << datum1.ToString();
  EXPECT_NEAR(3.2, datum1.GetWeight(schema), 1e-4);
  EXPECT_EQ(1, datum1.GetLabel(schema));
  EXPECT_EQ(0, datum1.GetFeatureVal(schema, "fam2:0"));

  std::string datum2_str = "1 | fam1 0:1 2:4 7:1.5";
  DatumBase datum2 = CreateDatumFromFamilyString(schema, datum2_str);
  EXPECT_NEAR(1, datum2.GetWeight(schema), 1e-4);
  EXPECT_EQ(1, datum2.GetLabel(schema));
  EXPECT_EQ(0, datum2.GetFeatureVal(schema, "fam2:0"));
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
