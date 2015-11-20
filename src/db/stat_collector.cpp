#include "db/stat_collector.hpp"
#include <glog/logging.h>

namespace hotbox {

StatCollector::StatCollector(std::vector<Stat>* stats) : stats_(stats),
  num_unique_(stats->size()) { }

std::vector<Stat>& StatCollector::GetStats() {
  return *stats_;
}

void StatCollector::DatumCreateBegin() {
  num_pending_updates_ = 0;
}

void StatCollector::AddFeatureStat(const Feature& feature) {
  for (int j = 0; j < stats_->size(); ++j) {
    (*stats_)[j].AddFeatureStat(feature);
  }
}

void StatCollector::UpdateStat(const Feature& feature, float val) {
  if (num_pending_updates_ == pending_update_features_.size()) {
    pending_update_features_.push_back(feature);
    vals_.push_back(val);
    num_pending_updates_++;
    num_unique_.resize(num_pending_updates_);
  } else {
    pending_update_features_[num_pending_updates_] = feature;
    vals_[num_pending_updates_++] = val;
  }
}

StatUpdateOutput StatCollector::DatumCreateEnd() {
  for (int j = 0; j < stats_->size(); ++j) {
    for (int i = 0; i < num_pending_updates_; ++i) {
      const Feature& f = pending_update_features_[i];
      int num_unique = (*stats_)[j].UpdateStat(f, vals_[i]);
      num_unique_[i] = std::max(num_unique_[i], num_unique);
    }
    (*stats_)[j].IncrementDataCount();
  }
  StatUpdateOutput output(pending_update_features_, num_unique_,
      num_pending_updates_);
  num_pending_updates_ = 0;
  return output;
}

}  // namespace hotbox
