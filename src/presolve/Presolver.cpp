//
// Created by rolf on 10-8-23.
//

#include "mipworkshop2024/presolve/Presolver.h"
#include "mipworkshop2024/presolve/IncidenceAddition.h"
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
    SparseMatrix rowMatrix = problem.matrix.transposedFormat();
    auto getColumnVector = [&](index_t index){
        return problem.matrix.getPrimaryVector(index);
    };
    auto getRowVector = [&](index_t index) {
        return rowMatrix.getPrimaryVector(index);
    };

    std::size_t numContinuousCols = 0;
    std::vector<bool> colIsBad(problem.numCols(),false);
    std::vector<index_t> badCols;
    for(std::size_t i = 0; i < problem.numCols(); ++i){
        if(problem.colType[i] == VariableType::CONTINUOUS &&(
                !(isInfinite(-problem.lb[i]) || isFeasIntegral(problem.lb[i])) || !(isInfinite(problem.rhs[i]) ||
                        isFeasIntegral(problem.rhs[i])) ) ){
            colIsBad[i] = true;
            badCols.push_back(i);
        }
    }

    std::vector<bool> rowIsBad(problem.numRows(),false);
    std::vector<index_t> badRows;
    for (index_t i = 0; i < problem.numRows(); ++i) {
        if( !(isInfinite(-problem.lhs[i]) || isFeasIntegral(problem.lhs[i])) || !(isInfinite(problem.rhs[i]) ||
                isFeasIntegral(problem.rhs[i]))){
            rowIsBad[i] = true;
            badRows.push_back(i);
        }
        for(const Nonzero& nonzero : getRowVector(i)){
            if(!isFeasIntegral(nonzero.value())){
                rowIsBad[i] = true;
                badRows.push_back(i);
                break;
            }
        }
    }

    //Now, find out the connected components within the matrix formed by all continuous columns x all rows
    //Use bfs
    {
        long currentComponent = 0;
        std::vector<long> rowComponent(problem.numRows(),0);
        std::vector<long> colComponent(problem.numCols(),0);
        std::vector<index_t> dfsStack;
        for(index_t i = 0; i < problem.numCols(); ++i){
            if(problem.colType[i] == VariableType::CONTINUOUS && colComponent[i] == 0 ){
                ++currentComponent;
                dfsStack.push_back(problem.numRows() + i);
                colComponent[i] = currentComponent;

                //Do DFS
                while(!dfsStack.empty()){
                    index_t index = dfsStack.back();
                    dfsStack.pop_back();
                    if(index >= problem.numRows()){
                        index_t column = index - problem.numRows();
                        for(const Nonzero& nonzero : getColumnVector(column)){
                            assert(rowComponent[nonzero.index()] == 0 || rowComponent[nonzero.index()] == currentComponent);
                            if(rowComponent[nonzero.index()] == 0){
                                dfsStack.push_back(nonzero.index());
                                rowComponent[nonzero.index()] = currentComponent; //TODO; necessary so that we do not
                            }
                        }
                    }else{
                        for(const Nonzero& nonzero : getRowVector(index)){
                            if(problem.colType[nonzero.index()] == VariableType::CONTINUOUS){
                                assert(colComponent[nonzero.index()] == 0 || colComponent[nonzero.index()] == currentComponent);
                                if(colComponent[nonzero.index()] == 0){
                                    dfsStack.push_back(problem.numRows() + nonzero.index());
                                    colComponent[nonzero.index()] = currentComponent;
                                }
                            }
                        }
                    }
                }
            }
        }

        std::cout<<currentComponent<<" connected components in continuous column submatrix\n";
    }

//    ///We look for a set of columns C which form a totally unimodular submatrix.
//    ///Let R be the rows which are nonzero when considering the submatrix formed by the columns C
//	enum ColumnClassification{
//		NON_TU, ///Integral columns with other entries, which must be fixed to imply the integrality of other columns
//		EITHER, ///Integral columns with all +1,-1 entries, which can be part of either the TU or the NON_TU submatrix
//		TU, ///Continuous columns with integral bounds and all +1,-1 entries, which have to be part of the TU submatrix
//		NEITHER ///Continuous columns with either non-integral bounds or non +1,-1 entries
//	};
//
//
//	std::vector<ColumnClassification> classification(problem.numColumns);
//
//    std::vector<bool> rowIsNonIntegral(problem.numRows,false);
//    std::vector<std::size_t> nonIntegralRows;
//
//    std::size_t numNonTU = 0;
//    std::size_t numEither = 0;
//    std::size_t numTU = 0;
//    std::size_t numNeither = 0;
//
//	for(std::size_t i = 0; i < problem.numColumns; ++i){
//        bool isPlusMinusOne = true;
//        for(const Nonzero& nonzero : problem.getColumnVector(i)){
//             if(fabs(nonzero.value()) != 1.0){
//                isPlusMinusOne = false;
//            }
//             if(!isFeasIntegral(nonzero.value())){
//                 index_t row = nonzero.index();
//                 if(!rowIsNonIntegral[row]){
//                     rowIsNonIntegral[row] = true;
//                     nonIntegralRows.push_back(row);
//                 }
//             }
//        }
//		if(problem.colType[i] == VariableType::CONTINUOUS){
//            bool boundsIntegral =
//                    (isInfinite(-problem.colLb[i]) || isFeasIntegral(problem.colLb[i])) && (isInfinite(problem.colUb[i]) || isFeasIntegral(problem.colUb[i]));
//            if(isPlusMinusOne && boundsIntegral){
//                classification[i] = ColumnClassification::TU;
//                ++numTU;
//            }else{
//                classification[i] = ColumnClassification::NEITHER;
//                ++numNeither;
//            }
//
//		}else{ //Variable is integer
//            if(isPlusMinusOne){
//                classification[i] = ColumnClassification::EITHER;
//                ++numEither;
//            }else{
//                classification[i] = ColumnClassification::NON_TU;
//                ++numNonTU;
//            }
//		}
//	}
//
//    //remark columns in rows with non-integral entries or rows which have bad bounds; this way, we can guarantee that the integral values are implied
//    for(index_t row = 0; row < problem.numRows; ++row){
//        bool integralLHS = isInfinite(-problem.rowLhs[row]) || isFeasIntegral(problem.rowLhs[row]);
//        bool integralRHS = isInfinite(problem.rowRhs[row]) || isFeasIntegral(problem.rowRhs[row]);
//        if(!integralLHS || !integralRHS){
//            if(!rowIsNonIntegral[row]){
//                rowIsNonIntegral[row] = true;
//                nonIntegralRows.push_back(row);
//            }
//        }
//    }
//    for(index_t row : nonIntegralRows){
//        for(const Nonzero& entry : problem.getRowVector(row)){
//            index_t col = entry.index();
//            if(classification[col] == EITHER){
//                classification[col] = NON_TU;
//                --numEither;
//                ++numNonTU;
//            }else if(classification[col] == TU){
//                classification[col] = NEITHER;
//                --numTU;
//                ++numNeither;
//            }
//        }
//    }
//    if(numEither + numTU == 0){
//        return;
//    }
//    std::cout<<"Num required TU: "<<numTU<<", Num TU candidates: "<<numEither<<", Num required nonTU: "<<numNonTU<<", Num required disconnected: "<<numNeither<<"\n";
//
//
//    std::vector<bool> rowInSubmatrix(problem.numRows,false);
//    std::vector<index_t> submatrixRows;
//
//    std::vector<bool> colInSubmatrix(problem.numColumns,false);
//    std::vector<index_t> submatrixCols;


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
const Problem& Presolver::presolvedProblem() const
{
	return problem;
}

