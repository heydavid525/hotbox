#include "schema/feature_finder.hpp"

namespace hotbox {

TypedFeatureFinder::TypedFeatureFinder(const FeatureFinder& finder,
    const FeatureType& t) : FeatureFinder(finder), type(t) { }

}  // namespace hotbox
