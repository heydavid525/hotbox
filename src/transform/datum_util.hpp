#pragma once

#include <string>
#include <cctype>
#include <cstdint>
#include <glog/logging.h>
#include "transform/proto/schema.pb.h"
#include "transform/schema.hpp"
#include "transform/datum_base.hpp"
#include "transform/schema_util.hpp"
#include <google/protobuf/text_format.h>

namespace mldb {
namespace datum_util {

namespace {

const int32_t kBase = 10;

// Read a family. Return the pointer pointing at | or just after last char
// of line.
//
// ptr should pass the '|' but at or before the first non-whitespace.
char* ReadFamily(const std::string& line, char* ptr, const Schema& schema,
    DatumBase* datum) {
  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  CHECK_EQ('|', *ptr);
  ++ptr;
  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  CHECK_LT(ptr - line.data(), line.size()) << "Empty family.";
  // atempt to read family name.
  std::string family_name = kDefaultFamily;
  if (!std::isdigit(*ptr)) {
    char* next_space = ptr;
    while (!std::isspace(*next_space)) ++next_space;
    family_name = std::string(ptr, next_space - ptr);
    ptr = next_space;
  }
  LOG(INFO) << "Read family name: " << family_name;
  const auto& family = schema.GetFamily(family_name);

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
    const Feature& feature = family.GetFeature(family_idx);
    //std::string loc_str;
    //CHECK(google::protobuf::TextFormat::PrintToString(feature.loc(),
    //&loc_str));
    //LOG(INFO) << "setting " << family_name << " family_idx: "
    //<< family_idx << " val: " << val << " feature loc: " << loc_str;
    datum->SetFeatureVal(feature.loc(), val);
  }
  return ptr;
}

// Create DatumProto with dense feature space allocated.
DatumProto* CreateDatumProtoFromOffset(const DatumProtoOffset& offset) {
  DatumProto* proto = new DatumProto;
  proto->mutable_dense_cat_store()->Resize(offset.dense_cat_store(), 0);
  proto->mutable_dense_num_store()->Resize(offset.dense_num_store(), 0.);
  proto->mutable_dense_bytes_store()->Reserve(offset.dense_bytes_store());
  return proto;
}

}   // anonymous namespace

// Parse string in the family format:
//
//    0 1 | family1 0:3.4 1:4 | family2 3:5.3 
//
// where the first 0 is label, 1 is the optional weight, indices (e.g., 0 in
// '0:3') are family index.
//
//    0 1 |0:3 1:4
//
// puts the features into default family. Whitespaces
//
// Log error message if value has different type
// than specified in the schema.
DatumBase CreateDatumFromFamilyString(const Schema& schema,
    const std::string& line) {
  DatumProto* proto =
    CreateDatumProtoFromOffset(schema.GetDatumProtoOffset());
  DatumBase datum(proto);

  char* ptr = nullptr, *endptr = nullptr;

  // Read label.
  float label = strtof(line.data(), &endptr);
  const auto& intern_family = schema.GetFamily(kInternalFamily);
  const Feature& feature = intern_family.GetFeature(kLabelFamilyIdx);
  datum.SetFeatureVal(feature.loc(), label);
  ptr = endptr;

  while (std::isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  if (ptr - line.data() == line.size()) {
    return datum;
  }

  // Read weight (if any).
  float weight = 1.;
  if (*ptr != '|') {
    weight = strtof(ptr, &endptr);
    ptr = endptr;
  }
  if (weight != 1.) {
    const Feature& feature = intern_family.GetFeature(kWeightFamilyIdx);
    datum.SetFeatureVal(feature.loc(), weight);
  }

  // Read families.
  while (ptr - line.data() < line.size()) {
    ptr = ReadFamily(line, ptr, schema, &datum);
  }

  return datum;
}

}  // namespace datum_util
}  // namespace mldb
