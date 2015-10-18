#include <string>
#include <glog/logging.h>
#include "db/db.hpp"
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include <snappy.h>
#include <cstdint>
#include <sstream>
#include "util/class_registry.hpp"
#include "util/file_util.hpp"
//#include "util/rocksdb_util.hpp"
#include "transform/all.hpp"
#include "schema/all.hpp"

namespace hotbox {

namespace {

const std::string kDBFile = "/DBFile";
const std::string kDBProto = "DBProto";

}  // anonymous namespace

DB::DB(const std::string& db_path) {
  auto db_file_path = db_path + kDBFile;
  //CHECK(io::Exists(db_file_path));
   /*
  std::unique_ptr<rocksdb::DB> db(OpenRocksDB(db_file_path));
  std::string db_str;
  rocksdb::Status s = db->Get(rocksdb::ReadOptions(), kDBProto, &db_str);
  LOG(INFO) << "Get Key (" << kDBProto << ") from DB (" << db_file_path << ")";
  CHECK(s.ok());
  // */
  ///*
  std::string db_str = io::ReadCompressedFile(db_file_path);
  // */
  DBProto proto;
  proto.ParseFromString(ReadCompressedString(db_str));
  meta_data_ = proto.meta_data();
  schema_ = make_unique<Schema>(proto.schema_proto());
  LOG(INFO) << "DB " << meta_data_.db_config().db_name() << " is initialized"
    " from " << db_file_path << " # features in schema: "
    << schema_->GetNumFeatures();
  // TODO(wdai): Throw exception and catch and handle it in DBServer.
  if (kFeatureIndexType == FeatureIndexType::INT32 &&
      meta_data_.feature_index_type() == FeatureIndexType::INT64) {
    LOG(FATAL) << "A 32-bit feature index build cannot read 64-bit database "
      << meta_data_.db_config().db_name();
  }
  LOG(INFO) << "Set atom path: " << meta_data_.file_map().atom_path();
}

DB::DB(const DBConfig& config) : schema_(new Schema(config.schema_config())) {
  auto db_config = meta_data_.mutable_db_config();
  *db_config = config;
  auto unix_timestamp = std::chrono::seconds(std::time(nullptr)).count();
  meta_data_.set_creation_time(unix_timestamp);
  meta_data_.set_feature_index_type(kFeatureIndexType);
  std::time_t read_timestamp = meta_data_.creation_time();
  meta_data_.mutable_file_map()->set_atom_path(
      meta_data_.db_config().db_dir() + "/atom.");
  LOG(INFO) << "Creating DB " << config.db_name() << ". Creation time: "
    << std::ctime(&read_timestamp);
}

std::string DB::ReadFile(const ReadFileReq& req) {
  DBAtom atom;
  BigInt num_features_before = schema_->GetNumFeatures();
  {
	//io::ifstream in(req.file_path());
    std::unique_ptr<dmlc::SeekStream> fp(io::OpenFileStream(req.file_path()));
    dmlc::istream in(fp.get());
    std::string line;
    auto& registry = ClassRegistry<ParserIf>::GetRegistry();
    std::unique_ptr<ParserIf> parser = registry.CreateObject(req.file_format());
    // Comment(wdai): parser_config is optional, and a default is config is
    // created automatically if necessary.
    parser->SetConfig(req.parser_config());
    // TODO(weiren): Store datum to disk properly, e.g., limit each atom file
    // to 64MB.
    // TODO(weiren): write file would also need more delicate control.
    while (std::getline(in, line)) {
      CHECK_NOTNULL(parser.get());
      DatumBase datum = parser->ParseAndUpdateSchema(line, schema_.get());
      // Let DBAtom takes the ownership of DatumProto release from datum.
      atom.mutable_datum_protos()->AddAllocated(datum.ReleaseProto());
    }
  }
  BigInt num_features_after = schema_->GetNumFeatures();

  // TODO(weiren): Use various compression library. Need to have some
  // interface like parser_if.hpp.
  std::string serialized_atom = SerializeProto(atom);

  // meta_data_.file_map().datum_ids_size() is the number of atom files.
  //
  // TODO(wdai): Don't always create a new file if the previous file hasn't
  // reached 64MB yet.
  int32_t next_atom_id = meta_data_.file_map().datum_ids_size();
  std::string output_file = meta_data_.file_map().atom_path()
    + std::to_string(next_atom_id);
  auto compressed_size = io::WriteCompressedFile(output_file, serialized_atom);
  float compress_ratio = static_cast<float>(compressed_size) /
    serialized_atom.size();
  std::stringstream ss;
  BigInt num_data_read = atom.datum_protos_size();
  ss << "Read " << num_data_read << " datum. Wrote to " << output_file
    << " " << SizeToReadableString(compressed_size) << " ("
    << std::to_string(compress_ratio) << " compression). # of features in"
    " schema: " << num_features_before << " (before) --> "
    << num_features_after << " (after).\n";

  BigInt num_data_before_read = meta_data_.file_map().num_data();
  meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
  meta_data_.mutable_file_map()->set_num_data(num_data_before_read + num_data_read);
  auto meta_data_str = PrintMetaData();
  LOG(INFO) << ss.str() << meta_data_str;

  CommitDB();

  return ss.str() + meta_data_str;
}

DBProto DB::GetProto() const {
  DBProto proto;
  *(proto.mutable_meta_data()) = meta_data_;
  *(proto.mutable_schema_proto()) = schema_->GetProto();
  return proto;
}

void DB::CommitDB() {
  std::string db_file = meta_data_.db_config().db_dir() + kDBFile;
  //CHECK(io::Exists(db_file));
  
  auto db_proto = GetProto();
  std::string serialized_db = SerializeProto(GetProto());
  auto original_size = serialized_db.size();
   /*
  std::unique_ptr<rocksdb::DB> db(OpenRocksDB(db_file));
  // */
  // /*
  auto compressed_size = io::WriteCompressedFile(db_file, serialized_db);
  // */
   /* ----- RocksDB Method ------
  auto compressed_size = WriteCompressedString(serialized_db);
  rocksdb::Status s = db->Put(rocksdb::WriteOptions(), kDBProto, serialized_db);
  assert(s.ok());
  //    --------------------------- */
  float db_compression_ratio = static_cast<float>(compressed_size)
    / original_size;
  LOG(INFO) << "Committed DB " << meta_data_.db_config().db_name()
    << " to DBfile: " << SizeToReadableString(compressed_size) << " ("
    << std::to_string(db_compression_ratio) << " of uncompressed size)\n";
}

SessionProto DB::CreateSession(const SessionOptionsProto& session_options) {
  LOG(INFO) << "DB Creating session";
  TransformConfigList configs =
    session_options.transform_config_list();
  auto& registry = ClassRegistry<TransformIf>::GetRegistry();
  Schema trans_schema = *schema_;
  SessionProto session;
  session.mutable_trans_params()->Reserve(configs.transform_configs_size());
  for (int i = 0; i < configs.transform_configs_size(); ++i) {
    const TransformConfig& config = configs.transform_configs(i);

    // Configure TransWriter.
    const auto output_family = config.base_config().output_family();
    FeatureStoreType store_type = config.base_config().output_store_type();
    TransformWriter trans_writer(&trans_schema, output_family, store_type);

    TransformParam trans_param(trans_schema, config);
    std::unique_ptr<TransformIf> transform =
      registry.CreateObject(config.config_case());
    transform->TransformSchema(trans_param, &trans_writer);
    auto range = session.add_transform_output_ranges();
    *range = trans_writer.GetTransformOutputRange();

    *(session.add_trans_params()) = trans_param.GetProto();
  }
  *(session.mutable_o_schema()) = trans_schema.GetOSchemaProto();
  session.set_session_id(session_options.session_id());
  session.set_compressor(meta_data_.db_config().compressor());
  *(session.mutable_file_map()) = meta_data_.file_map();
  *(session.mutable_internal_family_proto()) =
    trans_schema.GetFamily(kInternalFamily).GetProto();
  session.set_output_store_type(session_options.output_store_type());
  session.set_output_dim(
      trans_schema.GetAppendOffset().offsets(FeatureStoreType::OUTPUT));
  return session;
}

std::string DB::PrintMetaData() const {
  std::stringstream ss;
  std::time_t read_timestamp = meta_data_.creation_time();
  std::string feature_idx_type_str = (meta_data_.feature_index_type()
      == FeatureIndexType::INT64) ? "int64" : "int32";
  ss << "DB meta data:\n";
  ss << "Creation time: " << std::ctime(&read_timestamp)
    << "FeatureIndexType: " << feature_idx_type_str << "\n"
    << "num_data: " << meta_data_.file_map().num_data() << "\n";
  ss << "Feature family details:\n"
    "FeatureFamily: NumFeatures / MaxFeatureId\n";
  for (const auto& p : schema_->GetFamilies()) {
    ss << p.first << ": " << p.second.GetNumFeatures() << " / "
      <<  p.second.GetMaxFeatureId() << std::endl;
  }
  return ss.str();
}

}  // namespace hotbox
