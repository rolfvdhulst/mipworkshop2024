//
// Created by rolf on 12-9-23.
//

#include "mipworkshop2024/presolve/NetworkAddition.hpp"
#include <unordered_map>
#include <utility>

GraphicAddition::GraphicAddition(index_t numRows, index_t numCols, Submatrix::Initialization init, bool transposed) :
transposed{transposed}{

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
    const auto& graphRows = transposed ? columns : rows;
    const auto& graphCols = transposed ? rows : columns;

    removeComponents(dec,graphRows.data(),graphRows.size(),graphCols.data(),graphCols.size());

}
Submatrix GraphicAddition::createSubmatrix(index_t numRows, index_t numCols) const {
    Submatrix submatrix(numRows,numCols);
    if(transposed){
        for(index_t i = 0; i < numRows; ++i){
            if(decompositionHasCol(dec,i)){
                submatrix.addRow(i);
            }
        }
        for(index_t i = 0; i < numCols; ++i){
            if(decompositionHasRow(dec,i)){
                submatrix.addColumn(i);
            }
        }
    }else{
        for(index_t i = 0; i < numRows; ++i){
            if(decompositionHasRow(dec,i)){
                submatrix.addRow(i);
            }
        }
        for(index_t i = 0; i < numCols; ++i){
            if(decompositionHasCol(dec,i)){
                submatrix.addColumn(i);
            }
        }
    }

    return submatrix;
}

void GraphicRealization::addDecompositionMember(Decomposition *dec, member_id member,
                                         index_t parentMarkerHead,
                                         index_t parentMarkerTail) {
    switch(getMemberType(dec,member)){
        case RIGID:
            addRigidMember(dec,member,parentMarkerHead,parentMarkerTail);
            break;
        case SERIES:
            addSeriesMember(dec,member,parentMarkerHead,parentMarkerTail);
            break;
        case PARALLEL:
        case LOOP:
            addParallelMember(dec,member,parentMarkerHead,parentMarkerTail);
            break;
        case UNASSIGNED:
            throw std::logic_error("Aah");
    }

    //First, add all of the member's edges

}

index_t GraphicRealization::addDecompositionNode(node_id node) {
    index_t mapped = INVALID;
    auto it = decNodeToGraphNode.find(node);
    if(it == decNodeToGraphNode.end()){
        //Add new node
        mapped = numNodes;
        ++numNodes;
        decNodeToGraphNode.insert({node,mapped});
    }else{
        mapped = it->second;
    }
    return mapped;
}

void GraphicRealization::addRigidMember(Decomposition *dec, member_id member,
                                 index_t parentMarkerHead, index_t parentMarkerTail) {
    edge_id parentMarker = markerToParent(dec,member);
    if(edgeIsValid(parentMarker)){
        node_id head = findEdgeHead(dec,parentMarker);
        decNodeToGraphNode[head] = parentMarkerHead; //Map head to head first
        node_id tail = findEdgeTail(dec,parentMarker);
        decNodeToGraphNode[tail] = parentMarkerTail;
    }
    //Otherwise, we are the root.
    //Since the loop can deal with non-existing edges quite easily, we do not need to do extra work

    edge_id firstEdge = getFirstMemberEdge(dec,member);
    edge_id edge = firstEdge;
    do{
        if(edge == parentMarker){
            edge = getNextMemberEdge(dec,edge);
            continue;
        }
        node_id head = findEdgeHead(dec,edge);
        index_t headNode = addDecompositionNode(head);
        node_id tail = findEdgeTail(dec,edge);
        index_t tailNode = addDecompositionNode(tail);

        addDecompositionEdge(dec,edge,headNode,tailNode);

        edge = getNextMemberEdge(dec,edge);
    }while(edge != firstEdge);
}

