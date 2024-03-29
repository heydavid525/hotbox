#include "util/register.hpp"
#include "parse/all.hpp"
#include "db/proto/db.pb.h"
#include "util/util.hpp"
#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"
#include "transform/all.hpp"
#include <glog/logging.h>

namespace hotbox {

void RegisterAll() {
  RegisterParsers();
  RegisterCompressors();
  RegisterTransforms();
}

void RegisterParsers() {
  auto& registry = ClassRegistry<ParserIf, const ParserConfig&>::GetRegistry();
  registry.AddCreator(FileFormat::LIBSVM, Creator<ParserIf, LibSVMParser,
      const ParserConfig&>);
  registry.AddCreator(FileFormat::FAMILY, Creator<ParserIf, FamilyParser,
      const ParserConfig&>);
}

void RegisterCompressors() {
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  registry.AddCreator(Compressor::SNAPPY, Creator<CompressorIf, SnappyCompressor>);
}

void RegisterTransforms() {
  auto& registry = ClassRegistry<TransformIf>::GetRegistry();

  registry.AddCreator(TransformConfig::kOneHotTransform,
      Creator<TransformIf, OneHotTransform>);
  kConfigCaseToTransformName[TransformConfig::kOneHotTransform]
    = "OneHotTransform";
  registry.AddCreator(TransformConfig::kBucketizeTransform,
      Creator<TransformIf, BucketizeTransform>);
  kConfigCaseToTransformName[TransformConfig::kBucketizeTransform]
    = "BucketizeTransform";
  registry.AddCreator(TransformConfig::kConstantTransform,
      Creator<TransformIf, ConstantTransform>);
  kConfigCaseToTransformName[TransformConfig::kConstantTransform]
    = "ConstantTransform";
  registry.AddCreator(TransformConfig::kSelectTransform,
      Creator<TransformIf, SelectTransform>);
  kConfigCaseToTransformName[TransformConfig::kSelectTransform]
    = "SelectTransform";
  registry.AddCreator(TransformConfig::kNgramTransform,
      Creator<TransformIf, NgramTransform>);
  kConfigCaseToTransformName[TransformConfig::kNgramTransform]
    = "NgramTransform";
#ifdef USE_TF
  registry.AddCreator(TransformConfig::kTfTransform,
      Creator<TransformIf, TfTransform>);
  kConfigCaseToTransformName[TransformConfig::kTfTransform]
    = "TfTransform";
#endif
  registry.AddCreator(TransformConfig::kKmeansTransform,
      Creator<TransformIf, KmeansTransform>);
  kConfigCaseToTransformName[TransformConfig::kKmeansTransform]
    = "KmeansTransform";
  registry.AddCreator(TransformConfig::kNormalizeTransform,
      Creator<TransformIf, NormalizeTransform>);
  kConfigCaseToTransformName[TransformConfig::kNormalizeTransform]
    = "NormalizeTransform";
}

}  // namespace hotbox
