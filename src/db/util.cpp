#include "db/util.hpp"
#include "parse/all.hpp"
#include "db/proto/db.pb.h"
#include "util/util.hpp"
#include "util/class_registry.hpp"
#include "util/compressor/all.hpp"

namespace mldb {

void RegisterParsers() {
  auto& registry = ClassRegistry<ParserIf>::GetRegistry();
  registry.AddCreator(FileFormat::LIBSVM, Creator<ParserIf, LibSVMParser>);
  registry.AddCreator(FileFormat::FAMILY, Creator<ParserIf, FamilyParser>);
}

void RegisterCompressors() {
  auto& registry = ClassRegistry<CompressorIf>::GetRegistry();
  registry.AddCreator(Compressor::SNAPPY, Creator<CompressorIf, SnappyCompressor>);
}

}  // namespace mldb
