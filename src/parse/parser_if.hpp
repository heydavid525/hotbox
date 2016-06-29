#pragma once

#include "schema/schema.hpp"
#include "schema/datum_base.hpp"
#include "parse/proto/parser_config.pb.h"
#include <cmath>
#include <vector>

namespace hotbox {

class ParserIf {
public:
  virtual void SetConfig(const ParserConfig& config) = 0;

  // Parse and add features to schema if not found.
  DatumBase ParseAndUpdateSchema(const std::string& line, Schema* schema,
      StatCollector* stat_collector, bool* invalid) noexcept;

  virtual ~ParserIf();

protected:
  // datum's dense store is preallocated according to schema.  Return features
  // that are not found, and not parse this line. Caller needs to free datum
  // even during exception. 'invalid' = true if the line is a comment.
  virtual std::vector<TypedFeatureFinder> Parse(const std::string& line, Schema* schema,
      DatumBase* datum, bool* invalid) const = 0;

  // Infer float or int.
  static FeatureType InferType(float val);

  static void SetLabelAndWeight(Schema* schema, DatumBase* datum,
      float label, float weight = 1.);

private:
  // Create DatumProto with dense feature space allocated.
  static DatumProto* CreateDatumProtoFromOffset(
      const DatumProtoStoreOffset& offset);
};

class NoConfigParserIf : public ParserIf {
public:
  void SetConfig(const ParserConfig& config) override { }
};

}  // namespace hotbox
