//#ifdef USE_ROCKS
#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <string>

namespace hotbox {
	namespace io{

/*! \brief type of file */
enum RocksdbType {
  /*! \brief the file is file */
  kBulkData,
  /*! \brief the file is directory */
  kMetaData
};

// Usage Procedure: Open a rocksdb and then PutKey/GetKey.
// Pass the rocksdb pointer to PutKey/GetKey function.

// Open a rocksdb optimized for Metadata storage
rocksdb::DB* OpenRocksMetaDB(const std::string& dbname);
// Open a rocksdb optimized for record storage
rocksdb::DB* OpenRocksRecordDB(const std::string& dbname);
// Store in db a <key, value> pair
void PutKey(rocksdb::DB* db, const std::string& key, const std::string& value);
// Read from db a <key, value> pair
void GetKey(rocksdb::DB* db, const std::string& key, std::string* value);
// Get from db a range of kv pairs: <key1 - key2, values[]>
void GetKeyRange(rocksdb::DB* db, const std::string& key1, 
	const std::string& key2,  std::string& value);

// TODO(weiren): Add merge and column family.

}
}
//#endif
