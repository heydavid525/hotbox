#include "client/proxy_data_iterator.hpp"
#include "util/all.hpp"
#include <glog/logging.h>

namespace hotbox {

namespace {

// # of data to get in each call on the proxy server.
const int kBatchNumData = 1000;

}  // anonymous namespace

ProxyDataIterator::ProxyDataIterator(WarpClient& warp_client,
    const std::string& session_id, int iter_id,
    int64_t data_begin, int64_t data_end,
    int num_transform_threads, int num_io_threads,
    size_t buffer_limit, size_t batch_limit) :
  warp_client_(warp_client), session_id_(session_id),
  iter_id_(iter_id), data_(kBatchNumData) {
    ClientMsg client_msg;
    auto req = client_msg.mutable_proxy_create_iter_req();
    req->set_session_id(session_id_);
    req->set_iter_id(iter_id_);
    req->set_data_begin(data_begin);
    req->set_data_end(data_end);
    req->set_num_transform_threads(num_transform_threads);
    req->set_num_io_threads(num_io_threads);
    req->set_buffer_limit(buffer_limit);
    req->set_batch_limit(batch_limit);
    // Ignore the reply msg.
    warp_client_.SendRecv(client_msg);
    GetBatch();
  }

ProxyDataIterator::~ProxyDataIterator() {
  ClientMsg client_msg;
  auto req = client_msg.mutable_proxy_destroy_iter_req();
  req->set_session_id(session_id_);
  req->set_iter_id(iter_id_);
  // Ignore the reply msg.
  warp_client_.SendRecv(client_msg);
}

bool ProxyDataIterator::HasNext() const {
  return curr_ != data_.size();
}

void ProxyDataIterator::Restart() {
  ClientMsg client_msg;
  auto req = client_msg.mutable_proxy_restart_req();
  req->set_session_id(session_id_);
  req->set_iter_id(iter_id_);
  // Ignore the reply msg.
  warp_client_.SendRecv(client_msg);
  GetBatch();
}

void ProxyDataIterator::GetBatch() {
  ClientMsg client_msg;
  auto req = client_msg.mutable_proxy_get_batch_req();
  req->set_session_id(session_id_);
  req->set_iter_id(iter_id_);
  req->set_batch_size(kBatchNumData);
  warp_client_.Send(client_msg);
  bool compress = false;
  ServerMsg server_msg = warp_client_.Recv(compress);
  CHECK(server_msg.has_proxy_get_batch_reply());
  const auto& rep = server_msg.proxy_get_batch_reply();
  data_.resize(rep.data_size());
  for (int i = 0; i < rep.data_size(); ++i) {
    data_[i] = rep.data(i);
  }
  curr_ = 0;
}

FlexiDatum ProxyDataIterator::GetDatum() {
  CHECK(HasNext());
  FlexiDatum datum = std::move(data_[curr_++]);
  if (curr_ == data_.size() && data_.size() == kBatchNumData) {
    // load the next batch
    GetBatch();
  }
  return datum;
}

}  // namespace hotbox
