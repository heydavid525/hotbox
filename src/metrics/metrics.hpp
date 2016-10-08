#pragma once
#include "metrics/proto/metrics.pb.h"

namespace hotbox {
  // transformation stats per thread per transformation, used in mt_transformer
  typedef std::vector<std::vector<TransformMetricsProto>> ThreadTransStats;
    // thread->transformation->stats
  // transformation stats per transformation
  typedef std::vector<TransformMetricsProto> TransStats;
}

