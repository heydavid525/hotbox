#pragma once

#include "schema/proto/schema.pb.h"
#include "glog/logging.h"
#include "db/proto/db.pb.h"

namespace hotbox {

class TransformWriter {
public:
  // TransformWriter does not take ownership of schema. 'schema' must outlive
  // TransformWriter.
  TransformWriter(Schema* schema, const TransformWriterConfig& config) :
    schema_(schema), store_type_(config.store_type()) {
    InitializeOffset(&output_store_offset_begin_);
    InitializeOffset(&output_store_offset_end_);

    output_family_ = &(schema_->GetOrCreateFamily(
        config.output_family_name(), config.output_simple_family(),
        store_type_));
  }

  // Add a feature to the storage type (default is OUTPUT, but could be
  // changed by SetOutputType.
  void AddFeature(const std::string& feature_name) {
    Feature new_feature;
    new_feature.set_store_type(store_type_);
    new_feature.set_name(feature_name);
    schema_->AddFeature(output_family_, &new_feature);
    auto curr_end = output_store_offset_end_.offsets(store_type_);
    output_store_offset_end_.set_offsets(store_type_,
        std::max(curr_end, new_feature.store_offset() + 1));
  }

  // Add num_features anonymous features to output_family_. We assume
  // output_family_ is SimpleFeatureFamily.
  void AddFeatures(BigInt num_features) {
    schema_->AddFeatures(output_family_, num_features);
    auto curr_end = output_store_offset_end_.offsets(store_type_);
    output_store_offset_end_.set_offsets(store_type_, curr_end + num_features);
  }

  // Get the output range for each transform to be sent to client.
  TransformOutputRange GetTransformOutputRange() const {
    TransformOutputRange range;
    range.set_store_offset_begin(
        output_store_offset_begin_.offsets(store_type_));
    range.set_store_offset_end(output_store_offset_end_.offsets(store_type_));
    range.set_store_type(store_type_);
    return range;
  }

private:
  // Replace -1 in output_offset_begin_ to current schema's offset value.
  void InitializeOffset(DatumProtoStoreOffset* offset) {
    const auto& append_offset = schema_->GetAppendOffset();
    offset->mutable_offsets()->Resize(FeatureStoreType::NUM_STORE_TYPES, -1);
    for (int i = 0; i < FeatureStoreType::NUM_STORE_TYPES; ++i) {
      offset->set_offsets(i, append_offset.offsets(i));
    }
  }

private:
  // Does not take ownership of schema_ nor output_family_.
  Schema* schema_;
  FeatureFamilyIf* output_family_;

  // Transform output is only allowed between [output_store_offset_begin_,
  // output_store_offset_end_).
  DatumProtoStoreOffset output_store_offset_begin_;
  DatumProtoStoreOffset output_store_offset_end_;

  const FeatureStoreType store_type_;
};

}  // namespace hotbox
