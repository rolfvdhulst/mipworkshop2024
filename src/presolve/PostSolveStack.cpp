//
// Created by rolf on 6-8-23.
//

#include "mipworkshop2024/presolve/PostSolveStack.h"
void PostSolveStack::totallyUnimodularColumnSubmatrix(const TotallyUnimodularColumnSubmatrix& submatrix)
{
	reductions.emplace_back(submatrix);
    containsTUSubmatrix = true;
}

bool PostSolveStack::totallyUnimodularColumnSubmatrixFound() const {
    return containsTUSubmatrix;
}
