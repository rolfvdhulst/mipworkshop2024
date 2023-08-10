//
// Created by rolf on 1-8-23.
//

#include "mipworkshop2024/Problem.h"
#include <cassert>
#include "mipworkshop2024/ExternalSolution.h"

void Problem::addRow(const std::string& rowName,
                     double rowLHS, double rowRHS) {
  assert(!rowToIndex.contains(rowName));
  index_t index = numRows;
  rowToIndex[rowName] = index;
  rowNames.push_back(rowName);
  lhs.push_back(rowLHS);
  rhs.push_back(rowRHS);
  ++numRows;
  matrix.setNumSecondary(numRows);
}
Problem::Problem() : numRows{0}, numCols{0}, sense{ObjSense::MINIMIZE},objectiveOffset{0.0}{

}
void Problem::addColumn(const std::string &colName,
                        const std::vector<index_t>& entryRows,
                        const std::vector<double>& entryValues,
                        VariableType type,
                        double lowerBound,
                        double upperBound) {
  assert(!colToIndex.contains(colName));
  ++numCols;
  index_t index = matrix.addPrimary(entryRows,entryValues);
  colToIndex[colName] = index;
  colNames.push_back(colName);

  lb.push_back(lowerBound);
  ub.push_back(upperBound);
  obj.push_back(0.0);
  colType.push_back(type);

}
bool Problem::isFeasible(const Solution &solution) const{
  assert(solution.values.size() == numCols);
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
  std::vector<double> rowActivity(numRows,0.0);
  for(std::size_t i = 0; i < numCols; ++i){
    index_t nonzeros = matrix.primaryNonzeros(i);
    const double * vals = matrix.primaryValues(i);
    const index_t * rows = matrix.primaryIndices(i);

    double colVal = solution.values[i];
    for(index_t j = 0; j < nonzeros; ++j){
      rowActivity[rows[j]] += vals[j] * colVal;
    }
  }
  for(index_t i = 0; i < numRows; ++i){
    if(lhs[i] != -infinity && !isFeasGreaterOrEqual(rowActivity[i],lhs[i])){
      return false;
    }
    if(rhs[i] != infinity && !isFeasLessOrEqual(rowActivity[i],rhs[i])){
      return false;
    }
  }
  return true;
}
std::optional<Solution> Problem::convertExternalSolution(const ExternalSolution &solution) const {
  Solution sol(numCols);
  for(const auto& pair : solution.variableValues){
    auto it = colToIndex.find(pair.first);
    if(it == colToIndex.end()){
      return std::nullopt;
    }
    sol.values[it->second] = pair.second;
  }
  return sol;
}
