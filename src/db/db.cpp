#include <string>
#include "db/db.hpp"
#include "io/fstream.hpp"
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include "util/class_registry.hpp"

namespace mldb {

void DB::IngestFile(const IngestFileReq& req) {
  auto& registry = ClassRegistry<ParserIf>::GetRegistry();
  std::unique_ptr<ParserIf> parser = registry.CreateObject(req.file_format());
  // A default parser config is constructed if not set. ParserIf impl has to
  // handle it appropriately.
  parser->SetConfig(req.parser_config());
  io::ifstream in(req.file_path());
  std::string line;
  while (std::getline(in, line)) {
    parser->ParseAndUpdateSchema(line, &schema_);
  }

  // TODO(wdai): update stats_ and meta_data_.
}


}  // namespace mldb
