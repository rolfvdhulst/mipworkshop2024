//
// Created by rolf on 25-1-24.
//

#ifndef MIPWORKSHOP2024_NETWORKADDITIONCOMPLETE_HPP
#define MIPWORKSHOP2024_NETWORKADDITIONCOMPLETE_HPP


#include <cassert>
#include "mipworkshop2024/MatrixSlice.h"
#include "mipworkshop2024/Submatrix.h"
#include "mipworkshop2024/presolve/Network.h"
#include <stdexcept>

#define SPQR_CALL_THROW(x) \
   do                                                                                                   \
   {                                                                                                    \
      SPQR_ERROR throw_retcode;                                                                       \
      if( ((throw_retcode) = (x)) != SPQR_OKAY )                                                        \
         throw std::logic_error("Error <" + std::to_string((long long)throw_retcode) + "> in function call"); \
   }                                                                                                    \
   while( false )          \

/// This class contains the methods for an algorithm which tries to detect if a matrix has a network submatrix
class NetworkAddition
{
private:
    SPQR * env = NULL;
    SPQRNetworkDecomposition * dec = NULL;
    SPQRNetworkRowAddition * rowAddition = NULL;
    SPQRNetworkColumnAddition * colAddition = NULL;

    bool transposed;
    std::vector<index_t> sliceBuffer;
    std::vector<double> valueBuffer;

    template<typename Storage>
    bool tryAddNetworkRow(index_t index, const MatrixSlice<Storage>& slice)
    {
        sliceBuffer.clear();
        valueBuffer.clear();
        for(const auto& nonzero : slice){
            sliceBuffer.push_back(nonzero.index());
            valueBuffer.push_back(nonzero.value());
            if(fabs(nonzero.value()) != 1.0){
                return false;
            }
        }
        SPQR_CALL_THROW(SPQRNetworkRowAdditionCheck(dec,rowAddition,index,sliceBuffer.data(),
                                                    valueBuffer.data(),sliceBuffer.size()));
        if(SPQRNetworkRowAdditionRemainsNetwork(rowAddition)){
            SPQR_CALL_THROW(SPQRNetworkRowAdditionAdd(dec,rowAddition));
            return true;
        }
        return false;
    }

    template<typename Storage>
    bool tryAddNetworkCol(index_t index, const MatrixSlice<Storage>& slice)
    {
        sliceBuffer.clear();
        valueBuffer.clear();
        for(const auto& nonzero : slice){
            sliceBuffer.push_back(nonzero.index());
            valueBuffer.push_back(nonzero.value());
            if(fabs(nonzero.value()) != 1.0){
                return false;
            }
        }
        SPQR_CALL_THROW(SPQRNetworkColumnAdditionCheck(dec,colAddition,index,sliceBuffer.data(),
                                                       valueBuffer.data(),sliceBuffer.size()));
        if(SPQRNetworkColumnAdditionRemainsNetwork(colAddition)){
            SPQR_CALL_THROW(SPQRNetworkColumnAdditionAdd(dec,colAddition));
            return true;
        }
        return false;
    }

public:

    NetworkAddition(index_t numRows, index_t numCols,
                    Submatrix::Initialization init = Submatrix::INIT_NONE,
                    bool transposed = false);
    ~NetworkAddition();

    template<typename Storage>
    bool tryAddCol(index_t col, const MatrixSlice<Storage>& colSlice)
    {
        if (transposed)
        {
            return tryAddNetworkRow(col, colSlice);
        }
        else
        {
            return tryAddNetworkCol(col, colSlice);
        }
    }

    template<typename Storage>
    bool tryAddRow(index_t row, const MatrixSlice<Storage>& rowSlice)
    {
        if (transposed)
        {
            return tryAddNetworkCol(row, rowSlice);
        }
        else
        {
            return tryAddNetworkRow(row, rowSlice);
        }
    }


    [[nodiscard]] bool containsColumn(index_t col) const;
    [[nodiscard]] bool containsRow(index_t row) const;
    //Hacky method to remove. Assumes that the given rows/columns form one connected component in the current decomposition
    void removeComponent(const std::vector<index_t>& rows, const std::vector<index_t>& columns);
    [[nodiscard]] Submatrix createSubmatrix(index_t numRows, index_t numCols) const;

    [[nodiscard]] SPQRNetworkDecompositionStatistics statistics() const;
};


#endif //MIPWORKSHOP2024_NETWORKADDITIONCOMPLETE_HPP
