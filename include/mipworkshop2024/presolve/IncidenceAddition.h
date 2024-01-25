//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_PRESOLVE_INCIDENCEADDITION_H
#define MIPWORKSHOP2024_SRC_PRESOLVE_INCIDENCEADDITION_H

#include <cassert>
#include "mipworkshop2024/MatrixSlice.h"
#include "mipworkshop2024/Submatrix.h"

/// This class contains the methods for an algorithm which tries to detect if a matrix has a submatrix
/// such that it contains at most one +1 and one -1 in every column of the submatrix
class IncidenceAddition
{
private:
	std::vector<index_t> sparseDimNumNonzeros;
	struct SparseDimInfo
	{
		int lastDimRow;
		int lastDimSign;
	};
	std::vector<SparseDimInfo> sparseDimInfo;
	struct UnionFindInfo
	{
		int edgeSign;
		int representative;
	};
	std::vector<UnionFindInfo> unionFind;

	UnionFindInfo findDenseRepresentative(int index);
	int mergeRepresentatives(int first, int second, int sign);

	struct ComponentInfo
	{
		int numUnreflected;
		int numReflected;
	};
	std::vector<ComponentInfo> components; //Buffer used during dense addition
	std::vector<int> componentRepresentatives; //indicates which components are used

	bool transposed;
	std::vector<bool> containsSparse;
	std::vector<bool> containsDense;

	void cleanupComponents();

	template<typename Storage>
	bool tryAddSparse(index_t index, const MatrixSlice<Storage>& slice)
	{
		assert(!containsSparse[index]);
		int entryRepresentatives[2];
		int entryIndex = -1;
		int entrySign = 0;

		int signSum = 0;
		int numEntries = 0;
		for(const Nonzero& nonzero : slice){
			index_t secondaryIndex = nonzero.index();
			if(!containsDense[secondaryIndex]) continue;
			if (fabs(nonzero.value()) != 1.0) return false;
			if(numEntries >= 2){
				return false; //A third entry means we have no way to add it
			}
			UnionFindInfo info = findDenseRepresentative(secondaryIndex);
			entryRepresentatives[numEntries] = info.representative;
			assert(info.representative >= 0);
			assert(info.edgeSign == 1 || info.edgeSign == -1);
			int sign = nonzero.value() > 0.0 ? 1 : -1;
			signSum += info.edgeSign * sign;

			entryIndex = secondaryIndex;
			entrySign = sign;

			++numEntries;
		}
		if(numEntries <= 1){
			assert(sparseDimNumNonzeros[index] == 0);
			sparseDimNumNonzeros[index] = numEntries;
			containsSparse[index] = true;
			if(numEntries != 0){
				sparseDimInfo[index].lastDimRow = entryIndex;
				sparseDimInfo[index].lastDimSign = entrySign;
			}
			return true;
		}
		assert(numEntries == 2);
		if(entryRepresentatives[0] == entryRepresentatives[1]){
			//Column entries belong to the same component; check if the row signs differ correctly
			if(signSum == 0){
				sparseDimNumNonzeros[index] = 2;
				containsSparse[index] = true;
				sparseDimInfo[index].lastDimRow = entryIndex;
				sparseDimInfo[index].lastDimSign = entrySign;
				return true;
			}
			return false;
		}
		assert(entryRepresentatives[0] != entryRepresentatives[1]);
		int sign = signSum == 0 ?  1 : -1;
		mergeRepresentatives(entryRepresentatives[0],entryRepresentatives[1],sign);
		sparseDimNumNonzeros[index] = 2;
		containsSparse[index] = true;
		sparseDimInfo[index].lastDimRow = entryIndex;
		sparseDimInfo[index].lastDimSign = entrySign;

		return true;
	}

