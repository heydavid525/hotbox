#pragma once

#include <map>
#include <functional>
#include <memory>
#include <glog/logging.h>
#include "util/util.hpp"

namespace mldb {

// Singleton class ClassRegistry implements registration patterns.
template<typename BaseClass>
class ClassRegistry {
public:
  //typedef BaseClass* (*CreateFunc)();
  typedef std::function<BaseClass*(void)> CreateFunc;

  /*
  void SetDefaultCreator(CreateFunc creator) {
    default_creator_ = creator;
  }
  */

  void AddCreator(int key, CreateFunc creator) {
    const auto pair = creator_map_.insert(std::make_pair(key, creator));
    CHECK(pair.second) << "Key " << key << " already exist in class registry.";
  }

  std::unique_ptr<BaseClass> CreateObject(int key) {
    const auto& it = creator_map_.find(key);
    if (it == creator_map_.cend()) {
      LOG(FATAL) << "Unrecognized key in class registry: " << key;
      return std::unique_ptr<BaseClass>(nullptr);
    }
    CreateFunc creator = it->second;
    return std::unique_ptr<BaseClass>(creator());
  }

  static ClassRegistry<BaseClass>& GetRegistry() {
    static ClassRegistry<BaseClass> registry;
    return registry;
  }

private:
  ClassRegistry() {};
  ClassRegistry(const ClassRegistry&) = delete;
  void operator=(const ClassRegistry&)  = delete;

private:
  //CreateFunc default_creator_;
  std::map<int, CreateFunc> creator_map_;
};

template<typename BaseClass, typename ImplClass>
BaseClass* Creator() {
  return dynamic_cast<BaseClass*>(new ImplClass);
}

}   // namespace mldb
