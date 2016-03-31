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

void SelectTransform::SetTransformWriterConfig(const TransformConfig& config,
    TransformWriterConfig* writer_config) const {
  LOG(INFO) << "SelectTransform::SetTransformWriterConfig";
  writer_config->set_output_simple_family(true);
}

void SelectTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const auto& non_simple_input_features = param.GetInputFeaturesByFamily();
  const auto& non_simple_input_features_desc =
    param.GetInputFeaturesDescByFamily();
  std::vector<std::string> wide_families = param.GetFamilyWideFamilies();

  // Add features that are not in wide-family.
  for (const auto& p : non_simple_input_features) {
    std::string family_name = p.first;
    const auto& family_features = p.second;
    const auto& family_features_desc =
      non_simple_input_features_desc.at(family_name);
    if (!StringInVector(family_name, wide_families)) {
      // This family isn't selected by family-wide. Add them.
      for (int i = 0; i < p.second.size(); ++i) {
        const auto& input_feature = family_features[i];
        CHECK(IsNumber(input_feature)) << "family: " << family_name << " "
          << input_feature.DebugString();
        writer->AddFeature(family_features_desc[i]);
      }
    }
  }

  // Add the family-wide selected families. Note: Family-wide and wide_family
  // are used synonymously
  const std::map<std::string, StoreTypeAndOffset>& wide_family_offsets
    = param.GetFamilyWideStoreOffsets();
  for (const std::string& f : wide_families) {
    StoreTypeAndOffset offsets = wide_family_offsets.at(f);
    writer->AddFeatures(offsets.offset_end() - offsets.offset_begin());
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

  // wide_family_offsets is the offset on input store.
  const std::map<std::string, StoreTypeAndOffset>& wide_family_offsets
    = param.GetFamilyWideStoreOffsets();

  return [input_features, wide_families, wide_family_offsets,
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
    for (const auto& p : wide_family_offsets) {
      StoreTypeAndOffset type_and_offset = p.second;
      auto offset_begin = type_and_offset.offset_begin();
      auto offset_end = type_and_offset.offset_end();
      BigInt output_offset = wide_family_output_offsets.at(p.first);
      switch (type_and_offset.store_type()) {
        case SPARSE_NUM:
          {
            auto low = std::lower_bound(
                proto.sparse_num_store_idxs().cbegin(),
                proto.sparse_num_store_idxs().cend(), offset_begin);
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
        default:
          LOG(FATAL) << "Not supported yet.";
      }
    }
  };
}

}  // namespace hotbox
