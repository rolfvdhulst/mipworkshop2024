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

class GraphicAddition {
private:
    SPQR * env;
    Decomposition * dec;
    SPQRNewRow * rowAdd;
    SPQRNewColumn * colAdd;

    bool transposed;

    template<typename Storage>
    bool tryAddNetworkRow(index_t index, const MatrixSlice<Storage>& slice)
    {
        return true;
    }

    template<typename Storage>
    bool tryAddNetworkCol(index_t index, const MatrixSlice<Storage>& slice)
    {
        return true;
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
