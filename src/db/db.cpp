#include <string>
#include <glog/logging.h>
#include "db/db.hpp"
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include "schema/schema_util.hpp"
#include "util/global_config.hpp"
#include <snappy.h>
#include <cstdint>
#include <sstream>
#include "util/all.hpp"
#include "transform/all.hpp"
#include "schema/all.hpp"
#include <thread>
#include <future>
#include <utility>
#include <limits>



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
  atom_id_ = meta_data_.file_map().datum_ids_size();
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
  io::WriteCompressedFile(output_file, compressed_atom,
    Compressor::NO_COMPRESS);
  //LOG(INFO) << "Wrote to atom " << atom_id
  //  << " size: " << SizeToReadableString(compressed_atom.size());
    //<< " This ingestion has written: "
    //<< SizeToReadableString(cumulative_size + compressed_atom.size());
  return std::make_pair(compressed_atom.size(), uncompressed_size);
}


// With Atom sized to 64MB (or other size) limited chunks.
std::string DB::ReadFileMT(const ReadFileReq& req) {
  Timer timer;
  // Extend the default feature family if necessary.
  LOG(INFO) << "req.num_features_default(): " << req.num_features_default();
  if (req.num_features_default() != 0) {
    LOG(INFO) << "Add features up to " << req.num_features_default();
    bool is_simple = true;
    const FeatureFamilyIf& family_if =
      schema_->GetOrCreateFamily(kDefaultFamily,
      is_simple, FeatureStoreType::SPARSE_NUM);
    Feature feature = CreateFeature(FeatureStoreType::SPARSE_NUM);
    schema_->AddFeature("default", &feature,
      req.num_features_default() - 1);
  }
  auto& global_config = GlobalConfig::GetInstance();
  int num_io = global_config.Get<int>("num_io_ingest");

  size_t read_size = 0, write_size = 0, uncompressed_size = 0;
  int64_t num_records = 0;

  auto& registry =
    ClassRegistry<ParserIf, const ParserConfig&>::GetRegistry();
  // Comment(wdai): parser_config is optional, and a default is config is
  // created automatically if necessary.
  std::unique_ptr<ParserIf> parser = registry.CreateObject(
      req.file_format(), req.parser_config());

  float atom_space_used_threshold =
    EstimateAtomSize(parser.get(), req.file_paths(0));
  int num_batches = req.file_paths_size() / num_io;
  if (req.file_paths_size() % num_io != 0) num_batches++;
  for (int b = 0; b < num_batches; ++b) {
    // read file [f_begin, f_end).
    int f_begin = b * num_io;
    int f_end = (b == num_batches - 1) ? req.file_paths_size() :
      f_begin + num_io;
    std::vector<std::future<ProcessReturn>> futs;
    for (int i = f_begin; i < f_end; ++i) {
      futs.emplace_back(std::async(std::launch::async, &DB::ReadOneFileMT, this,
      parser.get(), req.file_paths(i), atom_space_used_threshold,
      std::ref(timer)));
    }
    for (auto& fut : futs) {
      ProcessReturn ret = fut.get();
      read_size += ret.read_size;
      write_size += ret.write_size;
      uncompressed_size += ret.uncompressed_size;
      num_records += ret.num_records;
    }
    LOG(INFO) << "Finished batch " << b << " num_records so far: " <<
      num_records << " write size: " << write_size;
  }
  if (req.commit()) {
    CommitDB();
  }
  float time = timer.elapsed();

  // Print Log.
  std::string output_file_dir = meta_data_.file_map().atom_path();
  BigInt num_features_after = schema_->GetNumFeatures();
  float compress_ratio = static_cast<float>(write_size)
    / uncompressed_size;
  std::stringstream ss;
  ss << "Read " << num_records << " datum.\n"
    << "Time: " << time << "s\n"
    << "Read size: " << SizeToReadableString(read_size) << "\n"
    << "Read throughput: "
    << SizeToReadableString(static_cast<float>(total_read_size_) / time) << "/sec\n"
    << "Wrote to " << output_file_dir
    << "\nWritten Size " << SizeToReadableString(write_size)
    << " (" << std::to_string(compress_ratio) << " compression).\n"
    << "Write throughput per sec: "
    << SizeToReadableString(static_cast<float>(write_size) / time) << "\n"
    << "# of features in schema: " << schema_->GetNumFeatures();
  auto meta_data_str = PrintMetaData();
  LOG(INFO) << ss.str() << meta_data_str;

  return ss.str() + meta_data_str;
}

