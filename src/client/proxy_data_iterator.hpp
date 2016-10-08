#pragma once
#include "client/data_iterator_if.hpp"
#include "db/proto/db.pb.h"
#include <string>
#include <glog/logging.h>

namespace hotbox {

// Iterate over the data returned by a range query on Session.
// DataIterator can only be created by Session.
class ProxyDataIterator : public DataIteratorIf {
public:
  bool HasNext() const override;

  void Restart() override;

  FlexiDatum GetDatum() override;

  std::unique_ptr<TransStats> GetMetrics() override {
    LOG(ERROR) << "GetMetrics is not implemented in proxy iterator.";
    return NULL;
  };

  ~ProxyDataIterator();

protected:
  // Create a corresponding DataIterator on the proxy.
  ProxyDataIterator(WarpClient& warp_client,
      const std::string& session_id, int iter_id,
      int64_t data_begin, int64_t data_end, int num_transform_threads,
      int num_io_threads, size_t buffer_limit, size_t batch_limit);

private:
  // Request a batch from a proxy server.
  void RequestOne(int server_id);
  // Get a batch of data_
  void GetBatch();
  // Return true if data on all servers are exhausted.
  bool AllDone() const;

private:
  // Can only be created by Session, and the parent Session needs to outlive
  // DataIterator.
  friend class Session;

  WarpClient& warp_client_;
  int num_servers_;
  std::string session_id_;
  int iter_id_{-1};

  // Data cache.
  int curr_{0};
  std::vector<FlexiDatum> data_;
  // done_[i] == true means no more data on server i.
  std::vector<bool> done_;
};

} // namespace hotbox

