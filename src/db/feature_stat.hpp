#pragma once

#include "db/proto/db.pb.h"
#include <memory>
#include <algorithm>

namespace hotbox {

// Wrapper around FeatureStatProto
class FeatureStat {
public:
  // Takes the ownership of proto.
  FeatureStat(FeatureStatProto* proto) : proto_(proto) { }

  float GetMax() const {
    return proto_->max();
  }

  void UpdateMax(float new_val) {
    proto_->set_max(std::max(proto_->max(), new_val));
  }

  float GetMin() const {
    return proto_->min();
  }

  void UpdateMin(float new_val) {
    proto_->set_min(std::min(proto_->min(), new_val));
  }


private:
  std::unique_ptr<FeatureStatProto> proto_;
};

}  // namespace hotbox
