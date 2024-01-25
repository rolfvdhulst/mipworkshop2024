//
// Created by rolf on 10-8-23.
//

#ifndef MIPWORKSHOP2024_PRESOLVER_H
#define MIPWORKSHOP2024_PRESOLVER_H

#include "PostSolveStack.h"
#include "mipworkshop2024/Problem.h"
#include "TUColumnSubmatrix.h"

class Presolver
{
public:
	Presolver() = default;
	std::vector<DetectionStatistics> doPresolve(const Problem& problem, const TUSettings& settings);
	[[nodiscard]] const Problem& presolvedProblem() const;
	[[nodiscard]] const PostSolveStack& postSolveStack() const;

    std::size_t numUpgraded = 0;
    std::size_t numDowngraded = 0;
private:
    std::vector<DetectionStatistics> findTUColumnSubmatrix(const TUSettings& settings);


	Problem problem;
	PostSolveStack stack;
};

#endif //MIPWORKSHOP2024_PRESOLVER_H
