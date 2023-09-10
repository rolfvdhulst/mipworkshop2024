//
// Created by rolf on 6-8-23.
//

#include "mipworkshop2024/presolve/IncidenceAddition.h"
IncidenceAddition::IncidenceAddition(index_t numRows,
		index_t numCols,
		Submatrix::Initialization init,
		bool runTransposed)
		:
		transposed{runTransposed}
{
	index_t sparseSize = transposed ? numRows : numCols;
	index_t denseSize = transposed ? numCols : numRows;
	sparseDimNumNonzeros.assign(sparseSize,0);
	sparseDimInfo.assign(sparseSize,SparseDimInfo{.lastDimRow = -1, .lastDimSign = 0});

	unionFind.assign(denseSize,UnionFindInfo{.edgeSign = 1, .representative = -1});

	bool startsSparse = false;
	bool startsDense = false;
	if(init != Submatrix::INIT_NONE){
		startsDense = (init == Submatrix::INIT_ROWS && !transposed) || (init == Submatrix::INIT_COLS && transposed);
		startsSparse = !startsDense;
	}
	if(!startsDense){
		components.assign(denseSize,ComponentInfo{.numUnreflected =0, .numReflected = 0});
	}
	componentRepresentatives.clear();

	containsSparse.assign(sparseSize,startsSparse);
	containsDense.assign(denseSize,startsDense);

}

void IncidenceAddition::cleanupComponents()
{

	for (const auto& representative : componentRepresentatives)
	{
		components[representative].numUnreflected = 0;
		components[representative].numReflected = 0;
	}
	componentRepresentatives.clear();
#ifndef NDEBUG
	for (const auto& component : components)
	{
		assert(component.numUnreflected == 0 && component.numReflected == 0);
	}
#endif
}

bool isNegative(int index)
{
	return index < 0;
}
IncidenceAddition::UnionFindInfo IncidenceAddition::findDenseRepresentative(int index)
{
	int current = index;
	int next;

	int totalSign = 1;
	while (!isNegative(next = unionFind[current].representative))
	{
		totalSign *= unionFind[current].edgeSign;
		current = next;
	}

	int root = current;
	current = index;

	int currentSign = totalSign;
	while (!isNegative(next = unionFind[current].representative))
	{
		unionFind[current].representative = root;
		int previousSign = unionFind[current].edgeSign;
		unionFind[current].edgeSign = currentSign;
		currentSign *= previousSign;
		current = next;
	}
	return UnionFindInfo{ .edgeSign=totalSign, .representative = root, };
}
Submatrix IncidenceAddition::createSubmatrix() const
{
	if(transposed){
		return {containsSparse,containsDense};
	}
	return {containsDense,containsSparse};

}
int IncidenceAddition::mergeRepresentatives(int first, int second, int sign)
{
	assert(first != second);
	int firstRank = unionFind[first].representative;
	int secondRank = unionFind[second].representative;
	assert(firstRank < 0 && secondRank < 0);
	if (firstRank > secondRank)
	{
		std::swap(first, second);
	}
	assert(unionFind[first].edgeSign == 1 && unionFind[second].edgeSign == 1);
	unionFind[second].representative = first;
	unionFind[second].edgeSign = sign;
	if (firstRank == secondRank)
	{
		--unionFind[first].representative;
	}
	return first;
}
void IncidenceAddition::removeComponent(const std::vector<index_t>& rows, const std::vector<index_t>& columns)
{
	const auto& sparseEntries = transposed ? rows : columns;
	const auto& denseEntries = transposed ? columns : rows;
	for (index_t entry : sparseEntries)
	{
		if (!containsSparse[entry]) continue;

		sparseDimInfo[entry].lastDimRow = -1;
		sparseDimInfo[entry].lastDimSign = 0;
		sparseDimNumNonzeros[entry] = 0;
		containsSparse[entry] = false;
	}

	for(index_t entry : denseEntries){
		unionFind[entry] = UnionFindInfo{.edgeSign = 1, .representative = -1};
		containsDense[entry] = false;
	}
}

