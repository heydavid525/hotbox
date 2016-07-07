#include <glog/logging.h>
#include <string>
#include <vector>
#include "transform/transform_api.hpp"
#include "transform/select_transform.hpp"
#include <sstream>
#include <algorithm>
#include "util/util.hpp"

namespace hotbox {

namespace {

// Return true if vec[i] == target for some i. False otherwise.
bool StringInVector(const std::string& target,
    const std::vector<std::string>& vec) {
  for (const auto& v : vec) {
    if (v == target) {
      return true;
    }
  }
  return false;
}

}  // anonymous namespace

void SelectTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const auto& non_simple_input_features = param.GetInputFeaturesByFamily();
  //const auto& non_simple_input_features_desc =
  //  param.GetInputFeaturesDescByFamily();
  std::vector<std::string> wide_families = param.GetFamilyWideFamilies();

  // Add features that are not in wide-family.
  for (const auto& p : non_simple_input_features) {
    std::string family_name = p.first;
    const auto& family_features = p.second;
    if (!StringInVector(family_name, wide_families)) {
      // This family isn't selected by family-wide. Add them.
      for (int i = 0; i < p.second.size(); ++i) {
        const auto& input_feature = family_features[i];
        CHECK(IsNumber(input_feature)) << "family: " << family_name << " "
          << input_feature.DebugString();
        writer->AddFeature(input_feature);
      }
    }
  }

  // Add the family-wide selected families. Note: Family-wide and wide_family
  // are used synonymously
  const std::multimap<std::string, WideFamilySelector>& wide_family_selectors
    = param.GetWideFamilySelectors();
  for (const std::string& f : wide_families) {
    const auto it = wide_family_selectors.find(f);
    CHECK(it != wide_family_selectors.cend()) << f
      << " is not found in wide_family_offset";
    StoreTypeAndOffset offsets = it->second.offset;
    BigInt num_features = offsets.offset_end() - offsets.offset_begin();
    auto range = it->second.range_selector;
    if (range.family_idx_end != range.family_idx_begin) {
      num_features = range.family_idx_end - range.family_idx_begin;
    }
    writer->AddFeatures(num_features);
  }
}


std::function<void(TransDatum*)> SelectTransform::GenerateTransform(
    const TransformParam& param) const {
  auto input_features = param.GetInputFeaturesByFamily();
  std::vector<std::string> wide_families = param.GetFamilyWideFamilies();
  // Remove family-wide families. Store # of features in each of them first.
  std::map<std::string, BigInt> wide_family_output_offsets;
  for (const std::string& f : wide_families) {
    wide_family_output_offsets[f] = input_features[f].size();
    input_features.erase(f);
  }

  // Find out the write/output offset of each wide-family.
  BigInt first_wide_family_offset = 0;
  for (const auto& p : input_features) {
    first_wide_family_offset += p.second.size();
  }
  BigInt curr_family_offset = first_wide_family_offset;
  for (auto& p : wide_family_output_offsets) {
    BigInt num_features_this_family = p.second;
    p.second = curr_family_offset;
    curr_family_offset += num_features_this_family;
  }

  // wide_family_selectors is the offset on input store.
  const std::multimap<std::string, WideFamilySelector>& wide_family_selectors
    = param.GetWideFamilySelectors();

  return [input_features, wide_families, wide_family_selectors,
         wide_family_output_offsets] (TransDatum* datum) {
    for (const auto& p : input_features) {
      std::string family_name = p.first;
      const auto& family_features = p.second;
      for (int i = 0; i < p.second.size(); ++i) {
        const auto& input_feature = family_features[i];
        float val = datum->GetFeatureVal(input_feature);
        if (val != 0) {
          datum->SetFeatureValRelativeOffset(i, val);
        }
      }
    }
    // Family-wide families that use single store.
    const DatumProto& proto = datum->GetDatumBase().GetDatumProto();
    for (const auto& p : wide_family_selectors) {
      StoreTypeAndOffset type_and_offset = p.second.offset;
      auto offset_begin = type_and_offset.offset_begin();
      auto offset_end = type_and_offset.offset_end();
      auto range = p.second.range_selector;
      BigInt family_idx_begin = range.family_idx_begin;
      BigInt family_idx_end = range.family_idx_end;
      // Further limit the offset using range, if available.
      if (family_idx_begin != family_idx_end) {
        offset_end = family_idx_end + offset_begin;
        offset_begin = family_idx_begin + offset_begin;
      }
      BigInt output_offset = wide_family_output_offsets.at(p.first);
      switch (type_and_offset.store_type()) {
        case FeatureStoreType::SPARSE_NUM:
          {
            auto low = std::lower_bound(
                proto.sparse_num_store_idxs().cbegin(),
                proto.sparse_num_store_idxs().cend(), offset_begin);

            // 'start' indexes the first non-zero element for family p.first,
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
              datum->SetFeatureValRelativeOffset(output_offset + family_idx,
                  proto.sparse_num_store_vals(i));
            }
            break;
          }
        case FeatureStoreType::DENSE_NUM:
          {
            for (BigInt i = offset_begin; i < offset_end; i++) {
              BigInt family_idx = i - offset_begin;
              datum->SetFeatureValRelativeOffset(output_offset + family_idx,
                  proto.dense_num_store(i));
            }
            break;
          }
        default:
          LOG(FATAL) << "store type " << type_and_offset.store_type()
            << " is not supported yet.";
      }
    }
  };
}

}  // namespace hotbox
