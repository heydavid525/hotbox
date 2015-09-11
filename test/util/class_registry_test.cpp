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

}  // anonymous namespace

TEST(ClassRegisteryTest, SmokeTest) {
  ClassRegistry<Base>::GetRegistry().AddCreator(0, Creator<Base, Derived1>);
  ClassRegistry<Base>::GetRegistry().AddCreator(1, Creator<Base, Derived2>);
  auto& registry = ClassRegistry<Base>::GetRegistry();
  EXPECT_EQ("Derived1", registry.CreateObject(0)->GetClassName());
  EXPECT_EQ("Derived2", registry.CreateObject(1)->GetClassName());
}

}  // namespace mldb

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
