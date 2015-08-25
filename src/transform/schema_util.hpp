#pragma once

#include "transform/proto/schema.pb.h"
#include "util/string_util.hpp"
#include "util/mldb_exception.hpp"
#include <string>
#include <utility>
#include <sstream>

namespace mldb {

const std::string kDefaultFamily = "default";

namespace schema_util {

class ParseException: public MLDBException {
public:
  ParseException(const std::string msg) : MLDBException(msg) { }
};

// Parse feature descriptor (e.g., "mobile:ctr,num_views"). Return (feature_family,
// name) pairs ordered by appearance in feature descriptor.
//
// Some valid feature descriptors:
// "feat1,feat2,fam1:feat3,:feat4" --> [(default, feat1), (default, feat2), (fam1,
// feat3), (default, feat4)]
// "feat5, feat6, fam2:feat7, feat8" --> [(default, feat5), (default, feat6),
// (fam2, feat7), (fam2, feat7)]
//
// TODO(wdai): Beef up the error checking and messages.
std::vector<std::pair<std::string, std::string>> ParseFeatureDesc(
    const std::string& feature_desc) {
  std::vector<std::pair<std::string, std::string>> pairs;
  std::vector<std::string> features = SplitString(feature_desc, ',');
  // With curr_family, family from features[0] carries to features[1] if
  // features[1] doesn't have a family specification.
  std::string curr_family = kDefaultFamily;
  for (int i = 0; i < features.size(); ++i) {
    auto desc = Trim(features[i]);    // trim leading & trailing whitespaces.
    if (desc.empty()) {
      throw ParseException("Empty feature descriptor.");
    }
    auto found = desc.find(":");
    std::string feature_name;
    if (found != std::string::npos) {
      if (found == 0) {
        // :feat4 in the above example.
        curr_family = kDefaultFamily;
      } else {
        // fam1:feat3 in the above example.
        curr_family = desc.substr(0, found);
      }
      feature_name = desc.substr(found + 1, desc.size() - found - 1);
    } else {
      // e.g., feat1 in the above example.
      feature_name = desc;
    }
    pairs.push_back(std::make_pair(curr_family, feature_name));
  }
  return pairs;
}

}  // namespace schema_util
}  // namespace mldb
