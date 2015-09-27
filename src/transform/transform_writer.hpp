#pragma once

#include "schema/proto/schema.pb.h"

namespace mldb {

class TransformWriter {
public:
  TransformWriter(std::shared_ptr<Schema> schema) : schema_(schema) {
    output_offset_start_.set_dense_cat_store(-1);
    output_offset_start_.set_dense_num_store(-1);
    output_offset_start_.set_dense_bytes_store(-1);
    output_offset_start_.set_sparse_cat_store(-1);
    output_offset_start_.set_sparse_num_store(-1);
    output_offset_start_.set_sparse_bytes_store(-1);
  }

  void AddDenseCatFeature(const std::string& family_name,
      const std::string& feature_name, bool not_in_final = false) {
    // offset could be 0 for, say, byte features, so we use -1 to indiate
    // uninitialized start_offset.
    int offset = AddFeature(family_name, feature_name,
        FeatureType::CATEGORICAL, FeatureStoreType::DENSE, not_in_final);
    if (output_offset_start_.dense_cat_store() == -1) {
      output_offset_start_.set_dense_cat_store(offset);
    }
    output_offset_end_.set_dense_cat_store(
        output_offset_end_.dense_cat_store() + 1);
  }

  void AddDenseNumFeature(const std::string& family_name,
      const std::string& feature_name, bool not_in_final = false) {
    // offset could be 0 for, say, byte features, so we use -1 to indiate
    // uninitialized start_offset.
    int offset = AddFeature(family_name, feature_name,
        FeatureType::NUMERICAL, FeatureStoreType::DENSE, not_in_final);
    if (output_offset_start_.dense_num_store() == -1) {
      output_offset_start_.set_dense_num_store(offset);
    }
    output_offset_end_.set_dense_num_store(
        output_offset_end_.dense_num_store() + 1);
  }

  void AddSparseCatFeature(const std::string& family_name,
      const std::string& feature_name, bool not_in_final = false) {
    // offset could be 0 for, say, byte features, so we use -1 to indiate
    // uninitialized start_offset.
    int offset = AddFeature(family_name, feature_name,
        FeatureType::CATEGORICAL, FeatureStoreType::SPARSE, not_in_final);
    if (output_offset_start_.sparse_cat_store() == -1) {
      output_offset_start_.set_sparse_cat_store(offset);
    }
    output_offset_end_.set_sparse_cat_store(
        output_offset_end_.sparse_cat_store() + 1);
  }

  void AddSparseNumFeature(const std::string& family_name,
      const std::string& feature_name, bool not_in_final = false) {
    // offset could be 0 for, say, byte features, so we use -1 to indiate
    // uninitialized start_offset.
    int offset = AddFeature(family_name, feature_name,
        FeatureType::NUMERICAL, FeatureStoreType::SPARSE, not_in_final);
    if (output_offset_start_.sparse_num_store() == -1) {
      output_offset_start_.set_sparse_num_store(offset);
    }
    output_offset_end_.set_sparse_num_store(
        output_offset_end_.sparse_num_store() + 1);
  }

  // Get the DatumProtoOffset range for this transform's output.
  const DatumProtoOffset& GetOutputOffsetStart() {
    FillInOffset(&output_offset_start_);
    return output_offset_start_;
  }

  const DatumProtoOffset& GetOutputOffsetEnd() {
    FillInOffset(&output_offset_end_);
    return output_offset_end_;
  }

private:
  // Return the offset for the added feature.
  int AddFeature(const std::string& family_name,
    const std::string& feature_name, FeatureType type,
    FeatureStoreType store_type, bool not_in_final) {
    Feature new_feature;
    FeatureLocator* loc = new_feature.mutable_loc();
    loc->set_type(type);
    loc->set_store_type(store_type);
    new_feature.set_name(feature_name);
    new_feature.set_not_in_final(not_in_final);
    schema_->AddFeature(family_name, output_family_idx_[family_name]++,
        &new_feature);
    return new_feature.loc().offset();
  }

  // Replace -1 in output_offset_start_ to current schema's offset value.
  void FillInOffset(DatumProtoOffset* offset) {
    const auto& schema_offset = schema_->GetDatumProtoOffset();
    if (offset->dense_cat_store() == -1) {
      offset->set_dense_cat_store(schema_offset.dense_cat_store());
    }
    if (offset->sparse_cat_store() == -1) {
      offset->set_sparse_cat_store(schema_offset.sparse_cat_store());
    }
    if (offset->dense_num_store() == -1) {
      offset->set_dense_num_store(schema_offset.dense_num_store());
    }
    if (offset->sparse_num_store() == -1) {
      offset->set_sparse_num_store(schema_offset.sparse_num_store());
    }
    if (offset->dense_bytes_store() == -1) {
      offset->set_dense_bytes_store(schema_offset.dense_bytes_store());
    }
    if (offset->sparse_bytes_store() == -1) {
      offset->set_sparse_bytes_store(schema_offset.sparse_bytes_store());
    }
  }

private:
  std::shared_ptr<Schema> schema_;

  // Track the output family indices.
  std::map<std::string, int> output_family_idx_;

  // Transform output is only allowed between [output_offset_start_,
  // output_offset_end_).
  DatumProtoOffset output_offset_start_;
  DatumProtoOffset output_offset_end_;
};

}  // namespace mldb
