#include <string>
#include "db/db.hpp"
#include "io/fstream.hpp"
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include "util/class_registry.hpp"
#include <snappy.h>
#include <cstdint>

namespace mldb {

std::string DB::ReadFile(const ReadFileReq& req) {
  DBAtom atom;
  int num_features_before = schema_.GetNumFeatures();
  {
    io::ifstream in(req.file_path());
    std::string line;
    auto& registry = ClassRegistry<ParserIf>::GetRegistry();
    std::unique_ptr<ParserIf> parser = registry.CreateObject(req.file_format());
    // Comment(wdai): parser_config is optional, and a default is config is
    // created automatically if necessary.
    parser->SetConfig(req.parser_config());
    // TODO(weiren): Store datum to disk properly, e.g., limit each atom file to 64MB.
    while (std::getline(in, line)) {
      DatumBase datum = parser->ParseAndUpdateSchema(line, &schema_);
      atom.add_datum_protos(datum.Serialize());
    }
  }
  int num_features_after = schema_.GetNumFeatures();

  // TODO(weiren): Use various compression library. Need to have some
  // interface like parser_if.hpp.
  std::string serialized_atom;
  atom.SerializeToString(&serialized_atom);

  std::string compressed_atom;
  snappy::Compress(serialized_atom.c_str(), serialized_atom.size(), &compressed_atom);

  std::string output_file = meta_data_.db_config().db_dir() + "/atom";
  {
    io::ofstream out(output_file, std::ios::out | std::ios::binary);
    out.write(compressed_atom.c_str(), compressed_atom.size());
  }
  float compress_ratio = static_cast<float>(compressed_atom.size()) /
    serialized_atom.size();
  std::stringstream ss;
  int num_datum = atom.datum_protos_size();
  ss << "Read " << num_datum << " datum. Wrote to " << output_file
    << " " << std::to_string(compressed_atom.size()) << " bytes ("
    << std::to_string(compress_ratio) << " of raw file). # of features in schema: "
    << num_features_before << " (before) --> " << num_features_after <<
    " (after).\n";
  ss << "FeatureFamily: MaxFeatureId\n";
  for (const auto& p : schema_.GetFamilies()) {
    ss << p.first << ": " << p.second.GetMaxFeatureId() << std::endl;
  }
  LOG(INFO) << ss.str();
  return ss.str();

  // TODO(wdai): update stats_ and meta_data_.
}

}  // namespace mldb
