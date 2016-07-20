#pragma once

#include <algorithm>
#include <cmath>

namespace petuum {
namespace fm {

namespace {

const float kCutoff = 1e-15;
const float kExpCutoff = 35;

}  // Anonymous namespace

float SafeLog(float x) {
  if (std::abs(x) < kCutoff) {
    x = kCutoff;
  }
  return std::log(x);
}

float SafeExp(float x) {
  x = std::max(std::min(x, kExpCutoff), -kExpCutoff);
  return std::exp(x);
}

float Sigmoid(float x) {
  return 1. / (1. + SafeExp(-x));
}

} // namespace fm
} // namespace petuum
