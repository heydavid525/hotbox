#ifdef USE_TF
#include <glog/logging.h>
#include <utility>
#include "transform/tf_session.hpp"
#include <Eigen/Dense>
#include <algorithm>
#include <cassert>

using namespace tensorflow;

namespace hotbox {

namespace {

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
  config_(config), input_dim_(config.input_dim),
  output_vars_(config.output_vars) {
  Init(config_);
}

TfSession::TfSession(const TfSession& other) :
  config_(other.config_), input_dim_(other.input_dim_),
  output_vars_(other.output_vars_) {
  LOG(INFO) << "Copy constructor";
  Init(config_);
}

TfSession& TfSession::operator=(const TfSession& other) {
  LOG(INFO) << "Assignment constructor";
  config_ = other.config_;
  input_dim_ = other.input_dim_;
  output_vars_ = other.output_vars_;
  Init(config_);
  return *this;
}

void TfSession::Init(const TfSessionConfig& config) {
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
  LOG(INFO) << "Graph loaded";
  LOG(INFO) << "Loading variables weights from " << config.weight_path;
  status = TfRestore(config.weight_path, session_.get());
  CheckStatus(status);
  LOG(INFO) << "Variable loaded";

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
    LOG(INFO) << "output " << output_vars_[i] << " has dim: " <<
      mat.dimensions()[1];
  }
  LOG(INFO) << "end of init, output_dim: " << output_dim_;
}

TfSession::~TfSession() {
  // TODO(wdai): Close resource safely in multi-thread.
  //session_->Close();
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

std::vector<std::vector<float>> TfSession::Transform(
  const std::vector<std::vector<float>>& v) {
  Tensor X(tensorflow::DT_FLOAT, TensorShape({v.size(), input_dim_}));
  auto dst = X.flat<float>().data();
  for (int i = 0; i < v.size(); ++i) {
    std::copy_n(v[i].cbegin(), input_dim_, dst);
    dst += input_dim_;
  }
  std::vector<tensorflow::Tensor> outputs;

  // Run the session, evaluating our "c" operation from the graph
  Status status = session_->Run({{"x", X}}, output_vars_, {}, &outputs);
  CheckStatus(status);

  std::vector<std::vector<float>> out(v.size());
  for (auto& o : out) {
    o.resize(output_dim_);
  }
  int curr = 0;
  for (const auto& t : outputs) {
    auto mat = t.matrix<float>();
    int dim = mat.dimensions()[1];
    for (int i = 0; i < out.size(); ++i) {
      std::copy_n(mat.data() + i * dim, dim, out[i].begin() + curr);
    }
    curr += dim;
  }
  return out;
}

int TfSession::GetOutputDim() const {
  return output_dim_;
}
}  // namespace hotbox
#endif
