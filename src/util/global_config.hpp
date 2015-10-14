#pragma once

#include <yaml-cpp/yaml.h>
#include <glog/logging.h>
#include <string>
#include "util/file_util.hpp"

namespace hotbox {

namespace {

const std::string kConfigPath = 
  io::ParentPath(io::ParentPath(
      io::Path(__FILE__))).append("/config.yaml");

}  // anonymous namespace

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
};

template<typename V>
V GlobalConfig::Get(const std::string& key, bool* found) const {
  if (found != nullptr) {
    *found = config_[key];
    return *found ? config_[key].as<V>() : 0;
  }
  CHECK(config_[key]) << key << " nout found in " << kConfigPath;
  return config_[key].as<V>();
}

}  // namespace
