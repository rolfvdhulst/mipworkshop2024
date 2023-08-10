//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_SUBMATRIX_H
#define MIPWORKSHOP2024_SRC_SUBMATRIX_H
#include "Shared.h"
#include <vector>

class Submatrix
{
public:
	enum Initialization
	{
		INIT_NONE,
		INIT_ROWS,
		INIT_COLS
	};

	Submatrix(std::vector<bool> containsRow, std::vector<bool> containsColumn);
	Submatrix() = default;
	Submatrix(index_t numOriginalRows,
			index_t numOriginalCols,
			Initialization init = INIT_NONE);

	void addRow(index_t row);
	void addColumn(index_t col);

	std::vector<bool> containsRow;
	std::vector<bool> containsColumn;
	std::vector<index_t> rows;
	std::vector<index_t> columns;
};

#endif //MIPWORKSHOP2024_SRC_SUBMATRIX_H
