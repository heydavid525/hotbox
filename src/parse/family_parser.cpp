#include <cctype>
#include <cstdint>
#include <utility>
#include <glog/logging.h>
#include "schema/constants.hpp"
#include "parse/family_parser.hpp"

namespace hotbox {

namespace {

// Return (new position of ptr, family_name). family_name would be
// kDefaultFamily if there's no string after '|'. 'ptr' must point at position
// before '|'.
std::pair<char*, std::string> ReadFamilyName(const std::string& line, char* ptr) {
  while (std::isspace(*ptr) && ptr - line.data() < line.size()) {
    ++ptr;
  }
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
  return std::make_pair(ptr, family_name);
}

}  // anonymous namespace

// Read a family. Return the pointer pointing at | or just after last char
// of line.
//
// ptr should pass the '|' but at or before the first non-whitespace.
char* FamilyParser::ReadFamily(const std::string& line, char* ptr, Schema* schema,
    DatumBase* datum, std::vector<TypedFeatureFinder>* not_found_features) {
  auto ret = ReadFamilyName(line, ptr);
  ptr = ret.first;
  std::string family_name = ret.second;
  const auto& family = schema->GetFamily(family_name);

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

namespace {

std::vector<TypedFeatureFinder> ReadFamilyDeclaration(const std::string& line,
    char* ptr, Schema* schema) {
  std::vector<TypedFeatureFinder> not_found_features;
  char *endptr = nullptr;
  while (ptr - line.data() < line.size()) {
    auto ret = ReadFamilyName(line, ptr);
    ptr = ret.first;
    std::string family_name = ret.second;

    // Read family range.
    while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
    if (ptr - line.data() == line.size()) {
      return not_found_features;
    }
    BigInt num_features = strtol(ptr, &endptr, 10);
    ptr = endptr;

    bool simple_family = true;
    const auto& family = schema->GetOrCreateFamily(family_name, simple_family,
        FeatureStoreType::SPARSE_NUM, num_features);
    if (family.GetNumFeatures() != num_features) {
      for (int i = 0; i < num_features; ++i) {
        FeatureFinder not_found_feature;
        not_found_feature.family_name = family_name;
        not_found_feature.family_idx = i;
        TypedFeatureFinder typed_finder(not_found_feature, FeatureType::NUMERICAL);
        not_found_features.push_back(typed_finder);
      }
    }
  }
  return not_found_features;
}

}  // anonymous namespace

std::vector<TypedFeatureFinder> FamilyParser::Parse(
    const std::string& line, Schema* schema,
    DatumBase* datum, bool* invalid) const {
  char* ptr = nullptr, *endptr = nullptr;

  std::vector<TypedFeatureFinder> not_found_features;

  // If first character is #, it denotes the number of features in each
  // family.
  if (line.at(0) == '#') {
    ptr = const_cast<char*>(line.data());
    *invalid = true;
    return ReadFamilyDeclaration(line, ++ptr, schema);
  }

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
