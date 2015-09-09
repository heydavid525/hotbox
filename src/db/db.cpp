#include <string>
#include "db/db.hpp"
#include "io/fstream.hpp"

namespace mldb {

void DB::IngestFile(const IngestFileReq& req) {
  io::ifstream in(req.file_path());
  std::string line;
  while (std::getline(in, line)) {

  }
}


}  // namespace mldb
