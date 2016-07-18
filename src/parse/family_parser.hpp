#pragma once

#include <string>
#include "parse/parser_if.hpp"
#include "schema/feature_finder.hpp"

namespace hotbox {

// Parse string in the family format. The file should start with first line
// defining number of features in each family:
//
//    # |family1 5 |family2 4
//
// (family1 has 5 features (0~4), family2 has 4 features). Each following
// line looks like:
//
//    0 1 | family1 0:3.4 1:4 | family2 3:5.3 
//
// where the first 0 is label, 1 is the optional weight, indices (e.g., 0 in
// '0:3') are family index.
//
//    0 1 |0:3 1:4
//
// puts the features into default family. Whitespaces
class FamilyParser : public ParserIf {
public:
  FamilyParser(const ParserConfig& config) : ParserIf(config) { }
protected:
  std::vector<TypedFeatureFinder> Parse(const std::string& line,
      Schema* schema, DatumBase* datum, bool* invalid) const override;

private:
  // Comment(wdai): ReadFamily uses InferType from ParserIf, thus has to be
  // under FamilyParser.
  static char* ReadFamily(const std::string& line, char* ptr, Schema* schema,
      DatumBase* datum, std::vector<TypedFeatureFinder>* not_found_features);
};

}  // namespace hotbox
