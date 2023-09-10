//
// Created by rolf on 10-8-23.
//

#include "mipworkshop2024/presolve/Presolver.h"
#include "mipworkshop2024/presolve/IncidenceAddition.h"
#include "mipworkshop2024/presolve/TUColumnSubmatrix.h"
#include <iostream>

const PostSolveStack& Presolver::postSolveStack() const
{
	return stack;
}

void Presolver::doPresolve(const Problem& t_problem)
{
    //TODO: make datastructure and technique for basic presolving reductions
    problem = t_problem;
    findTUColumnSubmatrix();
}
void Presolver::findTUColumnSubmatrix()
{
	TUColumnSubmatrixFinder finder(problem);
	auto submatrices = finder.computeTUSubmatrices();
	for(const auto& submatrix : submatrices){

#ifndef NDEBUG
		for(const auto& column : submatrix.implyingColumns){
			assert(problem.colType[column] == VariableType::INTEGER || problem.colType[column] == VariableType::BINARY);
		}
#endif
		for(const auto& column : submatrix.submatColumns){
			problem.colType[column] = VariableType::IMPLIED_INTEGER;
		}
		stack.totallyUnimodularColumnSubmatrix(submatrix);
	}


	//TODO: do the below for each connected set of required columns individually.
	//This way, we do not need all continuous columns to be TU for these columns to be implied integer

	//First find the required rows for the required columns. We first check if the rows all have
	//Integral coefficients in the non-required columns and integral/infinite lhs/rhs. If not, even if the required matrix is TU it is of no use
	//TODO: check for scaling/weakening row opportunities, though...

	// Then, test required rows x required column matrix.
	// For incidence/transpose incidence, we first test if the matrix has any columns or rows with at most 1 entry,
	// and remove these for the detection phase, as these can be arbitrarily added to any TU matrix

	//If TU; great! We keep looking to add more integral columns

}
const Problem& Presolver::presolvedProblem() const
{
	return problem;
}

