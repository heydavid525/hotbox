#pragma once

#include <ctime>
#include <chrono>
#include <memory>
#include <boost/noncopyable.hpp>
#include "db/proto/db.pb.h"
#include "parse/parser_if.hpp"
#include "util/proto/warp_msg.pb.h"
#include "schema/all.hpp"
#include "util/rocks_db.hpp"
#include <future>
#include <atomic>
#include <mutex>

namespace hotbox {

/*
class Epoch {
public:
private:
  std::unique_ptr<EpochProto> proto_;
};

// Stats behaves like index in traditional DB. It provides a fast way to read
// and transform data. A Stats object is built over some consecutive epoch.
// By default there's a Stats tracking from epoch 0 to the latest epoch. User
// may define additional epochs for specific epochs of interest.
class Stats {
public:
private:
};
*/

struct ProcessReturn {
  size_t read_size = 0;
  size_t write_size = 0;
  size_t uncompressed_size = 0;
  int64_t num_records = 0;
};

class DB : private boost::noncopyable {
public:
  // Initialize DB from db_path/DBFile which contains serialized DBProto.
  DB(const std::string& db_path_meta);

  DB(const DBConfig& config);

  // Initialize/augment schema accordingly. Return a message.
  std::string ReadFile(const ReadFileReq& req);
  std::string ReadFileMT(const ReadFileReq& req);

  // Return a server session containing transformed schema etc for next
  // client to use directly.
  SessionProto CreateSession(const SessionOptionsProto& session_options);

  DBProto GetProto() const;

  std::string PrintMetaData() const;

private:
  // Write all the states of DB to /DB file.
  void CommitDB();

  // Estimate atom.SpaceUsed() threshold to get kAtomSizeInBytes
  // (function of compression and serialization).
  size_t EstimateAtomSize(ParserIf* parser,
    const std::string& path);

  // Read one file for the MT version (ReadFileMT).
  ProcessReturn ReadOneFileMT(ParserIf* parser,
    const std::string& path,
    size_t atom_space_used_threshold, const Timer& timer);

  // Read one file for ReadFile().
  std::string ReadOneFile(const std::string& file_path,
    ParserIf* parser);

  // Write ‘atom’ data to Atom files. Return pair p. p.first is bytes
  // written (compressed size), p.second is uncompressed size.
  std::pair<size_t, size_t> WriteAtom(const DBAtom& atom,
      int atom_id, size_t cumulative_size = 0);

private:
  DBMetaData meta_data_;

  // TODO(wdai): Allows multiple schemas (schema evolution).
  std::unique_ptr<Schema> schema_;

  RocksDB meta_db_;

  // Currently we only support a single Stat
  std::vector<Stat> stats_;

  std::future<std::pair<size_t, size_t>> write_fut_;

  //std::vector<Epoch> epochs_;

  // stats_ does not have 1:1 relation with epochs_.
  //std::vector<Stats> stats_;

  // Parameters for multi-threaded ingest synchronization.
  int64_t num_data_read_{0};
  std::atomic<size_t> total_read_size_{0};
  size_t total_atom_space_used_{0};
  size_t total_write_size_{0};
  size_t total_uncompressed_size_{0};

  // Reconcile multi-threaded write (creation of atom files).
  std::atomic<int> atom_id_{0};
  std::mutex mut_;
};

}  // namespace hotbox
