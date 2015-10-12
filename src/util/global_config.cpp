
#include "util/global_config.hpp"
#include "util/file_util.hpp"
#include "util/util.hpp"
//#include "io/fstream.hpp"
#include "io.dmlc/filesys.h"
#include <yaml-cpp/yaml.h>
#include <string>
#include <sstream>
#include <glog/logging.h>

namespace hotbox {

GlobalConfig& GlobalConfig::GetInstance() {
  static GlobalConfig instance;
  return instance;
}

GlobalConfig::GlobalConfig() {
  std::string buffer = ReadFile(kConfigPath);
  try {
    config_ = YAML::Load(buffer);
  } catch(YAML::ParserException& e) {
    LOG(ERROR) << e.what();
  }
  CHECK(config_.IsMap()) << kConfigPath << " has to be key: value pairs";
}

}  // namespace hotbox
