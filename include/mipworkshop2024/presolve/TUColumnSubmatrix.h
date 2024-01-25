//
// Created by rolf on 10-8-23.
//

#ifndef MIPWORKSHOP2024_TUCOLUMNSUBMATRIX_H
#define MIPWORKSHOP2024_TUCOLUMNSUBMATRIX_H

#include "mipworkshop2024/Problem.h"
#include "mipworkshop2024/Submatrix.h"
#include "mipworkshop2024/presolve/PostSolveStack.h"
#include "mipworkshop2024/Logging.h"

struct TUSettings{
    bool doDowngrade; //downgrade binary/integer variables to implied integers?
    VariableType writeType; //What type to write the implied integers as?
    bool dynamic; //Dynamically decide if we should up/downgrade to
};

enum class TUColumnType{
	INTEGRAL_FIXED,
	INTEGRAL_EITHER,
	CONTINUOUS_REQUIRED,
	CONTINUOUS_DISCONNECTED,
};
class TUColumnSubmatrixFinder
{
public:
	explicit TUColumnSubmatrixFinder(Problem& problem,const TUSettings& settings);
	std::vector<TotallyUnimodularColumnSubmatrix> computeTUSubmatrices();
    [[nodiscard]] std::vector<DetectionStatistics> statistics() const;
private:
    TUSettings settings;
	Problem& problem;
	SparseMatrix rowMatrix;

	index_t numContinuousRequired;
	index_t numContinuousDisconnected;
	index_t numIntegralEither;
	index_t numIntegralFixed;
	std::vector<TUColumnType> types;

	std::vector<bool> isNonIntegralRow;
	std::vector<index_t> nonIntegralRows;

    std::vector<DetectionStatistics> detectionStatistics;

	void computeRowAndColumnTypes();
	[[nodiscard]] TotallyUnimodularColumnSubmatrix computeImplyingColumns(const Submatrix& submatrix) const;
	std::vector<TotallyUnimodularColumnSubmatrix> mixedComputeTUSubmatrices();
	std::vector<TotallyUnimodularColumnSubmatrix> integralComputeTUSubmatrices();
	struct Component{
		std::vector<index_t> rows;
		std::vector<index_t> cols;
	};
	[[nodiscard]] Submatrix computeIncidenceSubmatrix(bool transposed,
			const std::vector<Component>& components,
			const std::vector<bool>& componentValid,
			const std::vector<long>& nRowEntries,
			const std::vector<long>& nColEntries,
			const std::vector<long>& rowComponents);
    [[nodiscard]] Submatrix computeNetworkSubmatrix(
            bool transposed,
            const std::vector<Component>& components,
            const std::vector<bool>& componentValid,
            const std::vector<long>& rowComponents);

	[[nodiscard]] MatrixSlice<CompressedSlice> getRowVector(index_t index) const;
	[[nodiscard]] MatrixSlice<CompressedSlice> getColumnVector(index_t index) const;

};

#endif //MIPWORKSHOP2024_TUCOLUMNSUBMATRIX_H
