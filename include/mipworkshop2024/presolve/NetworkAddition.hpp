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
#include "mipworkshop2024/SparseMatrix.h"
#include <stdexcept>
#include <unordered_map>


class CamionGraph;

struct Arc{
    index_t index;
    index_t start;
    index_t end;
};
class GraphicRealization{
public:
    static GraphicRealization fromDecompositionComponent(Decomposition *dec, member_id componentRoot);
    friend class CamionGraph;
    std::size_t numTreeEdges() const;
    std::size_t numCotreeEdges() const;
private:
    void addDecompositionMember(Decomposition * dec, member_id member,
                                index_t parentMarkerHead,
                                index_t parentMarkerTail);
    void addParallelMember(Decomposition *dec, member_id member,
                           index_t parentMarkerHead,
                           index_t parentMarkerTail);
    void addSeriesMember(Decomposition *dec, member_id member,
                         index_t parentMarkerHead,
                         index_t parentMarkerTail);
    void addRigidMember(Decomposition *dec, member_id member,
                        index_t parentMarkerHead,
                        index_t parentMarkerTail);
    void addDecompositionEdge(Decomposition * dec, edge_id edge,
                              index_t head, index_t tail);
    index_t addDecompositionNode(node_id node);
    index_t addNonexistingDecompositionNode();

    std::unordered_map<node_id,index_t> decNodeToGraphNode;
    index_t numNodes = 0;
    std::vector<Arc> rowArcs;
    std::vector<Arc> columnArcs;
};



class SignAddition{
public:
    SignAddition(std::size_t numRows, std::size_t numColumns,
                 std::vector<bool> includedColumns);
    bool tryAddColumn(const SparseMatrix& rowMatrix,
                     const SparseMatrix& colMatrix,
                     index_t column);
private:
    struct GraphNode {
        index_t predecessor = -1;
        int predecessorValue = 0;
        int targetValue = 0;
        int status = 0;
    };
    std::vector<GraphNode> nodes;
    std::vector<index_t> bfsCallStack;
    std::vector<bool> columnIncluded;
    index_t numRows;

};
class CamionGraph{
public:
    static CamionGraph fromRealization(const GraphicRealization& realization);

    [[nodiscard]] std::pair<std::vector<index_t>,std::vector<index_t>> traceCycle(const Arc& arc) const;

    bool checkConnectedMatrix(const SparseMatrix& matrix, const std::vector<index_t>& connectedColumnOrdering);
    template<typename T>
    bool checkColumn(const Arc& arc, const MatrixSlice<T>& column){
        auto [forwardsNodes,backwardsNodes] = traceCycle(arc);
        ///Working buffer; +1 : fixed forwards, -1: fixed backwards, -2: backwards edge unfixed, +2, forwards edge unfixed
        for(const auto& node : forwardsNodes){
            int status = treeRowStatus[node];
            if(status == 0){
                workingBuffer[node] = 2;
            }else{
                workingBuffer[node] = status;
            }
        }
        for(const auto& node : backwardsNodes){
            int status = treeRowStatus[node];
            if(status == 0){
                workingBuffer[node] = -2;
            }else{
                workingBuffer[node] = -status;
            }
        }
        std::size_t fixedGoodForwards = 0;
        std::size_t fixedGoodReverse = 0;

        std::size_t numNonzeros = 0;
        for(const Nonzero& nonzero : column){
            auto it = rowToNode.find(nonzero.index());
            if(it == rowToNode.end()){
                continue; //This can happen when checking for transposed network matrix
            }
            ++numNonzeros;
            index_t node = it->second;
            if(nonzero.value() < 0){
                if(workingBuffer[node] == -1){
                    ++fixedGoodForwards;
                }else if(workingBuffer[node] == 1){
                    ++fixedGoodReverse;
                }
            }else{
                if(workingBuffer[node] == -1){
                    ++fixedGoodReverse;
                }else if(workingBuffer[node] == 1){
                    ++fixedGoodForwards;
                }
            }
        }
        if(numNonzeros != forwardsNodes.size() + backwardsNodes.size()){
            throw std::logic_error("aah");
        }
        if(fixedGoodForwards > 0 && fixedGoodReverse > 0){
            return false;
        }
        bool fixForwards = fixedGoodReverse == 0;
        for(const Nonzero& nonzero : column){
            auto it = rowToNode.find(nonzero.index());
            if(it == rowToNode.end()) continue;
            index_t node = it->second;
            if(workingBuffer[node] == 2){
                int value = (nonzero.value() > 0.0) ^ fixForwards ? -1 : 1;
                treeRowStatus[node] = value;
            }else if(workingBuffer[node] == -2){
                int value = (nonzero.value() > 0.0) ^ fixForwards ? 1 : -1;
                treeRowStatus[node] = value;
            }
        }
        return true;

    }
private:

    std::vector<index_t> treeParent; //indexed by node
    std::vector<index_t> treeRow; //indicates which row points to the parent of this node
    std::vector<index_t> treeDepth;
    std::unordered_map<index_t,index_t> rowToNode;
    std::vector<int> treeRowStatus; //0; orientation not yet determined; 1, orientation such as in the arborescence, -1: orientation reversed from arborescence
    std::vector<int> workingBuffer;
    std::vector<Arc> columnArcs;
    std::unordered_map<index_t,index_t> columnToArcIndex;

};

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


    template<typename Storage>
    bool checkNetworkRow(index_t index, const MatrixSlice<Storage>& slice){
        sliceBuffer.clear();
        for(const auto& nonzero : slice){
            if(decompositionHasCol(dec,nonzero.index())){
                sliceBuffer.push_back(nonzero.index());
            }
        }
        SPQR_CALL_THROW(checkNewRow(dec,rowAdd,index,sliceBuffer.data(),sliceBuffer.size()));
        return rowAdditionRemainsGraphic(rowAdd);
    }

    template<typename Storage>
    bool checkNetworkCol(index_t index, const MatrixSlice<Storage>& slice){
        sliceBuffer.clear();
        for(const auto& nonzero : slice){
            if(decompositionHasRow(dec,nonzero.index())){
                sliceBuffer.push_back(nonzero.index());
            }
        }
        SPQR_CALL_THROW(checkNewColumn(dec,colAdd,index,sliceBuffer.data(),sliceBuffer.size()));
        return columnAdditionRemainsGraphic(colAdd);
    }

    void addLastCheckedNetworkRow();
    void addLastCheckedNetworkCol();
public:
    GraphicAddition(index_t numRows, index_t numCols,
                             Submatrix::Initialization init = Submatrix::INIT_NONE,
                             bool transposed = false);
    ~GraphicAddition();

    void addLastCheckedCol();
    template<typename Storage>
    bool checkCol(index_t col, const MatrixSlice<Storage>& colSlice){
        if(transposed){
            return checkNetworkRow(col,colSlice);
        }
        return checkNetworkCol(col,colSlice);
    }

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

    [[nodiscard]] std::optional<CamionGraph> createComponentGraph(const std::vector<index_t>& rows, const std::vector<index_t>& columns) const;
};


#endif //MIPWORKSHOP2024_NETWORKADDITION_HPP
