#include "data_util.hpp"
#include "timer.hpp"
#include <sstream>
#include <fstream>

namespace petuum {
namespace fm {

// Return the label. feature_one_based = true assumes
// feature index starts from 1.
int ParseLibSVMLine(const std::string& line,
    std::vector<int64_t>* feature_ids,
    std::vector<float>* feature_vals, bool feature_one_based) {
  feature_ids->clear();
  feature_vals->clear();
  char *ptr = 0, *endptr = 0;
  // Read label.
  int label = strtol(line.data(), &endptr, 10);
  ptr = endptr;

  // first column is the constant offset.
  feature_ids->push_back(0);
  feature_vals->push_back(1);
  while (isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  while (ptr - line.data() < line.size()) {
    // read a feature_id:feature_val pair
    int32_t feature_id = strtol(ptr, &endptr, 10);
    if (!feature_one_based) {
      // since dim 0 is offset.
      ++feature_id;
    }
    feature_ids->push_back(feature_id);
    ptr = endptr;
    CHECK_EQ(':', *ptr);
    ++ptr;

    feature_vals->push_back(strtod(ptr, &endptr));
    ptr = endptr;
    while (isspace(*ptr) && ptr - line.data() < line.size()) ++ptr;
  }
  return label;
}

// Read libsvm format data. labels \in {0, 1}.
void ReadLibSVMPartition(const std::string& filename,
    int partition_id, int num_partitions, std::vector<SparseVec>* features,
    std::vector<int>* labels, bool feature_one_based) {
  Timer read_timer;
  std::ifstream in(filename, std::ios::binary | std::ios::ate);
  int64_t size = in.tellg();
  int64_t begin = size * partition_id / num_partitions;
  int64_t end = size * (partition_id + 1) / num_partitions;
  if (begin > 0) {
    in.seekg(begin - 1);
    char c;
    do {
      in.get(c);
    } while (c != '\n');
  } else {
    in.seekg(0);
  }

  int32_t i = 0;
  std::vector<int64_t> feature_ids;
  std::vector<float> feature_vals;
  int64_t nnz = 0;

  while (in.tellg() < end) {
    std::string line;
    std::getline(in, line);
    int label = ParseLibSVMLine(line, &feature_ids,
        &feature_vals, feature_one_based);
    CHECK(label == 1 || label == 0) << "label cannot be -1";
    labels->emplace_back(label);
    features->emplace_back(SparseVec(feature_ids, feature_vals));
    nnz += feature_ids.size();
    ++i;
  }
  LOG_IF(INFO, partition_id == 0 || partition_id == num_partitions - 1)
    << "Partition " << partition_id << " read " << i
    << " instances from " << filename << " in "
    << read_timer.elapsed()
    << " seconds. Avg nnz: " << static_cast<float>(nnz) / i;
}

}   // namespace fm
}   // namespace petuum
