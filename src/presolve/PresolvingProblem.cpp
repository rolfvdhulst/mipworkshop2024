//
// Created by rolf on 6-8-23.
//

#include "PresolvingProblem.h"
#include <cassert>
MatrixSlice<ListSlice> PresolvingProblem::getColumnVector(index_t col) const {
  return MatrixSlice<ListSlice>(matRow.data(),matValue.data(),colNext.data(),colHead[col]);
}
MatrixSlice<ListSlice> PresolvingProblem::getRowVector(index_t row) const {
  return MatrixSlice<ListSlice>(matColumn.data(),matValue.data(),rowNext.data(),rowHead[row]);
}

void PresolvingProblem::addNonzero(const index_t pos) {
  assert(matValue[pos] != 0.0);
  addNonzeroCol(pos);
  addNonzeroRow(pos);
}

void PresolvingProblem::removeNonzero(const index_t pos) {
  removeNonzeroCol(pos);
  removeNonzeroRow(pos);

  matValue[pos] = 0.0;
  freeSlots.push_back(pos);
}
void PresolvingProblem::addNonzeroCol(const index_t pos) {
  index_t column = matColumn[pos];
  colNext[pos] = colHead[column];
  colPrev[pos] = INVALID;
  colHead[column] = pos;
  if(colNext[pos] != INVALID) {
    colPrev[colNext[pos]] = pos;
  }
}
void PresolvingProblem::removeNonzeroCol(const index_t pos) {
  //Update the column linked list
  index_t next = colNext[pos];
  index_t prev = colPrev[pos];
  if(next != INVALID){
    colPrev[next] = prev;
  }
  if(prev != INVALID){
    colNext[prev] = next;
  }else{
    colHead[matColumn[pos]] = next;
  }
}
void PresolvingProblem::addNonzeroRow(const index_t pos) {
  index_t row = matRow[pos];
  rowNext[pos] = rowHead[row];
  rowPrev[pos] = INVALID;
  rowHead[row] = pos;
  if(rowNext[pos] != INVALID) {
    rowPrev[rowNext[pos]] = pos;
  }
}
void PresolvingProblem::removeNonzeroRow(const index_t pos) {
  //Update the row linked list
  index_t next = rowNext[pos];
  index_t prev = rowPrev[pos];
  if(next != INVALID){
    rowPrev[next] = prev;
  }
  if(prev != INVALID){
    rowNext[prev] = next;
  }else{
    rowHead[matRow[pos]] = next;
  }
}

PresolvingProblem::PresolvingProblem(const Problem &problem) :
rowLhs{problem.lhs},
rowRhs{problem.rhs},
colObj{problem.obj},
colLb{problem.lb},
colUb{problem.ub},
colType{problem.colType},
objectiveOffset{problem.objectiveOffset},
rowDeleted(problem.numRows,false),
colDeleted(problem.numCols,false),
matValue{problem.matrix.values},
matRow{problem.matrix.secondaryIndex}{
  assert(problem.matrix.format == SparseMatrixFormat::COLUMN_WISE); //TODO: fix CSR too?

  index_t numCols = problem.numCols;
  index_t numNonzeros = problem.matrix.values.size();
  matColumn.reserve(numNonzeros); //TODO; resize maybe?
  for(index_t i = 0 ; i < numCols; ++i ){
    index_t colLen = problem.matrix.primaryStart[i+1] - problem.matrix.primaryStart[i];
    matColumn.insert(matColumn.end(),colLen,i);
  }
  for(index_t i = 0; i < numNonzeros; ++i){
    addNonzero(i);
  }

  assert(freeSlots.empty());
}
