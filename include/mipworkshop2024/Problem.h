//
// Created by rolf on 1-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_PROBLEM_H
#define MIPWORKSHOP2024_SRC_PROBLEM_H

#include <vector>
#include <unordered_map>
#include <string>
#include "SparseMatrix.h"
#include "Shared.h"

enum class ObjSense{
  MINIMIZE,
  MAXIMIZE
};
enum class VariableType{
  CONTINUOUS = 0,
  BINARY = 1,
  INTEGER = 2,
  IMPLIED_INTEGER = 3
};
struct Problem {
  Problem();
  void addRow(const std::string& rowName,double rowLHS, double rowRHS);
  void addColumn(const std::string &colName,
                 const std::vector<index_t>& entryRows,
                 const std::vector<double>& entryValues,
                 VariableType type,
                 double lowerBound,
                 double upperBound);

  index_t numRows;
  index_t numCols;

  SparseMatrix matrix;
  //TODO: rename below so that col/row attribution is clearer
  std::vector<double> obj;
  std::vector<double> lb;
  std::vector<double> ub;
  std::vector<VariableType> colType;

  std::vector<double> lhs;
  std::vector<double> rhs;

  ObjSense sense;
  double objectiveOffset;

  std::string name;
  std::vector<std::string> colNames;
  std::vector<std::string> rowNames;

  std::unordered_map<std::string,index_t> colToIndex;
  std::unordered_map<std::string,index_t> rowToIndex;
};

#endif //MIPWORKSHOP2024_SRC_PROBLEM_H
