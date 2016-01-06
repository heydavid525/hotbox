#pragma once

#include <yaml-cpp/yaml.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <string>
#include "util/file_util.hpp"
#include <map>

DECLARE_string(hb_config_path);

namespace hotbox {

// Global map: TransformConfig::ConfigCase(int) --> Transform name (used as
// default output family name). Initialized in util/register.cpp
extern std::map<int, std::string> kConfigCaseToTransformName;

/*
namespace {

const std::string FLAGS_config_path = 
  io::ParentPath(io::ParentPath(
      io::Path(__FILE__))).append("/config.yaml");

}  // anonymous namespace
*/

class GlobalConfig {
public:
  static GlobalConfig& GetInstance();

  // Getters. 'found' is set to true if key is found (if 'found' is not
  // nullptr). If found is false then returned value is invalid.  otherwise
  // fails the program when not found. V = {int, double, std::string, bool}.
  template<typename V>
  V Get(const std::string& key, bool* found = nullptr) const;

private:
  GlobalConfig();

  GlobalConfig(const GlobalConfig&) = delete;
  void operator=(const GlobalConfig&) = delete;

private:
  YAML::Node config_;
  std::string config_path_;
};

template<typename V>
V GlobalConfig::Get(const std::string& key, bool* found) const {
  if (found != nullptr) {
    *found = config_[key];
    return *found ? config_[key].as<V>() : 0;
  }
  CHECK(config_[key]) << key << " nout found in " << config_path_;
  return config_[key].as<V>();
}

}  // namespace
