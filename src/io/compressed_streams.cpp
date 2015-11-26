#include <iostream>
#include <cmath>

#include <google/protobuf/stubs/common.h>
#include <glog/logging.h>

#include <google/protobuf/io/coded_stream.h>
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;

#include "io/compressed_streams.hpp"
#include <snappy.h>

namespace hotbox {

namespace {

const size_t kBlockSize = 64 * 1024;

}  // anonymous namespace

// ============== BlockCompressionInputStream ==============

BlockCompressionInputStream::BlockCompressionInputStream(
    ZeroCopyInputStream* sub_stream)
  : output_buffer_(nullptr), output_buffer_size_(0), sub_stream_(nullptr), 
  backed_up_bytes_(0), byte_count_(0) {
    raw_stream_ = sub_stream;
    sub_stream_ = new CodedInputStream(raw_stream_);
    sub_stream_->SetTotalBytesLimit(pow(1024,3), pow(1024,3));
  }

BlockCompressionInputStream::~BlockCompressionInputStream() {
  delete sub_stream_;
  delete[] output_buffer_;
}

bool BlockCompressionInputStream::Skip(int count) {
  CHECK(false);
};

void BlockCompressionInputStream::BackUp(int count) {
  backed_up_bytes_ += count;
};

void BlockCompressionInputStream::reset_input_stream() {
  delete sub_stream_;
  sub_stream_ = new CodedInputStream(raw_stream_);
  sub_stream_->SetTotalBytesLimit(pow(1024,3), pow(1024,3));
}

bool BlockCompressionInputStream::Next(const void** data, int* size) {
  if (backed_up_bytes_) {
    size_t skip = output_buffer_size_ - backed_up_bytes_;
    CHECK(skip >= 0);
    (*data) = output_buffer_ + skip;
    (*size) = backed_up_bytes_;
    backed_up_bytes_ = 0;
    return true;
  }

  uint32_t compressed_size = 0;
  CHECK(sub_stream_->ReadVarint32(&compressed_size));
  CHECK(compressed_size < kBlockSize*10);

  char * tempbuffer = new char[compressed_size];
  sub_stream_->ReadRaw(tempbuffer, compressed_size);
  RawUncompress(tempbuffer, compressed_size);
  delete[] tempbuffer;

  if (sub_stream_->BytesUntilLimit() < 1024*1024) reset_input_stream();
  // TODO: probably call this only every Limit/kBlockSize

  (*size) = output_buffer_size_;
  (*data) = output_buffer_;
  return true;
}

// ============== BlockCompressionOutputStream ==============

BlockCompressionOutputStream::BlockCompressionOutputStream(
    ZeroCopyOutputStream* sub_stream)
: input_buffer_(nullptr), sub_stream_(new CodedOutputStream(sub_stream)),
  backed_up_bytes_(0), byte_count_(0) {}

  BlockCompressionOutputStream::~BlockCompressionOutputStream() {
    if (input_buffer_) {
      // The buffer is not empty, there is stuff yet to be written.
      // This is necessary because we can't call virtual functions from any
      // destructor. Often this results in a pure virtual function call.
      // Call BlockCompressionOutputStream::Flush() before destroying
      // this object.
      CHECK(false);
      // Flush();
    }
    delete sub_stream_;
  }

void BlockCompressionOutputStream::BackUp(int count) {
  backed_up_bytes_ += count;
}

bool BlockCompressionOutputStream::Flush()
{
  size_t size = kBlockSize - backed_up_bytes_;
  if (!input_buffer_ || size == 0) return true;

  size_t compressed_size = 0;
  char * compressed_data = new char[MaxCompressedLength(size)];
  compressed_size = RawCompress(input_buffer_, size, compressed_data);

  CHECK(compressed_size <= 2*kBlockSize);

  uint32_t compressed_size_32 = static_cast<uint32_t>(compressed_size);
  sub_stream_->WriteVarint32(compressed_size_32);
  sub_stream_->WriteRaw(compressed_data, compressed_size_32);
  delete[] compressed_data;

  backed_up_bytes_ = 0;
  delete[] input_buffer_;
  input_buffer_ = 0;
  return true;
}

bool BlockCompressionOutputStream::Next(void** data, int* size) {
  if (backed_up_bytes_) {
    size_t skip = kBlockSize - backed_up_bytes_;
    CHECK(skip >= 0);
    (*data) = input_buffer_ + skip;
    (*size) = backed_up_bytes_;
    backed_up_bytes_ = 0;
    return true;
  }
  if(input_buffer_) Flush();
  input_buffer_ = new char[kBlockSize];
  (*data) = input_buffer_;
  (*size) = kBlockSize;
  return true;
}


// ============== SnappyInputStream ==============

void SnappyInputStream::RawUncompress(char* input_buffer, uint32_t compressed_size) {
  size_t uncompressed_size;
  bool success = ::snappy::GetUncompressedLength(
      input_buffer, compressed_size, &uncompressed_size);
  CHECK(success);

  if (uncompressed_size > output_buffer_size_) {
    delete[] output_buffer_;
    output_buffer_size_ = uncompressed_size;
    output_buffer_ = new char[output_buffer_size_];
  }
  success = ::snappy::RawUncompress(input_buffer, compressed_size,
      output_buffer_);
  CHECK(success);
}

// ============== SnappyOutputStream ==============

uint32_t SnappyOutputStream::MaxCompressedLength(size_t input_size) {
  return ::snappy::MaxCompressedLength(input_size);
}

uint32_t SnappyOutputStream::RawCompress(char* input_buffer,
    size_t input_size, char* output_buffer) {
  size_t compressed_size = 0;
  ::snappy::RawCompress(input_buffer, input_size, output_buffer,
      &compressed_size);
  return compressed_size;
}

}; // namespace hotbox
