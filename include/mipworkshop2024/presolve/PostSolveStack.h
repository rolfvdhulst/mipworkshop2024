//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_PRESOLVE_POSTSOLVESTACK_H
#define MIPWORKSHOP2024_SRC_PRESOLVE_POSTSOLVESTACK_H

#include "mipworkshop2024/Shared.h"
#include "mipworkshop2024/Solution.h"
#include <vector>
#include <variant>
#include "mipworkshop2024/json.hpp"

struct TotallyUnimodularColumnSubmatrix{
	std::vector<index_t> submatRows;
	std::vector<index_t> implyingColumns;
	std::vector<index_t> submatColumns;
};

class PostSolveStack {
public:

	void totallyUnimodularColumnSubmatrix(const TotallyUnimodularColumnSubmatrix& submatrix);
    [[nodiscard]] bool totallyUnimodularColumnSubmatrixFound() const;

  /// Indicates that a set of columns forms a TU submatrix.
  /// This means that if we are given a solution to the problem with all implyingColumns (which are necessarily integer),
  /// That there exists a solution of the same objective value with all submatColumns integral,
  /// and that this solution can be found by solving the lp of the matrix given by submatRows x submatColumns with the rhs
  /// adjusted so that it reflects the fixed values of the implyingColumns

  /// Although the solver will often find the integral values already, sometimes it will fail to find the integral solution,
  /// due to the addition cutting planes which ruin the integrality during the solving process


  std::vector<TotallyUnimodularColumnSubmatrix> reductions;

  bool containsTUSubmatrix = false;
};

nlohmann::json postSolveToJson(const PostSolveStack& stack);

PostSolveStack postSolveFromJson(const nlohmann::json& json);
#endif //MIPWORKSHOP2024_SRC_PRESOLVE_POSTSOLVESTACK_H
