#include <string>
#include <vector>
#include "sparse_vec.hpp"

namespace petuum {
namespace fm {

void ReadLibSVMPartition(const std::string& filename,
  int partition_id, int num_partitions, std::vector<SparseVec>* features,
  std::vector<int>* labels, bool feature_one_based);

}   // namespace fm
}   // namespace petuum
