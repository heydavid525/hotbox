#include "db/stat.hpp"
#include <algorithm>
#include <glog/logging.h>
#include <string>
#include "schema/constants.hpp"

namespace hotbox {

namespace {

const std::string kStatProtoPrefix = "stat_";

std::string MakePrefix(int id) {
  return kStatProtoPrefix + std::to_string(id);
}

std::string MakeSeqKey(int id, int segment_id) {
  return MakePrefix(id) + "_seg_" + std::to_string(segment_id);
}

}  // anonymous namespace

Stat::Stat(int epoch_begin, BigInt feature_dim) : proto_(new StatProto) {
  proto_->set_epoch_begin(epoch_begin);
}

Stat::Stat(StatProto* proto) : proto_(proto) { }

Stat::Stat(int id, RocksDB* db) {
  std::string stat_proto_str = db->Get(MakePrefix(id));
  StatProto stat_proto = StreamDeserialize<StatProto>(stat_proto_str);
  proto_.reset(new StatProto(stat_proto));

  // Get the segments.
  proto_->mutable_stats()->Reserve(proto_->num_stats());
  proto_->mutable_initialized()->Resize(proto_->num_stats(), false);
  for (int i = 0; i < proto_->num_segments(); ++i) {
    std::string seg_proto_str = db->Get(MakeSeqKey(id, i));
    auto segment_proto =
      StreamDeserialize<FeatureStatProtoSegment>(seg_proto_str);
    BigInt id_begin = segment_proto.id_begin();
    for (int j = 0; j < segment_proto.stats_size(); ++j) {
      *(proto_->add_stats()) = segment_proto.stats(j);
      proto_->set_initialized(j + id_begin, segment_proto.initialized(j));
    }
  }
  LOG(INFO) << "Stat reads " << proto_->num_segments() << " segments";
}

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

void Stat::Commit(int id, RocksDB* db) const {
  // stat_proto contains the non-repeated fields of proto_.
  StatProto stat_proto;
  stat_proto.set_num_data(proto_->num_data());
  stat_proto.set_epoch_begin(proto_->epoch_begin());
  int num_segments = std::ceil(static_cast<float>(proto_->stats_size())
      / kSeqBatchSize);
  stat_proto.set_num_segments(num_segments);
  stat_proto.set_num_stats(proto_->stats_size());
  db->Put(MakePrefix(id), StreamSerialize(stat_proto));

  // Chop repeated stats and initialized in StatProto to FeatureStatProtoSegment.
  for (int i = 0; i < num_segments; ++i) {
    BigInt id_begin = kSeqBatchSize * i;
    BigInt id_end = std::min(id_begin + kSeqBatchSize,
        static_cast<BigInt>(proto_->stats_size()));
    FeatureStatProtoSegment segment;
    segment.set_id_begin(id_begin);
    segment.mutable_stats()->Reserve(id_end - id_begin);
    segment.mutable_initialized()->Resize(id_end - id_begin, false);
    for (BigInt j = id_begin; j < id_end; ++j) {
      *segment.add_stats() = proto_->stats(j);
      segment.set_initialized(j - id_begin, proto_->initialized(j));
    }
    std::string seg_key = MakeSeqKey(id, i);
    db->Put(seg_key, StreamSerialize(segment));
    LOG(INFO) << "Commit: writing stats key: " << seg_key << " stat range: ["
      << id_begin << ", " << id_end << ")";
  }
  LOG(INFO) << "Stat commit " << num_segments << " segments";
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
