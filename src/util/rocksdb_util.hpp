#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

namespace mldb {

rocksdb::DB* OpenRocksDB(const std::string& dbname);




}