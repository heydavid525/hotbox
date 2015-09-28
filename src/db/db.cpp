#include <string>
#include <glog/logging.h>
#include "db/db.hpp"
#include "io/fstream.hpp"
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include "util/class_registry.hpp"
#include "util/file_util.hpp"
#include <snappy.h>
#include <cstdint>
#include <sstream>

namespace mldb {

namespace {

const std::string kDBFile = "/DBFile";

}  // anonymous namespace

DB::DB(const std::string& db_path) {
  std::string db_str = ReadCompressedFile(db_path + kDBFile);
  DBProto proto;
  proto.ParseFromString(db_str);
  meta_data_ = proto.meta_data();
  schema_ = make_unique<Schema>(proto.schema_proto());
}

DB::DB(const DBConfig& config) : schema_(new Schema(config.schema_config())) {
  auto db_config = meta_data_.mutable_db_config();
  *db_config = config;
  auto unix_timestamp = std::chrono::seconds(std::time(nullptr)).count();
  meta_data_.set_creation_time(unix_timestamp);
  std::time_t read_timestamp = meta_data_.creation_time();
  LOG(INFO) << "Creating DB " << config.db_name() << ". Creation time: "
    << std::ctime(&read_timestamp);
}

std::string DB::ReadFile(const ReadFileReq& req) {
  DBAtom atom;
  int num_features_before = schema_->GetNumFeatures();
  {
    io::ifstream in(req.file_path());
    std::string line;
    auto& registry = ClassRegistry<ParserIf>::GetRegistry();
    std::unique_ptr<ParserIf> parser = registry.CreateObject(req.file_format());
    // Comment(wdai): parser_config is optional, and a default is config is
    // created automatically if necessary.
    parser->SetConfig(req.parser_config());
    // TODO(weiren): Store datum to disk properly, e.g., limit each atom file
    // to 64MB.
    while (std::getline(in, line)) {
      DatumBase datum = parser->ParseAndUpdateSchema(line, schema_.get());
      atom.add_datum_protos(datum.Serialize());
    }
  }
  int num_features_after = schema_->GetNumFeatures();

  // TODO(weiren): Use various compression library. Need to have some
  // interface like parser_if.hpp.
  std::string serialized_atom = SerializeProto(atom);

  std::string output_file = meta_data_.db_config().db_dir() + "/atom";
  auto compressed_size = WriteCompressedFile(output_file, serialized_atom);
  float compress_ratio = static_cast<float>(compressed_size) /
    serialized_atom.size();
  std::stringstream ss;
  int num_datum = atom.datum_protos_size();
  ss << "Read " << num_datum << " datum. Wrote to " << output_file
    << " " << SizeToReadableString(compressed_size) << " ("
    << std::to_string(compress_ratio) << " of raw file). # of features in"
    " schema: " << num_features_before << " (before) --> "
    << num_features_after << " (after).\n";
  ss << "FeatureFamily: MaxFeatureId\n";
  for (const auto& p : schema_->GetFamilies()) {
    ss << p.first << ": " << p.second.GetMaxFeatureId() << std::endl;
  }
  LOG(INFO) << ss.str();

  CommitDB();

  return ss.str();

  // TODO(wdai): update stats_ and meta_data_.
}

DBProto DB::GetProto() const {
  DBProto proto;
  *(proto.mutable_meta_data()) = meta_data_;
  *(proto.mutable_schema_proto()) = schema_->GetProto();
  return proto;
}

void DB::CommitDB() {
  LOG(INFO) << "Committing DB " << meta_data_.db_config().db_name();
  std::string db_file = meta_data_.db_config().db_dir() + kDBFile;
  auto db_proto = GetProto();
  std::string serialized_db = SerializeProto(db_proto);
  auto compressed_size = WriteCompressedFile(db_file, serialized_db);
  float db_compression_ratio = static_cast<float>(compressed_size)
    / serialized_db.size();
  LOG(INFO) << "Written DBfile: "
    << SizeToReadableString(compressed_size) << " ("
    << std::to_string(db_compression_ratio) << " of uncompressed size)\n";
}

void DB::CreateSession(const SessionOptionsProto& session_options) {
  // TODO(wdai)
}

}  // namespace mldb
