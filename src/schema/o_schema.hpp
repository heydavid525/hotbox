#pragma once
#include "schema/constants.hpp"
#include "schema/proto/schema.pb.h"
#include <string>
#include <utility>

namespace hotbox {

// A zero-copy wrapper around OSchemaProto (OutputSchema).
class OSchema {
public:
  // Does NOT take the ownership of proto. proto
  OSchema(const OSchemaProto& proto);

  // Return <family_name, feature_name> pair.
  std::pair<std::string,std::string> GetName(BigInt feature_id) const;

  // Print schema as
  // [dim] |family1 feature_name1 feature_name2 |family2 feature_name3
  std::string ToString() const;

private:
  const OSchemaProto& proto_;
};

}  // namespace hotbox
