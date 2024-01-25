//
// Created by rolf on 25-1-24.
//

#include "mipworkshop2024/presolve/NetworkAdditionComplete.hpp"

Submatrix NetworkAddition::createSubmatrix(index_t numRows, index_t numCols) const{
    Submatrix submatrix(numRows,numCols);
    if(transposed){
        for(index_t i = 0; i < numRows; ++i){
            if(SPQRNetworkDecompositionContainsColumn(dec,i)){
                submatrix.addRow(i);
            }
        }
        for(index_t i = 0; i < numCols; ++i){
            if(SPQRNetworkDecompositionContainsRow(dec,i)){
                submatrix.addColumn(i);
            }
        }
    }else{
        for(index_t i = 0; i < numRows; ++i){
            if(SPQRNetworkDecompositionContainsRow(dec,i)){
                submatrix.addRow(i);
            }
        }
        for(index_t i = 0; i < numCols; ++i){
            if(SPQRNetworkDecompositionContainsColumn(dec,i)){
                submatrix.addColumn(i);
            }
        }
    }

    return submatrix;

}
bool NetworkAddition::containsColumn(index_t col) const {
    return transposed ? SPQRNetworkDecompositionContainsRow(dec,col) : SPQRNetworkDecompositionContainsColumn(dec,col);
}
bool NetworkAddition::containsRow(index_t row) const {
    return transposed ? SPQRNetworkDecompositionContainsColumn(dec,row) : SPQRNetworkDecompositionContainsRow(dec,row);
}

void NetworkAddition::removeComponent(const std::vector<index_t> &rows, const std::vector<index_t> &columns) {
    const auto& graphRows = transposed ? columns : rows;
    const auto& graphCols = transposed ? rows : columns;

    SPQRNetworkDecompositionRemoveComponents(dec,graphRows.data(),graphRows.size(),graphCols.data(),graphCols.size());
}

NetworkAddition::~NetworkAddition() {
    SPQRfreeNetworkColumnAddition(env,&colAddition);
    SPQRfreeNetworkRowAddition(env,&rowAddition);
    SPQRNetworkDecompositionFree(&dec);
    SPQRfreeEnvironment(&env);

}

NetworkAddition::NetworkAddition(index_t numRows, index_t numCols, Submatrix::Initialization init, bool transposed) :
    transposed{transposed}{
    SPQR_CALL_THROW(SPQRcreateEnvironment(&env));
    index_t numDecRows = transposed ? numCols : numRows;
    index_t numDecCols = transposed ? numRows : numCols;
    SPQR_CALL_THROW(SPQRNetworkDecompositionCreate(env,&dec,numDecRows,numDecCols));
    SPQR_CALL_THROW(SPQRcreateNetworkRowAddition(env,&rowAddition));
    SPQR_CALL_THROW(SPQRcreateNetworkColumnAddition(env,&colAddition));

}

SPQRNetworkDecompositionStatistics NetworkAddition::statistics() const {
    return SPQRNetworkDecompositionGetStatistics(dec);
}
