#include <string>
#include <glog/logging.h>
#include "db/db.hpp"
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include <snappy.h>
#include <cstdint>
#include <sstream>
#include "util/all.hpp"
#include "transform/all.hpp"
#include "schema/all.hpp"


namespace hotbox {

namespace {

const std::string kDBMeta = "/DBMeta";
const std::string kDBFile = "/DBRecord";
const std::string kDBProto = "DBProto";
const std::string kStatProtoSeqPrefix = "stat";
const int kInitIngestBatchSize = 100;

// Similar to kStatBatchSize, but for feature_ vector in Schema.
const int kFeatureBatchSize = 1e6;

}  // anonymous namespace

DB::DB(const std::string& db_path) : meta_db_(db_path + kDBMeta) {
  std::string db_proto_str = meta_db_.Get(kDBProto);
  LOG(INFO) << "Get Key (" << kDBProto << ") from DB ("
    << meta_db_.GetName() << ")";
  /*
  std::string db_str = DecompressString(db_proto_str);
  DBProto proto;
  CHECK(proto.ParseFromString(db_str));
  */
  DBProto proto = StreamDeserialize<DBProto>(db_proto_str);
  meta_data_ = proto.meta_data();
  schema_.reset(new Schema(&meta_db_));
  // const auto& intern_family = schema_->GetFamily(kInternalFamily);
  // const Feature& feature = intern_family.GetFeature(kLabelFamilyIdx);
  // LOG(INFO) << "label feature: " << feature.DebugString();

  // Assume only 1 stat.
  stats_.emplace_back(0, &meta_db_);
  // schema_ = make_unique<Schema>(proto.schema_proto());

  /*
  for (int i = 0; i < proto.num_stat_proto_seqs(); ++i) {
    std::string stat_key = kStatProtoSeqPrefix + std::to_string(i);
    std::string stat_proto_seq_str = meta_db_.Get(stat_key);
    FeatureStatProtoSeq proto_seq;
    proto_seq.ParseFromString(stat_proto_seq_str);
    StatProto* released_stat = nullptr;
    int num_stats = proto_seq.stats_size();
    CHECK_EQ(1, num_stats) << "Only support single stats for now";
    proto_seq.mutable_stats()->ExtractSubrange(0, num_stats, &released_stat);
    CHECK_NOTNULL(released_stat);
    for (BigInt j = 0; j < proto_seq.stats_size(); ++j) {
      stats_.emplace_back(&released_stat[i]);
    }
  }
  */

  //auto recorddb_file_path = db_path + kDBFile;

  // Take over DBProto::stats.
  /*
  StatProto* released_stat = nullptr;
  int num_stats = proto.stats_size();
  CHECK_EQ(1, num_stats) << "Only support single stats for now";
  proto.mutable_stats()->ExtractSubrange(0, num_stats, &released_stat);
  CHECK_NOTNULL(released_stat);
  for (int i = 0; i < num_stats; ++i) {
    stats_.emplace_back(&released_stat[i]);
  }
  */

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

DB::DB(const DBConfig& config) : schema_(new Schema(config.schema_config())),
meta_db_(config.db_dir() + kDBMeta) {
  auto db_config = meta_data_.mutable_db_config();
  *db_config = config;

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

int32_t DB::GetCurrentAtomID() {
  // meta_data_.file_map().datum_ids_size() is the number of atom files.
  int32_t atom_size_mb = kAtomSizeInBytes;
  int32_t curr_global_bytes_offset_size = meta_data_.file_map()
            .global_bytes_offsets_size();
  int64_t curr_global_bytes_offset = (curr_global_bytes_offset_size == 0)
    ? 0 : meta_data_.file_map().global_bytes_offsets(
        curr_global_bytes_offset_size - 1);

  //LOG(INFO) << "curr_global_bytes_offset_size: " 
  //          << curr_global_bytes_offset_size << ". ";
  //LOG(INFO) << "curr_global_bytes_offset: " 
  //          << curr_global_bytes_offset << ". ";
  int32_t curr_atom_id = curr_global_bytes_offset / atom_size_mb;
  return curr_atom_id;
}

size_t DB::WriteToAtomFiles(const DBAtom& atom, size_t* ori_sizes,
    size_t* comp_sizes) {
  std::string output_file_dir = meta_data_.file_map().atom_path();
  size_t serialized_size = 0;
  std::string compressed_atom = StreamSerialize(atom, &serialized_size);
  int curr_atom_id = io::WriteAtomFiles(output_file_dir, GetCurrentAtomID(),
      compressed_atom);
  *ori_sizes += serialized_size;
  *comp_sizes += compressed_atom.size();
  LOG(INFO) << "curr_atom_id: " << curr_atom_id
            << " This ingestion has written: "
            << SizeToReadableString(*comp_sizes);
  return compressed_atom.size();
}

void DB::UpdateReadMetaData(const DBAtom& atom, size_t new_len) {
  int32_t curr_global_bytes_offset_size = meta_data_.file_map()
    .global_bytes_offsets_size();
  int64_t curr_global_bytes_offset = (curr_global_bytes_offset_size == 0) ? 0 :
    meta_data_.file_map().global_bytes_offsets(curr_global_bytes_offset_size - 1);
  meta_data_.mutable_file_map()->add_global_bytes_offsets(
      curr_global_bytes_offset + new_len);
  //LOG(INFO) << "Total data offset: " << curr_global_bytes_offset + new_len;

  BigInt num_data_read = atom.datum_protos_size();
  BigInt num_data_before_read = meta_data_.file_map().num_data();
  meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
  meta_data_.mutable_file_map()->set_num_data(
      num_data_before_read + num_data_read);
  //LOG(INFO) << "# records read this interval: " << num_data_read << ". ";
  //LOG(INFO) << "# records before: " << num_data_before_read << ". ";
  //LOG(INFO) << "# records in DB: " << num_data_before_read + num_data_read 
  //                                    << ". ";
}

// With Atom sized to 64MB (or other size) limited chunks.
std::string DB::ReadFile(const ReadFileReq& req) {
  Timer timer;
  size_t ori_size = 0;
  size_t compressed_size = 0;
  int32_t rec_counter = 0;
  int32_t ori_atom_id = GetCurrentAtomID();
  BigInt num_features_before = schema_->GetNumFeatures();
  size_t read_size = io::GetFileSize(req.file_path());
  int time = 0;
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
    int32_t batch_size = kInitIngestBatchSize;
    while (!in.eof()) {
      DBAtom atom;
      for (int i = 0; i < batch_size && std::getline(in, line); i++) {
        ++rec_counter;
        CHECK_NOTNULL(parser.get());
        DatumBase datum = parser->ParseAndUpdateSchema(line,
            schema_.get(), &stat_collector);
        if (batch_size == kInitIngestBatchSize) {
          // For the first batch we make rough estimate, which will be adjust
          // after the first batch is written. 0.5 for snappy compression.
          batch_size =
            kAtomSizeInBytes / (datum.GetDatumProto().SpaceUsed() * 0.5);
          LOG(INFO) << "Initial batch size: " << batch_size;
        }
        // Let DBAtom take the ownership of DatumProto release from datum.
        atom.mutable_datum_protos()->AddAllocated(datum.ReleaseProto());
      }
      size_t write_size = WriteToAtomFiles(atom, &ori_size, &compressed_size);
      // Update batch_size estimate.
      float avg_bytes_per_datum = static_cast<float>(write_size) / batch_size;
      batch_size = kAtomSizeInBytes / avg_bytes_per_datum;
      LOG(INFO) << "Updated batch size: " << batch_size;
      UpdateReadMetaData(atom, write_size);
    }
    time = timer.elapsed();

    if (!req.no_commit()) {
      CommitDB();
    }
  }

  // Print Log.
  std::string output_file_dir = meta_data_.file_map().atom_path();
  BigInt num_features_after = schema_->GetNumFeatures();
  float compress_ratio = static_cast<float>(compressed_size)
    / ori_size;
  std::stringstream ss;
  ss << "Read " << rec_counter << " datum.\n"
    << "Time: " << time << "s\n"
    << "Read size: " << SizeToReadableString(read_size) << "\n"
    << "Read throughput per sec: "
    << SizeToReadableString(static_cast<float>(read_size) / time) << "\n"
    << "Wrote to " << output_file_dir
    <<" [" << ori_atom_id << " - " << GetCurrentAtomID() << "]\n"
    << "Written Size " << SizeToReadableString(compressed_size)
    << " (" << std::to_string(compress_ratio) << " compression).\n"
    << "Write throughput per sec: "
    << SizeToReadableString(static_cast<float>(compressed_size) / time)
    << "\n"
    << "# of features in schema: " << num_features_before 
    << " (before) --> " << num_features_after << " (after).\n";
  auto meta_data_str = PrintMetaData();
  LOG(INFO) << ss.str() << meta_data_str;

  return ss.str() + meta_data_str;
}

DBProto DB::GetProto() const {
  DBProto proto;
  *(proto.mutable_meta_data()) = meta_data_;
  // bool with_features = false;
  // *(proto.mutable_schema_proto()) = schema_->GetProto(with_features);
  // proto.set_num_seqs();
  // proto.set_num_features(schema_->GetFeatures()->size());
  /*
     for (const auto& stat : stats_) {
   *(proto.add_stats()) = stat.GetProto();
   }
   */
  return proto;
}

/*
   void DB::CommitStats() {
   int num_stat_batches = std::ceil(static_cast<float>(stats_.size())
   / kStatBatchSize);
   for (int i = 0; i < num_stat_batches; ++i) {
   BigInt id_begin = kStatBatchSize * i;
   BigInt id_end = std::min(id_begin + kStatBatchSize,
   static_cast<BigInt>(stats_.size()));
   FeatureStatProtoSeq stat_proto_seq;
   stat_proto_seq.set_id_begin(id_begin);
   stat_proto_seq.mutable_stats()->Reserve(kStatBatchSize);
   for (BigInt j = id_begin; j < id_end; ++j) {
 *stat_proto_seq.add_stats() = stats_[j].GetProto();
 }
 std::string stat_key = kStatProtoSeqPrefix + std::to_string(i);
 meta_db_.Put(stat_key, SerializeProto(stat_proto_seq));
 }
 }
 */

void DB::CommitDB() {
  Timer timer;
  std::string db_file = meta_data_.db_config().db_dir() + kDBMeta;
  DBProto db_proto = GetProto();
  std::string db_proto_str = StreamSerialize(db_proto);
  size_t total_size = db_proto_str.size();
  meta_db_.Put(kDBProto, db_proto_str);
  total_size += schema_->Commit(&meta_db_);

  // Commit Stats.
  for (int i = 0; i < stats_.size(); ++i) {
    total_size += stats_[i].Commit(i, &meta_db_);
  }
  LOG(INFO) << "Committed DB " << meta_data_.db_config().db_name()
    << " to DBfile: " << SizeToReadableString(total_size)
    << ". Time: " << timer.elapsed() << "s\n";
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
    LOG(INFO) << "Preparing transform params";
    TransformParam trans_param(trans_schema, config);
    LOG(INFO) << "Done preparing transform params";

    FeatureStoreType store_type = config.base_config().output_store_type();
    TransformWriter trans_writer(&trans_schema, output_family, store_type);

    std::unique_ptr<TransformIf> transform =
      registry.CreateObject(config.config_case());
    LOG(INFO) << "Transforming schema...";
    transform->TransformSchema(trans_param, &trans_writer);
    LOG(INFO) << "Done transforming schema";
    auto range = session.add_transform_output_ranges();
    *range = trans_writer.GetTransformOutputRange();

    LOG(INFO) << "Creating proto for trans_param...";
    *(session.add_trans_params()) = trans_param.GetProto();
    LOG(INFO) << " trans_param proto created. Size: "
      << SizeToReadableString(session.trans_params(i).SpaceUsed());
  }
  LOG(INFO) << "CreateSession finished transforms";
  *(session.mutable_o_schema()) = trans_schema.GetOSchemaProto();
  LOG(INFO) << " OSchemaProto. Size: "
    << SizeToReadableString(session.o_schema().SpaceUsed());
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
