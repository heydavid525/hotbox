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
  auto& registry = ClassRegistry<ParserIf>::GetRegistry();
  registry.AddCreator(FileFormat::LIBSVM, Creator<ParserIf, LibSVMParser>);
  registry.AddCreator(FileFormat::FAMILY, Creator<ParserIf, FamilyParser>);
}

void RegisterCompressors() {
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  registry.AddCreator(Compressor::SNAPPY, Creator<CompressorIf, SnappyCompressor>);
}

void RegisterTransforms() {
  auto& registry = ClassRegistry<TransformIf>::GetRegistry();
  registry.AddCreator(TransformConfig::kOneHotTransform,
      Creator<TransformIf, OneHotTransform>);
  registry.AddCreator(TransformConfig::kBucketizeTransform,
      Creator<TransformIf, BucketizeTransform>);
  registry.AddCreator(TransformConfig::kConstantTransform,
      Creator<TransformIf, ConstantTransform>);
}

}  // namespace hotbox
