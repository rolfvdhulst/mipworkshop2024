//
// Created by rolf on 1-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SHARED_H
#define MIPWORKSHOP2024_SRC_SHARED_H

#include <string>
#include <cmath>

using index_t = std::size_t;
static constexpr index_t INVALID = std::numeric_limits<std::size_t>::max();

constexpr double infinity = 1e100;
constexpr double feasTol = 1e-8;

inline bool isInfinite(double value){
  return value == infinity;
}
inline bool isFeasEq(double val1, double val2){
  return fabs(val1-val2) < feasTol;
}
inline bool isFeasIntegral(double value){
  return (value - floor(value + feasTol)) <= feasTol;
}
inline bool isFeasGreaterOrEqual(double value, double other){
  return value + feasTol >= other;
}
inline bool isFeasLessOrEqual(double value, double other){
  return value-feasTol <= other;
}
#endif //MIPWORKSHOP2024_SRC_SHARED_H
