#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <string>
#include <glog/logging.h>

namespace hotbox {

// Wrapper around RocksDB to provide OOP interface. The DB is opened for the
// entire lifetime of RocksDB.
class RocksDB {
public:
  // 'db_path' is a file system directory to create / read a DB.
  RocksDB(const std::string& db_path) {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    // Set Level-1 file size to 64 MB.
    options.target_file_size_base = 64 * 1024 * 1024;
    rocksdb::Status s = rocksdb::DB::Open(options, db_path, &db);
    db_.reset(db);
    CHECK(s.ok());
  }

  // Put a key-value pair. Fails the program if incurring any error.
  inline void Put(const std::string& key, const std::string& val) {
    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), key, val);
    CHECK(s.ok());
  }

  // Read the value for key. Fail the program if not found or other error.
  inline std::string Get(const std::string& key) {
    std::string val;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &val);
    CHECK(s.ok());
    return val;
  }

private:
  std::unique_ptr<rocksdb::DB> db_;
};

}   // namespace hotbox
