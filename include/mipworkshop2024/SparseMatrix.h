//
// Created by rolf on 2-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SPARSEMATRIX_H
#define MIPWORKSHOP2024_SRC_SPARSEMATRIX_H

#include "Shared.h"
#include <vector>
enum class SparseMatrixFormat{
  ROW_WISE,
  COLUMN_WISE
};
class SparseMatrix{
public:
  SparseMatrix();

  void setNumSecondary(index_t num);
  index_t addPrimary(const std::vector<index_t>& secondaryEntries,
                     const std::vector<double>& values);

  [[nodiscard]] const index_t * primaryIndices(index_t index) const;
  [[nodiscard]] index_t  primaryNonzeros(index_t index) const;
  [[nodiscard]] const double * primaryValues(index_t index) const;

  SparseMatrixFormat format;
  index_t num_rows;
  index_t num_cols;
  std::vector<index_t> primaryStart;
  std::vector<index_t> secondaryIndex;
  std::vector<double> values;
};


#endif //MIPWORKSHOP2024_SRC_SPARSEMATRIX_H
