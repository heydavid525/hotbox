#pragma once

#include "parse/parser_if.hpp"
#include "parse/proto/parser_config.pb.h"
#include "schema/datum_base.hpp"
#include "schema/feature_finder.hpp"

namespace hotbox {

class LibSVMParser : public ParserIf {
public:
  void SetConfig(const ParserConfig& config);

protected:
  std::vector<TypedFeatureFinder> Parse(const std::string& line, Schema* schema,
      DatumBase* datum, bool* invalid) const override;

private:
  bool feature_one_based_{false};
  bool label_one_based_{false};
};

}  // namespace hotbox
