#pragma once

#include "util/all.hpp"
#include "db/proto/db.pb.h"
#include <utility>

namespace hotbox {

// A zero-copy wrapper around StatProto. Not copyable.
class Stat {
public:
  // optional feature_dim will set # of FeatureStats. Otherwise grow as
  // features are added (slower).
  Stat(int epoch_begin, BigInt feature_dim = 0);

  // Takes the ownership of proto.
  Stat(StatProto* proto);

  // Reading off the id-th stat from db, the inverse of Commit().
  Stat(int id, RocksDB* db);

  // Get FeatureStat
  const FeatureStatProto& GetFeatureStat(const Feature& feature) const;

  FeatureStatProto& GetMutableFeatureStat(const Feature& feature);

  // Add a new feature. Resize StatProto::stats if necessary.
  void AddFeatureStat(const Feature& feature);

  // Update number types. Return # of unique values for this feature, so
  // caller can infer if feature is a factor feature. Return 0 for non-factor
  // features.
  int UpdateStat(const Feature& feature, float val);

  void IncrementDataCount();

  inline StatProto* Release() {
    return proto_.release();
  }

  inline const StatProto& GetProto() const {
    return *proto_;
  }

  // Save to RocksDB db as the id-th stat.
  void Commit(int id, RocksDB* db) const;

private:
  void UpdateStatCommon(FeatureStatProto* stat, float val);

private:
  std::unique_ptr<StatProto> proto_;
};

}  // namespace hotbox
