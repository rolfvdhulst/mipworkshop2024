//
// Created by rolf on 7-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SOLVE_H
#define MIPWORKSHOP2024_SRC_SOLVE_H

#include "Problem.h"
#include "ExternalSolution.h"
#include "mipworkshop2024/presolve/PostSolveStack.h"
#include <optional>

struct SolveStatistics
{
	std::string problemName;

	double primalBound;
	double dualBound;
	double rootLPValue;

	double timeTaken;
	std::size_t nodes;

	bool solvedOptimally;
	bool isUnbounded;
	bool isInfeasible;

	std::size_t numTUImpliedColumns;
	double TUDetectionTime;

	std::size_t numBinaryAfterPresolve;
	std::size_t numIntAfterPresolve;
	std::size_t numImpliedAfterPresolve;
	std::size_t numContinuousAfterPresolve;
	std::size_t numConsAfterPresolve;


	std::size_t numBinaryBeforePresolve;
	std::size_t numIntBeforePresolve;
	std::size_t numImpliedBeforePresolve;
	std::size_t numContinuousBeforePresolve;
	std::size_t numConsBeforePresolve;

	std::size_t numSolutions;
};
struct SCIPRunResult
{
	ExternalSolution solution;
	SolveStatistics statistics;
};
std::optional<SCIPRunResult> solveProblemSCIP(const Problem& problem, double timeLimit);

std::optional<Solution> doPostSolve(const Problem& originalProblem,
		const Solution& fractionalSolution,
		const PostSolveStack& postSolveStack);
#endif //MIPWORKSHOP2024_SRC_SOLVE_H
