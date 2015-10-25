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

rocksdb::DB* OpenRocksMetaDB(const std::string& dbname);
rocksdb::DB* OpenRocksRecordDB(const std::string& dbname);
void PutKey(rocksdb::DB* db, const std::string& key, const std::string& value);
void GetKey(rocksdb::DB* db, const std::string& key, std::string* value);
void GetKeyRange(rocksdb::DB* db, const std::string& key1, const std::string& key2,  std::string& value);

// TODO(weiren): Add merge and column family.

}
}
