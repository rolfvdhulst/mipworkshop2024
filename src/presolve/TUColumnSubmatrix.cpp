//
// Created by rolf on 10-8-23.
//

#include <algorithm>
#include <iostream>
#include "mipworkshop2024/presolve/TUColumnSubmatrix.h"
#include "mipworkshop2024/presolve/IncidenceAddition.h"
#include "mipworkshop2024/presolve/NetworkAdditionComplete.hpp"

struct TUColumnSubmatrixFinder;
TUColumnSubmatrixFinder::TUColumnSubmatrixFinder(Problem& problem, const TUSettings& settings)
:problem{problem},
rowMatrix{problem.matrix.transposedFormat()},
settings{settings}
{

}
MatrixSlice<CompressedSlice> TUColumnSubmatrixFinder::getRowVector(index_t index) const
{
	return rowMatrix.getPrimaryVector(index);
}
MatrixSlice<CompressedSlice> TUColumnSubmatrixFinder::getColumnVector(index_t index) const
{
	return problem.matrix.getPrimaryVector(index);
}
void TUColumnSubmatrixFinder::computeRowAndColumnTypes()
{
	numContinuousRequired = 0;
	numContinuousDisconnected = 0;
	numIntegralEither = 0;
	numIntegralFixed = 0;
	types.reserve(problem.numCols());

	isNonIntegralRow.assign(problem.numRows(),false);
	nonIntegralRows.clear();
	for(std::size_t i = 0; i < problem.numRows(); ++i){
		if (!(isInfinite(-problem.lhs[i]) || isFeasIntegral(problem.lhs[i])) || !(isInfinite(problem.rhs[i]) ||
				isFeasIntegral(problem.rhs[i])))
		{
			isNonIntegralRow[i] = true;
			nonIntegralRows.push_back(i);
		}
	}
	for(std::size_t i = 0; i < problem.numCols(); ++i){
        bool isPlusMinusOne = true;
		bool occursInBadRow = false;
        for(const Nonzero& nonzero : getColumnVector(i)){
             if(fabs(nonzero.value()) != 1.0){
                isPlusMinusOne = false;
            }
             if(!isFeasIntegral(nonzero.value())){
				 index_t row = nonzero.index();
                 if(!isNonIntegralRow[row]){
                     isNonIntegralRow[row] = true;
                     nonIntegralRows.push_back(row);
                 }
             }
			 occursInBadRow = occursInBadRow || isNonIntegralRow[nonzero.index()];
        }
		if(problem.colType[i] == VariableType::CONTINUOUS){
            bool boundsIntegral =
                    (isInfinite(-problem.lb[i]) || isFeasIntegral(problem.lb[i])) &&
					(isInfinite(problem.ub[i]) || isFeasIntegral(problem.ub[i]));
            if(isPlusMinusOne && boundsIntegral && !occursInBadRow){
                types.push_back(TUColumnType::CONTINUOUS_REQUIRED);
                ++numContinuousRequired;
            }else{
				types.push_back(TUColumnType::CONTINUOUS_DISCONNECTED);
				++numContinuousDisconnected;
            }

		}else{ //Variable is integer
            if(isPlusMinusOne && !occursInBadRow){
				types.push_back(TUColumnType::INTEGRAL_EITHER);
				++numIntegralEither;
            }else{
				types.push_back(TUColumnType::INTEGRAL_FIXED);
				++numIntegralFixed;
            }
		}
	}

}
std::vector<TotallyUnimodularColumnSubmatrix> TUColumnSubmatrixFinder::computeTUSubmatrices() {
    detectionStatistics.clear();
	//TODO: changing INTEGRAL_EITHER to INTEGRAL_FIXED can also be done only when we check to add INTEGRAL_EITHER
	//This potentially saves a lot of row-wise iterations over the matrix

	computeRowAndColumnTypes();
//	if(numContinuousRequired + numContinuousDisconnected == 0){
//		//We use a different algorithm (although largely the same ideas) in this case, because it is much easier to handle
//		//Continuous columns make things more difficult.
//		return integralComputeTUSubmatrices();
//	}
	return mixedComputeTUSubmatrices();
}

