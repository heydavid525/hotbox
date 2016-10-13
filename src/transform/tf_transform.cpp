#ifdef USE_TF
#include <glog/logging.h>
#include <string>
#include "transform/transform_api.hpp"
#include "transform/tf_transform.hpp"
#include "transform/transform_util.hpp"
#include "transform/tf_session.hpp"
#include <sstream>
#include "util/util.hpp"

namespace hotbox {

void TfTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const TfTransformConfig& config =
    param.GetConfig().tf_transform();
  const std::multimap<std::string, WideFamilySelector>& 
  w_family_selectors = param.GetWideFamilySelectors();
  CHECK_EQ(1, w_family_selectors.size());
  const auto& p = *(w_family_selectors.begin());
  StoreTypeAndOffset offsets = p.second.offset;
  auto range = p.second.range_selector;
  int64_t family_idx_begin = range.family_idx_begin;
  int64_t family_idx_end = range.family_idx_end;
  int64_t family_num_features = offsets.offset_end() -
    offsets.offset_begin();
  if (family_idx_begin != family_idx_end) {
    // family-wide selection with family index, e.g., "fam:10-20"
    family_num_features = family_idx_end - family_idx_begin;
  }
  int64_t input_dim = family_num_features;
  TfSessionConfig session_config;
  session_config.output_vars.resize(config.output_vars_size());
  for (int i = 0; i < config.output_vars_size(); ++i) {
    session_config.output_vars[i] = config.output_vars(i);
  }
  session_config.graph_path = config.graph_path();
  session_config.weight_path = config.weight_path();
  session_config.input_dim = input_dim;
  TfSession session(session_config);
  LOG(INFO) << "tf session output: " << session.GetOutputDim();
  writer->AddFeatures(session.GetOutputDim());
}

std::function<void(TransDatum*)> TfTransform::GenerateTransform(
    const TransformParam& param) const {
  const TfTransformConfig& config =
    param.GetConfig().tf_transform();
  const std::multimap<std::string, WideFamilySelector>& 
    w_family_selectors = param.GetWideFamilySelectors();
  WideFamilySelector selector = w_family_selectors.begin()->second;
  BigInt input_dim = selector.offset.offset_end() -
    selector.offset.offset_begin();
  auto range = selector.range_selector;
  if (range.family_idx_begin != range.family_idx_end) {
    input_dim = range.family_idx_end - range.family_idx_begin;
  }
  TfSessionConfig session_config;
  session_config.output_vars.resize(config.output_vars_size());
  for (int i = 0; i < config.output_vars_size(); ++i) {
    session_config.output_vars[i] = config.output_vars(i);
  }
  session_config.output_vars.resize(config.output_vars_size());
  session_config.graph_path = config.graph_path();
  session_config.weight_path = config.weight_path();
  session_config.input_dim = input_dim;
  TfSession session(session_config);
  return [session, selector, input_dim] (TransDatum* datum) mutable {
    const std::vector<float>& v = GetDenseVals(*datum, selector);
    const std::vector<float>& out = session.Transform(v);
    datum->SetFeatureValRelativeOffset(0, out);
  };
}

}  // namespace hotbox
#endif