void GraphicRealization::addParallelMember(Decomposition *dec, member_id member,
                                    index_t parentMarkerHead, index_t parentMarkerTail) {

    edge_id parentMarker = markerToParent(dec,member);
    if(!edgeIsValid(parentMarker)){ //This member is the root, then, we
        parentMarkerHead = addNonexistingDecompositionNode();
        parentMarkerTail = addNonexistingDecompositionNode();
    }

    edge_id firstEdge = getFirstMemberEdge(dec,member);
    edge_id edge = firstEdge;
    do{
        if(edge == parentMarker){
            edge = getNextMemberEdge(dec,edge);
            continue;
        }

        addDecompositionEdge(dec,edge,parentMarkerHead,parentMarkerTail);
        edge = getNextMemberEdge(dec,edge);
    }while(edge != firstEdge);
}

index_t GraphicRealization::addNonexistingDecompositionNode() {
    index_t id = numNodes;
    ++numNodes;
    return id;
}

void GraphicRealization::addDecompositionEdge(Decomposition *dec, edge_id edge, index_t head, index_t tail) {
    if(edgeIsMarker(dec,edge)){ //child virtual edge
        member_id childComponent = findEdgeChildMember(dec,edge);
        addDecompositionMember(dec,childComponent,head,tail);
    }else { //'normal' edge
        //Add arc
        spqr_element element = edgeGetElement(dec,edge);
        if(edgeIsTree(dec,edge)){
            assert(elementIsRow(element));
            Arc arc{.index = elementToRow(element),
                    .start = head,
                    .end = tail};
            rowArcs.push_back(arc);
        }else{
            assert(elementIsColumn(element));
            Arc arc{.index = elementToColumn(element),
                    .start = head,
                    .end = tail};
            columnArcs.push_back(arc);
        }
    }
}

void GraphicRealization::addSeriesMember(Decomposition *dec, member_id member,
                                  index_t parentMarkerHead, index_t parentMarkerTail) {
    edge_id parentMarker = markerToParent(dec,member);
    if(!edgeIsValid(parentMarker)){
        //Otherwise, we pick an arbitrary edge, and add it to fix this
        parentMarker = getFirstMemberEdge(dec,member);
        parentMarkerHead = addNonexistingDecompositionNode();
        parentMarkerTail = addNonexistingDecompositionNode();
        addDecompositionEdge(dec,parentMarker,parentMarkerHead,parentMarkerTail);
    }

    int numberOfEdges = getNumMemberEdges(dec,member);
    index_t previous = parentMarkerHead;

    int numberProcessedEdges = 1;

    edge_id firstEdge = getFirstMemberEdge(dec,member);
    edge_id edge = firstEdge;
    //Place the edges in a circular fashion
    do{
        if(edge == parentMarker){
            edge = getNextMemberEdge(dec,edge);
            continue;
        }

        index_t next = parentMarkerTail;
        numberProcessedEdges += 1;
        if(numberProcessedEdges != numberOfEdges){
            next = addNonexistingDecompositionNode();
        }
        addDecompositionEdge(dec,edge,previous,next);

        previous = next;
        edge = getNextMemberEdge(dec,edge);
    }while(edge != firstEdge);
}

GraphicRealization GraphicRealization::fromDecompositionComponent(Decomposition *dec, member_id componentRoot) {
    GraphicRealization realization;
    realization.addDecompositionMember(dec,componentRoot,INVALID,INVALID);
    return realization;
}

std::size_t GraphicRealization::numCotreeEdges() const {
    return columnArcs.size();
}

std::size_t GraphicRealization::numTreeEdges() const {
    return rowArcs.size();
}

std::optional<CamionGraph>
GraphicAddition::createComponentGraph(const std::vector<index_t> &rows, const std::vector<index_t> &columns) const {
    const auto& decRows = transposed ? columns : rows;
    const auto& decCols = transposed ? rows : columns;
    for(auto row : decRows){
        if(!decompositionHasRow(dec,row)){
            return std::nullopt;
        }
    }
    for(auto col : decCols){
        if(!decompositionHasCol(dec,col)){
            return std::nullopt;
        }
    }

    edge_id edge = getDecompositionRowEdge(dec,decRows[0]);

    member_id member = findEdgeMember(dec,edge);
    member_id root = member;
    while(true){
        member_id parent = findMemberParent(dec,root);
        if(memberIsInvalid(parent)){
            break;
        }
        root = parent;
    }
    auto realization = GraphicRealization::fromDecompositionComponent(dec,root);
    assert(realization.numCotreeEdges() == decCols.size());
    assert(realization.numTreeEdges() == decRows.size());
    return CamionGraph::fromRealization(realization);
}

