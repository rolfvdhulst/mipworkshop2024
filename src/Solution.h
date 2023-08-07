//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SOLUTION_H
#define MIPWORKSHOP2024_SRC_SOLUTION_H

#include <vector>
struct Solution {
  Solution() = default;
  explicit Solution(std::size_t numCols) : values(numCols,0.0){};
  std::vector<double> values;
};

#endif //MIPWORKSHOP2024_SRC_SOLUTION_H
