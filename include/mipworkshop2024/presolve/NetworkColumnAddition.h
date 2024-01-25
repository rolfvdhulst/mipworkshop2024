//
// Created by rolf on 23-11-23.
//

#ifndef MIPWORKSHOP2024_NETWORKCOLUMNADDITION_H
#define MIPWORKSHOP2024_NETWORKCOLUMNADDITION_H

#ifdef __cplusplus
extern "C"{
#endif
#include "NetworkDecomposition.h"

/**
 * This class stores all data for performing sequential column additions to a network decomposition,
 * determining whether the addition of the new column preserves if it is network or not
 */
typedef struct NetworkColumnAdditionImpl NetworkColumnAddition;

/**
 * @brief Creates the data structure for the network column addition algorithm
 */
SPQR_ERROR createNetworkColumnAddition(SPQR* env, NetworkColumnAddition ** pNetworkColumnAddition );
/**
 * @brief frees the data structure for the network column addition algorithm
 */
void freeNetworkColumnAddition(SPQR* env, NetworkColumnAddition ** pNetworkColumnAddition);

/**
 * @brief Checks if adding a column of the given matrix maintains maintains the network property.
 * Adding a column which is already in the decomposition is undefined behavior and unchecked.
 * @param dec Current network decomposition
 * @param networkColumnAddition Data structure to store information on how to add the new column (if applicable).
 * @param column The index of the column to be added
 * @param rows An array with the row indices of the nonzero entries of the column.
 * @param signs An array with the signs (+-1) of the nonzero entries
 * @param numRows The number of nonzero entries of the column
 */
SPQR_ERROR networkColumnAdditionCheck(Decomposition * dec,
                                      NetworkColumnAddition * networkColumnAddition,
                                      col_idx column,
                                      const row_idx * nonzeroRows,
                                      const double * nonzeroValues, //TODO: think about whether using this or int or bool is a more convenient interface..
                                      size_t numRows);
/**
 * @brief Adds the most recently checked column to the Network Decomposition.
 * In Debug mode, adding a column for which networkColumnAdditionRemainsNetwork() returns false will exit the program.
 * In Release mode, adding a column for which networkColumnAdditionRemainsNetwork() return false is undefined behavior
 * @param dec Current SPQR-decomposition
 * @param networkColumnAddition Data structure containing information on how to add the new column.
 */
SPQR_ERROR networkColumnAdditionAdd(Decomposition *dec, NetworkColumnAddition *networkColumnAddition);

/**
 * @param newColumn
 * @return True if the most recently checked column is addable to the network decomposition to it, so that the
 * submatrix given by the submatrix of the current decomposition augmented with the last checked row remains network
 */
bool networkColumnAdditionRemainsNetwork(NetworkColumnAddition *newCol);

#ifdef __cplusplus
}
#endif

#endif //MIPWORKSHOP2024_NETWORKCOLUMNADDITION_H