namespace {

size_t ComputeAtomSpaceThreshold(size_t compressed_size,
  size_t uncompressed_size, size_t atom_space_used) {
  // compression_rate =  compressed_size / serialized_size.
  float compression_rate = static_cast<float>(compressed_size) /
    uncompressed_size;
  // over_est_rate = SpaceUsed() / serialized_size
  float over_est_rate = static_cast<float>(atom_space_used) /
    uncompressed_size;
  return static_cast<float>(kAtomSizeInBytes) /
    compression_rate * over_est_rate;
}

}  // anonymous namespace

size_t DB::EstimateAtomSize(ParserIf* parser,
  const std::string& path) {
  auto fp = io::OpenFileStream(path);
  dmlc::istream in(fp.get());
  DBAtom atom;
  int num_records = 0;
  for (std::string line; std::getline(in, line)
    && num_records < 40000; ) {
    bool invalid = false;
    DatumBase datum = parser->ParseLine(line,
        schema_.get(), nullptr, &invalid);
    if (invalid) {
      continue;   // possibly a comment line.
    }
    num_records++;
    atom.mutable_datum_protos()->AddAllocated(
        datum.ReleaseProto());
  }
  size_t uncompressed_size = 0;
  std::string compressed_atom =
    StreamSerialize(atom, &uncompressed_size);
  size_t compressed_size = compressed_atom.size();
  float atom_space_used_threshold = ComputeAtomSpaceThreshold(compressed_size,
    uncompressed_size, atom.SpaceUsed());
  //LOG(INFO) << "Estimated compression rate: " << compression_rate;
  //LOG(INFO) << "Estimated space_used / serialize rate: " << over_est_rate;
  //LOG(INFO) << "Estimated atom space used threshold: " <<
  //  atom_space_used_threshold;
  return atom_space_used_threshold;
}

ProcessReturn DB::ReadOneFileMT(ParserIf* parser,
  const std::string& path,
  size_t atom_space_used_threshold, const Timer& timer) {
  ProcessReturn ret;
  ret.read_size = io::GetFileSize(path);
  // fp is a smart pointer.
  auto fp = io::OpenFileStream(path);
  dmlc::istream in(fp.get());
  //std::vector<Stat> stats_;
  //StatCollector stat_collector(&stats_);

  DBAtom atom;
  int64_t curr_batch_size = 0;
  int batch_size = 0;
  for (std::string line; std::getline(in, line); ) {
    bool invalid = false;
    DatumBase datum = parser->ParseLine(line,
        schema_.get(), nullptr, &invalid);
    if (invalid) {
      continue;   // possibly a comment line.
    }
    atom.mutable_datum_protos()->AddAllocated(
        datum.ReleaseProto());
    // SpaceUsed() is expensive, so check infrequently.
    if (curr_batch_size > batch_size && curr_batch_size % 40000 == 0) {
      size_t space_used = atom.SpaceUsed();
      if (space_used >= atom_space_used_threshold) {
        int atom_id = -1;
        {
          std::lock_guard<std::mutex> lock(mut_);
          atom_id = atom_id_++;
          int64_t num_data_before_read = meta_data_.file_map().num_data();
          meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
          meta_data_.mutable_file_map()->set_num_data(
              num_data_before_read + atom.datum_protos_size());
        }
        auto p = WriteAtom(atom, atom_id, 0);
        float time = timer.elapsed();
        LOG(INFO) << "Atom #" << atom_id
          << "\nWrite compressed size     : "
          << SizeToReadableString(p.first)
          << "\nBatch size: " << curr_batch_size
          << "\nApprox read throughput: "
          << SizeToReadableString(static_cast<float>(total_read_size_) / time)
          << "/s\nTime: " << time << "s";
        ret.write_size += p.first;
        ret.uncompressed_size += p.second;
        {
          std::lock_guard<std::mutex> lock(mut_);
          num_data_read_ += atom.datum_protos_size();
          total_atom_space_used_ += space_used;
          total_write_size_ += p.first;
          total_uncompressed_size_ += p.second;

          // Recompute atom_space_used_threshold
          atom_space_used_threshold = ComputeAtomSpaceThreshold(
            total_write_size_, total_uncompressed_size_,
            total_atom_space_used_);
          batch_size = static_cast<float>(kAtomSizeInBytes) /
            (total_write_size_ / num_data_read_);
          LOG(INFO) << "Estimated atom space threshold: " <<
            atom_space_used_threshold << " batch size: " << batch_size;
        }
        atom = DBAtom();
        curr_batch_size = 0;
      }
    }
    ret.num_records++;
    curr_batch_size++;
  }
  // Write the last datum if necessary.
  if (atom.datum_protos_size() > 0) {
    int atom_id = -1;
    {
      std::lock_guard<std::mutex> lock(mut_);
      atom_id = atom_id_++;
      int64_t num_data_before_read = meta_data_.file_map().num_data();
      meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
      meta_data_.mutable_file_map()->set_num_data(
          num_data_before_read + atom.datum_protos_size());
    }
    auto p = WriteAtom(atom, atom_id, 0);
    LOG(INFO) << "Atom #" << atom_id
      << "\nWrite compressed size     : "
      << SizeToReadableString(p.first);
    ret.write_size += p.first;
    ret.uncompressed_size += p.second;
    {
      std::lock_guard<std::mutex> lock(mut_);
      total_atom_space_used_ += atom.SpaceUsed();
      total_write_size_ += p.first;
      total_uncompressed_size_ += p.second;
    }
  }
  total_read_size_ += ret.read_size;
  LOG(INFO) << "num records from file " << path << ": " << ret.num_records
    << " write size: " << ret.write_size << " read size: " << ret.read_size;
  return ret;
}

