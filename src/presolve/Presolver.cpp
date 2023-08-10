//
// Created by rolf on 10-8-23.
//

#include "mipworkshop2024/presolve/Presolver.h"
#include "mipworkshop2024/presolve/IncidenceAddition.h"

const PostSolveStack& Presolver::postSolveStack() const
{
	return stack;
}
Presolver::Presolver(const Problem& problem) : problem{problem}
{

}
void Presolver::doPresolve()
{
	findTUColumnSubmatrix();
}
void Presolver::findTUColumnSubmatrix()
{
	enum ColumnClassification{
		NON_TU_REQUIRED,
		CANDIDATES, ///Integral columns with all +1,-1 entries
		REQUIRED, ///Continuous columns with integral bounds and all +1,-1 entries
		FIXED_ZERO ///Continuous columns with either non-integral bounds or non +1,-1 entries
	};

	std::size_t nFixedZero;
	std::vector<ColumnClassification> classification(problem.numColumns);
	std::vector<std::size_t> requiredColumns;
	std::vector<std::size_t> candidateColumns;
	//TODO:
	for(std::size_t i = 0; i < problem.numColumns; ++i){
		if(problem.colType[i] == VariableType::CONTINUOUS){

		}else{

		}
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

//	IncidenceAddition addition(problem.numRows,problem.numColumns,INIT)
}
Problem Presolver::presolvedProblem() const
{
	return problem.toProblem();
}
