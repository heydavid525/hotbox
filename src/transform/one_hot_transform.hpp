#pragma once

#include "transform_if.hpp"
#include "transform/proto/schema.pb.h"
#include "transform/proto/transforms.pb.h"
#include "transform/schema.hpp"
#include "transform/datum_base.hpp"
#include "transform/schema_util.hpp"

namespace mldb {

class OneHotTransform : public TransformIf {
public:
  OneHotTransform(const OneHotTransformConfig& config) : config_(config) { }

  void TransformSchema(Schema* schema) override {
    auto finders = ParseFeatureDesc(config_.input_features());
    FeatureFamily output_family;
    // When this family is added to schema, it will increment schema's offset
    // by offset_inc_.
    DatumProtoOffset offset_inc;
    const auto& schema_offset = schema->GetDatumProtoOffset();
    std::vector<Feature> new_features;
    for (const auto& finder : finders) {
      if (finder.all_family) {
        // Family-wide selection.
        const auto& input_family = schema->GetFamily(finder.family_name);
        const auto& input_features = input_family.GetFeatures();
        for (int i = 0; i < input_features.size(); ++i) {
          auto onehot_features = TransformOneFeature(input_features[i],
              schema_offset, &offset_inc);
          new_features.insert(new_features.end(),
              onehot_features.begin(), onehot_features.end());
        }
      } else {
        const Feature& input_feature = schema->GetFeature(finder);
        auto onehot_features = TransformOneFeature(input_feature,
              schema_offset, &offset_inc);
        new_features.insert(new_features.end(),
            onehot_features.begin(), onehot_features.end());
      }
    }
    output_family.AddFeatures(new_features);
    schema->AddFamily(config_.output_feature_family(), output_family,
        offset_inc);
    this->recv_schema_ = true;
  }

  void Transform(DatumBase* datum) const override {
    CHECK(this->recv_schema_);
    for (int i = 0; i < transforms_.size(); ++i) {
      transforms_[i](datum);
    }
  }

private:
  std::vector<Feature> TransformOneFeature(
      const Feature& input_feature, const DatumProtoOffset& schema_offset,
      DatumProtoOffset* offset_inc) {
    CHECK(IsNumeral(input_feature));
    std::vector<Feature> new_features;
    // Compute # of buckets
    int num_buckets = 0;
    std::function<void(DatumBase*)> binning_func;
    const auto& input_feature_loc = input_feature.loc();
    // 'offset_inc' could carry offsets from other features added to
    // output family.
    auto offset_start = schema_offset + *offset_inc;
    int offset_start_sparse_cat = offset_start.sparse_cat_store();
    if (config_.buckets_size() == 0) {
      // Use input_feature (need to be categorical) to decide buckets.
      CHECK(IsCategorical(input_feature))
        << "Must specify bucket for NUMERIC feature type.";
      // min~max.
      int max = input_feature.stats().max();
      int min = input_feature.stats().min();
      num_buckets = max - min + 1;

      // bin into num_buckets in sparse_cat_store starting with
      // 'offset_start_sparse_cat'.
      binning_func =
        [input_feature_loc, offset_start_sparse_cat, max, min] (DatumBase* datum) {
          int val = static_cast<int>(datum->GetFeatureVal(input_feature_loc));
          CHECK_LE(val, max);
          CHECK_LE(min, val);
          int bin_id = val - min;
          FeatureLocator output_loc;
          output_loc.set_type(FeatureType::CATEGORICAL);
          output_loc.set_store_type(FeatureStoreType::SPARSE);
          output_loc.set_offset(offset_start_sparse_cat + bin_id);
          datum->SetFeatureVal(output_loc, 1);
        };
    } else {
      num_buckets = config_.buckets_size() + 1;

      const auto& buckets = config_.buckets();
      binning_func =
        [input_feature_loc, offset_start_sparse_cat, buckets]
        (DatumBase* datum) {
          float val = datum->GetFeatureVal(input_feature_loc);
          FeatureLocator output_loc;
          output_loc.set_type(FeatureType::CATEGORICAL);
          output_loc.set_store_type(FeatureStoreType::SPARSE);
          CHECK_GT(buckets.size(), 0);
          if (val < buckets.Get(0)) {
            output_loc.set_offset(offset_start_sparse_cat);  // first bucket.
          } else {
            for (int i = 0; i < buckets.size(); ++i) {
              if (val > buckets.Get(i)) {
                output_loc.set_offset(offset_start_sparse_cat + i + 1);
                break;
              }
            }
            datum->SetFeatureVal(output_loc, 1);
          }
        };
    }
    for (int i = 0; i < num_buckets; ++i) {
      Feature new_feature;
      FeatureLocator* loc = new_feature.mutable_loc();
      loc->set_type(FeatureType::CATEGORICAL);
      loc->set_store_type(FeatureStoreType::SPARSE);
      loc->set_offset(schema_offset.sparse_cat_store() +
          offset_inc->sparse_cat_store());
      offset_inc->set_sparse_cat_store(offset_inc->sparse_cat_store() + 1);

      new_feature.set_name(input_feature.name() + " one-hot "
          "bucket " + std::to_string(i));
      new_feature.set_in_final(config_.in_final());

      new_features.push_back(new_feature);
    }
    transforms_.push_back(binning_func);

    return new_features;
  }

private:
  OneHotTransformConfig config_;

  std::vector<std::function<void(DatumBase*)>> transforms_;
};

}  // namespace mldb