struct ColumnInfo{
	index_t index;
	double score;
};
std::vector<TotallyUnimodularColumnSubmatrix> TUColumnSubmatrixFinder::integralComputeTUSubmatrices()
{
	assert(numContinuousRequired == 0 && numContinuousDisconnected == 0);
	//First, we change all columns of INTEGRAL_EITHER with entries in non-integral rows to INTEGRAL_FIXED
	for(index_t row : nonIntegralRows ){
		for(const Nonzero& nonzero : getRowVector(row)){
			if(types[nonzero.index()] == TUColumnType::INTEGRAL_EITHER){
				types[nonzero.index()] = TUColumnType::INTEGRAL_FIXED;
				--numIntegralEither;
				++numIntegralFixed;
			}
		}
	}
	//Check if we can find a TU matrix at all
	if(numIntegralEither == 0){
		return {};
	}

	//TODO; separate method for network matrices here
	std::vector<bool> rowIsUnit(problem.numRows(),false);
	std::vector<index_t> unitRows;

	for(index_t row = 0; row < problem.numRows(); ++row){
		if(isNonIntegralRow[row]) continue;
		index_t numNonzero = 0;
		for(const Nonzero& nonzero : getRowVector(row)){
			if(types[nonzero.index()] == TUColumnType::INTEGRAL_EITHER){
				++numNonzero;
				if(numNonzero > 1){
					break;
				}
			}
		}
		if(numNonzero == 1){
			rowIsUnit[row] = true;
			unitRows.push_back(row);
		}
	}


	double maxObj = 0.0;
	for(index_t i = 0; i < problem.numCols(); ++i){
		double absObj = std::abs(problem.obj[i]);
		if(absObj > maxObj){
			maxObj = absObj;
		}
	}

	std::vector<index_t> unitColumns;
	std::vector<ColumnInfo> candidateColumns;
	for(index_t i = 0; i < problem.numCols(); ++i){
		if(types[i] == TUColumnType::INTEGRAL_EITHER){
			index_t numColNonzeros = 0;
			for(const Nonzero& nonzero : getColumnVector(i)){
				assert(!isNonIntegralRow[nonzero.index()]);
				if(!rowIsUnit[nonzero.index()]){ //TODO: we could also simply substract one from relevant columns when computing unit rows
					++numColNonzeros;
				}
			}
			if(numColNonzeros <= 1){
				unitColumns.push_back(i);
				continue;
			}
			double score = 0.0;
			if(problem.colType[i] == VariableType::INTEGER){
				score += 1.0;
			}
			if(maxObj > 0.0){
				score += 1-(std::abs(problem.obj[i]) / maxObj);
			}
			//TODO: take bounds into account?
			//zero objective? Normal integer over binary, depending on bound width integer?

			candidateColumns.push_back({i,score});
		}
	}

	std::sort(candidateColumns.begin(),candidateColumns.end(),[](const ColumnInfo& first, const ColumnInfo& second){
		return first.score > second.score;
	});
	std::vector<Submatrix> submatrices;
	for(bool transposed : {false,true}){
		IncidenceAddition addition(problem.numRows(),problem.numCols(),Submatrix::INIT_NONE,transposed);
		for(index_t i = 0; i < problem.numRows(); ++i){
			if(!isNonIntegralRow[i] && !rowIsUnit[i]){
				addition.tryAddRow(i,MatrixSlice<EmptySlice>());
			}
		}

		for(const auto& candidate : candidateColumns){
			addition.tryAddCol(candidate.index, getColumnVector(candidate.index));
		}
		Submatrix submatrix = addition.createSubmatrix();
		for(const auto& unitCol : unitColumns){
			submatrix.addColumn(unitCol);
		}
		for(const auto& unitRow : unitRows){
			submatrix.addRow(unitRow);
		}
		submatrices.push_back(submatrix);
	}
	//For now, simply pick submatrix with largest size
	auto best = std::max_element(submatrices.begin(),submatrices.end(),[](const auto& first, const auto& second){
		return first.columns.size() < second.columns.size();
	});
	if(best->columns.empty()){
		return {};
	}
	std::cout<<"Submatrix found: "<<double(best->columns.size()) / double(problem.numCols())<<"\n";

	TotallyUnimodularColumnSubmatrix submatrix = computeImplyingColumns(*best);
	return {submatrix};
}
std::vector<TotallyUnimodularColumnSubmatrix> TUColumnSubmatrixFinder::mixedComputeTUSubmatrices()
{
	//Find connected components of submatrix formed by continuous columns
	//Iterate over the bad rows and bad columns and mark the component containing the row/column as bad if it contains a bad row or column
	//For newly marked bad rows, change any columns intersecting it of INTEGRAL_EITHER to INTEGRAL_FIXED

	long currentComponent = 0;

	std::vector<Component> components;
	std::vector<long> rowComponent(problem.numRows(), 0);
	std::vector<long> colComponent(problem.numCols(), 0);

	std::vector<long> cSubMatRowEntries(problem.numRows(),0);
	std::vector<long> cSubMatColEntries(problem.numCols(),0);
	{
		std::vector<index_t> dfsStack;
		for (index_t i = 0; i < problem.numCols(); ++i)
		{
			if (problem.colType[i] == VariableType::CONTINUOUS && colComponent[i] == 0){
				Component component;
				++currentComponent;
				dfsStack.push_back(problem.numRows() + i);
				colComponent[i] = currentComponent;

				//Do DFS
				while (!dfsStack.empty())
				{
					index_t index = dfsStack.back();
					dfsStack.pop_back();
					if (index >= problem.numRows())
					{

						long nEntries = 0;

						index_t column = index - problem.numRows();
						component.cols.push_back(column);
						for (const Nonzero& nonzero : getColumnVector(column))
						{
							++nEntries;
							assert(rowComponent[nonzero.index()] == 0
									|| rowComponent[nonzero.index()] == currentComponent);
							if (rowComponent[nonzero.index()] == 0)
							{
								dfsStack.push_back(nonzero.index());
								rowComponent[nonzero.index()] = currentComponent;
							}
						}
						cSubMatColEntries[column] = nEntries;
					}
					else
					{
						component.rows.push_back(index);

						long nEntries = 0;
						for (const Nonzero& nonzero : getRowVector(index))
						{
							if (problem.colType[nonzero.index()] == VariableType::CONTINUOUS)
							{
								assert(colComponent[nonzero.index()] == 0
										|| colComponent[nonzero.index()] == currentComponent);
								if (colComponent[nonzero.index()] == 0)
								{
									dfsStack.push_back(problem.numRows() + nonzero.index());
									colComponent[nonzero.index()] = currentComponent;
								}
								++nEntries;
							}
						}
						cSubMatRowEntries[index] = nEntries;
					}
				}
				components.push_back(component);
			}
		}
	}

	std::vector<bool> componentValid(currentComponent, true);

	std::size_t numValidComponents = currentComponent;

	for (index_t col = 0; col < problem.numCols(); ++col)
	{
		if (colComponent[col] == 0 || types[col] != TUColumnType::CONTINUOUS_DISCONNECTED) continue;
		index_t component = colComponent[col] - 1;
		if (componentValid[component])
		{
			--numValidComponents;
		}
		componentValid[component] = false;
	}

	for (const index_t row : nonIntegralRows)
	{
		if (rowComponent[row] == 0) continue;
		index_t component = rowComponent[row] - 1;
		if (componentValid[component])
		{
			--numValidComponents;
		}
		componentValid[component] = false;
	}

	//Although there may be some no components at this point, we could still find a submatrix in the integer part

	index_t numCandidateComponents = numValidComponents;
	//For each component, determine if it is TU
	//If it is not TU, iterate over the component rows and change any column entries in the row of type INTEGRAL_EITHER to INTEGRAL_FIXED

	//TODO: test if combining transposed/non-transposed makes any sense

	std::vector<Submatrix> submatrices;
	for(bool transposed : {true,false}){
		Submatrix submatrix = computeIncidenceSubmatrix(transposed,components,componentValid,
				cSubMatRowEntries,cSubMatColEntries,rowComponent);
		submatrices.push_back(submatrix);
		submatrices.push_back(computeNetworkSubmatrix(transposed,components,componentValid,rowComponent));
	}
    auto best = std::max_element(submatrices.begin(),submatrices.end(),[](const auto& first, const auto& second){
        return first.columns.size() < second.columns.size();
    });
	auto& bestSubmatrix = *best;
	if(bestSubmatrix.columns.empty()){
		return {};
	}
//	std::cout<<"Selecting submatrix with: "<<bestSubmatrix.columns.size()<<" columns\n";
	return {computeImplyingColumns(bestSubmatrix)};

}
TotallyUnimodularColumnSubmatrix TUColumnSubmatrixFinder::computeImplyingColumns(const Submatrix& submatrix) const
{
	std::vector<bool> implyingColumnAdded(problem.numCols(),false);
	std::vector<index_t> implyingColumns;
	for(const auto& row : submatrix.rows){
		for(const auto& entry : getRowVector(row)){
			index_t column = entry.index();
			if(!implyingColumnAdded[column] && !submatrix.containsColumn[column]){
				assert(types[column] == TUColumnType::INTEGRAL_FIXED || types[column] == TUColumnType::INTEGRAL_EITHER);
				implyingColumnAdded[column] = true;
				implyingColumns.push_back(column);
			}
		}
	}

#ifndef NDEBUG
	for(const auto& column : submatrix.columns){
		assert(types[column] == TUColumnType::CONTINUOUS_REQUIRED || types[column] == TUColumnType::INTEGRAL_EITHER);
	}
	for(const auto& row : submatrix.rows){
		assert(!isNonIntegralRow[row]);
	}
#endif
	return 	TotallyUnimodularColumnSubmatrix {
			.submatRows = submatrix.rows,
			.implyingColumns = implyingColumns,
			.submatColumns = submatrix.columns,
	};
}
Submatrix TUColumnSubmatrixFinder::computeIncidenceSubmatrix(bool transposed,
		const std::vector<Component>& components,
		const std::vector<bool>& componentValid,
		const std::vector<long>& nRowEntries,
		const std::vector<long>& nColEntries,
		const std::vector<long>& rowComponents)
{
    DetectionStatistics stats;
    auto tStart = std::chrono::high_resolution_clock::now();

    IncidenceAddition addition(problem.numRows(),problem.numCols(),Submatrix::INIT_NONE,transposed);
	std::vector<index_t> validComponents;
	std::vector<index_t> invalidComponents;

	std::vector<index_t> unitRows;
	std::vector<index_t> unitCols;

	std::vector<index_t> unitRowColumns;
	std::vector<index_t> unitColRows;

    index_t numErasedComponents = 0;

	for(std::size_t i = 0; i < components.size(); ++i){
		if(!componentValid[i])
		{
			invalidComponents.push_back(i);
			continue;
		}
		auto& component = components[i];
		std::vector<index_t> componentUnitRows;
		std::vector<index_t> componentUnitCols;
		for(index_t row : component.rows){
			if(!transposed && nRowEntries[row] == 1){
				componentUnitRows.push_back(row);
			}else{
				bool added = addition.tryAddRow(row,MatrixSlice<EmptySlice>());
				assert(added);
			}
		}
		bool good = true;
		for(index_t col : component.cols){
			if(transposed && nColEntries[col] == 1)
			{
				componentUnitCols.push_back(col);
			}else{
				if (!addition.tryAddCol(col, getColumnVector(col)))
				{
					good = false;
					break;
				}
			}
		}
		if(good){
			validComponents.push_back(i);
			if(!transposed && componentUnitRows.size() == component.rows.size() && componentUnitRows.size() <= 2)
			{
				for (index_t col : component.cols)
				{
					assert(addition.containsColumn(col));
				}
				for (index_t row : component.rows)
				{
					bool added = addition.tryAddRow(row, getRowVector(row));
					assert(added);
				}

			}
			else
			{
				unitRows.insert(unitRows.end(), componentUnitRows.begin(), componentUnitRows.end());
				if (!transposed && componentUnitRows.size() == component.rows.size())
				{
                    ++numErasedComponents;
					addition.removeComponent(component.rows,component.cols);
					assert(component.cols.size() <= 1);
					unitRowColumns.insert(unitRowColumns.end(),component.cols.begin(),component.cols.end());
				}
			}
			if(transposed && componentUnitCols.size() == component.cols.size() && componentUnitCols.size() < 2){
				for(index_t row : component.rows){
					assert(addition.containsRow(row));
				}
				for(index_t col : component.cols){
					bool added = addition.tryAddCol(col, getColumnVector(col));
					assert(added);
				}
			}
			else{
				unitCols.insert(unitCols.end(),componentUnitCols.begin(),componentUnitCols.end());
				if (transposed && componentUnitCols.size() == component.cols.size())
				{
                    ++numErasedComponents;
					addition.removeComponent(component.rows,component.cols);
					assert(component.rows.size() <= 1);
					unitColRows.insert(unitColRows.end(),component.rows.begin(),component.rows.end());
				}
			}
		}else{
			invalidComponents.push_back(i);
			addition.removeComponent(component.rows,component.cols);
		}
	}

    index_t expandedColumns = 0;

    if(numIntegralEither > 0 && settings.doDowngrade){
		// Clean up rows; If we removed any components because the continuous columns are not TU,
		// integral_either entries need to become integral_fixed, and the row excluded from any further candidates
		std::vector<bool> extendedInvalidRows = isNonIntegralRow;

		for(index_t index : invalidComponents){
			const auto& component = components[index];
			for(index_t row : component.rows){
				extendedInvalidRows[row] = true;
			}
		}
		//Mark unit rows as invalid; if our column these we cannot use the unit row /unit col argument anymore
		for(index_t row : unitRows){
			assert(!addition.containsRow(row));
			extendedInvalidRows[row] = true;
		}
		//Not strictly speaking necessary, but
		for(index_t row : unitColRows){
			assert(!addition.containsRow(row));
			extendedInvalidRows[row] = true;
		}

		struct ColCandidateData{
			index_t column;
			index_t nContinuousComponents;
			index_t nonComponentNonzeros;
			index_t nonzeros;
			double obj;
			bool isBinary;
		};

		std::vector<ColCandidateData> candidates;
		std::unordered_map<long,index_t> colComponentEntries; //component idx -> number of entries
		for(index_t i = 0; i < problem.numCols(); ++i){
			if(types[i] != TUColumnType::INTEGRAL_EITHER) continue;
			colComponentEntries.clear();
			bool containsInvalidRow = false;
			index_t nonzeros = 0;
			for(const auto& nonzero : getColumnVector(i)){
				if(extendedInvalidRows[nonzero.index()]){
					containsInvalidRow = true;
					break;
				}
				++nonzeros;
				long component = rowComponents[nonzero.index()] - 1 ;
				auto hashmapEntry = colComponentEntries.find(component);
				if(hashmapEntry == colComponentEntries.end()){
					colComponentEntries.insert({component,1});
				}else{
					++hashmapEntry->second;
				}
			}
			if(containsInvalidRow) continue;
			index_t nNonComponentEntries = colComponentEntries[-1];
			ColCandidateData data{
				.column = i,
				.nContinuousComponents = colComponentEntries.size() -1,
				.nonComponentNonzeros = nNonComponentEntries,
				.nonzeros = nonzeros,
				.obj = problem.obj[i],
				.isBinary = problem.colType[i] == VariableType::BINARY,
			};
			candidates.push_back(data);
		}

        std::sort(candidates.begin(),candidates.end(),[](const ColCandidateData& first, const ColCandidateData& second){
            if(first.nonzeros == second.nonzeros){
                return first.obj < second.obj;
            }
            return first.nonzeros < second.nonzeros;
        });

		for(index_t row = 0; row < problem.numRows(); ++row){
			if(!addition.containsRow(row) && !extendedInvalidRows[row] )
			{
				bool good = addition.tryAddRow(row,MatrixSlice<EmptySlice>());
				assert(good);
			}

		}
		for(const auto& candidate : candidates){
            assert(problem.colType[candidate.column] != VariableType::CONTINUOUS);
			if(addition.tryAddCol(candidate.column, getColumnVector(candidate.column))){
				++expandedColumns;
			}
		}
	}
    std::size_t contColumns = 0;
    for(const auto& component : validComponents){
        contColumns += components[component].cols.size();
    }

    auto tEnd = std::chrono::high_resolution_clock::now();

    std::cout<<"Incidence: Continuous columns: "<< contColumns  <<", expanded with "<<expandedColumns<<" integral columns, "
             << unitCols.size()<< " unit columns, time: "<<(tEnd-tStart).count()/1e9<<" s"<<std::endl;

	Submatrix submatrix = addition.createSubmatrix();
	if(!transposed){
		for(const auto& unitRow : unitRows){
			assert(!submatrix.containsRow[unitRow]);
			submatrix.addRow(unitRow);
		}
		for(const auto& column : unitRowColumns){
			assert(!submatrix.containsColumn[column]);
			submatrix.addColumn(column);
		}
	}else{
		for(const auto& unitCol : unitCols){
			assert(!submatrix.containsColumn[unitCol]);
			submatrix.addColumn(unitCol);
		}
		for(const auto& row : unitColRows){
			assert(!submatrix.containsRow[row]);
			submatrix.addRow(row);
		}
	}

    stats.method = transposed ? "transposed incidence addition" : "incidence addition";
    stats.timeTaken = (tEnd-tStart).count() /1e9;
    stats.numUpgraded = contColumns;
    stats.numDowngraded= expandedColumns;

    stats.numErasedComponents = numErasedComponents;
    stats.numComponents = addition.numComponents();
    stats.numNodesTypeS = 0;
    stats.numNodesTypeP = 0;
    stats.numNodesTypeQ = 0;
    stats.numNodesTypeR = 0;
    stats.largestRNumArcs = 0;
    stats.numTotalRArcs = 0;

    stats.numRows = submatrix.rows.size();
    stats.numColumns = submatrix.columns.size();

    detectionStatistics.push_back(stats);

	return submatrix;
}

