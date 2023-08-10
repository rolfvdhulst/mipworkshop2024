//
// Created by rolf on 2-8-23.
//

#include "mipworkshop2024/SparseMatrix.h"

SparseMatrix::SparseMatrix() : num_cols{0},
num_rows{0},primaryStart{0}, format{SparseMatrixFormat::COLUMN_WISE}{

}

index_t SparseMatrix::addPrimary(const std::vector<index_t>& entrySecondary,
                                 const std::vector<double>& entryValues) {
  //TODO: check if move vs copy makes a difference in performance here

  //TODO: check if sorted and if no double coefficients
#ifndef NDEBUG
  for(const auto& entry : entrySecondary){
    if(format == SparseMatrixFormat::ROW_WISE){
      assert(entry >= 0 &&  entry < num_cols );
    }else{
      assert(entry >= 0 &&  entry < num_rows);
    }
  }
#endif
  assert(!primaryStart.empty());
  index_t primary = format == SparseMatrixFormat::ROW_WISE ? num_rows : num_cols;
  if(format == SparseMatrixFormat::ROW_WISE){
    ++num_rows;
  }else{
    assert(format == SparseMatrixFormat::COLUMN_WISE);
    ++num_cols;
  }
  primaryStart.push_back(primaryStart.back() + entrySecondary.size());
  secondaryIndex.insert(secondaryIndex.end(),entrySecondary.begin(),entrySecondary.end());
  values.insert(values.end(),entryValues.begin(),entryValues.end());
  return primary;
}
void SparseMatrix::setNumSecondary(index_t num) {
  if(format == SparseMatrixFormat::ROW_WISE){
    num_cols = num;
  }else{
    assert(format == SparseMatrixFormat::COLUMN_WISE);
    num_rows = num;
  }

}
const index_t *SparseMatrix::primaryIndices(index_t index) const {
  return secondaryIndex.data() + primaryStart[index];
}
index_t SparseMatrix::primaryNonzeros(index_t index) const {
  return primaryStart[index+1] - primaryStart[index];
}
const double *SparseMatrix::primaryValues(index_t index) const {
  return values.data() + primaryStart[index];
}
