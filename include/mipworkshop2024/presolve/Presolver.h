//
// Created by rolf on 10-8-23.
//

#ifndef MIPWORKSHOP2024_PRESOLVER_H
#define MIPWORKSHOP2024_PRESOLVER_H

#include "PresolvingProblem.h"
#include "PostSolveStack.h"
class Presolver
{
public:
	Presolver(const Problem& problem);
	void doPresolve();
	[[nodiscard]] Problem presolvedProblem() const;
	[[nodiscard]] const PostSolveStack& postSolveStack() const;
private:

	void findTUColumnSubmatrix();
	PresolvingProblem problem;
	PostSolveStack stack;
};

#endif //MIPWORKSHOP2024_PRESOLVER_H
