//
// Created by rolf on 7-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SOLVE_H
#define MIPWORKSHOP2024_SRC_SOLVE_H

#include "Problem.h"
#include "ExternalSolution.h"
#include <optional>

struct SolveStatistics
{
	double primalBound;
	double dualBound;
	double rootLPValue;

	double timeTaken;
	std::size_t nodes;
};
struct SCIPRunResult
{
	ExternalSolution solution;
	SolveStatistics statistics;
};
std::optional<SCIPRunResult> solveProblemSCIP(const Problem& problem, double timeLimit);
#endif //MIPWORKSHOP2024_SRC_SOLVE_H
