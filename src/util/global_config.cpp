
#include "util/global_config.hpp"
#include "util/file_util.hpp"
#include "util/util.hpp"
#include <yaml-cpp/yaml.h>
#include <gflags/gflags.h>
#include <string>
#include <sstream>
#include <glog/logging.h>

DEFINE_string(hb_config_path, "", "yaml config file path.");

namespace hotbox {

// kConfigCaseToTransformName is populated in util/register.cpp
std::map<int, std::string> kConfigCaseToTransformName = {};

GlobalConfig& GlobalConfig::GetInstance() {
  static GlobalConfig instance;
  return instance;
}

GlobalConfig::GlobalConfig() {
  config_path_ = FLAGS_hb_config_path;
  if (config_path_.empty()) {
    config_path_ = io::ParentPath(io::ParentPath(
        io::Path(__FILE__))).append("/config.yaml");
    LOG(INFO) << "db_config_path flag unset. Use default config path: "
      << config_path_;
  }
  std::string buffer = io::ReadFile(config_path_);
  try {
    config_ = YAML::Load(buffer);
  } catch(YAML::ParserException& e) {
    LOG(ERROR) << e.what();
  }
  CHECK(config_.IsMap()) << config_path_ << " has to be key: value pairs";
}

}  // namespace hotbox
