#include "util/util.hpp"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include "test/io/test_proto.pb.h"
#include "schema/proto/schema.pb.h"
#include "util/timer.hpp"
#include "util/register.hpp"
#include <fstream>
#include <streambuf>
#include "test/facility/test_facility.hpp"
#include <google/protobuf/text_format.h>

namespace hotbox {

namespace {

// store 0,....,test_size in the test container.
const int32_t kTestSize = 1e6;

}  // anonymous namespace

TEST(UtilTest, SmokeTest) {
  FeatureSegment seg;
  std::ifstream t(GetResourceDir() + "/schema_seg.prototxt");
  std::string proto_str((std::istreambuf_iterator<char>(t)),
      std::istreambuf_iterator<char>());
  EXPECT_TRUE(google::protobuf::TextFormat::ParseFromString(proto_str,
        &seg));
  std::string seg_proto_str = StreamSerialize(seg);
  LOG(INFO) << "seg_proto serialized size: " << seg_proto_str.size();
  FeatureSegment seg2 = StreamDeserialize<FeatureSegment>(seg_proto_str);
  EXPECT_EQ(seg.DebugString(), seg2.DebugString());
  LOG(INFO) << "segment has " << seg.features_size() << " features PASSED!";
}

/*
 * This test demonstrates that stream de-/serialization is better than the
 * non-stream version. Using kTestSize = 5e8, the serialization time for
 * stream version is 3s, less than 5.8s by non-stream. Decompress +
 * Deserialization time for stream is 10.8s, while decompress time for
 * non-stream is 7.7s. Note that there are 3 large malloc in non-stream
 * version, but only one in stream version. The stream version produces a
 * slightly larger protobuf str (~0.16% larger) as it breaks up the large
 * stream to multiple batches to compress.
 *
 * Non-stream version (which only decompress but not deserialize):
 * tcmalloc: large alloc 2000003072 bytes == 0x79242000 @  0x2ab7f6e75172
 * 0x2ab7f85b8c49
 * tcmalloc: large alloc 2333335552 bytes == 0xf059c000 @  0x2ab7f6e75172
 * 0x2ab7f85b8c49
 * WARNING: Logging before InitGoogleLogging() is written to STDERR
 * I1124 13:56:24.835360 29689 util_test.cpp:62] Serialized in 5.81243 secs,
 * size: 93902603
 * tcmalloc: large alloc 2000003072 bytes == 0x79242000 @  0x2ab7f6e75172
 * 0x2ab7f85b8c49
 * I1124 13:56:26.805279 29689 util_test.cpp:65] Decompressed in 7.78241 secs
 *
 * Stream version:
 * I1124 13:55:03.101788 29329 util_test.cpp:45] proto_str size: 94056482
 * stream-serialized in 3.00155 secs
 * tcmalloc: large alloc 2147491840 bytes == 0x10206c000 @  0x2b8338fef392
 * 0x41a2c2 0x41b53b 0x2b83392efec0 0x79fd8000
 * I1124 13:55:13.923771 29329 util_test.cpp:49] Stream-Deserialized in
 * 10.8217 secs
 *
 * Using kTestSize = 1e7:
 *
 * I1124 14:00:40.306367 30148 util_test.cpp:56] proto_str size: 1883107
 * stream-serialized in 0.0504174 secs
 * I1124 14:00:40.769786 30148 util_test.cpp:60] Stream-Deserialized in
 * 0.463138 secs
 * I1124 14:00:40.849031 30148 util_test.cpp:65] Serialized in 0.0791861 secs,
 * size: 1878066
 * I1124 14:00:40.888206 30148 util_test.cpp:68] Decompressed in 0.118364 secs
 * I1124 14:00:41.151870 30148 util_test.cpp:72] Decompress + deserialize in
 * 0.382024 secs
 */
TEST(UtilTest, PerfTest) {
  RegisterAll();
  FloatContainer float_container;
  float_container.mutable_values()->Resize(kTestSize, 0);
  for (int i = 0; i < kTestSize; ++i) {
    float_container.set_values(i, 1);
  }

  Timer timer;
  std::string proto_str = StreamSerialize(float_container);
  LOG(INFO) << "proto_str size: " << proto_str.size() << " stream-serialized in "
    << timer.elapsed() << " secs";
  timer.restart();
  FloatContainer float_container2 = StreamDeserialize<FloatContainer>(proto_str);
  LOG(INFO) << "Stream-Deserialized in " << timer.elapsed() << " secs";

  timer.restart();
  proto_str = CompressedStreamSerialize(float_container);
  LOG(INFO) << "proto_str size: " << proto_str.size()
    << " compressed-stream-serialized in " << timer.elapsed() << " secs";
  timer.restart();
  FloatContainer float_container3 =
    CompressedStreamDeserialize<FloatContainer>(proto_str);
  LOG(INFO) << "Compressed-Stream-Deserialized in " << timer.elapsed()
    << " secs";

  timer.restart();
  std::string serialized_str = SerializeAndCompressProto(float_container);
  LOG(INFO) << "Serialized in " << timer.elapsed() << " secs, size: "
    << serialized_str.size();
  FloatContainer container =
    DeserializeAndUncompressProto<FloatContainer>(serialized_str);
  LOG(INFO) << "Decompress + deserialize in " << timer.elapsed() << " secs.";

  for (int i = 0; i < float_container.values_size(); ++i) {
    EXPECT_EQ(float_container.values(i), float_container2.values(i));
  }
}

}  // namespace hotbox

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
