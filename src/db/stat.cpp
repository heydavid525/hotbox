#include "db/stat.hpp"
#include <algorithm>
#include <glog/logging.h>

namespace hotbox {

Stat::Stat(int epoch_begin, BigInt feature_dim) : proto_(new StatProto) {
  proto_->set_epoch_begin(epoch_begin);
}

Stat::Stat(StatProto* proto) : proto_(proto) { }

const FeatureStatProto& Stat::GetFeatureStat(const Feature& feature) const {
  return proto_->stats(feature.global_offset());
}

FeatureStatProto& Stat::GetMutableFeatureStat(const Feature& feature) {
  return *(proto_->mutable_stats(feature.global_offset()));
}

void Stat::AddFeatureStat(const Feature& feature) {
  auto offset = feature.global_offset();
  if (offset >= proto_->stats_size()) {
    const auto curr_stats_size = proto_->stats_size();
    proto_->mutable_stats()->Reserve(offset + 1);
    proto_->mutable_initialized()->Resize(offset + 1, false);
    // Add stats to feature's offset.
    for (int i = curr_stats_size; i < offset + 1; ++i) {
      proto_->add_stats();
    }
  }
  CHECK(!proto_->initialized(offset)) << "Feature "
    << offset << " already exists in stat.";
  proto_->set_initialized(offset, true);
}

int Stat::UpdateStat(const Feature& feature, float val) {
  auto global_offset = feature.global_offset();
  CHECK_LT(global_offset, proto_->stats_size()) << " feature: "
    << feature.DebugString() << " feature val: " << val
    << " num data: " << proto_->num_data();
  auto stat = proto_->mutable_stats(global_offset);
  if (feature.is_factor()) {
    // LOG(INFO) << "feature " << feature.global_offset() << " is factor";
    bool exist = false;
    for (int i = 0; i < stat->unique_cat_values_size(); ++i) {
      if (val == stat->unique_cat_values(i)) {
        exist = true;
        break;
      }
    }
    if (!exist) {
      // LOG(INFO) << "Adding to unique values";
      stat->add_unique_cat_values(static_cast<int>(val));
    }
  }
  UpdateStatCommon(stat, val);
  return stat->unique_cat_values_size();
}

void Stat::IncrementDataCount() {
  proto_->set_num_data(proto_->num_data() + 1);
}

void Stat::UpdateStatCommon(FeatureStatProto* stat, float val) {
  double min = std::min(stat->min(), static_cast<double>(val));
  double max = std::max(stat->max(), static_cast<double>(val));
  double sum = stat->sum() + val;
  stat->set_min(min);
  stat->set_max(max);
  stat->set_sum(sum);
}

}  // namespace hotbox
