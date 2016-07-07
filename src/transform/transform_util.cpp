#include "transform/transform_util.hpp"

namespace hotbox {

namespace {

void GetVals(const TransDatum& datum, const WideFamilySelector& selector,
    std::vector<std::pair<BigInt, float>>* sparse_vec,
    std::vector<float>* dense_vec) {
  // Exactly one has to be nullptr
  CHECK(sparse_vec != nullptr || dense_vec != nullptr);
  CHECK(sparse_vec == nullptr || dense_vec == nullptr);
  bool use_sparse = sparse_vec != nullptr;

  // Find the range on family.
  StoreTypeAndOffset type_and_offset = selector.offset;
  auto offset_begin = selector.offset.offset_begin();
  auto offset_end = selector.offset.offset_end();
  auto range = selector.range_selector;
  BigInt family_idx_begin = range.family_idx_begin;
  BigInt family_idx_end = range.family_idx_end;
  if (family_idx_begin != family_idx_end) {
    // Further limit the offset using range, if applicable.
    offset_end = family_idx_end + offset_begin;
    offset_begin = family_idx_begin + offset_begin;
  }
  if (!use_sparse) {
    dense_vec->resize(offset_end - offset_begin);
  }
  const DatumProto& proto = datum.GetDatumBase().GetDatumProto();
  switch (type_and_offset.store_type()) {
    case FeatureStoreType::SPARSE_NUM:
      {
        auto low = std::lower_bound(
            proto.sparse_num_store_idxs().cbegin(),
            proto.sparse_num_store_idxs().cend(), offset_begin);

        // 'start' indexes the first non-zero element for family p.first,
        // if the family isn't all zero (if so, 'start' indexes the
        // beginning of next family.)
        auto start = low - proto.sparse_num_store_idxs().cbegin();
        for (int i = start; i < proto.sparse_num_store_idxs_size(); ++i) {
          if (proto.sparse_num_store_idxs(i) < offset_begin) {
            continue;
          }
          if (proto.sparse_num_store_idxs(i) >= offset_end) {
            break;
          }
          BigInt family_idx = proto.sparse_num_store_idxs(i)
            - offset_begin;
          if (use_sparse) {
            sparse_vec->push_back(std::make_pair(family_idx,
                  proto.sparse_num_store_vals(i)));
          } else {
            (*dense_vec)[family_idx] = proto.sparse_num_store_vals(i);
          }
        }
        break;
      }
    case FeatureStoreType::DENSE_NUM:
      {
        for (BigInt i = offset_begin; i < offset_end; i++) {
          BigInt family_idx = i - offset_begin;
          if (use_sparse) {
            sparse_vec->push_back(std::make_pair(family_idx,
                  proto.dense_num_store(i)));
          } else {
            (*dense_vec)[family_idx] = proto.dense_num_store(i);
          }
        }
        break;
      }
    default:
      LOG(FATAL) << "store type " << type_and_offset.store_type()
        << " is not supported yet.";
  }
}

}  // anonymous namespace

std::vector<std::pair<BigInt, float>> GetSparseVals(const TransDatum& datum,
    const WideFamilySelector& selector) {
  std::vector<std::pair<BigInt, float>> sparse_vec;
  GetVals(datum, selector, &sparse_vec, nullptr);
  return sparse_vec;
}

std::vector<float> GetDenseVals(const TransDatum& datum,
    const WideFamilySelector& selector) {
  std::vector<float> dense_vec;
  GetVals(datum, selector, nullptr, &dense_vec);
  return dense_vec;
}
}  // namespace hotbox
