#include "util/mldb_exceptions.hpp"

namespace mldb {

// MLDBException

MLDBException::MLDBException() { }

MLDBException::MLDBException(const std::string& msg) : msg_(msg) { }

const char* MLDBException::what() const throw() {
  return msg_.empty() ? "Unspecified MLDB Exception." : msg_.c_str();
}

// ParseException

ParseException::ParseException(const std::string msg) : MLDBException(msg) { }

// FamilyNotFoundException

FamilyNotFoundException::FamilyNotFoundException(const std::string& family_name)
  : family_name_(family_name)
{ }

std::string FamilyNotFoundException::GetFamilyName() const {
  return family_name_;
}

// FeatureNotFoundException

FeatureNotFoundException::FeatureNotFoundException(
    const FeatureFinder& not_found_feature) :
  not_found_feature_(not_found_feature)
{ }

const FeatureFinder& FeatureNotFoundException::GetNotFoundFeature() const {
  return not_found_feature_;
}

// TypedFeaturesNotFoundException

void TypedFeaturesNotFoundException::SetNotFoundTypedFeatures(
    std::vector<TypedFeatureFinder>&& finder) {
  not_found_features_ = finder;
}

const std::vector<TypedFeatureFinder>&
TypedFeaturesNotFoundException::GetNotFoundTypedFeatures() const {
  return not_found_features_;
}

}  // namespace mldb
