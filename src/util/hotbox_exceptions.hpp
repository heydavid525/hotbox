#pragma once
#include <exception>
#include <string>
#include "schema/feature_finder.hpp"

namespace hotbox {

// A custom exception with error message.
class HotboxException: public std::exception {
public:
  HotboxException();
  HotboxException(const std::string& msg);
  virtual const char* what() const throw();

private:
  std::string msg_;
};


class ParseException: public HotboxException {
public:
  ParseException(const std::string& msg);
};


class FamilyNotFoundException : public HotboxException {
public:
  FamilyNotFoundException(const std::string& family_name);

  std::string GetFamilyName() const;

private:
  const std::string family_name_;
};


class FeatureNotFoundException: public HotboxException {
public:
  FeatureNotFoundException(const FeatureFinder& not_found_feature);

  const FeatureFinder& GetNotFoundFeature() const;

private:
  FeatureFinder not_found_feature_;
};


class TypedFeaturesNotFoundException: public HotboxException {
public:
  void SetNotFoundTypedFeatures(
      std::vector<TypedFeatureFinder>&& not_found_features);

  const std::vector<TypedFeatureFinder>& GetNotFoundTypedFeatures() const;

private:
  std::vector<TypedFeatureFinder> not_found_features_;
};

class FailedToUncompressException : public HotboxException {
public:
  FailedToUncompressException(const std::string& msg);
};

class FailedFileOperationException : public HotboxException {
public:
  FailedFileOperationException(const std::string& msg);
};

}  // namespace hotbox
