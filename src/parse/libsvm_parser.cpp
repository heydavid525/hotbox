#include <glog/logging.h>
#include <cctype>
#include <utility>
#include <cstdint>
#include <string>
#include "parse/libsvm_parser.hpp"
#include "schema/constants.hpp"

namespace hotbox {

void LibSVMParser::SetConfig(const ParserConfig& config) {
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
    Schema* schema,
    DatumBase* datum) const {
  char* ptr = nullptr, *endptr = nullptr;

  // Read label.
  float label = strtof(line.data(), &endptr);
  this->SetLabelAndWeight(schema, datum, label);
  ptr = endptr;

  // Use only single store type.
  bool simple_family = true;
  const auto& family = schema->GetOrCreateFamily(kDefaultFamily, simple_family,
      FeatureStoreType::SPARSE_NUM);

  std::vector<TypedFeatureFinder> not_found_features;

  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  while (ptr - line.data() < line.size()) {
    // read a feature_id:feature_val pair
    int32_t feature_id = strtol(ptr, &endptr, kBase);
    if (feature_one_based_) {
      --feature_id;
    }
    ptr = endptr;
    CHECK_EQ(':', *ptr);
    ++ptr;
    float val = strtof(ptr, &endptr);
    ptr = endptr;
    std::pair<Feature, bool> ret = family.GetFeatureNoExcept(feature_id);
    if (ret.second == false) {
      FeatureFinder not_found_feature;
      not_found_feature.family_name = kDefaultFamily;
      not_found_feature.family_idx = feature_id;
      TypedFeatureFinder typed_finder(not_found_feature,
          FeatureType::NUMERICAL);
      not_found_features.push_back(typed_finder);
    } else {
      datum->SetFeatureVal(ret.first, val);
    }
    while (isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  }
  return not_found_features;
}
}  // namespace hotbox
