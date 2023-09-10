//
// Created by rolf on 8-8-23.
//

#ifndef MIPWORKSHOP2024_INCLUDE_MIPWORKSHOP2024_LOGGING_H
#define MIPWORKSHOP2024_INCLUDE_MIPWORKSHOP2024_LOGGING_H

#include "Solve.h"
#include "mipworkshop2024/json.hpp"
struct ProblemLogData {

  std::optional<SolveStatistics> presolvedStatistics;
  std::optional<SolveStatistics> baseLineStatistics;

  [[nodiscard]] nlohmann::json toJson() const;
  static ProblemLogData fromJson(const nlohmann::json& json);
};

#endif //MIPWORKSHOP2024_INCLUDE_MIPWORKSHOP2024_LOGGING_H
