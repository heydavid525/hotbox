#include <glog/logging.h>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include "schema/schema_util.hpp"
#include "schema/constants.hpp"
#include "util/string_util.hpp"
#include "util/hotbox_exceptions.hpp"

namespace hotbox {

Feature CreateFeature(FeatureStoreType store_type, const std::string& name) {
  Feature f;
  f.set_name(name);
  f.set_store_type(store_type);
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_NUM:
    case FeatureStoreType::SPARSE_NUM:
      // Use default false value for is_factor().
      return f;
    default:
      f.set_is_factor(true);
  }
  return f;
}

namespace {

FeatureFinder CreateFeatureFinder(const std::string& family,
    const std::string& feature_name) {
  FeatureFinder finder;
  finder.family_name = family;
  auto trimmed = Trim(feature_name);
  if (std::isdigit(trimmed[0])) {
    // interpret as family_idx
    finder.family_idx = std::stoi(trimmed);
  } else {
    finder.feature_name = trimmed;
  }
  return finder;
}

}  // anonymous namespace

std::vector<FeatureFinder> ParseFeatureDesc(const std::string& feature_desc) {
  std::vector<FeatureFinder> finders;
  // remove trailing/leading commas
  auto trimmed_desc = Trim(feature_desc, ",");
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
    auto trimmed_feature_name = Trim(feature_name);
    if (trimmed_feature_name.empty() || trimmed_feature_name == "*") {
      FeatureFinder finder;
      finder.family_name = family;
      finder.mode = kRangeSelect;
      finders.push_back(finder);
    } else {
      found = feature_name.find("+");
      bool has_plus = found != std::string::npos;
      found = feature_name.find("-");
      bool has_minus = found != std::string::npos;
      CHECK(!has_plus || !has_minus)
        << "Can't have both + and - in feature name.";
      if (has_plus) {
        auto split_names = SplitString(feature_name, '+');
        for (const auto& name : split_names) {
          finders.push_back(CreateFeatureFinder(family, name));
        }
      } else if (has_minus) {
        auto split_names = SplitString(feature_name, '-');
        // example: "fam:1-10". Expect exactly 2 numeric features around '-'
        CHECK_EQ(2, split_names.size());
        BigInt start_idx = std::stoi(split_names[0]);
        BigInt end_idx = std::stoi(split_names[1]);
        FeatureFinder finder;
        finder.family_name = family;
        finder.mode = kRangeSelect;
        finder.range_selector.family_idx_begin = start_idx;
        finder.range_selector.family_idx_end = end_idx + 1;
        finders.push_back(finder);
      }
    }
  }
  return finders;
}

bool IsNumber(const Feature& f) {
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_CAT:
    case FeatureStoreType::DENSE_NUM:
    case FeatureStoreType::SPARSE_CAT:
    case FeatureStoreType::SPARSE_NUM:
    case FeatureStoreType::OUTPUT:
      return true;
    default:
      return false;
  }
  return false;
}

bool IsCategorical(const Feature& f) {
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_CAT:
    case FeatureStoreType::SPARSE_CAT:
      return true;
    default:
      return false;
  }
  return false;
}

bool IsNumerical(const Feature& f) {
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_NUM:
    case FeatureStoreType::SPARSE_NUM:
      return true;
    default:
      return false;
  }
  return false;
}

bool IsDense(const Feature& f) {
  switch (f.store_type()) {
    case FeatureStoreType::DENSE_CAT:
    case FeatureStoreType::DENSE_NUM:
    case FeatureStoreType::DENSE_BYTES:
      return true;
    default:
      return false;
  }
  return false;
}

bool IsSparse(const Feature& f) {
  switch (f.store_type()) {
    case FeatureStoreType::SPARSE_CAT:
    case FeatureStoreType::SPARSE_NUM:
    case FeatureStoreType::SPARSE_BYTES:
      return true;
    default:
      return false;
  }
  return false;
}

}  // namespace hotbox
