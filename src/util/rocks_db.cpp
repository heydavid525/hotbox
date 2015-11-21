#include "util/rocks_db.hpp"

namespace hotbox {

  // 'db_path' is a file system directory to create / read a DB.
 RocksDB::RocksDB(const std::string& db_path) {
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



}   // namespace hotbox
