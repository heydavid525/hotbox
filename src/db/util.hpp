#pragma once

#include "db/proto/db.pb.h"
#include "util/class_registry.hpp"
#include "parse/all.hpp"

namespace mldb {

void RegisterParsers() {
  auto& registry = ClassRegistry<ParserIf>::GetRegistry();
  registry.AddCreator(FileFormat::LIBSVM, Creator<ParserIf, LibSVMParser>);
  registry.AddCreator(FileFormat::FAMILY, Creator<ParserIf, FamilyParser>);
}

} // namespace mldb
