#include <cctype>
#include <cstdint>
#include <utility>
#include <glog/logging.h>
#include "schema/constants.hpp"
#include "parse/family_parser.hpp"

namespace mldb {

// Read a family. Return the pointer pointing at | or just after last char
// of line.
//
// ptr should pass the '|' but at or before the first non-whitespace.
char* FamilyParser::ReadFamily(const std::string& line, char* ptr, Schema* schema,
    DatumBase* datum) {
  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  CHECK_EQ('|', *ptr);
  ++ptr;
  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  CHECK_LT(ptr - line.data(), line.size()) << "Empty family.";
  // attempt to read family name.
  std::string family_name = kDefaultFamily;
  if (!std::isdigit(*ptr)) {
    char* next_space = ptr;
    while (!std::isspace(*next_space)) ++next_space;
    family_name = std::string(ptr, next_space - ptr);
    ptr = next_space;
  }
  const auto& family = schema->GetOrCreateFamily(family_name);

  char *endptr = nullptr;
  std::vector<TypedFeatureFinder> not_found_features;
  while (ptr - line.data() < line.size()) {
    while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
    if (*ptr == '|' || ptr - line.data() == line.size()) break;
    int32_t family_idx = strtol(ptr, &endptr, kBase);
    ptr = endptr;
    CHECK_EQ(':', *ptr);
    ++ptr;
    float val = strtod(ptr, &endptr);
    ptr = endptr;
    try {
      const Feature& feature = family.GetFeature(family_idx);
      //std::string loc_str;
      //CHECK(google::protobuf::TextFormat::PrintToString(feature.loc(),
      //&loc_str));
      //LOG(INFO) << "setting " << family_name << " family_idx: "
      //<< family_idx << " val: " << val << " feature loc: " << loc_str;
      datum->SetFeatureVal(feature.loc(), val);
    } catch (const FeatureNotFoundException& e) {
      TypedFeatureFinder typed_finder(e.GetNotFoundFeature(), InferType(val));
      // TODO(wdai): Remove these checks.
      CHECK_NE(-1, typed_finder.family_idx);
      CHECK_EQ(0, typed_finder.family_name.compare(family_name));
      not_found_features.push_back(typed_finder);
    }
  }
  if (not_found_features.size() > 0) {
    TypedFeaturesNotFoundException e;
    e.SetNotFoundTypedFeatures(std::move(not_found_features));
    throw e;
  }
  return ptr;
}

void FamilyParser::Parse(const std::string& line, Schema* schema,
    DatumBase* datum) const {
    char* ptr = nullptr, *endptr = nullptr;

    // Read label.
    float label = strtof(line.data(), &endptr);
    ptr = endptr;

    while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
    if (ptr - line.data() == line.size()) {
      return;
    }

    // Read weight (if any).
    float weight = 1.;
    if (*ptr != '|') {
      weight = strtof(ptr, &endptr);
      ptr = endptr;
    }
    this->SetLabelAndWeight(schema, datum, label, weight);

    // Read families.
    while (ptr - line.data() < line.size()) {
      ptr = ReadFamily(line, ptr, schema, datum);
    }
  }

}  // namespace mldb
