#include <glog/logging.h>
#include <functional>
#include "schema/schema_util.hpp"
#include "transform/one_hot_transform.hpp"
#include "transform/proto/transform_configs.pb.h"

namespace mldb {

OneHotTransform::OneHotTransform(const OneHotTransformOp& op) {
  for (int i = 0; i < op.units_size(); ++i) {
    const auto& unit = op.units(i);
    const auto& input_feature_loc = unit.input_loc();
    std::function<void(DatumBase*)> binning_func;
    int32_t output_offset_start = unit.output_offset_start();
    if (unit.buckets_size() == 0) {
      // Categorical feature into natural binning.
      int min = unit.min();
      int max = unit.max();
      // bin into num_buckets in sparse_cat_store starting with
      // 'output_offset_start'.
      binning_func =
        [input_feature_loc, output_offset_start, min, max]
        (DatumBase* datum) {
          int val = static_cast<int>(datum->GetFeatureVal(input_feature_loc));
          CHECK_LE(val, max);
          CHECK_LE(min, val);
          int bin_id = val - min;
          datum->SetSparseCatFeatureVal(output_offset_start + bin_id, 1);
        };
      transforms_.push_back(binning_func);
    } else {
      // Use the specified buckets.
      const auto& buckets = unit.buckets();
      binning_func =
        [input_feature_loc, output_offset_start, buckets]
        (DatumBase* datum) {
          float val = datum->GetFeatureVal(input_feature_loc);
          if (val < buckets.Get(0)) {
            // First bucket
            datum->SetSparseCatFeatureVal(output_offset_start, 1);
          } else {
            for (int i = 0; i < buckets.size(); ++i) {
              if (val > buckets.Get(i)) {
                datum->SetSparseCatFeatureVal(output_offset_start + i + 1, 1);
                break;
              }
            }
          }
        };
      transforms_.push_back(binning_func);
    }
  }
}

void OneHotTransform::Transform(DatumBase* datum) const {
  //CHECK(this->recv_schema_);
  for (int i = 0; i < transforms_.size(); ++i) {
    transforms_[i](datum);
  }
}

}  // namespace mldb
