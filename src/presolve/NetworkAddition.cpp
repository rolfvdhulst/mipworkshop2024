//
// Created by rolf on 12-9-23.
//

#include "mipworkshop2024/presolve/NetworkAddition.hpp"
#include <stdexcept>

// a macro that throws if the output is not okay somehow
#define SPQR_CALL_THROW(x) \
   do                                                                                                   \
   {                                                                                                    \
      SPQR_ERROR throw_retcode;                                                                       \
      if( ((throw_retcode) = (x)) != SPQR_OKAY )                                                        \
         throw std::logic_error("Error <" + std::to_string((long long)throw_retcode) + "> in function call"); \
   }                                                                                                    \
   while( false )


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