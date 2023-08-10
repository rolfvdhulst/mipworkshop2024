//
// Created by rolf on 6-8-23.
//

#include <numeric>
#include <utility>
#include "mipworkshop2024/Submatrix.h"
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
Submatrix::Submatrix(std::vector<bool> contains_row, std::vector<bool> contains_col) : containsRow{std::move(contains_row)},
containsColumn{std::move(contains_col)}
{
	for (std::size_t i = 0; i < containsRow.size(); ++i)
	{
		if(containsRow[i]){
			rows.push_back(i);
		}
	}
	for (std::size_t i = 0; i < containsColumn.size(); ++i)
	{
		if(containsColumn[i]){
			columns.push_back(i);
		}
	}
}
