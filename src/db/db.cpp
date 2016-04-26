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
#include <thread>
#include <future>
#include <utility>


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

DB::DB(const std::string& db_path_meta) :
  meta_db_(db_path_meta + kDBMeta) {
  std::string db_proto_str = meta_db_.Get(kDBProto);
  LOG(INFO) << "Get Key (" << kDBProto << ") from DB ("
    << meta_db_.GetName() << ")";
  DBProto proto = StreamDeserialize<DBProto>(db_proto_str);
  meta_data_ = proto.meta_data();
  schema_.reset(new Schema(&meta_db_));

  // Assume only 1 stat.
  stats_.emplace_back(0, &meta_db_);

  LOG(INFO) << "DB " << meta_data_.db_config().db_name() << " is initialized ";
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
meta_db_(config.db_dir_meta() + kDBMeta) {
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

std::pair<size_t, size_t> DB::WriteAtom(const DBAtom& atom, int atom_id,
    size_t cumulative_size) {
  std::string output_file_dir = meta_data_.file_map().atom_path();
  size_t uncompressed_size = 0;
  std::string compressed_atom =
    StreamSerialize(atom, &uncompressed_size);
  std::string output_file = output_file_dir + std::to_string(atom_id);
  io::WriteCompressedFile(output_file, compressed_atom);
  LOG(INFO) << "Wrote to atom " << atom_id
    << " size: " << SizeToReadableString(compressed_atom.size())
    << " This ingestion has written: "
    << SizeToReadableString(cumulative_size + compressed_atom.size());
  return std::make_pair(compressed_atom.size(), uncompressed_size);
}

// With Atom sized to 64MB (or other size) limited chunks.
std::string DB::ReadFile(const ReadFileReq& req) {
  Timer timer;
  size_t total_write_size = 0, total_uncompressed_size = 0;
  int32_t rec_counter = 0;
  int32_t prev_atom_id = meta_data_.file_map().datum_ids_size();
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
    DBAtom atom1, atom2;
    DBAtom* curr_atom_ptr = &atom1;
    DBAtom* next_atom_ptr = &atom2;
    while (!in.eof()) {
      for (int i = 0; i < batch_size && std::getline(in, line); i++) {
        ++rec_counter;
        CHECK_NOTNULL(parser.get());
        bool invalid = false;
        DatumBase datum = parser->ParseAndUpdateSchema(line,
            schema_.get(), &stat_collector, &invalid);
        if (invalid) {
          continue;   // possibly a comment line.
        }
        if (batch_size == kInitIngestBatchSize) {
          // For the first batch we make rough estimate, which will be adjust
          // after the first batch is written. 0.5 for snappy compression.
          batch_size =
            kAtomSizeInBytes / (datum.GetDatumProto().SpaceUsed() * 0.5);
          LOG(INFO) << "Initial batch size: " << batch_size;
        }
        // Let DBAtom take the ownership of DatumProto release from datum.
        curr_atom_ptr->mutable_datum_protos()->AddAllocated(
            datum.ReleaseProto());
      }
      // Wait till last write finish first.
      size_t write_size = 64 * 1e6;
      if (write_fut_.valid()) {
        auto ret = write_fut_.get();
        write_size = ret.first;
        total_write_size += write_size;
        total_uncompressed_size += ret.second;
        CHECK_GT(next_atom_ptr->datum_protos_size(), 0);
      }
      int atom_id = meta_data_.file_map().datum_ids_size();
      write_fut_ = std::async(std::launch::async,
          [this, curr_atom_ptr, total_write_size, atom_id] {
          return WriteAtom(*curr_atom_ptr, atom_id, total_write_size);
          }
          );
      int64_t num_data_before_read = meta_data_.file_map().num_data();
      meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
      meta_data_.mutable_file_map()->set_num_data(
          num_data_before_read + curr_atom_ptr->datum_protos_size());
      std::swap(curr_atom_ptr, next_atom_ptr);
      *curr_atom_ptr = DBAtom();
      // Update batch_size estimate.
      float avg_bytes_per_datum = static_cast<float>(write_size)
        / batch_size;
      batch_size = kAtomSizeInBytes / avg_bytes_per_datum;
      // Wait for the last write.
      if (in.eof()) {
        auto ret = write_fut_.get();
        write_size = ret.first;
        total_write_size += write_size;
        total_uncompressed_size += ret.second;
        CHECK_GT(next_atom_ptr->datum_protos_size(), 0);
      }
    }
    time = timer.elapsed();

    LOG(INFO) << "committing DB";
    if (req.commit()) {
      CommitDB();
    }
  }

  // Print Log.
  std::string output_file_dir = meta_data_.file_map().atom_path();
  BigInt num_features_after = schema_->GetNumFeatures();
  float compress_ratio = static_cast<float>(total_write_size)
    / total_uncompressed_size;
  std::stringstream ss;
  ss << "Read " << rec_counter << " datum.\n"
    << "Time: " << time << "s\n"
    << "Read size: " << SizeToReadableString(read_size) << "\n"
    << "Read throughput per sec: "
    << SizeToReadableString(static_cast<float>(read_size) / time) << "\n"
    << "Wrote to " << output_file_dir
    <<" [" << prev_atom_id << " - " << meta_data_.file_map().datum_ids_size()
    << ")\nWritten Size " << SizeToReadableString(total_write_size)
    << " (" << std::to_string(compress_ratio) << " compression).\n"
    << "Write throughput per sec: "
    << SizeToReadableString(static_cast<float>(total_write_size) / time) << "\n"
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
    TransformWriterConfig writer_config;
    auto output_family = config.base_config().output_family();
    if (output_family.empty()) {
      // Set default family name when output_family isn't set.
      output_family = kConfigCaseToTransformName[config.config_case()] +
        std::to_string(i);
    }
    writer_config.set_output_family_name(output_family);
    // Create transform param before TransformWriter modifies trans_schema.
    TransformParam trans_param(trans_schema, config);

    std::unique_ptr<TransformIf> transform =
      registry.CreateObject(config.config_case());
    transform->UpdateTransformWriterConfig(config, &writer_config);
    TransformWriter trans_writer(&trans_schema, writer_config);

    transform->TransformSchema(trans_param, &trans_writer);
    auto range = session.add_transform_output_ranges();
    *range = trans_writer.GetTransformOutputRange();

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
  *(session.mutable_label()) =
      trans_schema.GetFamily(kInternalFamily).GetFeature(kLabelFamilyIdx);
  *(session.mutable_weight()) =
      trans_schema.GetFamily(kInternalFamily).GetFeature(kWeightFamilyIdx);
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
    ss << p.first << ": " << p.second->GetNumFeatures() << " / "
      <<  p.second->GetMaxFeatureId() << std::endl;
  }
  return ss.str();
}

}  // namespace hotbox
