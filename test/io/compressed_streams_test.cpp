#include "io/compressed_streams.hpp"
#include "test/facility/test_facility.hpp"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include "test/io/test_proto.pb.h"
#include <memory>
#include <string>
#include <cstdint>
#include <limits>

namespace hotbox {

namespace {

// store 0,....,test_size in the test container.
const int32_t kTestSize = 1e5; // 30000000;

}  // anonymous namespace

TEST(CompressedStreamTest, SmokeTest) {
  FloatContainer float_container;
  float_container.mutable_values()->Resize(kTestSize, 0);
  for (int i = 0; i < kTestSize; ++i) {
    float_container.set_values(i, 1);
  }
  std::string buffer;
  {
    // Write to buffer
    buffer.reserve(kTestSize * sizeof(float));
    google::protobuf::io::StringOutputStream ostream_str(&buffer);
    std::unique_ptr<SnappyOutputStream> ostream_snappy(
      new SnappyOutputStream(&ostream_str));
    google::protobuf::io::CodedOutputStream ostream_coded(
      ostream_snappy.get());
    //google::protobuf::io::CodedOutputStream ostream_coded(
    //  &ostream_str);
    float_container.SerializeToCodedStream(&ostream_coded);
    ostream_snappy->Flush();
  }
  LOG(INFO) << "buffer size:" << buffer.size();
  const int buffer_limit = std::numeric_limits<int>::max();
  LOG(INFO) << "coded stream byte upper limit: " << buffer_limit;
  {
    // Read back to another FloatContainer
    google::protobuf::io::ArrayInputStream istream_arr(
      buffer.data(), buffer.size());
    std::unique_ptr<SnappyInputStream> istream_snappy(
      new SnappyInputStream(&istream_arr));
    google::protobuf::io::CodedInputStream istream_coded(
      istream_snappy.get());
    //google::protobuf::io::CodedInputStream istream_coded(
    //  &istream_arr);
    istream_coded.SetTotalBytesLimit(buffer_limit, buffer_limit);

    FloatContainer float_container2;
    EXPECT_TRUE(float_container2.ParseFromCodedStream(&istream_coded));

    EXPECT_EQ(float_container.values_size(), float_container2.values_size());
    for (int i = 0; i < float_container.values_size(); ++i) {
      EXPECT_EQ(float_container.values(i), float_container2.values(i));
    }
    LOG(INFO) << "stream buffer works.";
  }
}

}  // namespace hotbox

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

