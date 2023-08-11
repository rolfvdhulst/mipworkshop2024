//
// Created by rolf on 2-8-23.
//

#include "mipworkshop2024/SparseMatrix.h"

SparseMatrix::SparseMatrix() : num_cols{0},
num_rows{0},primaryStart{0}, format{SparseMatrixFormat::COLUMN_WISE}{

}

index_t SparseMatrix::addPrimaryVector(const std::vector<index_t>& entrySecondary,
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

MatrixSlice<CompressedSlice> SparseMatrix::getPrimaryVector(index_t index) const {
    index_t start = primaryStart[index];
    index_t end = primaryStart[index+1];
    return {secondaryIndex.data() + start,values.data() + start, end-start};
}

SparseMatrix SparseMatrix::transposedFormat() const {
    SparseMatrix transposed;
    transposed.format = format  == SparseMatrixFormat::ROW_WISE ? SparseMatrixFormat::COLUMN_WISE : SparseMatrixFormat::ROW_WISE;
    transposed.num_rows = num_rows;
    transposed.num_cols = num_cols;

    //First, count the number of nonzeros in each row
    index_t numPrimary = format == SparseMatrixFormat::ROW_WISE ? num_rows : num_cols;
    index_t numSecondary = format == SparseMatrixFormat::ROW_WISE ? num_cols : num_rows;
    transposed.primaryStart.assign(numSecondary+1,0);

    index_t numNonzeros = values.size();
    for(index_t i = 0; i < numNonzeros; ++i){
        ++transposed.primaryStart[secondaryIndex[i]+1];
    }
    for(index_t i = 1; i < numSecondary + 1; ++i){
        transposed.primaryStart[i] += transposed.primaryStart[i-1];
    }
    transposed.values.assign(numNonzeros,0.0);
    transposed.secondaryIndex.assign(values.size(),INVALID);

    for(index_t i = 0; i < numPrimary; ++i){
        index_t begin = primaryStart[i];
        index_t end = primaryStart[i+1];
        for(index_t pos = begin; pos < end; ++pos){
            index_t secondary = secondaryIndex[pos];
            double value = values[pos];
            index_t newIndex = transposed.primaryStart[secondary];
            transposed.secondaryIndex[newIndex] = i;
            transposed.values[newIndex] = value;
            ++transposed.primaryStart[secondary];
        }
    }
    for(index_t i = numSecondary; i > 0; --i){
        transposed.primaryStart[i] = transposed.primaryStart[i-1];
    }
    transposed.primaryStart[0] = 0;

    return transposed;
}
