#include "client/session.hpp"
#include "transform/all.hpp"
#include <glog/logging.h>

namespace hotbox {

Session::Session(WarpClient& warp_client, Status status,
    const SessionProto& session_proto) :
  warp_client_(warp_client), status_(status),
  session_proto_(session_proto) {
    LOG(INFO) << "Session created. session_id: " << session_proto_.session_id();
    auto& registry = ClassRegistry<TransformIf>::GetRegistry();
    for (int i = 0; i < session_proto_.trans_params_size(); ++i) {
      TransformParam trans_param =
        TransformParam(session_proto_.trans_params(i));
      const TransformConfig& config = trans_param.GetConfig();
      std::unique_ptr<TransformIf> transform =
        registry.CreateObject(config.config_case());
      transforms_.push_back(transform->GenerateTransform(trans_param));
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

BigInt Session::GetNumData() const {
  return session_proto_.file_map().num_data();
}

/*
DataIterator Session::NewDataIterator(BigInt data_begin,
    BigInt data_end) const {
  if (data_end == -1) {
    data_end = GetNumData();
  }
  LOG(INFO) << "NewDataIterator [" << data_begin << ", " << data_end << ")";
  // TODO(wdai): Do checks on data_begin and data_end.
  return DataIterator(session_proto_, transforms_, data_begin, data_end);
}
*/

DataIterator Session::NewDataIterator(BigInt data_begin,
        BigInt data_end, bool use_multi_threads,
      BigInt num_io_threads, BigInt num_transform_threads,
      BigInt buffer_limit, BigInt batch_limit) const {
  if (data_end == -1) {
    data_end = GetNumData();
  }
  LOG(INFO) << "NewDataIterator [" << data_begin << ", " << data_end << ")";
  
  if (use_multi_threads) {
    LOG(INFO) << "\twith " << num_io_threads << " io threads, "
              << num_transform_threads << " transform threads";
  }

  return DataIterator(session_proto_, transforms_, data_begin, data_end,
          use_multi_threads, num_io_threads, num_transform_threads,
          buffer_limit, batch_limit);
}

MTTransformer* Session::NewMTTransformer(BigInt data_begin,
      BigInt data_end, int io_threads, int transform_threads,
      int buffer_limit, int batch_limit) const {
      if (data_end == -1)
        data_end = GetNumData();
      LOG(INFO) << "NewMTTransformer [" << data_begin << ", " << data_end << ")";
      return new MTTransformer(session_proto_,transforms_,
        data_begin, data_end, io_threads, transform_threads,
        buffer_limit, batch_limit);
}


Status Session::GetStatus() const {
  return status_;
}

}  // namespace hotbox
