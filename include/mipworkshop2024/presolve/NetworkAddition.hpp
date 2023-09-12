//
// Created by rolf on 12-9-23.
//

#ifndef MIPWORKSHOP2024_NETWORKADDITION_HPP
#define MIPWORKSHOP2024_NETWORKADDITION_HPP

#include "mipworkshop2024/MatrixSlice.h"
#include "mipworkshop2024/Submatrix.h"
#include "SPQRDecomposition.h"
#include "SPQRRowAddition.h"
#include "SPQRColumnAddition.h"
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
class GraphicAddition {
private:
    SPQR * env;
    Decomposition * dec;
    SPQRNewRow * rowAdd;
    SPQRNewColumn * colAdd;

    bool transposed;
    std::vector<index_t> sliceBuffer;

    template<typename Storage>
    bool tryAddNetworkRow(index_t index, const MatrixSlice<Storage>& slice)
    {
        sliceBuffer.clear();
        for(const auto& nonzero : slice){
            if(decompositionHasCol(dec,nonzero.index())){
                sliceBuffer.push_back(nonzero.index());
            }
        }
        SPQR_CALL_THROW(checkNewRow(dec,rowAdd,index,sliceBuffer.data(),sliceBuffer.size()));
        if(rowAdditionRemainsGraphic(rowAdd)){
            SPQR_CALL_THROW(addNewRow(dec,rowAdd));
            return true;
        }
        return false;
    }

    template<typename Storage>
    bool tryAddNetworkCol(index_t index, const MatrixSlice<Storage>& slice)
    {
        sliceBuffer.clear();
        for(const auto& nonzero : slice){
            if(decompositionHasRow(dec,nonzero.index())){
                sliceBuffer.push_back(nonzero.index());
            }
        }
        SPQR_CALL_THROW(checkNewColumn(dec,colAdd,index,sliceBuffer.data(),sliceBuffer.size()));
        if(columnAdditionRemainsGraphic(colAdd)){
            SPQR_CALL_THROW(addNewColumn(dec,colAdd));
            return true;
        }
        return false;
    }


public:
    GraphicAddition(index_t numRows, index_t numCols,
                             Submatrix::Initialization init = Submatrix::INIT_NONE,
                             bool transposed = false);
    ~GraphicAddition();
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
    [[nodiscard]] Submatrix createSubmatrix() const;
};


#endif //MIPWORKSHOP2024_NETWORKADDITION_HPP
