#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <iostream>
#include <dmlc/io.h>
//#include <sstream>
#include "io.dmlc/filesys.h"
#include "test/facility/test_facility.hpp"

namespace mldb {

namespace {

const std::string kContent{"Hello World!\n"};
std::string kTestPath = GetTestBedDir() + "/stream_test_file";
std::string kTestPath2 = GetTestBedDir() + "/helloworld";

}  // anonymous namespace

TEST(StreamTest, SmokeTest) {
  // We do not own fs.

  { // Use this method to write to a file. 
    // We do own the Stream pointer.  
    // *** Only After deletion will the file be written to hence for the file to be read.
    std::unique_ptr<dmlc::Stream> os(dmlc::Stream::Create(kTestPath.c_str(), "w"));
    os->Write(kContent.c_str(), kContent.size());
  }
  { // Use this method to list directory.   
    dmlc::io::URI path(GetTestBedDir().c_str());
    // We do not own the FileSystem object.
    dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
    std::vector<dmlc::io::FileInfo> info;
    fs->ListDirectory(path, &info);
    for (size_t i = 0; i < info.size(); ++i) {
      fprintf(stdout, "%s\t%lu\tis_dir=%d\n", info[i].path.name.c_str(), info[i].size,
             info[i].type == dmlc::io::kDirectory);
    }
    fflush(stdout);
  }
  { // Use this method to read the full file, once.
    dmlc::io::URI path(kTestPath2.c_str());
    // We do not own the FileSystem pointer.
    dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
    dmlc::io::FileInfo info = fs->GetPathInfo(path);
    // We do own the SeekStream reading pointer.
    std::unique_ptr<dmlc::SeekStream> fp(fs->OpenForRead(path));
    // size_t pos_rd_offset = fp->Tell(); 
    size_t size = info.size;
    std::string buf(size, ' ');
    size_t nread = fp->Read(&buf[0], size);
    fprintf(stdout, "%s \n", std::string(buf, nread).c_str());
    fflush(stdout);
    LOG(INFO) << "TestPath: " << path.name;    
    LOG(INFO) << "PROTOCOL:" << path.protocol;
    LOG(INFO) << "File size: " << size;
    LOG(INFO) << "nread is " << nread;
    EXPECT_EQ(kContent, buf);
  }
  { // Use this method to stream a file with a small buffer, incrementally.
    // Test 1: Read Hello world from fresh files.
    // We do own the read Stream pointer.
    std::unique_ptr<dmlc::Stream> src(dmlc::Stream::Create(kTestPath.c_str(), "r"));
    char buffer[32];
    size_t nread;
    while ((nread = src->Read(buffer, 32)) != 0) {
      LOG(INFO) << "nread is " << nread;
      fprintf(stdout, "%s \n", std::string(buffer, nread).c_str());
    }
    fflush(stdout);
    EXPECT_EQ(kContent, std::string(buffer, kContent.size()));
  }
  LOG(INFO) << "stream test passed";
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

/*
  {    
    dmlc::io::URI path(kTestPath2.c_str());
    dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
    dmlc::SeekStream *fp = fs->OpenForRead(path);
    size_t size = fp->Tell();
    LOG(INFO) << "File Position is " << size;
    LOG(INFO) << "Path name is " << path.name;
    char buf[32] = {0};
    while (true) {
      size_t nread = fp->Read(buf, 32);
      if (nread == 0) break;
      fprintf(stdout, "%s", std::string(buf, nread).c_str());
    }
    fflush(stdout);
    delete fp;
    EXPECT_EQ(kContent, buf);
  }
  */