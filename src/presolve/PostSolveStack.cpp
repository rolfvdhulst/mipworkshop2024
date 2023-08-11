//
// Created by rolf on 6-8-23.
//

#include "mipworkshop2024/presolve/PostSolveStack.h"
void PostSolveStack::totallyUnimodularColumnSubmatrix(
		const std::vector<index_t>& tu_columns,
		const std::vector<index_t>& tu_rows,
		const std::vector<index_t>& fixed_columns)
{
	reductions.emplace_back(TotallyUnimodularColumnSubmatrix{
			.submatRows = tu_rows,
			.implyingColumns = fixed_columns,
			.submatColumns = tu_columns,
	});
    containsTUSubmatrix = true;
}

bool PostSolveStack::totallyUnimodularColumnSubmatrixFound() const {
    return containsTUSubmatrix;
}
