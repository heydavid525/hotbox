#pragma once

#include "parser/parser_if.hpp"
#include "parser/proto/parser_configs.pb.h"
#include "schema/datum_base.hpp"

namespace mldb {

class LibSVMParser : public ParserIf {
public:
  LibSVMParser(const LibSVMParserConfig& config);

  void Parse(const std::string& line, Schema* schema,
      DatumBase* datum) const override;

private:
  bool feature_one_based_;
  bool label_one_based_;
};

}  // namespace mldb
