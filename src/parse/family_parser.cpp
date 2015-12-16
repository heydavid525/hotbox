#include <cctype>
#include <cstdint>
#include <utility>
#include <glog/logging.h>
#include "schema/constants.hpp"
#include "parse/family_parser.hpp"

namespace hotbox {

// Read a family. Return the pointer pointing at | or just after last char
// of line.
//
// ptr should pass the '|' but at or before the first non-whitespace.
char* FamilyParser::ReadFamily(const std::string& line, char* ptr, Schema* schema,
    DatumBase* datum, std::vector<TypedFeatureFinder>* not_found_features) {
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
  while (ptr - line.data() < line.size()) {
    while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
    if (*ptr == '|' || ptr - line.data() == line.size()) break;
    int32_t family_idx = strtol(ptr, &endptr, kBase);
    ptr = endptr;
    CHECK_EQ(':', *ptr);
    ++ptr;
    float val = strtod(ptr, &endptr);
    ptr = endptr;
    std::pair<Feature, bool> ret = family.GetFeatureNoExcept(family_idx);
    if (ret.second == false) {
      FeatureFinder not_found_feature;
      not_found_feature.family_name = family_name;
      not_found_feature.family_idx = family_idx;
      TypedFeatureFinder typed_finder(not_found_feature, InferType(val));
      not_found_features->push_back(typed_finder);
    } else {
      datum->SetFeatureVal(ret.first, val);
    }
  }
  return ptr;
}

std::vector<TypedFeatureFinder> FamilyParser::Parse(
    const std::string& line, Schema* schema,
    DatumBase* datum) const {
  char* ptr = nullptr, *endptr = nullptr;

  std::vector<TypedFeatureFinder> not_found_features;

  // Read label.
  float label = strtof(line.data(), &endptr);
  ptr = endptr;

  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  if (ptr - line.data() == line.size()) {
    return not_found_features;
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
    ptr = ReadFamily(line, ptr, schema, datum, &not_found_features);
  }
  return not_found_features;
}

}  // namespace hotbox
