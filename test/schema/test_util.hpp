#pragma once

#include "schema/schema.hpp"
#include "schema/proto/schema.pb.h"

namespace hotbox {

// Add a family with 'family_name' of 'num_cat_features' categorical features
// (named 'cati', i = [0, num_cat_features)) and 'num_num_features' numerical
// features (named 'numi'),
void AddCatNumFamily(const std::string& family_name,
    int num_cat_features, int num_num_features, Schema* schema) {
  const auto& offset = schema->GetDatumProtoOffset();
  std::vector<Feature> cat_features;
  for (int i = 0; i < num_cat_features; ++i) {
    Feature feature;
    FeatureLocator* loc = feature.mutable_loc();
    loc->set_type(FeatureType::CATEGORICAL);
    loc->set_store_type(FeatureStoreType::DENSE);
    loc->set_offset(offset.dense_cat_store() + i);
    feature.set_name("cat" + std::to_string(i));
    cat_features.push_back(feature);
  }
  FeatureFamily family;
  family.AddFeatures(cat_features);

  std::vector<Feature> numerical_features;
  for (int i = 0; i < num_num_features; ++i) {
    Feature feature;
    FeatureLocator* loc = feature.mutable_loc();
    loc->set_type(FeatureType::NUMERICAL);
    loc->set_store_type(FeatureStoreType::SPARSE);
    // offset for sparse_num_store is 0.
    loc->set_offset(offset.sparse_num_store() + i);
    feature.set_name("num" + std::to_string(i));
    numerical_features.push_back(feature);
  }
  std::vector<int> family_idxes(num_num_features);
  std::iota(family_idxes.begin(), family_idxes.end(), num_cat_features);
  family.AddFeatures(numerical_features, family_idxes);

  DatumProtoOffset offset_inc;
  offset_inc.set_dense_cat_store(num_cat_features);
  offset_inc.set_sparse_num_store(num_num_features);

  schema->AddFamily(family_name, family, offset_inc);
}

}  // namespace hotbox
