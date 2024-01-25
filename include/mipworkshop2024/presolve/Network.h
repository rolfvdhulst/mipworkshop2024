//
// Created by rolf on 27-11-23.
//

#ifndef NETWORKDETECTION_NETWORK_H
#define NETWORKDETECTION_NETWORK_H

#ifdef __cplusplus
extern "C"{
#endif

#include "SPQRShared.h"

/**
 * This class stores the SPQR decomposition
 */
typedef struct SPQRNetworkDecompositionImpl SPQRNetworkDecomposition;

SPQR_ERROR SPQRNetworkDecompositionCreate(SPQR * env, SPQRNetworkDecomposition **pDecomposition, int numRows, int numColumns);

void SPQRNetworkDecompositionFree(SPQRNetworkDecomposition **pDecomposition);

/**
 * Returns if the SPQR decomposition contains the given row
 */
bool SPQRNetworkDecompositionContainsRow(const SPQRNetworkDecomposition * decomposition, spqr_row row);

/**
 * Returns if the SPQR decomposition contains the given column
 */
bool SPQRNetworkDecompositionContainsColumn(const SPQRNetworkDecomposition *decomposition, spqr_col column);

/**
 * Checks if the SPQR decomposition of the graph is minimal
 */
bool SPQRNetworkDecompositionIsMinimal(const SPQRNetworkDecomposition * decomposition);

//TODO: fix to actually work correctly
void SPQRNetworkDecompositionRemoveComponents(SPQRNetworkDecomposition *dec, const spqr_row * componentRows,
                                             size_t numRows, const spqr_col  * componentCols, size_t numCols);

typedef struct {
    size_t numComponents; //number of SPQR trees
    size_t numSkeletonsTypeS;
    size_t numSkeletonsTypeP;
    size_t numSkeletonsTypeQ;
    size_t numSkeletonsTypeR;
    size_t numArcsLargestR;
    size_t numArcsTotalR;
} SPQRNetworkDecompositionStatistics;
SPQRNetworkDecompositionStatistics SPQRNetworkDecompositionGetStatistics(SPQRNetworkDecomposition *dec);
//TODO: method to convert decomposition into a realization

/**
 * A method to check if the cycle stored in the SPQR cycle matches the given array. Mostly useful in testing.
 */
bool SPQRNetworkDecompositionVerifyCycle(const SPQRNetworkDecomposition * dec, spqr_col column, spqr_row * column_rows,
                                         double * column_values, int num_rows,
                                         spqr_row * computed_column_storage,
                                         bool * computedSignStorage);

/**
 * This class stores all data for performing sequential column additions to a matrix and checking if it is network or not.
 */
typedef struct SPQRNetworkColumnAdditionImpl SPQRNetworkColumnAddition;

/**
 * @brief Creates the data structure for managing column-addition for an SPQR decomposition
 */
SPQR_ERROR SPQRcreateNetworkColumnAddition(SPQR* env, SPQRNetworkColumnAddition** pNewCol );
/**
 * @brief Destroys the data structure for managing column-addition for SPQR decomposition
 */
void SPQRfreeNetworkColumnAddition(SPQR* env, SPQRNetworkColumnAddition ** pNewCol);

/**
 * Checks if adding a column of the given matrix creates a network SPQR decomposition.
 * Adding a column which is already in the decomposition is undefined behavior and not checked for.
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure to store information on how to add the new column (if applicable).
 * @param column The index of the column to be added
 * @param rows An array with the row indices of the nonzero entries of the column.
 * @param numRows The number of nonzero entries of the column
 */
SPQR_ERROR SPQRNetworkColumnAdditionCheck(SPQRNetworkDecomposition * dec, SPQRNetworkColumnAddition * newCol, spqr_col column,
                                          const spqr_row * nonzeroRows, const double * nonzeroValues, size_t numNonzeros);
/**
 * @brief Adds the most recently checked column from checkNewRow() to the Decomposition.
 * //TODO: specify (and implement) behavior in special cases (e.g. zero columns, columns with a single entry)
 * In Debug mode, adding a column for which SPQRNetworkColumnAdditionRemainsNetwork() returns false will exit the program.
 * In Release mode, adding a column for which SPQRNetworkColumnAdditionRemainsNetwork() return false is undefined behavior
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure containing information on how to add the new column.
 */
SPQR_ERROR SPQRNetworkColumnAdditionAdd(SPQRNetworkDecomposition *dec, SPQRNetworkColumnAddition *newCol);

/**
 * TODO: specify (and implement) behavior in special cases (e.g. zero columns, columns with a single entry)
 * @param newColumn
 * @return True if the most recently checked column is addable to the SPQR decomposition passed to it, i.e. the submatrix
 * given by both remains network.
 */
bool SPQRNetworkColumnAdditionRemainsNetwork(SPQRNetworkColumnAddition *newCol);


/**
 * This class stores all data for performing sequential row-additions to a matrix and checking if it is network or not.
 */
typedef struct SPQRNetworkRowAdditionImpl SPQRNetworkRowAddition;

/**
 * @brief Creates the data structure for managing row-addition for an SPQR decomposition
 */
SPQR_ERROR SPQRcreateNetworkRowAddition(SPQR* env, SPQRNetworkRowAddition** pNewRow );
/**
 * @brief Destroys the data structure for managing row-addition for SPQR decomposition
 */
void SPQRfreeNetworkRowAddition(SPQR* env, SPQRNetworkRowAddition ** pNewRow);

/**
 * Checks if adding a row of the given matrix creates a network SPQR decomposition.
 * Adding a row which is already in the decomposition is undefined behavior and not checked for.
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure to store information on how to add the new row (if applicable).
 * @param row The index of the row to be added
 * @param columns An array with the column indices of the nonzero entries of the row.
 * @param numColumns The number of nonzero entries of the row
 */
SPQR_ERROR SPQRNetworkRowAdditionCheck(SPQRNetworkDecomposition * dec, SPQRNetworkRowAddition * newRow, spqr_row row,
                                       const spqr_col * nonzeroCols, const double * nonzeroValues, size_t numNonzeros);
/**
 * @brief Adds the most recently checked column from checkNewRow() to the Decomposition.
 * //TODO: specify (and implement) behavior in special cases (e.g. zero rows, rows with a single entry?)
 * In Debug mode, adding a column for which rowAdditionRemainsNetwork() returns false will exit the program.
 * In Release mode, adding a column for which rowAdditionRemainsNetwork() return false is undefined behavior
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure containing information on how to add the new row.
 */
SPQR_ERROR SPQRNetworkRowAdditionAdd(SPQRNetworkDecomposition *dec, SPQRNetworkRowAddition *newRow);

/**
 * TODO: specify (and implement) behavior in special cases (e.g. zero rows, rows with a single entry?)
 * @param newRow
 * @return True if the most recently checked row is addable to the SPQR decomposition passed to it, i.e. the submatrix
 * given by both remains network.
 */
bool SPQRNetworkRowAdditionRemainsNetwork(const SPQRNetworkRowAddition *newRow);


#ifdef __cplusplus
}
#endif


#endif //NETWORKDETECTION_NETWORK_H
