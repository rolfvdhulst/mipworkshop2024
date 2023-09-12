//
// Created by rolf on 04-07-22.
//

#ifndef NETWORKDETECTION_SPQRROWADDITION_H
#define NETWORKDETECTION_SPQRROWADDITION_H
#ifdef __cplusplus
extern "C"{
#endif

#include "SPQRDecomposition.h"

/**
 * This class stores all data for performing sequential row-additions to a matrix and checking if it is graphic or not.
 */
typedef struct SPQRNewRowImpl SPQRNewRow;

/**
 * @brief Creates the data structure for managing row-addition for an SPQR decomposition
 */
SPQR_ERROR createNewRow(SPQR* env, SPQRNewRow** pNewRow );
/**
 * @brief Destroys the data structure for managing row-addition for SPQR decomposition
 */
void freeNewRow(SPQR* env, SPQRNewRow ** pNewRow);

/**
 * Checks if adding a row of the given matrix creates a graphic SPQR decomposition.
 * Adding a row which is already in the decomposition is undefined behavior and not checked for.
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure to store information on how to add the new row (if applicable).
 * @param row The index of the row to be added
 * @param columns An array with the column indices of the nonzero entries of the row.
 * @param numColumns The number of nonzero entries of the row
 */
SPQR_ERROR checkNewRow(Decomposition * dec, SPQRNewRow * newRow, row_idx row, const col_idx * columns, size_t numColumns);
/**
 * @brief Adds the most recently checked column from checkNewRow() to the Decomposition.
 * //TODO: specify (and implement) behavior in special cases (e.g. zero rows, rows with a single entry?)
 * In Debug mode, adding a column for which rowAdditionRemainsGraphic() returns false will exit the program.
 * In Release mode, adding a column for which rowAdditionRemainsGraphic() return false is undefined behavior
 * @param dec Current SPQR-decomposition
 * @param newRow Data structure containing information on how to add the new row.
 */
SPQR_ERROR addNewRow(Decomposition *dec, SPQRNewRow *newRow);

/**
 * TODO: specify (and implement) behavior in special cases (e.g. zero rows, rows with a single entry?)
 * @param newRow
 * @return True if the most recently checked row is addable to the SPQR decomposition passed to it, i.e. the submatrix
 * given by both remains graphic.
 */
bool rowAdditionRemainsGraphic(SPQRNewRow *newRow);


#ifdef __cplusplus
}
#endif

#endif //NETWORKDETECTION_SPQRROWADDITION_H
