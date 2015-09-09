#pragma once
#include <exception>
#include <string>
#include "schema/feature_finder.hpp"

namespace mldb {

// A custom exception with error message.
class MLDBException: public std::exception {
public:
  MLDBException();
  MLDBException(const std::string& msg);
  virtual const char* what() const throw();

private:
  std::string msg_;
};


class ParseException: public MLDBException {
public:
  ParseException(const std::string msg);
};


class FamilyNotFoundException : public MLDBException {
public:
  FamilyNotFoundException(const std::string& family_name);

  std::string GetFamilyName() const;

private:
  const std::string family_name_;
};


class FeatureNotFoundException: public MLDBException {
public:
  FeatureNotFoundException(const FeatureFinder& not_found_feature);

  const FeatureFinder& GetNotFoundFeature() const;

private:
  FeatureFinder not_found_feature_;
};


class TypedFeaturesNotFoundException: public MLDBException {
public:
  void SetNotFoundTypedFeatures(
      std::vector<TypedFeatureFinder>&& not_found_features);

  const std::vector<TypedFeatureFinder>& GetNotFoundTypedFeatures() const;

private:
  std::vector<TypedFeatureFinder> not_found_features_;
};

}  // namespace mldb
