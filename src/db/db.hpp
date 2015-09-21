#pragma once

#include <ctime>
#include <chrono>
#include <memory>
#include <glog/logging.h>
#include <boost/noncopyable.hpp>
#include "db/proto/db.pb.h"
#include "util/proto/warp_msg.pb.h"
#include "schema/schema.hpp"
#include "schema/proto/schema.pb.h"

namespace mldb {

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

class DB : private boost::noncopyable {
public:
  DB(const DBConfig& config) : schema_(config.schema_config()) {
    auto db_config = meta_data_.mutable_db_config();
    *db_config = config;
    auto unix_timestamp = std::chrono::seconds(std::time(nullptr)).count();
    meta_data_.set_creation_time(unix_timestamp);
    std::time_t read_timestamp = meta_data_.creation_time();
    LOG(INFO) << "Creating DB " << config.db_name() << ". Creation time: "
      << std::ctime(&read_timestamp);
  }

  // Initialize/augment schema accordingly. Return a message.
  std::string ReadFile(const ReadFileReq& req);

private:
  DBMetaData meta_data_;

  // TODO(wdai): Allows multiple schemas (schema evolution).
  Schema schema_;

  std::vector<Epoch> epochs_;

  // stats_ does not have 1:1 relation with epochs_.
  std::vector<Stats> stats_;
};

}  // namespace mldb
