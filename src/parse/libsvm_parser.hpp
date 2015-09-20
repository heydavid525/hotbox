#pragma once

#include "parse/parser_if.hpp"
#include "parse/proto/parser_configs.pb.h"
#include "schema/datum_base.hpp"

namespace mldb {

class LibSVMParser : public ParserIf {
public:
  void SetConfig(const ParserConfig& config);

protected:
  void Parse(const std::string& line, Schema* schema,
      DatumBase* datum) const override;

private:
  bool feature_one_based_;
  bool label_one_based_;
};

}  // namespace mldb
