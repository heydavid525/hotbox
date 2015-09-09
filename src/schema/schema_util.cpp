#include <glog/logging.h>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include "schema/schema_util.hpp"
#include "schema/constants.hpp"
#include "util/string_util.hpp"
#include "util/mldb_exceptions.hpp"

namespace mldb {

Feature CreateFeature(const FeatureType& type, const FeatureStoreType& store_type,
    const std::string& name) {
  Feature f;
  f.set_name(name);
  FeatureLocator* loc = f.mutable_loc();
  loc->set_type(type);
  loc->set_store_type(store_type);
  return f;
}

std::vector<FeatureFinder> ParseFeatureDesc(const std::string& feature_desc) {
  std::vector<FeatureFinder> finders;
  auto trimmed_desc = Trim(feature_desc, ",");  // remove trailing/leading commas
  std::vector<std::string> features = SplitString(trimmed_desc, ',');

  for (int i = 0; i < features.size(); ++i) {
    auto desc = Trim(features[i]);    // trim leading & trailing whitespaces.
    if (desc.empty()) {
      throw ParseException("Empty feature descriptor.");
    }
    auto found = desc.find(":");
    std::string family;
    std::string feature_name;
    if (found != std::string::npos) {
      if (found == 0) {
        // :feat4 in the above example.
        family = kDefaultFamily;
      } else {
        // fam1:feat3 in the above example.
        family = desc.substr(0, found);
      }
      feature_name = desc.substr(found + 1, desc.size() - found - 1);
    } else {
      family = kDefaultFamily;
      // e.g., feat1 in the above example.
      feature_name = desc;
    }
    if (Trim(feature_name).empty()) {
      FeatureFinder finder;
      finder.family_name = family;
      finder.all_family = true;
      finders.push_back(finder);
    } else {
      auto split_names = SplitString(feature_name, '+');
      for (const auto& name : split_names) {
        FeatureFinder finder;
        finder.family_name = family;
        auto trimmed = Trim(name);
        if (std::isdigit(trimmed[0])) {
          // interpret as family_idx
          finder.family_idx = std::atoi(trimmed.c_str());
        } else {
          finder.feature_name = trimmed;
        }
        finders.push_back(finder);
      }
    }
  }
  return finders;
}

bool IsNumeral(const Feature& f) {
  return (f.loc().type() == FeatureType::CATEGORICAL) ||
    (f.loc().type() == FeatureType::NUMERICAL);
}

bool IsNumeral(const FeatureLocator& loc) {
  return (loc.type() == FeatureType::CATEGORICAL) ||
    (loc.type() == FeatureType::NUMERICAL);
}

bool IsCategorical(const Feature& f) {
  return f.loc().type() == FeatureType::CATEGORICAL;
}

bool IsNumerical(const Feature& f) {
  return f.loc().type() == FeatureType::NUMERICAL;
}

bool IsDense(const Feature& f) {
  return f.loc().store_type() == FeatureStoreType::DENSE;
}

bool IsSparse(const Feature& f) {
  return f.loc().store_type() == FeatureStoreType::SPARSE;
}

/*
DatumProtoOffset operator+(const DatumProtoOffset& o1,
    const DatumProtoOffset& o2) {
  DatumProtoOffset offset;
  offset.set_dense_cat_store(o1.dense_cat_store() + o2.dense_cat_store());
  offset.set_dense_num_store(o1.dense_num_store() + o2.dense_num_store());
  offset.set_dense_bytes_store(o1.dense_bytes_store() + o2.dense_bytes_store());

  offset.set_sparse_cat_store(o1.sparse_cat_store() + o2.sparse_cat_store());
  offset.set_sparse_num_store(o1.sparse_num_store() + o2.sparse_num_store());
  offset.set_sparse_bytes_store(o1.sparse_bytes_store() + o2.sparse_bytes_store());

  return offset;
}
*/

}  // namespace mldb
