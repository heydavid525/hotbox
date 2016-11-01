#include <glog/logging.h>
#include <cctype>
#include <utility>
#include <cstdint>
#include <string>
#include "parse/libsvm_parser.hpp"
#include "schema/constants.hpp"
#include "util/util.hpp"

namespace hotbox {

LibSVMParser::LibSVMParser(const ParserConfig& config) :
  ParserIf(config) {
  // Default is to not change feature nor label (feature_one_based_ and
  // label_one_based_ are false).
  if (config.has_libsvm_config()) {
    const LibSVMParserConfig& libsvm_config = config.libsvm_config();
    feature_one_based_ = libsvm_config.feature_one_based();
    label_one_based_ = libsvm_config.label_one_based();
  }
  LOG(INFO) << "feature_one_based_: " << feature_one_based_;
  LOG(INFO) << "label_one_based_: " << label_one_based_;
}

std::vector<TypedFeatureFinder> LibSVMParser::Parse(const std::string& line,
    Schema* schema, DatumBase* datum, bool* invalid) const {
  char* ptr = nullptr, *endptr = nullptr;

  // Read label.
  float label = StringToFloat(line.data(), &endptr);
  this->SetLabelAndWeight(schema, datum, label);
  ptr = endptr;

  // Use only single store type.
  bool simple_family = true;
  const FeatureFamilyIf& family_if =
    schema->GetOrCreateFamily(kDefaultFamily,
    simple_family, FeatureStoreType::SPARSE_NUM);
  const auto& family =
    dynamic_cast<const SimpleFeatureFamily&>(family_if);
  BigInt num_features = family.GetNumFeatures();
  auto store_type_offset = family.GetStoreTypeAndOffset();
  BigInt store_offset = store_type_offset.offset_begin();
  BigInt global_offset = family.GetGlobalOffset();

  // A bit hacky: construct features instead of looking up in schema.
  Feature f;
  f.set_store_type(store_type_offset.store_type());

  std::vector<TypedFeatureFinder> not_found_features;

  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  while (ptr - line.data() < line.size()) {
    // read a feature_id:feature_val pair
    int feature_id = StringToInt(ptr, &endptr);
    if (feature_one_based_) {
      --feature_id;
    }
    ptr = endptr;
    CHECK_EQ(':', *ptr);
    ++ptr;
    float val = StringToFloat(ptr, &endptr);
    ptr = endptr;
    if (feature_id >= num_features) {
      FeatureFinder not_found_feature;
      not_found_feature.family_name = kDefaultFamily;
      not_found_feature.family_idx = feature_id;
      TypedFeatureFinder typed_finder(not_found_feature,
          FeatureType::NUMERICAL);
      not_found_features.push_back(typed_finder);
    } else {
      f.set_store_offset(store_offset + feature_id);
      f.set_global_offset(global_offset + feature_id);
      datum->SetFeatureVal(f, val);
    }
    while (isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  }
  return not_found_features;
}
}  // namespace hotbox
