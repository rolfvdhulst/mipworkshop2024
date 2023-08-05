//
// Created by rolf on 1-8-23.
//

#include "mipworkshop2024/Problem.h"
#include <cassert>
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
