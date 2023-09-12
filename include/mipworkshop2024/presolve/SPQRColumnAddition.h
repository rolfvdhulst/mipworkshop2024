//
// Created by rolf on 5-6-23.
//

#ifndef NETWORKDETECTION_SPQRCOLUMNADDITION_H
#define NETWORKDETECTION_SPQRCOLUMNADDITION_H
#ifdef __cplusplus
extern "C"{
#endif
#include "SPQRDecomposition.h"

/**
 * This class stores all data for performing sequential column additions to a matrix and checking if it is graphic or not.
 */
typedef struct SPQRNewColumnImpl SPQRNewColumn;

/**
 * @brief Creates the data structure for managing column-addition for an SPQR decomposition
 */
SPQR_ERROR createNewColumn(SPQR* env, SPQRNewColumn** pNewCol );
/**
 * @brief Destroys the data structure for managing column-addition for SPQR decomposition
 */
void freeNewColumn(SPQR* env, SPQRNewColumn ** pNewCol);

/**
 * Checks if adding a column of the given matrix creates a graphic SPQR decomposition.
 * Adding a column which is already in the decomposition is undefined behavior and not checked for.
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure to store information on how to add the new column (if applicable).
 * @param column The index of the column to be added
 * @param rows An array with the row indices of the nonzero entries of the column.
 * @param numRows The number of nonzero entries of the column
 */
SPQR_ERROR checkNewColumn(Decomposition * dec, SPQRNewColumn * newCol, col_idx column, const row_idx * rows, size_t numRows);
/**
 * @brief Adds the most recently checked column from checkNewRow() to the Decomposition.
 * //TODO: specify (and implement) behavior in special cases (e.g. zero columns, columns with a single entry)
 * In Debug mode, adding a column for which columnAdditionRemainsGraphic() returns false will exit the program.
 * In Release mode, adding a column for which columnAdditionRemainsGraphic() return false is undefined behavior
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure containing information on how to add the new column.
 */
SPQR_ERROR addNewColumn(Decomposition *dec, SPQRNewColumn *newCol);

/**
 * TODO: specify (and implement) behavior in special cases (e.g. zero columns, columns with a single entry)
 * @param newColumn
 * @return True if the most recently checked column is addable to the SPQR decomposition passed to it, i.e. the submatrix
 * given by both remains graphic.
 */
bool columnAdditionRemainsGraphic(SPQRNewColumn *newCol);

#ifdef __cplusplus
}
#endif

#endif //NETWORKDETECTION_SPQRCOLUMNADDITION_H
