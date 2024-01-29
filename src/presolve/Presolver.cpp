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

std::vector<DetectionStatistics> Presolver::doPresolve(const Problem& t_problem,const TUSettings& settings)
{
    //TODO: make datastructure and technique for basic presolving reductions
    problem = t_problem;
    return findTUColumnSubmatrix(settings);
}
std::vector<DetectionStatistics> Presolver::findTUColumnSubmatrix(const TUSettings& settings)
{
    numUpgraded = 0;
    numDowngraded = 0;
	TUColumnSubmatrixFinder finder(problem,settings);
	auto submatrices = finder.computeTUSubmatrices();
    for(const auto& submatrix : submatrices){
        for(const auto& column : submatrix.submatColumns) {
            if (problem.colType[column] == VariableType::CONTINUOUS) {
                ++numUpgraded;
            } else {
                ++numDowngraded;
            }
        }
    }

    if(settings.dynamic){
        double fractionDowngraded= double(numDowngraded) / double(problem.numCols());
        if(numUpgraded == 0 && fractionDowngraded > 0.5){
            std::cout<<"Do dynamic downgrading\n";
            for(const auto& submatrix : submatrices){

#ifndef NDEBUG
                for(const auto& column : submatrix.implyingColumns){
                    assert(problem.colType[column] == VariableType::INTEGER || problem.colType[column] == VariableType::BINARY);
                }
#endif
                for(const auto& column : submatrix.submatColumns){
                    VariableType type = settings.writeType;
                    if(type == VariableType::INTEGER || type == VariableType::BINARY){
                        if(problem.lb[column] == 0.0 && problem.ub[column] == 1.0){
                            type = VariableType::BINARY;
                        }else{
                            type = VariableType::INTEGER;
                        }
                    }

                    problem.colType[column] = type;
                }
                stack.totallyUnimodularColumnSubmatrix(submatrix);
            }
        }
    }else{
        for(const auto& submatrix : submatrices){

#ifndef NDEBUG
            for(const auto& column : submatrix.implyingColumns){
                assert(problem.colType[column] == VariableType::INTEGER || problem.colType[column] == VariableType::BINARY);
            }
#endif
            for(const auto& column : submatrix.submatColumns){
                VariableType type = settings.writeType;
                if(type == VariableType::INTEGER || type == VariableType::BINARY){
                    if(problem.lb[column] == 0.0 && problem.ub[column] == 1.0){
                        type = VariableType::BINARY;
                    }else{
                        type = VariableType::INTEGER;
                    }
                }

                problem.colType[column] = type;
            }
            stack.totallyUnimodularColumnSubmatrix(submatrix);
        }
    }

    return finder.statistics();

}
const Problem& Presolver::presolvedProblem() const
{
	return problem;
}

