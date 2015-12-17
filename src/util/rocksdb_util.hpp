#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <string>

namespace hotbox {
namespace io {

/*! \brief type of file */
enum RocksdbType {
  /*! \brief the file is file */
  kBulkData,
  /*! \brief the file is directory */
  kMetaData
};

// Usage:
//    std::unique_ptr<rocksdb::DB> db(io::OpenRocksMetaDB(kTestPath));
//    io::Put(db.get(), "Hello", kContent);
//    std::string value;
//    io::Get(db.get(), "Hello", &value);

// Open a rocksdb optimized for metadata storage
rocksdb::DB* OpenRocksMetaDB(const std::string& dbname);

// Open a rocksdb optimized for record storage
rocksdb::DB* OpenRocksRecordDB(const std::string& dbname);

// Store in db a <key, value> pair
void Put(rocksdb::DB* db, const std::string& key, const std::string& value);

// Read from db a <key, value> pair
void Get(rocksdb::DB* db, const std::string& key, std::string* value);

// Get from db a range of kv pairs: <key1 - key2, values[]>
void GetRange(rocksdb::DB* db, const std::string& key1, 
	const std::string& key2,  std::string& value);

// TODO(weiren): Add merge and column family.

}   // namespace io
}   // namespace hotbox
