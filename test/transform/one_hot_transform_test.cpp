#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <numeric>
#include "transform/schema.hpp"
#include "transform/one_hot_transform.hpp"
#include "transform/proto/schema.pb.h"
#include "transform/proto/transforms.pb.h"
#include "test/transform/test_util.hpp"

namespace mldb {

const int kNumCatFeatures = 10;
const int kNumNumFeatures = 10;

TEST(OneHotTransformTest, SmokeTest) {
  SchemaConfig schema_config;
  schema_config.set_int_label(true);
  schema_config.set_use_dense_weight(false);
  Schema schema(schema_config);
  AddCatNumFamily("magic", kNumCatFeatures, kNumNumFeatures, &schema);

  OneHotTransformConfig one_hot_config;
  one_hot_config.set_input_features("magic:cat1+cat2");
  one_hot_config.set_output_feature_family("onehot_magic_cat");
  one_hot_config.set_in_final(true);

  OneHotTransform one_hot_transform(one_hot_config);
  one_hot_transform.TransformSchema(&schema);
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