Submatrix TUColumnSubmatrixFinder::computeNetworkSubmatrix(bool transposed,
                                                           const std::vector<Component> &components,
                                                           const std::vector<bool>& componentValid,
                                                           const std::vector<long> &rowComponents) {
    auto tStart = std::chrono::high_resolution_clock::now();

    NetworkAddition addition(problem.numRows(),problem.numCols(),Submatrix::INIT_NONE,transposed);

    std::size_t numErasedComponents = 0;

    std::vector<index_t> invalidComponents;
    std::vector<index_t> validComponents;
	for(std::size_t i = 0; i < components.size(); ++i){
		auto& component = components[i];
        if(!componentValid[i]){
            invalidComponents.push_back(i);
            continue;
        }
        bool good = true;

        for (index_t col: component.cols) {
            if (!addition.tryAddCol(col, getColumnVector(col))) {
                good = false;
                break;
            }
        }

		if(good){
			validComponents.push_back(i);
		}else{
			invalidComponents.push_back(i);
			addition.removeComponent(component.rows,component.cols);
            ++numErasedComponents;
		}
	}
    auto tMid = std::chrono::high_resolution_clock::now();

    index_t expandedColumns = 0;

    if(numIntegralEither > 0 && settings.doDowngrade){
        // Clean up rows; If we removed any components because the continuous columns are not TU,
        // integral_either entries need to become integral_fixed, and the row excluded from any further candidates
        std::vector<bool> extendedInvalidRows = isNonIntegralRow;

        for(index_t index : invalidComponents){
            const auto& component = components[index];
            for(index_t row : component.rows){
                extendedInvalidRows[row] = true;
            }
        }

        struct ColCandidateData{
            index_t column;
            index_t nContinuousComponents;
            index_t nonComponentNonzeros;
            index_t nonzeros;
            double obj;
            bool isBinary;
        };

        std::vector<ColCandidateData> candidates;
        std::unordered_map<long,index_t> colComponentEntries; //component idx -> number of entries
        for(index_t i = 0; i < problem.numCols(); ++i){
            if(types[i] != TUColumnType::INTEGRAL_EITHER) continue;
            colComponentEntries.clear();
            bool containsInvalidRow = false;
            index_t nonzeros = 0;
            for(const auto& nonzero : getColumnVector(i)){
                if(extendedInvalidRows[nonzero.index()]){
                    containsInvalidRow = true;
                    break;
                }
                ++nonzeros;
                long component = rowComponents[nonzero.index()] - 1 ;
                auto hashmapEntry = colComponentEntries.find(component);
                if(hashmapEntry == colComponentEntries.end()){
                    colComponentEntries.insert({component,1});
                }else{
                    ++hashmapEntry->second;
                }
            }
            if(containsInvalidRow) continue;
            index_t nNonComponentEntries = colComponentEntries[-1];
            ColCandidateData data{
                    .column = i,
                    .nContinuousComponents = colComponentEntries.size() -1,
                    .nonComponentNonzeros = nNonComponentEntries,
                    .nonzeros = nonzeros,
                    .obj = problem.obj[i],
                    .isBinary = problem.colType[i] == VariableType::BINARY,
            };
            candidates.push_back(data);
        }

        std::vector<bool> colsInCurrentMatrix(problem.numCols(),false);
        for(index_t component : validComponents){
            for(const auto& col : components[component].cols){
                colsInCurrentMatrix[col] = true;
            }
        }


        std::sort(candidates.begin(),candidates.end(),[](const ColCandidateData& first, const ColCandidateData& second){
            if(first.nonzeros == second.nonzeros){
                return first.obj < second.obj;
            }
            return first.nonzeros < second.nonzeros;
        });

        std::size_t triedAdditions = candidates.size();
        for(const auto& candidate : candidates){
            assert(problem.colType[candidate.column] != VariableType::CONTINUOUS);
            if(addition.tryAddCol(candidate.column, getColumnVector(candidate.column))){
                ++expandedColumns;
            }
        }
    }

    auto tEnd = std::chrono::high_resolution_clock::now();

    std::size_t contColumns = 0;
    for(const auto& component : validComponents){
        contColumns += components[component].cols.size();
    }

    std::cout<<"Network: Continuous columns: "<< contColumns  <<", expanded with "<<expandedColumns<<" integral columns, total time: "
    << (tEnd-tStart).count()/1e9<<" s, cont time: "<<(tMid-tStart).count()/1e9<<" s"
    <<std::endl;

    Submatrix matrix = addition.createSubmatrix(problem.numRows(),problem.numCols());

    DetectionStatistics stats;
    stats.method = transposed ? "transposed network addition" : "network addition";
    stats.timeTaken = (tEnd-tStart).count() /1e9;
    stats.numUpgraded = contColumns;
    stats.numDowngraded= expandedColumns;

    stats.numErasedComponents = numErasedComponents;

    auto spqrStats = addition.statistics();
    stats.numComponents = spqrStats.numComponents;
    stats.numNodesTypeS = spqrStats.numSkeletonsTypeS;
    stats.numNodesTypeP = spqrStats.numSkeletonsTypeP;
    stats.numNodesTypeQ = spqrStats.numSkeletonsTypeQ;
    stats.numNodesTypeR = spqrStats.numSkeletonsTypeR;
    stats.largestRNumArcs = spqrStats.numArcsLargestR;
    stats.numTotalRArcs = spqrStats.numArcsTotalR;

    stats.numRows = matrix.rows.size();
    stats.numColumns = matrix.columns.size();

    detectionStatistics.push_back(stats);

    return matrix;
}

std::vector<DetectionStatistics> TUColumnSubmatrixFinder::statistics() const {
    return detectionStatistics;
}
