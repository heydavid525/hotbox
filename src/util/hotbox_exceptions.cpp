#include "util/hotbox_exceptions.hpp"
#include <string>

namespace hotbox {

// HotboxException

HotboxException::HotboxException() { }

HotboxException::HotboxException(const std::string& msg) : msg_(msg) { }

const char* HotboxException::what() const throw() {
  return msg_.empty() ? "Unspecified Hotbox Exception." : msg_.c_str();
}

// ParseException

ParseException::ParseException(const std::string& msg) : HotboxException(msg) { }

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
  not_found_feature_(not_found_feature) {
    this->msg_ = "Requested feature: " + not_found_feature_.family_name + ":" +
      std::to_string(not_found_feature_.family_idx);
  }

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

// FailedToUncompressException

FailedToUncompressException::FailedToUncompressException(
    const std::string& msg) : HotboxException(msg) { }

// FailedFileOperationException

FailedFileOperationException::FailedFileOperationException(
    const std::string& msg) : HotboxException(msg) { }

}  // namespace hotbox
