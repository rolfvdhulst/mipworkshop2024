//
// Created by rolf on 1-8-23.
//

#include "mipworkshop2024/Problem.h"
#include <cassert>
#include "mipworkshop2024/ExternalSolution.h"

void Problem::addRow(const std::string& rowName,
                     double rowLHS, double rowRHS) {
  assert(!rowToIndex.contains(rowName));
  index_t index = matrix.numRows();
  rowToIndex[rowName] = index;
  rowNames.push_back(rowName);
  lhs.push_back(rowLHS);
  rhs.push_back(rowRHS);
  matrix.setNumSecondary(index+1);
}
Problem::Problem() : sense{ObjSense::MINIMIZE},objectiveOffset{0.0}{

}
void Problem::addColumn(const std::string &colName,
                        const std::vector<index_t>& entryRows,
                        const std::vector<double>& entryValues,
                        VariableType type,
                        double lowerBound,
                        double upperBound) {
  assert(!colToIndex.contains(colName));
  index_t index = matrix.addPrimaryVector(entryRows,entryValues);
  colToIndex[colName] = index;
  colNames.push_back(colName);

  lb.push_back(lowerBound);
  ub.push_back(upperBound);
  obj.push_back(0.0);
  colType.push_back(type);

}
bool Problem::isFeasible(const Solution &solution) const{
  assert(solution.values.size() == numCols());
  for(std::size_t i = 0; i < solution.values.size(); ++i){
    double value = solution.values[i];
    if(!isFeasGreaterOrEqual(value,lb[i]) || !isFeasLessOrEqual(value,ub[i])){
      return false;
    }
    if(colType[i] != VariableType::CONTINUOUS && !isFeasIntegral(value)){
      return false;
    }
  }
  //Check the constraints
  std::vector<double> rowActivity(numRows(),0.0);
  for(std::size_t i = 0; i < numCols(); ++i){
      double colVal = solution.values[i];
      for(const Nonzero& nonzero : matrix.getPrimaryVector(i)){
          rowActivity[nonzero.index()] += nonzero.value() * colVal;
      }
  }
  for(index_t i = 0; i < numRows(); ++i){
    if(lhs[i] != -infinity && !isFeasGreaterOrEqualSum(rowActivity[i],lhs[i])){
      return false;
    }
    if(rhs[i] != infinity && !isFeasLessOrEqualSum(rowActivity[i],rhs[i])){
      return false;
    }
  }
  return true;
}
std::optional<Solution> Problem::convertExternalSolution(const ExternalSolution &solution) const {
  Solution sol(matrix.numCols());
  for(const auto& pair : solution.variableValues){
    auto it = colToIndex.find(pair.first);
    if(it == colToIndex.end()){
      return std::nullopt;
    }
    sol.values[it->second] = pair.second;
  }
  return sol;
}

std::size_t Problem::numRows() const {
    return matrix.numRows();
}

std::size_t Problem::numCols() const {
    return matrix.numCols();
}
double Problem::computeObjective(const Solution& solution) const
{
	assert(solution.values.size() == obj.size());
	double sum = objectiveOffset;
	for(index_t i = 0; i < solution.values.size(); ++i){
		sum += solution.values[i] * obj[i];
	}
	return sum;
}
ExternalSolution Problem::convertSolution(const Solution& solution) const
{
	ExternalSolution externalSol;
	externalSol.objectiveValue = computeObjective(solution);

	assert(solution.values.size() == numCols());
	for(index_t i = 0; i < solution.values.size(); ++i){
		externalSol.variableValues[colNames[i]] = solution.values[i];
	}
	return externalSol;
}

void Problem::scale(const std::vector<double> &rowScale, const std::vector<double> &colScale) {
    assert(rowScale.size() == numRows());
    for(index_t i = 0; i < numRows(); ++i){
        double scale = rowScale[i];
        if(lhs[i] != -infinity){
            lhs[i] *= scale;
        }
        if(rhs[i] != infinity){
            rhs[i] *= scale;
        }
    }

    for(index_t i = 0; i < numCols(); ++i){
        assert(colScale[i] != 0.0);
        if(colType[i] == VariableType::CONTINUOUS){
            if(lb[i] != -infinity){
                lb[i] /= colScale[i];
            }
            if(ub[i] != infinity){
                ub[i] /= colScale[i];
            }
            obj[i] *= colScale[i];
        }else{
            assert(colScale[i] == 1.0);
        }
    }
    matrix.scale(rowScale,colScale);

}
