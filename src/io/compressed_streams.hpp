#pragma once

// Adapted from https://github.com/JohannesEbke/protobuf-zerocopy-compression

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream.h>
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;

#include <google/protobuf/io/coded_stream.h>
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;

namespace hotbox {

// Interface (If) for all compressed input streams with an ExpectAtEnd
// method.
class CompressedInputStreamIf :
  public google::protobuf::io::ZeroCopyInputStream {
public:
  /// ExpectAtEnd returns true if there is no more compressed data to process
  virtual bool ExpectAtEnd() = 0;
};

// Interface (If) for all compressed output streams with additional methods
class CompressedOutputStreamIf :
  public google::protobuf::io::ZeroCopyOutputStream {
  public:
    /// Make sure that all data is compressed and written
    virtual bool Flush() = 0;
    virtual bool Close() = 0;
  };

// ============= Block Compression Streams ==============

// This class provides scaffolding for implementing compression streams based
// on compression algorithms that do not support streaming operations but must
// be operated blockwise.
class BlockCompressionInputStream : public CompressedInputStreamIf {
public:
  // Does not take the ownership of sub_stream.
  explicit BlockCompressionInputStream(ZeroCopyInputStream* sub_stream);
  virtual ~BlockCompressionInputStream();

  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  google::protobuf::int64 ByteCount() const { return byte_count_; }
  bool ExpectAtEnd() { return true; }

protected:
  virtual void RawUncompress(char* input_buffer,
      uint32_t compressed_size) = 0;

  char* output_buffer_;
  size_t output_buffer_size_;

private:
  CodedInputStream* sub_stream_;
  ZeroCopyInputStream* raw_stream_;

  int backed_up_bytes_;
  size_t byte_count_;

  void reset_input_stream();

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(BlockCompressionInputStream);
};

class BlockCompressionOutputStream : public CompressedOutputStreamIf {
public:
  // Create a BlockCompressionOutputStream with default options. Does not
  // takes the ownership of sub_stream.
  explicit BlockCompressionOutputStream(ZeroCopyOutputStream* sub_stream);
  virtual ~BlockCompressionOutputStream();

  bool Next(void** data, int* size);
  void BackUp(int count);
  google::protobuf::int64 ByteCount() const { return byte_count_; };

  bool Flush();
  bool Close() { return Flush(); }

protected:
  virtual uint32_t MaxCompressedLength(size_t input_size) = 0;
  virtual uint32_t RawCompress(char* input_buffer, size_t input_size,
      char* output_buffer) = 0;
  char* input_buffer_;

private:
  CodedOutputStream* sub_stream_;

  int backed_up_bytes_;
  size_t byte_count_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(BlockCompressionOutputStream);
};

// ============= Snappy Compression Streams ==============

class SnappyInputStream : public BlockCompressionInputStream {
public:
  explicit SnappyInputStream(ZeroCopyInputStream* sub_stream) : BlockCompressionInputStream(sub_stream) {};

  virtual void RawUncompress(char* input_buffer, uint32_t compressed_size);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SnappyInputStream);
};

class SnappyOutputStream : public BlockCompressionOutputStream {
public:
  explicit SnappyOutputStream(ZeroCopyOutputStream* sub_stream) :
    BlockCompressionOutputStream(sub_stream) {};

  virtual uint32_t MaxCompressedLength(size_t input_size);
  virtual uint32_t RawCompress(char* input_buffer, size_t input_size,
      char* output_buffer);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SnappyOutputStream);
};

}; // namespace hotbox
