#include "schema/feature_finder.hpp"

namespace mldb {

TypedFeatureFinder::TypedFeatureFinder(const FeatureFinder& finder,
    const FeatureType& t) : FeatureFinder(finder), type(t) { }

}  // namespace mldb