	template<typename Storage>
	bool tryAddDense(index_t index, const MatrixSlice<Storage>& slice)
	{
		assert(!containsDense[index]);
		cleanupComponents();
		for (const Nonzero& nonzero : slice)
		{
			//There can be at most two nonzeros in each of the contained submatrices columns
			index_t secondary = nonzero.index();
			if (!containsSparse[secondary]) continue;
			if (fabs(nonzero.value()) != 1.0) return false;
			if (sparseDimNumNonzeros[secondary] >= 2) return false;
			assert((sparseDimNumNonzeros[secondary] == 0) == (sparseDimInfo[secondary].lastDimRow == -1));
			int componentPrimary = sparseDimInfo[secondary].lastDimRow;
			if (componentPrimary != -1)
			{
				UnionFindInfo info = findDenseRepresentative(componentPrimary);
				auto& component = components[info.representative];
				//TODO: test if instead of allocating full size vector here we can simply check vectors size and resize?
				if (component.numUnreflected == 0 && component.numReflected == 0)
				{
					componentRepresentatives.push_back(info.representative);
				}
				int value = sparseDimInfo[secondary].lastDimSign * info.edgeSign;
				assert(value == 1 || value == -1);
				//Row needs to be reflected if the entries are the same sign, unreflected otherwise
				if ((nonzero.value() > 0) == (value > 0))
				{
					++components[info.representative].numReflected;
				}
				else
				{
					++components[info.representative].numUnreflected;
				}
				//TODO: we can already check here if a comoponent has both reflected and unreflected entries.
				// If so, we can terminate early and remove the below loop
			}
		}

		for (int representative : componentRepresentatives)
		{
			assert(components[representative].numUnreflected > 0 || components[representative].numReflected > 0);
			if (components[representative].numUnreflected > 0 && components[representative].numReflected > 0)
			{
				return false;
			}
		}

		int rootMovedSign = 1;
		int thisRepresentative = index;
		for (const auto& representative : componentRepresentatives)
		{
			assert(components[representative].numUnreflected == 0 || components[representative].numReflected == 0);
			int sign = components[representative].numUnreflected == 0 ? -1 : 1;
			//TODO: check if this sign business can somehow be simplified (TODO only when scaling...)
			//Need to keep track of the proper signing when merging representatives
			sign *= rootMovedSign;
			int newRepresentative = mergeRepresentatives(representative, thisRepresentative, sign);
			if (newRepresentative != thisRepresentative)
			{
				rootMovedSign *= unionFind[thisRepresentative].edgeSign;
			}
			thisRepresentative = newRepresentative;
		}

		int thisDenseIndex = index;
		containsDense[index] = true;

		for (const Nonzero& nonzero : slice)
		{
			index_t sparseIndex = nonzero.index();
			if (!containsSparse[sparseIndex]) continue;
			if (sparseDimNumNonzeros[sparseIndex] == 0)
			{ //TODO: can we remove this if branch?
				sparseDimInfo[sparseIndex].lastDimRow = thisDenseIndex;
				sparseDimInfo[sparseIndex].lastDimSign = nonzero.value() > 0.0 ? 1 : -1;
			}
			++sparseDimNumNonzeros[sparseIndex];
		}
		return true;
	}

public:

	IncidenceAddition(index_t numRows, index_t numCols,
			Submatrix::Initialization init = Submatrix::INIT_NONE,
			bool transposed = false);

	template<typename Storage>
	bool tryAddCol(index_t col, const MatrixSlice<Storage>& colSlice)
	{
		if (transposed)
		{
			return tryAddDense(col, colSlice);
		}
		else
		{
			return tryAddSparse(col, colSlice);
		}
	}

	template<typename Storage>
	bool tryAddRow(index_t row, const MatrixSlice<Storage>& rowSlice)
	{
		if (transposed)
		{
			return tryAddSparse(row, rowSlice);
		}
		else
		{
			return tryAddDense(row, rowSlice);
		}
	}

	[[nodiscard]] bool containsColumn(index_t col) const {
		return transposed ? containsDense[col] : containsSparse[col];
	}

	[[nodiscard]] bool containsRow(index_t row) const {
		return transposed ? containsSparse[row] : containsDense[row];
	}
	//Hacky method to remove. Assumes that the given rows/columns form one connected component in the current decomposition
	void removeComponent(const std::vector<index_t>& rows, const std::vector<index_t>& columns);
	[[nodiscard]] Submatrix createSubmatrix() const;

    [[nodiscard]] std::size_t numComponents() const;

};

#endif //MIPWORKSHOP2024_SRC_PRESOLVE_INCIDENCEADDITION_H
