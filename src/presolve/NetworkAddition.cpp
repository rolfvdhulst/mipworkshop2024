//
// Created by rolf on 12-9-23.
//

#include "mipworkshop2024/presolve/NetworkAddition.hpp"


GraphicAddition::GraphicAddition(index_t numRows, index_t numCols, Submatrix::Initialization init, bool transposed) {
    SPQR_CALL_THROW(spqrCreateEnvironment(&env));
    index_t numDecRows = transposed ? numCols : numRows;
    index_t numDecCols = transposed ? numRows : numCols;
    SPQR_CALL_THROW(createDecomposition(env,&dec,numDecRows,numDecCols));
    SPQR_CALL_THROW(createNewRow(env,&rowAdd));
    SPQR_CALL_THROW(createNewColumn(env,&colAdd));

}

GraphicAddition::~GraphicAddition() {
    freeNewColumn(env,&colAdd);
    freeNewRow(env,&rowAdd);
    freeDecomposition(&dec);
    SPQR_CALL_THROW(spqrFreeEnvironment(&env));

}

bool GraphicAddition::containsColumn(index_t col) const {
    return transposed ? decompositionHasRow(dec,col) : decompositionHasCol(dec,col);
}
bool GraphicAddition::containsRow(index_t row) const {
    return transposed ? decompositionHasCol(dec,row) : decompositionHasRow(dec,row);
}

void GraphicAddition::removeComponent(const std::vector<index_t>& rows, const std::vector<index_t>& columns)
{

}
Submatrix GraphicAddition::createSubmatrix() const {
    Submatrix submatrix;

    //TODO:

    return submatrix;
}
