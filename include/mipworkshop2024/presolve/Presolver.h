//
// Created by rolf on 10-8-23.
//

#ifndef MIPWORKSHOP2024_PRESOLVER_H
#define MIPWORKSHOP2024_PRESOLVER_H

#include "PostSolveStack.h"
#include "mipworkshop2024/Problem.h"

class Presolver
{
public:
	Presolver() = default;
	void doPresolve(const Problem& problem);
	[[nodiscard]] const Problem& presolvedProblem() const;
	[[nodiscard]] const PostSolveStack& postSolveStack() const;
private:
    void findTUColumnSubmatrix();
	Problem problem;
	PostSolveStack stack;
};

#endif //MIPWORKSHOP2024_PRESOLVER_H
