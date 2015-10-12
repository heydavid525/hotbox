#include "util/rocksdb_util.hpp"



namespace mldb {

rocksdb::DB* OpenRocksDB(const std::string& dbname) {
	  // **** RocksDB Persistency. **********
  rocksdb::DB* db_ptr;
  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  // open DB
  rocksdb::Status s = rocksdb::DB::Open(options, dbname, &db_ptr);
  assert(s.ok());
  return db_ptr;
}


}
