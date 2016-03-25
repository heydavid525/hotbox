#include "util/util.hpp"
#include "util/proto/util.pb.h"
#include <string>
#include <algorithm>

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>

#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include "util/file_util.hpp"
#include "util/hotbox_exceptions.hpp"

namespace hotbox {



//////////////////////////////////////////////////////////////////////////////
//
// process_mem_usage(double &, double &) - takes two doubles by reference,
// attempts to read the system-dependent data for a process' virtual memory
// size and resident set size, and return the results in MB.
//
// On failure, returns 0.0, 0.0
double process_mem_usage()
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   double vm_usage     = 0.0;
   double resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize / 1024.0 / 1024.0;
   resident_set = rss * page_size_kb / 1024.0;
   return resident_set;
}

std::string SizeToReadableString(size_t size) {
  double s = static_cast<double>(size);
  const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

  int i = 0;
  while (s > 1024) {
    s /= 1024;
    i++;
  }

  char buf[100];
  sprintf(buf, "%.*f %s", std::max(i, 2), s, units[i]);
  return std::string(buf);
}

std::string SerializeProto(const google::protobuf::Message& msg) {
  std::string data;
  CHECK(msg.SerializeToString(&data));
  CHECK_LT(data.size(), kProtoSizeLimitInBytes) << "Exceeds proto "
    "message size limit " << SizeToReadableString(kProtoSizeLimitInBytes);
  return data;
}

std::string SerializeAndCompressProto(const google::protobuf::Message& msg) {
  SnappyCompressor compressor;
  return compressor.Compress(SerializeProto(msg));
}


std::string DecompressString(const std::string& input,
    Compressor compressor) {
  // Uncompress
  if (compressor == Compressor::NO_COMPRESS) {
    return input;
  }
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  std::unique_ptr<CompressorIf> compressor_if =
    registry.CreateObject(compressor);
  try {
    return compressor_if->Uncompress(input);
  } catch (const FailedToUncompressException& e) {
    throw FailedFileOperationException("Failed to uncompress " + input
        + "\n" + e.what());
  }
  // Should never get here.
  return "";
}


std::string DecompressString(const char* data, size_t size,
    Compressor compressor) {
  // Uncompress
  if (compressor == Compressor::NO_COMPRESS) {
    std::string ret((const char *)data, size);
    return ret;
  }
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  std::unique_ptr<CompressorIf> compressor_if =
    registry.CreateObject(compressor);
  try {
    return compressor_if->Uncompress(data, size);
  } catch (const FailedToUncompressException& e) {
    throw FailedFileOperationException(std::string("Failed to uncompress ")
        + "\n" + e.what());
  }
  // Should never get here.
  return "";
}

size_t WriteCompressedString(std::string& input,
    Compressor compressor) {
  if (compressor != Compressor::NO_COMPRESS) {
    // Compress always succeed.
    auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
    std::unique_ptr<CompressorIf> compressor_if =
      registry.CreateObject(compressor);
    input = compressor_if->Compress(input);
  }
  return input.size();
}

}  // namespace hotbox