void GraphicAddition::addLastCheckedNetworkRow() {
    SPQR_CALL_THROW(addNewRow(dec,rowAdd));
}

void GraphicAddition::addLastCheckedNetworkCol() {
    SPQR_CALL_THROW(addNewColumn(dec,colAdd));
}

void GraphicAddition::addLastCheckedCol() {
    if(transposed){
        addLastCheckedNetworkRow();
    }else{
        addLastCheckedNetworkCol();
    }

}

CamionGraph CamionGraph::fromRealization(const GraphicRealization &realization) {
    CamionGraph graph;

    graph.columnArcs = realization.columnArcs;
    graph.treeDepth = std::vector<index_t>(realization.numNodes);
    graph.treeParent = std::vector<index_t>(realization.numNodes,INVALID);
    graph.treeRow = std::vector<index_t>(realization.numNodes,INVALID);
    graph.treeRowStatus = std::vector<int>(realization.numNodes,0);
    graph.workingBuffer = std::vector<int>(realization.numNodes,0);


    std::vector<std::vector<index_t>> nodeAdjacentTreeArcs(realization.numNodes);

    for(index_t edge = 0; edge < realization.rowArcs.size(); ++edge){
        const auto& arc = realization.rowArcs[edge];
        nodeAdjacentTreeArcs[arc.start].push_back(edge);
        nodeAdjacentTreeArcs[arc.end].push_back(edge);
    }
    index_t root = 0; //TODO: try if selecting root with highest degree helps running time

    std::vector<bool> nodeExplored(realization.numNodes,false);
    std::vector<index_t> nodeStack = {root};
    graph.treeParent[root] = INVALID;
    graph.treeRow[root] = INVALID;
    graph.treeDepth[root] = 0;

    while(!nodeStack.empty()){
        index_t node = nodeStack.back();
        nodeStack.pop_back();
        nodeExplored[node] = true;

        for(const auto& edge : nodeAdjacentTreeArcs[node]){
            index_t start = realization.rowArcs[edge].start;
            index_t end = realization.rowArcs[edge].end;
            assert(node == start || node == end);
            index_t otherNode = start == node ? end : start;
            if(!nodeExplored[otherNode]){
                nodeStack.push_back(otherNode);
                assert(graph.treeParent[otherNode] == INVALID);
                graph.treeDepth[otherNode] = graph.treeDepth[node] + 1;
                graph.treeParent[otherNode] = node;
                graph.treeRow[otherNode] = realization.rowArcs[edge].index;
            }else{
                assert(graph.treeDepth[otherNode] == graph.treeDepth[node]-1);
            }
        }
    }
    for(index_t node = 0; node < graph.treeRow.size(); ++node){
        if(node == root) continue;
        index_t row = graph.treeRow[node];
        graph.rowToNode.insert({row,node});
    }
    for(index_t index = 0; index < graph.columnArcs.size(); ++index){
        graph.columnToArcIndex[graph.columnArcs[index].index] = index;
    }

    return graph;
}

std::pair<std::vector<index_t>, std::vector<index_t>> CamionGraph::traceCycle(const Arc &arc) const {
    std::vector<index_t> leftRows;
    std::vector<index_t> rightRows;

    index_t leftNode = arc.start;
    index_t rightNode = arc.end;
    index_t leftDepth = treeDepth[leftNode];
    index_t rightDepth = treeDepth[rightNode];

    while(leftDepth > rightDepth){
        leftRows.push_back(leftNode);
        assert(treeDepth[leftNode] == treeDepth[treeParent[leftNode]] + 1);
        leftNode = treeParent[leftNode];
        --leftDepth;
    }
    while(rightDepth > leftDepth){
        rightRows.push_back(rightNode);
        assert(treeDepth[rightNode] == treeDepth[treeParent[rightNode]] + 1);
        rightNode = treeParent[rightNode];
        --rightDepth;
    }
    while(leftNode != rightNode){
        leftRows.push_back(leftNode);
        rightRows.push_back(rightNode);

        rightNode = treeParent[rightNode];
        leftNode = treeParent[leftNode];
        --leftDepth;
        --rightDepth;
    }
    return {leftRows,rightRows};
}

