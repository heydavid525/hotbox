#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <iostream>
#include <dmlc/io.h>
//#include <sstream>
#include "io/filesys.hpp"
#include "test/facility/test_facility.hpp"
#include <memory>


#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "io/general_fstream.hpp"
#include "util/util.hpp"

const std::string kContent{"Hello World!\n"};
std::string kTestPath = "hdfs://localhost:9000/user/yu/url_combined";

int main() {
	
  /*
  petuum::io::ifstream in(kTestPath, std::ifstream::binary);
  CHECK(in) << "Failed to open " << kTestPath;
  */
  
  // We do own the SeekStream reading pointer.
  std::unique_ptr<dmlc::SeekStream> sk
        (dmlc::SeekStream::CreateForRead(kTestPath.c_str())); 
  //std::unique_ptr<dmlc::SeekStream> fp(fs->OpenForRead(path));
  dmlc::istream in(sk.get());

  bool b_succeed = true;
  size_t len = 65536;
  const char* buffer;
  int size = 65536;
  auto fp = hotbox::make_unique<google::protobuf::io::IstreamInputStream>
      (dynamic_cast<std::basic_istream<char>*>(&in), len);
  fp->Skip(0); // offset to start reading.
  while (b_succeed && len == size) {
    // Read a bulk of size len, into buffer, length returned in size
    bool b_succeed = fp->Next(reinterpret_cast<const void**>(&buffer), &size);
    if(b_succeed) {
      LOG(INFO) << "read n bytes: " << size;
    } else {
      LOG(FATAL) << "Failed to read file: " + kTestPath + "\n";
    }
  }
}

/*

int main() {
	// Use this method to read the full file, once.
	dmlc::io::URI path(kTestPath.c_str());
	// We do not own the FileSystem pointer.
	dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
	dmlc::io::FileInfo info = fs->GetPathInfo(path);
	// We do own the SeekStream reading pointer.
    size_t size = info.size;
	//std::unique_ptr<dmlc::SeekStream> fp(fs->OpenForRead(path));
	std::unique_ptr<dmlc::Stream> fp(dmlc::Stream::Create(kTestPath.c_str(), "r"));
	// size_t pos_rd_offset = fp->Tell(); 

    char buffer[32];
    size_t nread;
    while ((nread = fp->Read(buffer, 32)) != 0) {
      LOG(INFO) << "nread is " << nread;
      // fprintf(stdout, "%s \n", std::string(buffer, nread).c_str());
    }
	LOG(INFO) << "TestPath: " << path.name;    
	LOG(INFO) << "PROTOCOL:" << path.protocol;
	LOG(INFO) << "File size: " << size;
	LOG(INFO) << "nread is " << nread;
}
*/
