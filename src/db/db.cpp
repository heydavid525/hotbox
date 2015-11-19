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
#include "transform/all.hpp"
#include "schema/all.hpp"


namespace hotbox {

namespace {

const std::string kDBMeta = "/DBMeta";
const std::string kDBFile = "/DBRecord";
const std::string kDBProto = "DBProto";

}  // anonymous namespace

void DB::InitRocksdb(const std::string db_path) {
  LOG(INFO) << "Open DB db_path: " << db_path;
  auto metadb_file_path = db_path + kDBMeta;
  meta_db_.reset(io::OpenRocksMetaDB(metadb_file_path));
}

DB::DB(const std::string& db_path) {
  
  InitRocksdb(db_path);
  std::string rocks_str;
  io::GetKey(meta_db_.get(), kDBProto, &rocks_str);
  LOG(INFO) << "Get Key (" << kDBProto << ") from DB (" << meta_db_->GetName() << ")";
  std::string db_str = ReadCompressedString(rocks_str);
  /*
  auto metadb_file_path = db_path + kDBMeta;
  std::string db_str = io::ReadCompressedFile(metadb_file_path);
  */
  DBProto proto;
  proto.ParseFromString(db_str);
  meta_data_ = proto.meta_data();
  schema_ = make_unique<Schema>(proto.schema_proto());
  
  auto recorddb_file_path = db_path + kDBFile;

  // Take over DBProto::stats.
  StatProto* released_stat = nullptr;
  int num_stats = proto.stats_size();
  CHECK_EQ(1, num_stats) << "Only support single stats for now";
  proto.mutable_stats()->ExtractSubrange(0, num_stats, &released_stat);
  CHECK_NOTNULL(released_stat);
  for (int i = 0; i < num_stats; ++i) {
    stats_.emplace_back(&released_stat[i]);
  }

  LOG(INFO) << "DB " << meta_data_.db_config().db_name() << " is initialized ";
            // << " from " << metadb_file_path << ". "
  LOG(INFO) << "# features in schema: "  << schema_->GetNumFeatures();
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
  InitRocksdb(meta_data_.db_config().db_dir());

  std::time_t read_timestamp = meta_data_.creation_time();
  LOG(INFO) << "Creating DB " << config.db_name() << ". Creation time: "
    << std::ctime(&read_timestamp);

  auto unix_timestamp = std::chrono::seconds(std::time(nullptr)).count();
  meta_data_.set_creation_time(unix_timestamp);
  meta_data_.set_feature_index_type(kFeatureIndexType);
  meta_data_.mutable_file_map()->set_atom_path(
      meta_data_.db_config().db_dir() + "/atom.");
  // Always has a stat starting at epoch 0.
  // TODO(wdai): Implement epoch so to have multiple starting epoch points.
  stats_.emplace_back(0);
}

void DB::GenerateDBAtom(const DBAtom& atom, const ReadFileReq& req) {

}

int32_t DB::GetCurrentAtomID() {
  // meta_data_.file_map().datum_ids_size() is the number of atom files.
  int32_t atom_size_mb = kATOM_SIZE_MB;
  int32_t curr_global_bytes_offset_size = meta_data_.file_map()
            .global_bytes_offsets_size();
  int64_t curr_global_bytes_offset = (curr_global_bytes_offset_size == 0) ? 0 :
            meta_data_.file_map().
            global_bytes_offsets(curr_global_bytes_offset_size - 1);
  //LOG(INFO) << "curr_global_bytes_offset_size: " 
  //          << curr_global_bytes_offset_size << ". ";
  //LOG(INFO) << "curr_global_bytes_offset: " 
  //          << curr_global_bytes_offset << ". ";
  int32_t curr_atom_id = curr_global_bytes_offset / atom_size_mb;
  return curr_atom_id;
}

size_t DB::WriteToAtomFiles(const DBAtom& atom, int32_t* ori_sizes, 
    int32_t* comp_sizes) {
  std::string output_file_dir = meta_data_.file_map().atom_path();
  std::string serialized_atom = SerializeProto(atom);
  int32_t curr_atom_id = GetCurrentAtomID();
  auto compressed_size = io::WriteAtomFiles(output_file_dir,
      curr_atom_id, serialized_atom);
  *ori_sizes += serialized_atom.size();
  *comp_sizes += compressed_size;
  LOG(INFO) << "curr_atom_id: " << curr_atom_id << ". "
            << "This ingestion wrote: " << *comp_sizes << ". ";  
  return compressed_size;
}

void DB::UpdateReadMetaData(const DBAtom& atom, const int32_t new_len) {
  int32_t curr_global_bytes_offset_size = meta_data_.file_map()
          .global_bytes_offsets_size();
  int64_t curr_global_bytes_offset = (curr_global_bytes_offset_size == 0) ? 0 :
               meta_data_.file_map().
                    global_bytes_offsets(curr_global_bytes_offset_size - 1);
  meta_data_.mutable_file_map()->add_global_bytes_offsets(
      curr_global_bytes_offset + new_len);
  LOG(INFO) << "Total data offset: " << curr_global_bytes_offset + new_len;
  
  BigInt num_data_read = atom.datum_protos_size();
  BigInt num_data_before_read = meta_data_.file_map().num_data();
  meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
  meta_data_.mutable_file_map()->set_num_data(
      num_data_before_read + num_data_read);
  LOG(INFO) << "# records read this interval: " << num_data_read << ". ";
  LOG(INFO) << "# records before: " << num_data_before_read << ". ";
  LOG(INFO) << "# records in DB: " << num_data_before_read + num_data_read 
                                      << ". ";
}

int32_t DB::GuessBatchSize(const int32_t size) {
  int32_t limit = kATOM_SIZE_MB;
  return limit / (size * 1.3) ;
}

// With Atom sized to 64MB (or other size) limited chunks.
std::string DB::ReadFile(const ReadFileReq& req) {
  int32_t ori_size = 0;
  int32_t compressed_size = 0;
  int32_t rec_counter = 0;
  int32_t ori_atom_id = GetCurrentAtomID();
  BigInt num_features_before = schema_->GetNumFeatures();
  {
    // fp is a smart pointer.
    auto fp = io::OpenFileStream(req.file_path());
    dmlc::istream in(fp.get());
    std::string line;
    auto& registry = ClassRegistry<ParserIf>::GetRegistry();
    std::unique_ptr<ParserIf> parser = registry.CreateObject(
        req.file_format());
    // Comment(wdai): parser_config is optional, and a default is config is
    // created automatically if necessary.
    parser->SetConfig(req.parser_config());
    StatCollector stat_collector(&stats_);
    while (!in.eof()) {
      DBAtom atom;
      int32_t batch_size = 100;
      for (int i=0; i < batch_size && std::getline(in, line); i++) {
        ++rec_counter;
        // LOG(INFO) << "Parsing DatumBase. ";
        CHECK_NOTNULL(parser.get());
        DatumBase datum = parser->ParseAndUpdateSchema(line, 
          schema_.get(), &stat_collector);
        // LOG(INFO) << "DatumBase Created. ";
        if (i == 0) {
          batch_size = GuessBatchSize(datum.GetDatumProto().SpaceUsed());
          // LOG(INFO) << "Single Datum Size: " 
          //          << datum.GetDatumProto().SpaceUsed() << ". "
          //          << "Interval: " << batch_size << ".";
        }
        // Let DBAtom take the ownership of DatumProto release from datum.
        // LOG(INFO) << "Inserting Datum. ";
        atom.mutable_datum_protos()->AddAllocated(datum.ReleaseProto());
      }
      // LOG(INFO) << "DBAtom Created. ";
      size_t len = WriteToAtomFiles(atom, &ori_size, &compressed_size);
      UpdateReadMetaData(atom, len);
      LOG(INFO) << " ----------------- ------------ ------------";
    }
    CommitDB();
  }

  // Print Log.
  std::string output_file_dir = meta_data_.file_map().atom_path();
  BigInt num_features_after = schema_->GetNumFeatures();
  float compress_ratio = static_cast<float>(compressed_size)
           / ori_size;
  std::stringstream ss;
  ss << "Read " << rec_counter << " datum. "
     << "Wrote to " << output_file_dir 
     <<" [" << ori_atom_id << " - " << GetCurrentAtomID() << "]"<<". " 
     << "Written Size " << SizeToReadableString(compressed_size)  << ". "
     << " (" << std::to_string(compress_ratio) << " compression). "
     << "# of features in schema: " << num_features_before 
     << " (before) --> " << num_features_after << " (after).\n";
  auto meta_data_str = PrintMetaData();
  LOG(INFO) << ss.str() << meta_data_str;

  return ss.str() + meta_data_str;
}

DBProto DB::GetProto() const {
  DBProto proto;
  *(proto.mutable_meta_data()) = meta_data_;
  *(proto.mutable_schema_proto()) = schema_->GetProto();
  for (const auto& stat : stats_) {
    *(proto.add_stats()) = stat.GetProto();
  }
  return proto;
}

void DB::CommitDB() {
  std::string db_file = meta_data_.db_config().db_dir() + kDBMeta;
  //CHECK(io::Exists(db_file));
  auto db_proto = GetProto();
  std::string serialized_db = SerializeProto(GetProto());
  auto original_size = serialized_db.size();

  auto compressed_size = WriteCompressedString(serialized_db);
  io::PutKey(meta_db_.get(), kDBProto, serialized_db);
  /* // File Storage
  auto compressed_size = io::WriteCompressedFile(db_file, serialized_db);
  */
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
    auto output_family = config.base_config().output_family();
    if (output_family.empty()) {
      // Set default family name when output_family isn't set.
      output_family = kConfigCaseToTransformName[config.config_case()] +
        std::to_string(i);
    }
    // Create transform param before TransformWriter modifies trans_schema.
    TransformParam trans_param(trans_schema, config);

    FeatureStoreType store_type = config.base_config().output_store_type();
    TransformWriter trans_writer(&trans_schema, output_family, store_type);

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
    trans_schema.GetFamily(kInternalFamily).GetSelfContainedProto();
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
