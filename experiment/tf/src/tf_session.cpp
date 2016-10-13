#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include <utility>
#include "tf_session.hpp"
#include <Eigen/Dense>
#include <iostream>
#include <algorithm>
#include <cassert>

using namespace tensorflow;

namespace hotbox {

namespace {

// Use shared_ptr to support copy constructor.
std::shared_ptr<tensorflow::Session> session_;

void CheckStatus(Status status) {
  if (!status.ok()) {
    std::cerr << status.ToString() << "\n";
    assert(false);
  }
}


Status TfRestore(const string& path, Session* session) {
  TensorProto path_proto;
  path_proto.set_dtype(DataType::DT_STRING);
  TensorShape({1, 1}).AsProto(path_proto.mutable_tensor_shape());
  path_proto.add_string_val(path);
  Tensor path_tensor;

  if (!path_tensor.FromProto(path_proto)) {
    return errors::Internal("Tensor::FromProto failed.");
  }
  return session->Run(
      {{"save/Const:0", path_tensor}},
        {}, {"save/restore_all"}, nullptr);
}

}  // anonymous namespace

TfSession::TfSession(const TfSessionConfig& config) :   
  input_dim_(config.input_dim), output_vars_(config.output_vars) {
  // Initialize a tensorflow session
  tensorflow::SessionOptions options;
  session_.reset(tensorflow::NewSession(options));

  // Read in the protobuf graph we exported
  GraphDef graph_def;
  Status status = ReadBinaryProto(Env::Default(),
    config.graph_path, &graph_def);
  CheckStatus(status);

  // Add the graph to the session
  status = session_->Create(graph_def);
  CheckStatus(status);
  status = TfRestore(config.weight_path, session_.get());
  CheckStatus(status);
  std::cout << "Variable loaded\n";

  // Figure out output dim.
  Tensor X(tensorflow::DT_FLOAT, TensorShape({1, input_dim_}));
  // Sum the dim over all outputs
  std::vector<tensorflow::Tensor> outputs;

  // Run the session, evaluating our "c" operation from the graph
  status = session_->Run({{"x", X}}, output_vars_, {}, &outputs);
  CheckStatus(status);
  for (int i = 0; i < outputs.size(); ++i) {
    auto mat = outputs[i].matrix<float>();
    output_dim_ += mat.dimensions()[1];
    std::cout << "output " << output_vars_[i] << " has dim: " <<
      mat.dimensions()[1] << "\n";
  }
}

TfSession::~TfSession() {
  session_->Close();
}

std::vector<float> TfSession::Transform(const std::vector<float>& v) {
  Tensor X(tensorflow::DT_FLOAT, TensorShape({1, input_dim_}));
  auto dst = X.flat<float>().data();
  std::copy_n(v.cbegin(), input_dim_, dst);
  std::vector<tensorflow::Tensor> outputs;

  // Run the session, evaluating our "c" operation from the graph
  Status status = session_->Run({{"x", X}}, output_vars_, {}, &outputs);
  CheckStatus(status);

  std::vector<float> out(output_dim_);
  int curr = 0;
  for (const auto& t : outputs) {
    auto mat = t.matrix<float>();
    std::copy_n(mat.data(), mat.dimensions()[1], out.begin() + curr);
    curr += mat.dimensions()[1];
  }
  return out;
}
}  // namespace hotbox