std::string DB::ReadFile(const ReadFileReq& req) {
  auto& registry = ClassRegistry<ParserIf, const ParserConfig&>::GetRegistry();
  // Comment(wdai): parser_config is optional, and a default is config is
  // created automatically if necessary.
  std::unique_ptr<ParserIf> parser = registry.CreateObject(
      req.file_format(), req.parser_config());
  std::string reply_msg;
  for (int f = 0; f < req.file_paths_size(); ++f) {
    reply_msg += ReadOneFile(req.file_paths(f), parser.get());
    if (req.commit()) {
      CommitDB();
    }
  }
  return reply_msg;
}

// With Atom sized to 64MB (or other size) limited chunks.
std::string DB::ReadOneFile(const std::string& file_path,
  ParserIf* parser) {
  Timer timer;
  size_t total_write_size = 0, total_uncompressed_size = 0;
  int32_t rec_counter = 0;
  int32_t prev_atom_id = meta_data_.file_map().datum_ids_size();
  BigInt num_features_before = schema_->GetNumFeatures();
  size_t read_size = io::GetFileSize(file_path);
  int time = 0;
  {
    // fp is a smart pointer.
    auto fp = io::OpenFileStream(file_path);
    dmlc::istream in(fp.get());
    std::string line;
    StatCollector stat_collector(&stats_);
    int32_t batch_size = kInitIngestBatchSize;
    DBAtom* curr_atom_ptr = nullptr;
    float compression_rate = 0.5;
    float over_estimate_rate = 1.0; // space used / memory size
    size_t total_atom_space_used = 0;  // size in bytes
    size_t atom_space_used_threshold = 0; // size in bytes
    bool use_global_estimate = false;
    bool has_estimate_init = false;
    LOG(INFO) << "Initial batch size: " << batch_size;
    while (!in.eof()) {
      curr_atom_ptr = new DBAtom();
      if (atom_space_used_threshold) {
        batch_size = std::numeric_limits<int>::max(); // infinite batch size
      }
      for (int i = 0; i < batch_size && std::getline(in, line); i++) {
        ++rec_counter;
        bool invalid = false;
        DatumBase datum = parser->ParseAndUpdateSchema(line,
            schema_.get(), &stat_collector, &invalid);
        if (invalid) {
          continue;   // possibly a comment line.
        }
        // For the first two batch we make rough estimate, which will be adjust
        // after the first two batch is written. 0.5 for snappy compression.
        if (!atom_space_used_threshold && batch_size == kInitIngestBatchSize) {
          batch_size = kAtomSizeInBytes /
            (datum.GetDatumProto().SpaceUsed() * 0.5);
        }
        // Let DBAtom take the ownership of DatumProto release from datum.
        curr_atom_ptr->mutable_datum_protos()->AddAllocated(
            datum.ReleaseProto());
        // TODO(Yangyang): is SpaceUsed very expensive?
        if (atom_space_used_threshold && i % 40000 == 0) {
            int space_used = curr_atom_ptr->SpaceUsed();
            if (space_used >= atom_space_used_threshold) {
              batch_size = i;
              break;
            }
        }
      }
      // Wait till last write finish first.
      size_t write_size = 64 * 1e6;
      int atom_id = meta_data_.file_map().datum_ids_size();
      if (write_fut_.valid()) {
        auto ret = write_fut_.get();
        write_size = ret.first;
        total_write_size += write_size;
        total_uncompressed_size += ret.second;
        if (use_global_estimate || !has_estimate_init){
          compression_rate = (float)total_write_size / total_uncompressed_size;
          over_estimate_rate = (float)total_atom_space_used /
            total_uncompressed_size;
          atom_space_used_threshold = kAtomSizeInBytes /
            compression_rate * over_estimate_rate;
          LOG(INFO) << "\nCompression Rate  : " << compression_rate * 100 << "% "
                    << "\nOver Estimate Rate: " << over_estimate_rate * 100 << "%"
                    << "\nAtom Space Used Threshold: "
                    << (double)atom_space_used_threshold / (1<<20) << " MB";
          has_estimate_init = true;
        }
      }
      total_atom_space_used += curr_atom_ptr->SpaceUsed();
      int64_t num_data_before_read = meta_data_.file_map().num_data();
      meta_data_.mutable_file_map()->add_datum_ids(num_data_before_read);
      meta_data_.mutable_file_map()->set_num_data(
          num_data_before_read + curr_atom_ptr->datum_protos_size());
      write_fut_ = std::async(std::launch::async,
          [this, curr_atom_ptr, total_write_size, atom_id, batch_size] {
          auto ret = WriteAtom(*curr_atom_ptr, atom_id, total_write_size);
          LOG(INFO) << "Atom #" << atom_id
            << "\nBatch Size    : " << batch_size
            << "\nFile Size     : " << SizeToReadableString(ret.first)
            << "\nSpace Used    : "
            << SizeToReadableString(curr_atom_ptr->SpaceUsed())
            << "\nRaw Size      : " << SizeToReadableString(ret.second)
            << "\nCompress Rate : " << (double) ret.first / ret.second * 100 << "%"
            << "\nOver Estimate : "
            << (double) curr_atom_ptr->SpaceUsed() / ret.second;
          delete curr_atom_ptr;
          return ret;
      });
      // Update batch_size estimate.
      float avg_bytes_per_datum = static_cast<float>(write_size) / batch_size;
      batch_size = kAtomSizeInBytes / avg_bytes_per_datum;
      // Wait for the last write.
      if (in.eof()) {
        auto ret = write_fut_.get();
        write_size = ret.first;
        total_write_size += write_size;
        total_uncompressed_size += ret.second;
        // CHECK_GT(curr_atom_ptr->datum_protos_size(), 0);
      }
    }
    time = timer.elapsed();

    LOG(INFO) << "committing DB";
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
    TransformParam trans_param(trans_schema, config, stats_[0]);

    std::unique_ptr<TransformIf> transform =
      registry.CreateObject(config.config_case());
    transform->UpdateTransformWriterConfig(config, &writer_config);
    TransformWriter trans_writer(&trans_schema, writer_config);

    transform->TransformSchema(trans_param, &trans_writer);
    auto range = session.add_transform_output_ranges();
    *range = trans_writer.GetTransformOutputRange();
    int64_t r = range->store_offset_end() - range->store_offset_begin();
    CHECK_GT(r, 0) << "Empty output detected. OutputFamily: " << output_family
      << " end: " << range->store_offset_end() << " begin: "
      << range->store_offset_begin();

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
