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
  RocksDB(const std::string& db_path);

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

  inline std::string GetName() const {
    return db_->GetName();
  }

private:
  std::unique_ptr<rocksdb::DB> db_;
};

}   // namespace hotbox
