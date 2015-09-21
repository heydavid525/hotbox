#pragma once

#include <string>
#include "schema/proto/schema.pb.h"
#include "schema/schema.hpp"

namespace mldb {

// A zero-copy wrapper class around DatumProto. Not copyable.
class DatumBase {
public:
  // DatumBase takes the ownership of proto.
  DatumBase(DatumProto* proto);

  DatumBase(const DatumBase& other);

  DatumProto* Release();

  float GetLabel(const Schema& schema) const;

  // Weight cannot be 0 since it can be stored sparsely and 0 means default 1.
  float GetWeight(const Schema& schema) const;

  // feature_desc (feature descriptor) can only result in 1 feature.
  float GetFeatureVal(const Schema& schema,
      const std::string& feature_desc) const;

  // Get numeral feature value (CATEGORICAL or NUMERIC). Error otherwise.
  // TODO(wdai): Return flexitype in the future.
  float GetFeatureVal(const Feature& feature) const;
  float GetFeatureVal(const FeatureLocator& loc) const;

  // Assumes the dense feature stores are resized already.
  void SetFeatureVal(const FeatureLocator& loc, float val);

  // Directly set in dense_cat_store()
  void SetDenseCatFeatureVal(int offset, int val);

  // Directly set in sparse_cat_store()
  void SetSparseCatFeatureVal(int offset, int val);

  // Directly set in dense_num_store()
  void SetDenseNumFeatureVal(int offset, float val);

  // Directly set in sparse_num_store()
  void SetSparseNumFeatureVal(int offset, float val);

  std::string ToString() const;

  // Print with schema info. Only print numeral features (no bytes).
  std::string ToString(const Schema& schema) const;

  // Return the serialized bytes from proto_.
  std::string Serialize() const;

private:
  std::unique_ptr<DatumProto> proto_;
};
}  // namespace mldb
