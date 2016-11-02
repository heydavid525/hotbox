#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/normalize_transform.hpp"
#include "transform/transform_util.hpp"
#include <sstream>
#include "util/util.hpp"

namespace hotbox {

void NormalizeTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const std::multimap<std::string, WideFamilySelector>& 
  w_family_selectors = param.GetWideFamilySelectors();
  CHECK_EQ(1, w_family_selectors.size())
    << "Only support 1 simple family for now";
  for (const auto& p : w_family_selectors) {
    StoreTypeAndOffset offsets = p.second.offset;
    auto range = p.second.range_selector;
    int64_t family_idx_begin = range.family_idx_begin;
    int64_t family_idx_end = range.family_idx_end;
    int64_t family_num_features = offsets.offset_end() -
      offsets.offset_begin();
    if (family_idx_begin != family_idx_end) {
      // family-wide selection with family index, e.g., "fam:10-20"
      family_num_features = family_idx_end - family_idx_begin;
    }
    writer->AddFeatures(family_num_features);
  }
}

// Only support single family.
std::function<void(TransDatum*)> NormalizeTransform::GenerateTransform(
    const TransformParam& param) const {
  const std::multimap<std::string, WideFamilySelector>& 
    w_family_selectors = param.GetWideFamilySelectors();
  WideFamilySelector selector = w_family_selectors.begin()->second;
  return [selector] (TransDatum* datum) {
    // Get the sparse value indexed by family_idx for both families.
    const auto& sparse_val = GetSparseVals(*datum, selector);
    float L2_sum = 0.;
    for (const auto& p : sparse_val) {
      L2_sum += p.second * p.second;
    }
    for (const auto& p : sparse_val) {
      datum->SetFeatureValRelativeOffset(p.first, p.second / L2_sum);
    }
  };
}

}  // namespace hotbox
