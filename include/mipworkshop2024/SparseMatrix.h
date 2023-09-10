//
// Created by rolf on 2-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SPARSEMATRIX_H
#define MIPWORKSHOP2024_SRC_SPARSEMATRIX_H

#include "Shared.h"
#include <vector>
#include "MatrixSlice.h"
#include <cassert>

enum class SparseMatrixFormat{
  ROW_WISE,
  COLUMN_WISE
};
class SparseMatrix{
public:
  SparseMatrix();

  [[nodiscard]] index_t numRows() const{
      return num_rows;
  }
  [[nodiscard]] index_t numCols() const{
      return num_cols;
  }

  void setNumSecondary(index_t numSecondary);

  template<typename StorageType>
  index_t addPrimaryVector(const MatrixSlice<StorageType>& slice){
	  //TODO: check if sorted and if no double coefficients
#ifndef NDEBUG
	  for(const Nonzero& entry : slice){
		  if(format == SparseMatrixFormat::ROW_WISE){
			  assert(entry.index() >= 0 &&  entry.index() < num_cols );
		  }else{
			  assert(entry.index() >= 0 &&  entry.index() < num_rows);
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
	  std::size_t numEntries = 0;
	  for(const Nonzero& entry : slice){
		  secondaryIndex.push_back(entry.index());
		  values.push_back(entry.value());
		  ++numEntries;
	  }
	  primaryStart.push_back(primaryStart.back() + numEntries);
	  return primary;
  }
  index_t addPrimaryVector(const std::vector<index_t>& secondaryEntries,
                     const std::vector<double>& values);

  [[nodiscard]] MatrixSlice<CompressedSlice> getPrimaryVector(index_t index) const;

  [[nodiscard]] SparseMatrix transposedFormat() const;

  [[nodiscard]] index_t numSecondarySliceEntries(index_t primary) const{
	  return primaryStart[primary+1] - primaryStart[primary];
  }

private:
  SparseMatrixFormat format;
  index_t num_rows;
  index_t num_cols;
  std::vector<index_t> primaryStart;
  std::vector<index_t> secondaryIndex;
  std::vector<double> values;
};


#endif //MIPWORKSHOP2024_SRC_SPARSEMATRIX_H
