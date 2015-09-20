#pragma once

#include <map>
#include <functional>
#include <memory>
#include <glog/logging.h>
#include "util/util.hpp"

namespace mldb {

// Singleton class ClassRegistry implements registration patterns.
// 
// Usage:
//
// class Base {
// public:
//   virtual std::string GetClassName() const = 0;
// };
// 
// class Derived1 : public Base {
// public:
//   std::string GetClassName() const override {
//     return "Derived1";
//   }
// };
// 
// class Derived2 : public Base {
// public:
//   std::string GetClassName() const override {
//     return "Derived2";
//   }
// };
//
// ClassRegistry<Base>::GetRegistry().AddCreator(0, Creator<Base, Derived1>);
// ClassRegistry<Base>::GetRegistry().AddCreator(1, Creator<Base, Derived2>);
// auto& registry = ClassRegistry<Base>::GetRegistry();
// EXPECT_EQ("Derived1", registry.CreateObject(0)->GetClassName());
// EXPECT_EQ("Derived2", registry.CreateObject(1)->GetClassName());
//
// This can apply to multiple Base class, each having their own registry.
template<typename BaseClass>
class ClassRegistry {
public:
  //typedef BaseClass* (*CreateFunc)();
  typedef std::function<BaseClass*(void)> CreateFunc;

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
  std::map<int, CreateFunc> creator_map_;
};

template<typename BaseClass, typename ImplClass>
BaseClass* Creator() {
  return dynamic_cast<BaseClass*>(new ImplClass);
}

}   // namespace mldb
