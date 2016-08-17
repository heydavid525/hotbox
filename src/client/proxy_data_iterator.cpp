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
  warp_client_(warp_client),
  num_servers_(warp_client_.GetNumServers()),
  session_id_(session_id),
  iter_id_(iter_id), data_(kBatchNumData), done_(num_servers_) {
    ClientMsg client_msg;
    auto req = client_msg.mutable_proxy_create_iter_req();
    req->set_session_id(session_id_);
    req->set_iter_id(iter_id_);
    req->set_num_transform_threads(num_transform_threads);
    req->set_num_io_threads(num_io_threads);
    req->set_buffer_limit(buffer_limit);
    req->set_batch_limit(batch_limit);
    int64_t num_data_per_server = static_cast<float>(
        data_end - data_begin) / num_servers_;
    CHECK_GT(num_data_per_server, 0);
    for (int s = 0; s < num_servers_; ++s) {
      int64_t begin = data_begin + num_data_per_server * s;
      req->set_data_begin(begin);
      req->set_data_end(begin + num_data_per_server);
      // Ignore the reply msg.
      warp_client_.SendRecv(client_msg, true, s);
    }
    // Request for transformed data must come after create_iter_req to avoid
    // receiving GenericReply mixed with transformed data.
    for (int s = 0; s < num_servers_; ++s) {
      RequestOne(s);
    }
    GetBatch();
  }

void ProxyDataIterator::RequestOne(int server_id) {
  ClientMsg client_msg;
  auto req = client_msg.mutable_proxy_get_batch_req();
  req->set_session_id(session_id_);
  req->set_iter_id(iter_id_);
  req->set_batch_size(kBatchNumData);
  bool compress = true;
  warp_client_.Send(client_msg, compress, server_id);
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
  bool compress = false;
  int server_id = -1;
  ServerMsg server_msg = warp_client_.RecvAny(compress, &server_id);
  CHECK_LT(server_id, num_servers_);
  CHECK_GE(server_id, 0);
  CHECK(server_msg.has_proxy_get_batch_reply());
  const auto& rep = server_msg.proxy_get_batch_reply();
  data_.resize(rep.data_size());
  for (int i = 0; i < rep.data_size(); ++i) {
    data_[i] = rep.data(i);
  }
  curr_ = 0;
  if (rep.data_size() < kBatchNumData) {
    done_[server_id] = true;
  } else {
    RequestOne(server_id);
  }
}

bool ProxyDataIterator::AllDone() const {
  for (const auto& b : done_) {
    if (!b) {
      return false;
    }
  }
  return true;
}

FlexiDatum ProxyDataIterator::GetDatum() {
  CHECK(HasNext());
  FlexiDatum datum = std::move(data_[curr_++]);
  if (curr_ == data_.size() && !AllDone()) {
    // load the next batch
    GetBatch();
  }
  return datum;
}

}  // namespace hotbox
