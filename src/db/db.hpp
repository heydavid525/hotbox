#pragma once

#include <ctime>
#include <chrono>
#include <memory>
#include <boost/noncopyable.hpp>
#include "db/proto/db.pb.h"
#include "util/proto/warp_msg.pb.h"
#include "schema/all.hpp"

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

class DB : private boost::noncopyable {
public:
  // Initialize DB from db_path/DBFile which contains serialized DBProto.
  DB(const std::string& db_path);

  DB(const DBConfig& config);

  // Initialize/augment schema accordingly. Return a message.
  std::string ReadFile(const ReadFileReq& req);

  // Return a server session containing transformed schema etc for next
  // client to use directly.
  SessionProto CreateSession(const SessionOptionsProto& session_options);

  // Write all the states of DB to /DB file.
  void CommitDB();

  DBProto GetProto() const;

  std::string PrintMetaData() const;

private:
  DBMetaData meta_data_;

  // TODO(wdai): Allows multiple schemas (schema evolution).
  std::unique_ptr<Schema> schema_;

  // We only support a single Stat
  std::vector<Stat> stats_;

  void GenerateDBAtom(const DBAtom& atom, const ReadFileReq& req);

  int32_t GetCurrentAtomID();
  void UpdateReadMetaData(const DBAtom& atom, const int32_t compressed_size);
  size_t WriteToAtomFiles(const DBAtom& atom, int32_t* ori_sizes, int32_t* comp_size);

  //std::vector<Epoch> epochs_;

  // stats_ does not have 1:1 relation with epochs_.
  //std::vector<Stats> stats_;
};

}  // namespace hotbox
