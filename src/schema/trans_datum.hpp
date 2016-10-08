#pragma once

#include <string>
#include "schema/proto/schema.pb.h"
#include "schema/schema.hpp"
#include "schema/datum_base.hpp"
#include "db/proto/db.pb.h"
#include "schema/flexi_datum.hpp"

namespace hotbox {

// TransDatum is a wrapper class around DatumBase to provide simple interface
// for transforms to write to DatumBase.
class TransDatum {
public:
  // This takes the ownership of base.
  // ranges is the output range of the last transform. ranges is used
  // to allocate base->dense_*_store() for all intermediate transforms.
  TransDatum(DatumBase* base, const Feature& label, const Feature& weight,
      OutputStoreType output_store_type, BigInt output_dim,
      const std::vector<TransformOutputRange>& ranges);

  // Get number feature value (CATEGORICAL or NUMERIC). Error otherwise.
  // Equivalent to DatumBase::GetFeatureVal().
  float GetFeatureVal(const Feature& feature) const;

  // Set the store_type and offset for relative set.
  void ReadyTransform(const TransformOutputRange& output_range);

  // Set value in a specified storage and use offset relative to offset_begin_.
  // Note that for OutputStoreType::SPARSE the order of relative_offset needs
  // to be added in ascending order.
  void SetFeatureValRelativeOffset(BigInt relative_offset, float val);

  // Get the output. Can only be called once.
  FlexiDatum GetFlexiDatum();

  inline const DatumBase& GetDatumBase() const {
    return *base_;
  }

  inline const BigInt GetOutputCounter() const {
    return output_counter_;
  };

private:
  std::unique_ptr<DatumBase> base_;
  const Feature& label_;
  const Feature& weight_;

  // Output store format.
  const OutputStoreType output_store_type_;

  // All writes to value will be on store_type_ storage in base_ and relative
  // to offset_begin_. Thus all modification is in [offset_begin_, offset_end_)
  // range.
  FeatureStoreType store_type_;
  BigInt offset_begin_;
  BigInt offset_end_;
  BigInt range_;
  
  // Feature dimension of the output vector.
  BigInt output_feature_dim_;

  // Only one of dense/sparse will be in use based on output_store_type_.
  std::vector<float> dense_vals_;

  std::vector<BigInt> sparse_idx_;
  std::vector<float> sparse_vals_;

  // For metrics on space usage
  BigInt output_counter_ = 0;

};

}  // namespace hotbox
