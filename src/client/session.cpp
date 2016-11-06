#include "client/session.hpp"
#include <glog/logging.h>
#include "transform/all.hpp"
#include "client/data_iterator_if.hpp"
#include "client/proxy_data_iterator.hpp"

namespace hotbox {

Session::Session(WarpClient& warp_client, Status status,
    const SessionProto& session_proto, bool use_proxy) :
  use_proxy_(use_proxy), warp_client_(warp_client), status_(status),
  session_proto_(session_proto) {
    LOG(INFO) << "Session created. session_id: "
      << session_proto_.session_id();
    if (use_proxy) {
      // No need to perform transform in proxy mode.
      return;
    }
    auto& registry = ClassRegistry<TransformIf>::GetRegistry();
    for (int i = 0; i < session_proto_.trans_params_size(); ++i) {
      TransformParam trans_param =
        TransformParam(session_proto_.trans_params(i));
      const TransformConfig& config = trans_param.GetConfig();
      std::unique_ptr<TransformIf> transform =
        registry.CreateObject(config.config_case());
      transforms_.push_back(
          transform->GenerateBatchTransform(trans_param));
    }
  }

Session::~Session() {
  ClientMsg client_msg;
  auto mutable_req = client_msg.mutable_close_session_req();
  mutable_req->set_session_id(session_proto_.session_id());
  ServerMsg server_reply = warp_client_.SendRecv(client_msg);
  CHECK(server_reply.has_generic_reply());
  LOG(INFO) << server_reply.generic_reply().msg();
}

OSchema Session::GetOSchema() const {
  return OSchema(session_proto_.o_schema());
}

int64_t Session::GetNumData() const {
  return session_proto_.file_map().num_data();
}

std::unique_ptr<DataIteratorIf> Session::NewDataIterator(
    int64_t data_begin, int64_t data_end,
    int32_t num_transform_threads, int32_t num_io_threads,
    size_t buffer_limit, size_t batch_limit) {
  if (data_end == kDataEnd) {
    data_end = GetNumData();
  }
  if (use_proxy_) {
    return std::unique_ptr<DataIteratorIf>(
        new ProxyDataIterator(warp_client_,
          session_proto_.session_id(), proxy_iter_id_++,
          data_begin, data_end, num_transform_threads,
          num_io_threads, buffer_limit, batch_limit));
  }
  //bool use_multi_threads = num_transform_threads > 1
  //  || num_io_threads > 1;

  return std::unique_ptr<DataIterator>(new DataIterator(session_proto_,
        transforms_, data_begin, data_end, true,
        num_io_threads, num_transform_threads, buffer_limit,
        batch_limit, true));
}

Status Session::GetStatus() const {
  return status_;
}

void Session::SetTransformsToCache(const std::vector<int>& s) {
  session_proto_.mutable_transforms_tocache()->Clear();
  for (auto& i : s) {
    session_proto_.add_transforms_tocache(i);
  }
}

void Session::SetTransformsCached(const std::vector<int>& s) {
  session_proto_.mutable_transforms_cached()->Clear();
  for (auto& i : s) {
    session_proto_.add_transforms_cached(i);
  }
}

}  // namespace hotbox
