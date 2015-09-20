#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include "util/class_registry.hpp"

namespace mldb {

namespace {

class Base {
public:
  virtual std::string GetClassName() const = 0;
};

class Derived1 : public Base {
public:
  std::string GetClassName() const override {
    return "Derived1";
  }
};

class Derived2 : public Base {
public:
  std::string GetClassName() const override {
    return "Derived2";
  }
};

class BaseX {
public:
  virtual std::string GetClassName() const = 0;
};

class DerivedX1 : public BaseX {
public:
  std::string GetClassName() const override {
    return "DerivedX1";
  }
};

class DerivedX2 : public BaseX {
public:
  std::string GetClassName() const override {
    return "DerivedX2";
  }
};

}  // anonymous namespace

TEST(ClassRegisteryTest, SmokeTest) {
  ClassRegistry<Base>::GetRegistry().AddCreator(0, Creator<Base, Derived1>);
  ClassRegistry<Base>::GetRegistry().AddCreator(1, Creator<Base, Derived2>);
  auto& registry = ClassRegistry<Base>::GetRegistry();
  EXPECT_EQ("Derived1", registry.CreateObject(0)->GetClassName());
  EXPECT_EQ("Derived2", registry.CreateObject(1)->GetClassName());

  ClassRegistry<BaseX>::GetRegistry().AddCreator(0, Creator<BaseX, DerivedX1>);
  ClassRegistry<BaseX>::GetRegistry().AddCreator(1, Creator<BaseX, DerivedX2>);
  auto& registryX = ClassRegistry<BaseX>::GetRegistry();
  EXPECT_EQ("DerivedX1", registryX.CreateObject(0)->GetClassName());
  EXPECT_EQ("DerivedX2", registryX.CreateObject(1)->GetClassName());
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
