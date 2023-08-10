//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_PRESOLVE_PRESOLVINGPROBLEM_H
#define MIPWORKSHOP2024_SRC_PRESOLVE_PRESOLVINGPROBLEM_H

#include <vector>
#include "mipworkshop2024/MatrixSlice.h"
#include "mipworkshop2024/Problem.h"

///A presentation of the problem which is easy to modify
///This class contains some 'extra' data which makes it faster to do certain preprocessing steps
class PresolvingProblem {
public:
  explicit PresolvingProblem(const Problem& problem);

  [[nodiscard]] Problem toProblem() const;
  [[nodiscard]] MatrixSlice<ListSlice> getColumnVector(index_t col) const;
  [[nodiscard]] MatrixSlice<ListSlice> getRowVector(index_t row) const;

  /// Note that this method relies on the user first setting matValue, matRow and matPos correctly,
  /// and that the user ensures that the column and row specified do not yet have a nonzero
  void addNonzero(index_t pos);

  void removeNonzero(index_t pos);

  std::vector<double> matValue;
  std::vector<index_t> matRow;
  std::vector<index_t> matColumn;

  //TODO: implied row bounds
  std::vector<index_t> freeSlots;


  void addNonzeroCol(index_t pos);
  void removeNonzeroCol(index_t pos);
  std::vector<index_t> colHead;
  std::vector<index_t> colNext;
  std::vector<index_t> colPrev;

  void addNonzeroRow(index_t pos);
  void removeNonzeroRow(index_t pos);
  std::vector<index_t> rowHead;
  std::vector<index_t> rowNext;
  std::vector<index_t> rowPrev;

  std::vector<double> rowLhs;
  std::vector<double> rowRhs;

  std::vector<double> colObj;
  std::vector<double> colLb;
  std::vector<double> colUb;
  std::vector<VariableType> colType;

  double objectiveOffset;

  std::vector<bool> rowDeleted; //TODO: fix to a nicer implementation
  std::vector<bool> colDeleted;

  std::size_t numRows;
  std::size_t numColumns;

  //Extra data which is non-crucial but helpful for many of the
};

#endif //MIPWORKSHOP2024_SRC_PRESOLVE_PRESOLVINGPROBLEM_H
