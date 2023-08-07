//
// Created by rolf on 6-8-23.
//

#include <numeric>
#include "Submatrix.h"
void Submatrix::addRow(index_t row) {
  if(!containsRow[row]){
    rows.push_back(row);
    containsRow[row] = true;
  }
}
void Submatrix::addColumn(index_t col) {
  if(!containsColumn[col]){
    columns.push_back(col);
    containsColumn[col] = true;
  }

}
Submatrix::Submatrix(index_t numOriginalRows, index_t numOriginalCols, Submatrix::Initialization init) :
containsRow(numOriginalRows, init == INIT_ROWS),
containsColumn(numOriginalCols,init == INIT_COLS){
  if(init == INIT_ROWS){
    rows.resize(numOriginalRows);
    std::iota(rows.begin(),rows.end(),0);
  }else if(init == INIT_COLS){
    columns.resize(numOriginalRows);
    std::iota(columns.begin(),columns.end(),0);
  }
}
