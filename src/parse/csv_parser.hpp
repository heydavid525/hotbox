#pragma once

#include "parse/parser_if.hpp"
#include "parse/proto/parser_config.pb.h"
#include "schema/datum_base.hpp"

namespace hotbox {
  
class CSVParser : public ParserIf {
public:
  void SetConfig(const ParserConfig& config);
    
protected:
  std::vector<TypedFeatureFinder> Parse(const std::string& line, Schema* schema,
               DatumBase* datum) const override;
    
private:
  mutable int header_position_{0};
  bool first_column_label_{false};
 // int myHasHeader{0};
  // bool label_end{false};
};

}  // namespace hotbox
