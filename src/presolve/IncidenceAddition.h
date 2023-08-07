//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_PRESOLVE_INCIDENCEADDITION_H
#define MIPWORKSHOP2024_SRC_PRESOLVE_INCIDENCEADDITION_H

#include "MatrixSlice.h"
#include "Submatrix.h"

/// This class contains the methods for an algorithm which tries to detect if
/// a given sparse matrix can be scaled row/column wise,
/// such that it contains at most one +1 and one -1 in every column
class IncidenceAddition {
public:

  IncidenceAddition(index_t numRows, index_t numCols,
                    Submatrix::Initialization init = Submatrix::INIT_NONE,
                    bool transposed = false);

  void clear(Submatrix::Initialization init = Submatrix::INIT_NONE);

private:
  std::vector<index_t> sparseDimNumNonzeros;
  struct SparseDimInfo{
    index_t lastDimRow;
    int lastDimSign;
  };
  std::vector<SparseDimInfo> sparseDimInfo;
  struct UnionFindInfo{
    int edgeSign;
    int representative;
  };
  std::vector<UnionFindInfo> unionFind;

  UnionFindInfo findDenseRepresentative(int index);

  struct ComponentInfo{
    int numUnreflected;
    int numReflected;
  };
  std::vector<ComponentInfo> components; //Temporary buffer used during dense addition

  std::vector<double> colScale;
  std::vector<double> rowScale;

  bool transposed;
  Submatrix incidenceSubmatrix;

  template<typename Storage>
  bool tryAddSparse(index_t index, const MatrixSlice<Storage>& slice){
    int entryRowRepresentative[2];
    int entryRow = -1;
    int entrySign = 0;

    int signSum = 0;
    index_t numEntries = 0;

    auto& picked = transposed ? incidenceSubmatrix.containsColumn : incidenceSubmatrix.containsRow;
    for(const auto& nonzero : slice){
      if(!picked[nonzero.index()]) continue;
      if(numEntries >= 2){
        return false; //If we find a third entry in the sparse dimension,we have no way to deal with it
      }
      int ufIndex = nonzero.index();
      UnionFindInfo entry = findDenseRepresentative(ufIndex);
      signSum += entry.edgeSign * nonzero.value();

      assert()
      ++numEntries;
    }
//    for (index_t i = 0; i < nColumnNonzeros; ++i) {
//      int row = (int) columnRows[i];
//      if(!addition->rowPicked[row]) continue;
//
//      if(numEntries >= 2){
//        return false; //We found a third entry in the column; No way to add it.
//      }
      int rowSign;
      findRowRepresentative(addition,row,&entryRowRepresentative[numEntries],&rowSign);
      assert(entryRowRepresentative[numEntries] >= 0);
      assert(rowSign == 1 || rowSign == -1);
      assert(columnValues[i] == 1 || columnValues[i] == -1);
      signSum += rowSign * columnValues[i];
      entrySign = columnValues[i];
      entryRow = row;

      ++numEntries;
    }
    if(numEntries <= 1){
      addition->columnInfo[column].picked = true;
      addition->columnInfo[column].numEntries = numEntries;
      if(numEntries != 0){
        addition->columnInfo[column].lastEntryRow = entryRow;
        addition->columnInfo[column].lastEntrySign = entrySign;
      }
      return true;
    }
    assert(numEntries == 2);
    if(entryRowRepresentative[0] == entryRowRepresentative[1]){
      //Columns are in the same component. Then, we can only add it if the signs differ correctly
      if(signSum == 0){
        addition->columnInfo[column].picked = true;
        addition->columnInfo[column].lastEntryRow = entryRow;
        addition->columnInfo[column].lastEntrySign = entrySign;
        addition->columnInfo[column].numEntries = 2;
        return true;
      }
      return false;
    }
    assert(entryRowRepresentative[0] != entryRowRepresentative[1]);
    //merge the two components into one, optionally reflecting one if necessary
    //if signSum == 0, we do not need to change the reflection, otherwise we do need to do so
    int sign = signSum == 0 ? 1 : -1;
    mergeRowRepresentatives(addition, entryRowRepresentative[0], entryRowRepresentative[1], sign);
    addition->columnInfo[column].picked = true;
    addition->columnInfo[column].lastEntryRow = entryRow;
    addition->columnInfo[column].lastEntrySign = entrySign;
    addition->columnInfo[column].numEntries = 2;
    return true;
  }

  template<typename Storage>
  bool tryAddDense(index_t index, const MatrixSlice<Storage>& slice);
public:
  template<typename Storage>
  bool tryAddCol(index_t col, const MatrixSlice<Storage>& colSlice){
    if(transposed){
      return tryAddDense(col,colSlice);
    }else{
      return tryAddSparse(col,colSlice);
    }
  }

  template<typename Storage>
  bool tryAddRow(index_t row, const MatrixSlice<Storage>& rowSlice){
    if(transposed){
      return tryAddDense(row,rowSlice);
    }else{
      return tryAddSparse(row,rowSlice);
    }
  }

  double getRowScale(index_t row) const;
  double getColScale(index_t col) const;

  const Submatrix& submatrix() const;

};

class TransposedIncidenceAddition {
public:
  TransposedIncidenceAddition(index_t numRows,
                              index_t numCols,
                              Submatrix::Initialization init = Submatrix::INIT_NONE);

  void clear(Submatrix::Initialization init = Submatrix::INIT_NONE);

  template<typename Storage>
  bool tryAddCol(index_t col, const MatrixSlice<Storage>& colSlice);

  template<typename Storage>
  bool tryAddRow(index_t row, const MatrixSlice<Storage>& rowSlice);

  double getRowScale(index_t row) const;
  double getColScale(index_t col) const;

};
#endif //MIPWORKSHOP2024_SRC_PRESOLVE_INCIDENCEADDITION_H