bool
CamionGraph::checkConnectedMatrix(const SparseMatrix &matrix, const std::vector<index_t> &connectedColumnOrdering) {
    for(index_t column : connectedColumnOrdering){
        auto it = columnToArcIndex.find(column);
        if(it == columnToArcIndex.end()){
            throw std::logic_error("Aah");
        }
        index_t arcIndex = it->second;
        assert(columnArcs[arcIndex].index == column);
        if(!checkColumn(columnArcs[arcIndex],matrix.getPrimaryVector(column))){
            return false;
        }
    }
    return true;
}

SignAddition::SignAddition(std::size_t numRows, std::size_t numColumns, std::vector<bool> includedColumns) :
nodes(numRows+numColumns),
bfsCallStack(numRows+numColumns),
columnIncluded(std::move(includedColumns)),
numRows{numRows}{

}

bool SignAddition::tryAddColumn(const SparseMatrix &rowMatrix, const SparseMatrix &colMatrix, index_t column) {
    std::size_t nonzeros = 0;
    for(auto& node : nodes){
        node.targetValue = 0;
        node.predecessor = INVALID;
        node.predecessorValue = 0;
        node.status = 0;
    }
    for(const Nonzero& nonzero : colMatrix.getPrimaryVector(column)){
        nodes[nonzero.index()].targetValue = nonzero.value() > 0.0 ? 1 : -1;
        ++nonzeros;
    }
    if(nonzeros == 0){
        return true;
    }
    for(const Nonzero& rootNonzero : colMatrix.getPrimaryVector(column)){
        index_t rootSearchNode = rootNonzero.index();
        if(nodes[rootSearchNode].status != 0) continue;
        bfsCallStack.clear();
        bfsCallStack.push_back(rootSearchNode);
        nodes[rootSearchNode].status = 1;
        while(!bfsCallStack.empty()){
            index_t currentNode = bfsCallStack.back();
            bfsCallStack.pop_back();
            assert(nodes[currentNode].status == 1);
            nodes[currentNode].status = 2;
            if(currentNode >= numRows){
                index_t nodeCol = currentNode - numRows;
                assert(columnIncluded[nodeCol]);
                for(const Nonzero& nonzero : colMatrix.getPrimaryVector(nodeCol)){
                    index_t entryRow = nonzero.index();
                    if(nodes[entryRow].status != 0) continue;
                    nodes[entryRow].status = 1;
                    nodes[entryRow].predecessor = currentNode;
                    nodes[entryRow].predecessorValue = nonzero.value() > 0.0 ? 1 : -1;
                    //If column is new, push it onto the bfs stack

                    bfsCallStack.push_back(entryRow);
                    //If we reach a target column for the first time, trace back to the previous target column
                    if(nodes[entryRow].targetValue == 0) continue;

                    int sum = nodes[entryRow].targetValue;
                    index_t pathNode = entryRow;
                    do{
                        sum += nodes[pathNode].predecessorValue;
                        pathNode = nodes[pathNode].predecessor;
                    }while(nodes[pathNode].targetValue == 0);

                    sum += nodes[pathNode].targetValue;

                    if(sum % 4 != 0){
                        assert(sum % 4 == -2 || sum % 4 == 2);
                        columnIncluded[column] = false;
                        return false;
                    }
                }
            }else{
                index_t nodeRow = currentNode;
                for(const Nonzero& nonzero : rowMatrix.getPrimaryVector(nodeRow)){
                    if(!columnIncluded[nonzero.index()]) continue;
                    index_t nodeIndex = nonzero.index() + numRows;
                    if(nodes[nodeIndex].status == 0){
                        nodes[nodeIndex].status = 1;
                        nodes[nodeIndex].predecessor = currentNode;
                        nodes[nodeIndex].predecessorValue = nonzero.value() > 0.0 ? 1 : -1;
                        bfsCallStack.push_back(nodeIndex);
                    }
                }
            }

        }
    }

    columnIncluded[column] = true;
    return true;
}
