#pragma once

#include <string>
#include "schema/proto/schema.pb.h"
#include "schema/schema.hpp"

namespace hotbox {

// A zero-copy wrapper class around DatumProto. Not copyable.
//
// TODO(wdai): Use multiple dense_*_store() to store dimension larger
// than range of int32_t
class DatumBase {
public:
  // DatumBase takes the ownership of proto.
  DatumBase(DatumProto* proto);

  DatumBase(const DatumBase& other);

  DatumProto* Release();

  float GetLabel(const FeatureFamily& internal_family) const;

  // Weight cannot be 0 since it can be stored sparsely and 0 means default 1.
  float GetWeight(const FeatureFamily& internal_family) const;

  // feature_desc (feature descriptor) can only result in 1 feature.
  //float GetFeatureVal(const Schema& schema,
  //    const std::string& feature_desc) const;

  // Get number feature value (CATEGORICAL or NUMERIC). Error otherwise.
  // TODO(wdai): Return flexitype in the future.
  float GetFeatureVal(const Feature& feature) const;

  // Assumes the dense feature stores are resized already.
  void SetFeatureVal(const Feature& f, float val);

  // Directly set in dense_cat_store()
  void SetDenseCatFeatureVal(BigInt offset, int val);

  // Directly set in sparse_cat_store()
  void SetSparseCatFeatureVal(BigInt offset, int val);

  // Directly set in dense_num_store()
  void SetDenseNumFeatureVal(BigInt offset, float val);

  // Directly set in sparse_num_store()
  void SetSparseNumFeatureVal(BigInt offset, float val);

  std::string ToString() const;

  // Print with schema info. Only print numeral features (no bytes).
  std::string ToString(const Schema& schema) const;

  DatumProto* ReleaseProto();

  // Return the serialized bytes from proto_.
  std::string Serialize() const;

private:
  std::unique_ptr<DatumProto> proto_;
};
}  // namespace hotbox
