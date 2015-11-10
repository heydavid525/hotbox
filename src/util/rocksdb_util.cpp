#ifdef USE_ROCKS
#include "util/rocksdb_util.hpp"
#include <glog/logging.h>



namespace hotbox {
  namespace io {

void PutKey(rocksdb::DB* db, const std::string& key, const std::string& value) {
  //TODO(Weiren): Read/Write options should be configured differently for meta/record access.
  //std::unique_ptr<rocksdb::DB> db = std::move(p);
  rocksdb::Status s = db->Put(rocksdb::WriteOptions(), key, value);
  assert(s.ok());
}

void GetKey(rocksdb::DB* db, const std::string& key, std::string* value) {  
  //TODO(Weiren): Read/Write options should be configured differently for meta/record access.
  rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, value);
  assert(s.ok());
}

//string RangePutRocks(string dbname, string key, string& value);
void GetKeyRange(rocksdb::DB* db, const std::string& key1, const std::string& key2,  std::string& value) {
  rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    // Do something.
    //cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
  }
  assert(it->status().ok());  // Check for any errors found during the scan
  delete it;
}



rocksdb::DB* OpenRocksMetaDB(const std::string& dbname) {
  // TODO(weiren): different types of DB should have different config at open. same for now.
  rocksdb::DB* db_ptr;
  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  // Set Level-1 file size to 64 MB.
  options.target_file_size_base = 64 * 1024 * 1024;
  // open DB
  rocksdb::Status s = rocksdb::DB::Open(options, dbname, &db_ptr);
  LOG(INFO) << "OpenDB: " << dbname;
  assert(s.ok());
  return db_ptr;
}

rocksdb::DB* OpenRocksRecordDB(const std::string& dbname) {
  rocksdb::DB* db_ptr;
  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  // Set Level-1 file size to 64 MB.
  options.target_file_size_base = 64 * 1024 * 1024;
  // open DB
  rocksdb::Status s = rocksdb::DB::Open(options, dbname, &db_ptr);
  LOG(INFO) << "OpenDB: " << dbname;
  assert(s.ok());
  return db_ptr;
}

}
}
#endif
