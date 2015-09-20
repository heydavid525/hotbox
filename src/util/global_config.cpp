
#include "util/global_config.hpp"
#include "io/fstream.hpp"
#include <yaml-cpp/yaml.h>
#include <string>
#include <sstream>

namespace mldb {

GlobalConfig& GlobalConfig::GetInstance() {
  static GlobalConfig instance;
  return instance;
}

GlobalConfig::GlobalConfig() {
  io::ifstream fin(kConfigPath);
  CHECK(fin) << "Can't open " << kConfigPath;
  std::stringstream buffer;
  buffer << fin.rdbuf();
  try {
    config_ = YAML::Load(buffer.str());
  } catch(YAML::ParserException& e) {
    LOG(ERROR) << e.what();
  }
  CHECK(config_.IsMap()) << kConfigPath << " has to be key: value pairs";
}

}  // namespace mldb
