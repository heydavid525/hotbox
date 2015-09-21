#include <glog/logging.h>
#include <cctype>
#include <utility>
#include <cstdint>
#include <string>
#include "parse/libsvm_parser.hpp"
#include "schema/constants.hpp"

namespace mldb {

void LibSVMParser::SetConfig(const ParserConfig& config) {
  // Default is to not change feature nor label (feature_one_based_ and
  // label_one_based_ are false).
  if (config.has_libsvm_config()) {
    const LibSVMParserConfig& libsvm_config = config.libsvm_config();
    feature_one_based_ = libsvm_config.feature_one_based();
    label_one_based_ = libsvm_config.label_one_based();
  }
}

void LibSVMParser::Parse(const std::string& line, Schema* schema,
    DatumBase* datum) const {
  char* ptr = nullptr, *endptr = nullptr;

  // Read label.
  float label = strtof(line.data(), &endptr);
  this->SetLabelAndWeight(schema, datum, label);
  ptr = endptr;

  const auto& family = schema->GetOrCreateFamily(kDefaultFamily);

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
    try {
      const Feature& feature = family.GetFeature(feature_id);
      datum->SetFeatureVal(feature.loc(), val);
    } catch (const FeatureNotFoundException& e) {
      TypedFeatureFinder typed_finder(e.GetNotFoundFeature(),
          this->InferType(val));
      // TODO(wdai): Remove these checks.
      CHECK_NE(-1, typed_finder.family_idx);
      CHECK_EQ(0, typed_finder.family_name.compare(kDefaultFamily));
      not_found_features.push_back(typed_finder);
    }
    while (isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  }
  if (not_found_features.size() > 0) {
    TypedFeaturesNotFoundException e;
    e.SetNotFoundTypedFeatures(std::move(not_found_features));
    throw e;
  }
}
}  // namespace mldb
