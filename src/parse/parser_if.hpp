#pragma once

#include "schema/schema.hpp"
#include "schema/datum_base.hpp"
#include "parse/proto/parser_config.pb.h"
#include <cmath>

namespace hotbox {

// useful for strtol function.
const int kBase = 10;

class ParserIf {
public:
  virtual void SetConfig(const ParserConfig& config) = 0;

  // Parse and add features to schema if not found.
  DatumBase ParseAndUpdateSchema(const std::string& line, Schema* schema)
    noexcept;

  virtual ~ParserIf();

protected:
  // datum's dense store is preallocated according to schema.
  // May throw TypedFeaturesNotFoundException. Caller needs to free datum even
  // during exception.
  virtual void Parse(const std::string& line, Schema* schema,
      DatumBase* datum) const = 0;

  // Infer float or int.
  static FeatureType InferType(float val);

  static void SetLabelAndWeight(Schema* schema, DatumBase* datum,
      float label, float weight = 1.);

private:
  // Create DatumProto with dense feature space allocated.
  static DatumProto* CreateDatumProtoFromOffset(const DatumProtoOffset& offset);
};

class NoConfigParserIf : public ParserIf {
public:
  void SetConfig(const ParserConfig& config) override { }
};

}  // namespace hotbox
