#pragma once

#include <string>
#include "parse/parser_if.hpp"

namespace mldb {

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
class FamilyParser : public NoConfigParserIf {
protected:
  void Parse(const std::string& line, Schema* schema, DatumBase* datum)
    const override;

private:
  // Comment(wdai): ReadFamily uses InferType from ParserIf, thus has to be
  // under FamilyParser.
  static char* ReadFamily(const std::string& line, char* ptr, Schema* schema,
      DatumBase* datum);

};

}  // namespace mldb
