#include <glog/logging.h>
#include <string>
#include <vector>
#include "transform/transform_api.hpp"
#include "transform/ngram_transform.hpp"
#include <sstream>
#include <algorithm>
#include "util/util.hpp"
#include "schema/all.hpp"

namespace hotbox {

// Currently only support 2-gram of two simple families.
void NgramTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const std::multimap<std::string, StoreTypeAndOffset>& wide_family_offsets
    = param.GetFamilyWideStoreOffsets();
  CHECK_EQ(2, wide_family_offsets.size())
    << "Only support 2-gram of two simple family for now";
  std::vector<BigInt> num_features;
  for (const auto& p : wide_family_offsets) {
    StoreTypeAndOffset offsets = p.second;
    num_features.push_back(offsets.offset_end() - offsets.offset_begin());
  }
  // Add only anonymous features to reduce schema size.
  writer->AddFeatures(num_features[0] * num_features[1]);
}

namespace {

// Get sparse value of a simple family based on type_and_offset, indexed by
// family_idx (BigInt).
std::vector<std::pair<BigInt, float>> GetSparseVals(const TransDatum& datum,
    const StoreTypeAndOffset& type_and_offset) {
  std::vector<std::pair<BigInt, float>> sparse_vals;
  const DatumProto& proto = datum.GetDatumBase().GetDatumProto();
  auto offset_begin = type_and_offset.offset_begin();
  auto offset_end = type_and_offset.offset_end();
  switch (type_and_offset.store_type()) {
    case SPARSE_NUM:
      {
        auto low = std::lower_bound(
            proto.sparse_num_store_idxs().cbegin(),
            proto.sparse_num_store_idxs().cend(), offset_begin);

        // 'start' indexes the first non-zero element for the family,
        // if the family isn't all zero (if so, 'start' indexes the
        // beginning of next family.)
        auto start = low - proto.sparse_num_store_idxs().cbegin();
        for (int i = start; i < proto.sparse_num_store_idxs_size(); ++i) {
          if (proto.sparse_num_store_idxs(i) < offset_begin) {
            continue;
          }
          if (proto.sparse_num_store_idxs(i) >= offset_end) {
            break;
          }
          BigInt family_idx = proto.sparse_num_store_idxs(i)
            - offset_begin;
          sparse_vals.push_back(std::make_pair(family_idx,
                proto.sparse_num_store_vals(i)));
        }
        break;
      }
    default:
      LOG(FATAL) << type_and_offset.store_type() << " is not supported yet.";
  }
  return sparse_vals;
}
}  // anonymous namespace

std::function<void(TransDatum*)> NgramTransform::GenerateTransform(
    const TransformParam& param) const {
  const std::multimap<std::string, StoreTypeAndOffset>& wide_family_offsets
    = param.GetFamilyWideStoreOffsets();
  StoreTypeAndOffset type_and_offset = (++wide_family_offsets.begin())->second;
  BigInt num_features_fam2 = type_and_offset.offset_end() -
    type_and_offset.offset_begin();
  return [wide_family_offsets, num_features_fam2] (TransDatum* datum) {
    // Get the sparse value indexed by family_idx for both families.
    const auto& sparse_val1 = GetSparseVals(*datum,
        wide_family_offsets.begin()->second);
    const auto& sparse_val2 = GetSparseVals(*datum,
        (++wide_family_offsets.begin())->second);
    for (int i = 0; i < sparse_val1.size(); ++i) {
      for (int j = 0; j < sparse_val2.size(); ++j) {
        datum->SetFeatureValRelativeOffset(
            sparse_val1[i].first * num_features_fam2 + sparse_val2[j].first,
            sparse_val1[i].second * sparse_val2[j].second);
      }
    }
  };
}

}  // namespace hotbox
