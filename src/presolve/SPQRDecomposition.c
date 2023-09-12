//
// Created by rolf on 01-07-22.
//


#include "mipworkshop2024/presolve/SPQRDecomposition.h"
#include <assert.h>
//TODO: all checks with mem... to num...? In which cases do we store contiguously?


void swap_ints(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

bool nodeIsInvalid(node_id node) {
    return node < 0;
}

bool nodeIsValid(node_id node) {
    return !nodeIsInvalid(node);
}

bool memberIsInvalid(member_id member) {
    return member < 0;
}
bool memberIsValid(member_id member) {
    return !memberIsInvalid(member);
}

bool edgeIsInvalid(edge_id edge) {
    return edge == INVALID_EDGE;
}
bool edgeIsValid(edge_id edge){
    return !edgeIsInvalid(edge);
}


bool elementIsRow(spqr_element element){
    return element < 0;
}
bool elementIsColumn(spqr_element element){
    return !elementIsRow(element);
}
row_idx elementToRow(spqr_element element){
    assert(elementIsRow(element));
    return (row_idx) (-element - 1);
}
spqr_element rowToElement(row_idx row){
    assert(rowIsValid(row));
    return (spqr_element) -row-1;
}
col_idx elementToColumn(spqr_element element){
    assert(elementIsColumn(element));
    return (col_idx) element;
}
spqr_element columnToElement(col_idx column){
    assert(colIsValid(column));
    return (spqr_element) column;
}

bool nodeIsRepresentative(const Decomposition *dec, node_id node) {
    assert(dec);
    assert(node < dec->memNodes);
    assert(nodeIsValid(node));

    return nodeIsInvalid(dec->nodes[node].representativeNode);
}

node_id findNode(Decomposition *dec, node_id node) {
    assert(dec);
    assert(nodeIsValid(node));
    assert(node < dec->memNodes);

    node_id current = node;
    node_id next;

    //traverse down tree to find the root
    while (nodeIsValid(next = dec->nodes[current].representativeNode)) {
        current = next;
        assert(current < dec->memNodes);
    }

    node_id root = current;
    current = node;

    //update all pointers along path to point to root, flattening the tree
    while (nodeIsValid(next = dec->nodes[current].representativeNode)) {
        dec->nodes[current].representativeNode = root;
        current = next;
        assert(current < dec->memNodes);
    }
    return root;
//    node_id current = node;
//    node_id next;
//    while(nodeIsValid(next = dec->nodes[current].representativeNode)){
//        node_id nextParent = dec->nodes[next].representativeNode;
//        if(nodeIsInvalid(nextParent)){
//            current = next;
//            break;
//        }
//        dec->nodes[current].representativeNode = nextParent;
//        current = next;
//    }
//
//    return current;
}

node_id findNodeNoCompression(const Decomposition *dec, node_id node) {
    assert(dec);
    assert(nodeIsValid(node));
    assert(node < dec->memNodes);

    node_id current = node;
    node_id next;

    //traverse down tree to find the root
    while (nodeIsValid(next = dec->nodes[current].representativeNode)) {
        current = next;
        assert(current < dec->memNodes);
    }
    node_id root = current;
    return root;
}

node_id mergeNodes(Decomposition *dec, node_id first, node_id second) {
    assert(dec);
    assert(nodeIsRepresentative(dec, first));
    assert(nodeIsRepresentative(dec, second));
    assert(first != second); //We cannot merge a node into itself
    assert(first < dec->memNodes);
    assert(second < dec->memNodes);

    //The rank is stored as a negative number: we decrement it making the negative number larger.
    // We want the new root to be the one with 'largest' rank, so smallest number. If they are equal, we decrement.
    node_id firstRank = dec->nodes[first].representativeNode;
    node_id secondRank = dec->nodes[second].representativeNode;
    if (firstRank > secondRank) {
        swap_ints(&first, &second);
    }
    //first becomes representative; we merge all of the edges of second into first
    mergeNodeEdgeList(dec,first,second);
    dec->nodes[second].representativeNode = first;
    if (firstRank == secondRank) {
        --dec->nodes[first].representativeNode;
    }
    return first;
}

bool memberIsRepresentative(const Decomposition *dec, member_id member) {
    assert(dec);
    assert(member < dec->memMembers);
    assert(memberIsValid(member));

    return memberIsInvalid(dec->members[member].representativeMember);
}

member_id findMember(Decomposition *dec, member_id member) {
    assert(dec);
    assert(member < dec->memMembers);
    assert(memberIsValid(member));

    member_id current = member;
    member_id next;

    //traverse down tree to find the root
    while (memberIsValid(next = dec->members[current].representativeMember)) {
        current = next;
        assert(current < dec->memMembers);
    }

    member_id root = current;
    current = member;

    //update all pointers along path to point to root, flattening the tree
    while (memberIsValid(next = dec->members[current].representativeMember)) {
        dec->members[current].representativeMember = root;
        current = next;
        assert(current < dec->memMembers);
    }
    return root;
}

member_id findMemberNoCompression(const Decomposition *dec, member_id member) {
    assert(dec);
    assert(member < dec->memMembers);
    assert(memberIsValid(member));

    member_id current = member;
    member_id next;

    //traverse down tree to find the root
    while (memberIsValid(next = dec->members[current].representativeMember)) {
        current = next;
        assert(current < dec->memMembers);
    }

    member_id root = current;
    return root;
}

member_id mergeMembers(Decomposition *dec, member_id first, member_id second) {
    assert(dec);
    assert(memberIsRepresentative(dec, first));
    assert(memberIsRepresentative(dec, second));
    assert(first != second); //We cannot merge a member into itself
    assert(first < dec->memMembers);
    assert(second < dec->memMembers);

    //The rank is stored as a negative number: we decrement it making the negative number larger.
    // We want the new root to be the one with 'largest' rank, so smallest number. If they are equal, we decrement.
    member_id firstRank = dec->members[first].representativeMember;
    member_id secondRank = dec->members[second].representativeMember;
    if (firstRank > secondRank) {
        swap_ints(&first, &second);
    }
    dec->members[second].representativeMember = first;
    if (firstRank == secondRank) {
        --dec->members[first].representativeMember;
    }
    return first;
}

node_id findEdgeTail(Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    node_id representative = findNode(dec, dec->edges[edge].tail);
    dec->edges[edge].tail = representative; //update the edge information

    return representative;
}

node_id findEdgeHead(Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    node_id representative = findNode(dec, dec->edges[edge].head);
    dec->edges[edge].head = representative;//update the edge information

    return representative;
}

node_id findEdgeHeadNoCompression(const Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    node_id representative = findNodeNoCompression(dec, dec->edges[edge].head);
    return representative;
}

node_id findEdgeTailNoCompression(const Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    node_id representative = findNodeNoCompression(dec, dec->edges[edge].tail);
    return representative;
}

member_id findEdgeMember(Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    member_id representative = findMember(dec, dec->edges[edge].member);
    dec->edges[edge].member = representative;
    return representative;
}

member_id findEdgeMemberNoCompression(const Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    member_id representative = findMemberNoCompression(dec, dec->edges[edge].member);
    return representative;
}

member_id findMemberParent(Decomposition *dec, member_id member) {
    assert(dec);
    assert(member < dec->memMembers);
    assert(memberIsValid(member));
    assert(memberIsRepresentative(dec,member));


    if(memberIsInvalid(dec->members[member].parentMember)){
        return dec->members[member].parentMember;
    }
    member_id parent_representative = findMember(dec, dec->members[member].parentMember);
    dec->members[member].parentMember = parent_representative;

    return parent_representative;
}

member_id findMemberParentNoCompression(const Decomposition *dec, member_id member) {
    assert(dec);
    assert(member < dec->memMembers);
    assert(memberIsValid(member));
    assert(memberIsRepresentative(dec,member));

    if(memberIsInvalid(dec->members[member].parentMember)){
        return dec->members[member].parentMember;
    }
    member_id parent_representative = findMemberNoCompression(dec, dec->members[member].parentMember);
    return parent_representative;
}

member_id findEdgeChildMember(Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    member_id representative = findMember(dec, dec->edges[edge].childMember);
    dec->edges[edge].childMember = representative;
    return representative;
}

member_id findEdgeChildMemberNoCompression(const Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    member_id representative = findMemberNoCompression(dec, dec->edges[edge].childMember);
    return representative;
}

//TODO: fix usages, is misleading. Only accounts for CHILD markers, not parent markers!
bool edgeIsMarker(const Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    return memberIsValid(dec->edges[edge].childMember);
}

bool edgeIsTree(const Decomposition *dec, edge_id edge) {
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    return elementIsRow(dec->edges[edge].element);
}

spqr_element edgeGetElement(const Decomposition * dec, edge_id edge){
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);

    return dec->edges[edge].element;
}

bool decompositionHasRow(Decomposition *dec, row_idx row){
    assert( rowIsValid(row) && (int) row < dec->memRows);
    assert(dec);

    return edgeIsValid(dec->rowEdges[row]);
}
bool decompositionHasCol(Decomposition *dec, col_idx col){
    assert( colIsValid(col) && (int) col< dec->memColumns);
    assert(dec);

    return edgeIsValid(dec->columnEdges[col]);
}
void setDecompositionColumnEdge(Decomposition *dec, col_idx col, edge_id edge){
    assert( colIsValid(col) && (int)col< dec->memColumns);
    assert(dec);
    assert(edgeIsValid(edge));
    dec->columnEdges[col] = edge;
}
void setDecompositionRowEdge(Decomposition *dec, row_idx row, edge_id edge){
    assert( rowIsValid(row) && (int) row< dec->memRows);
    assert(dec);
    assert(edgeIsValid(edge));
    dec->rowEdges[row] = edge;
}
edge_id getDecompositionColumnEdge(const Decomposition *dec, col_idx col){
    assert( colIsValid(col) && (int) col< dec->memColumns);
    assert(dec);
    return dec->columnEdges[col];
}
edge_id getDecompositionRowEdge(const Decomposition *dec, row_idx row){
    assert( rowIsValid(row) && (int) row < dec->memRows);
    assert(dec);
    return dec->rowEdges[row];
}

SPQR_ERROR createDecomposition(SPQR *env, Decomposition **pDecomposition, int numRows, int numColumns) {
    assert(env);
    assert(pDecomposition);
    assert(!*pDecomposition);

    SPQR_CALL(SPQRallocBlock(env, pDecomposition));
    Decomposition *dec = *pDecomposition;
    dec->env = env;

    //Initialize edge array data
    int initialMemEdges = 8;
    {
        assert(initialMemEdges > 0);
        dec->memEdges = initialMemEdges;
        dec->numEdges = 0;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->edges, (size_t) dec->memEdges));
        for (edge_id i = 0; i < dec->memEdges; ++i) {
            dec->edges[i].edgeListNode.next = i + 1;
            dec->edges[i].member = INVALID_MEMBER;
        }
        dec->edges[dec->memEdges - 1].edgeListNode.next = INVALID_EDGE;
        dec->firstFreeEdge = 0;
    }

    //Initialize member array data
    int initialMemMembers = 8;
    {
        assert(initialMemMembers > 0);
        dec->memMembers = initialMemMembers;
        dec->numMembers = 0;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->members, (size_t) dec->memMembers));
    }

    //Initialize node array data
    int initialMemNodes = 8;
    {
        assert(initialMemNodes > 0);
        dec->memNodes = initialMemNodes;
        dec->numNodes = 0;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->nodes, (size_t) dec->memNodes));
    }

    //Initialize mappings for rows
    {
        dec->memRows = numRows;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->rowEdges, (size_t) dec->memRows));
        for (int i = 0; i < dec->memRows; ++i) {
            dec->rowEdges[i] = INVALID_EDGE;
        }
    }
    //Initialize mappings for columns
    {
        dec->memColumns = numColumns;
        dec->numColumns = 0;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->columnEdges, (size_t) dec->memColumns));
        for (int i = 0; i < dec->memColumns; ++i) {
            dec->columnEdges[i] = INVALID_EDGE;
        }
    }

    dec->numConnectedComponents = 0;
    return SPQR_OKAY;
}

void freeDecomposition(Decomposition **pDec) {
    assert(pDec);
    assert(*pDec);

    Decomposition *dec = *pDec;
    SPQRfreeBlockArray(dec->env, &dec->columnEdges);
    SPQRfreeBlockArray(dec->env, &dec->rowEdges);
    SPQRfreeBlockArray(dec->env, &dec->nodes);

    SPQRfreeBlockArray(dec->env, &dec->members);
    SPQRfreeBlockArray(dec->env, &dec->edges);



    SPQRfreeBlock(dec->env, pDec);

}

SPQR_ERROR createEdge(Decomposition *dec, member_id member, edge_id *pEdge) {
    assert(dec);
    assert(pEdge);
    assert(memberIsInvalid(member) || memberIsRepresentative(dec, member));

    edge_id index = dec->firstFreeEdge;
    if (edgeIsValid(index)) {
        dec->firstFreeEdge = dec->edges[index].edgeListNode.next;
    } else {
        //Enlarge array, no free nodes in edge list
        int newSize = 2 * dec->memEdges;
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &dec->edges, (size_t) newSize));
        for (int i = dec->memEdges + 1; i < newSize; ++i) {
            dec->edges[i].edgeListNode.next = i + 1;
            dec->edges[i].member = INVALID_MEMBER;
        }
        dec->edges[newSize - 1].edgeListNode.next = INVALID_EDGE;
        dec->firstFreeEdge = dec->memEdges + 1;
        index = dec->memEdges;
        dec->memEdges = newSize;
    }
    //TODO: Is defaulting these here necessary?
    dec->edges[index].tail = INVALID_NODE;
    dec->edges[index].head = INVALID_NODE;
    dec->edges[index].member = member;
    dec->edges[index].childMember = INVALID_MEMBER;

    dec->edges[index].headEdgeListNode.next = INVALID_EDGE;
    dec->edges[index].headEdgeListNode.previous = INVALID_EDGE;
    dec->edges[index].tailEdgeListNode.next = INVALID_EDGE;
    dec->edges[index].tailEdgeListNode.previous = INVALID_EDGE;

    dec->numEdges++;

    *pEdge = index;

    return SPQR_OKAY;
}
SPQR_ERROR createRowEdge(Decomposition *dec, member_id member, edge_id *pEdge, row_idx row){
    SPQR_CALL(createEdge(dec,member,pEdge));
    setDecompositionRowEdge(dec,row,*pEdge);
    addEdgeToMemberEdgeList(dec,*pEdge,member);
    dec->edges[*pEdge].element = rowToElement(row);

    return SPQR_OKAY;
}
SPQR_ERROR createColumnEdge(Decomposition *dec, member_id member, edge_id *pEdge, col_idx column){
    SPQR_CALL(createEdge(dec,member,pEdge));
    setDecompositionColumnEdge(dec,column,*pEdge);
    addEdgeToMemberEdgeList(dec,*pEdge,member);
    dec->edges[*pEdge].element = columnToElement(column);

    return SPQR_OKAY;
}
SPQR_ERROR createMember(Decomposition *dec, MemberType type, member_id * pMember){
    assert(dec);
    assert(pMember);

    if(dec->numMembers == dec->memMembers){
        dec->memMembers *= 2;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&dec->members,(size_t) dec->memMembers));
    }
    DecompositionMemberData *data = &dec->members[dec->numMembers];
    data->markerOfParent = INVALID_EDGE;
    data->markerToParent = INVALID_EDGE;
    data->firstEdge = INVALID_EDGE;
    data->representativeMember = INVALID_MEMBER;
    data->num_edges = 0;
    data->parentMember = INVALID_MEMBER;
    data->type = type;

    *pMember = dec->numMembers;

    dec->numMembers++;
    return SPQR_OKAY;
}

SPQR_ERROR createNode(Decomposition *dec, node_id * pNode){

    if(dec->numNodes == dec->memNodes){
        dec->memNodes*=2;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&dec->nodes,(size_t) dec->memNodes));
    }
    *pNode = dec->numNodes;
    dec->nodes[dec->numNodes].representativeNode = INVALID_NODE;
    dec->nodes[dec->numNodes].firstEdge = INVALID_EDGE;
    dec->nodes[dec->numNodes].numEdges = 0;
    dec->numNodes++;

    return SPQR_OKAY;
}
void removeEdgeFromNodeEdgeList(Decomposition *dec, edge_id edge, node_id node, bool nodeIsHead){
    EdgeListNode * edgeListNode = nodeIsHead ? &dec->edges[edge].headEdgeListNode : &dec->edges[edge].tailEdgeListNode;

    if(dec->nodes[node].numEdges == 1){
        dec->nodes[node].firstEdge = INVALID_EDGE;
    }else{
        edge_id next_edge = edgeListNode->next;
        edge_id prev_edge = edgeListNode->previous;
        EdgeListNode * nextListNode = findEdgeHead(dec,next_edge) == node ?  &dec->edges[next_edge].headEdgeListNode : &dec->edges[next_edge].tailEdgeListNode;//TODO: finds necessary?
        EdgeListNode * prevListNode = findEdgeHead(dec,prev_edge) == node ?  &dec->edges[prev_edge].headEdgeListNode : &dec->edges[prev_edge].tailEdgeListNode;//TODO: finds necessary?

        nextListNode->previous = prev_edge;
        prevListNode->next = next_edge;

        if(dec->nodes[node].firstEdge == edge){
            dec->nodes[node].firstEdge = next_edge; //TODO: fix this if we want fixed ordering for tree/nontree edges in memory
        }
    }
    //TODO: empty edgeListNode's data? Might not be all that relevant
    --(dec->nodes[node].numEdges);
}
void addEdgeToNodeEdgeList(Decomposition *dec, edge_id edge, node_id node, bool nodeIsHead){
    assert(nodeIsRepresentative(dec,node));

    edge_id firstNodeEdge = getFirstNodeEdge(dec,node);

    EdgeListNode * edgeListNode = nodeIsHead ? &dec->edges[edge].headEdgeListNode : &dec->edges[edge].tailEdgeListNode;
    if(edgeIsValid(firstNodeEdge)){
        bool nextIsHead = findEdgeHead(dec,firstNodeEdge) == node;
        EdgeListNode *nextListNode = nextIsHead ? &dec->edges[firstNodeEdge].headEdgeListNode : &dec->edges[firstNodeEdge].tailEdgeListNode;
        edge_id lastNodeEdge = nextListNode->previous;

        edgeListNode->next = firstNodeEdge;
        edgeListNode->previous = lastNodeEdge;


        bool previousIsHead = findEdgeHead(dec,lastNodeEdge) == node;
        EdgeListNode *previousListNode = previousIsHead ? &dec->edges[lastNodeEdge].headEdgeListNode : &dec->edges[lastNodeEdge].tailEdgeListNode;
        previousListNode->next = edge;
        nextListNode->previous = edge;

    }else{
        edgeListNode->next = edge;
        edgeListNode->previous = edge;
    }
    dec->nodes[node].firstEdge = edge; //TODO: update this in case of row/column edges to make memory ordering nicer?er?
    ++dec->nodes[node].numEdges;
    if(nodeIsHead){
        dec->edges[edge].head = node;
    }else{
        dec->edges[edge].tail = node;
    }
}
void setEdgeHeadAndTail(Decomposition *dec, edge_id edge, node_id head, node_id tail){
    addEdgeToNodeEdgeList(dec,edge,head,true);
    addEdgeToNodeEdgeList(dec,edge,tail,false);
}
void clearEdgeHeadAndTail(Decomposition *dec, edge_id edge){
    removeEdgeFromNodeEdgeList(dec,edge,findEdgeHead(dec,edge),true);
    removeEdgeFromNodeEdgeList(dec,edge,findEdgeTail(dec,edge),false);
    dec->edges[edge].head = INVALID_NODE;
    dec->edges[edge].tail = INVALID_NODE;
}
void changeEdgeHead(Decomposition *dec, edge_id edge,node_id oldHead, node_id newHead){
    assert(nodeIsRepresentative(dec,oldHead));
    assert(nodeIsRepresentative(dec,newHead));
    removeEdgeFromNodeEdgeList(dec,edge,oldHead,true);
    addEdgeToNodeEdgeList(dec,edge,newHead,true);
}
void changeEdgeTail(Decomposition *dec, edge_id edge,node_id oldTail, node_id newTail){
    assert(nodeIsRepresentative(dec,oldTail));
    assert(nodeIsRepresentative(dec,newTail));
    removeEdgeFromNodeEdgeList(dec,edge,oldTail,false);
    addEdgeToNodeEdgeList(dec,edge,newTail,false);
}
void flipEdge(Decomposition *dec, edge_id edge){
    swap_ints(&dec->edges[edge].head,&dec->edges[edge].tail);

    EdgeListNode temp = dec->edges[edge].headEdgeListNode;
    dec->edges[edge].headEdgeListNode = dec->edges[edge].tailEdgeListNode;
    dec->edges[edge].tailEdgeListNode = temp;

}
int nodeDegree(Decomposition *dec, node_id node){
    assert(dec);
    assert(nodeIsValid(node));
    assert(node < dec->memNodes);
    return dec->nodes[node].numEdges;
}
MemberType getMemberType(const Decomposition *dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].type;
}
void updateMemberType(const Decomposition *dec, member_id member, MemberType type){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));

    dec->members[member].type = type;
}
edge_id markerToParent(const Decomposition *dec,member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].markerToParent;
}
char typeToChar(MemberType type){
    switch (type) {
        case RIGID:
            return 'R';
        case PARALLEL:
            return 'P';
        case SERIES:
            return 'S';

        default:
            return '?';
    }
}
static void edgeToDot(FILE * stream, const Decomposition * dec,
                      edge_id edge, unsigned long dot_head, unsigned long dot_tail,bool useElementNames){
    assert(edgeIsValid(edge));
    member_id member = findEdgeMemberNoCompression(dec,edge);
    MemberType member_type = getMemberType(dec,member);
    char type = typeToChar(member_type);
    const char* color = edgeIsTree(dec,edge) ? ",color=red" :",color=blue";

    int edge_name = edge;

    if(markerToParent(dec,member) == edge){
        if(useElementNames){
            edge_name = -1;
        }
        fprintf(stream, "    %c_%d_%lu -> %c_p_%d [label=\"%d\",style=dashed%s];\n", type, member, dot_head, type, member, edge_name, color);
        fprintf(stream, "    %c_p_%d -> %c_%d_%lu [label=\"%d\",style=dashed%s];\n", type, member, type, member, dot_tail, edge_name, color);
        fprintf(stream, "    %c_%d_%lu [shape=box];\n", type, member, dot_head);
        fprintf(stream, "    %c_%d_%lu [shape=box];\n", type, member, dot_tail);
        fprintf(stream, "    %c_p_%d [style=dashed];\n", type, member);
    }else if(edgeIsMarker(dec,edge)){
        member_id child = findEdgeChildMemberNoCompression(dec,edge);
        char childType = typeToChar(getMemberType(dec,child));
        if(useElementNames){
            edge_name = -1;
        }
        fprintf(stream, "    %c_%d_%lu -> %c_c_%d [label=\"%d\",style=dotted%s];\n", type, member, dot_head, type, child, edge_name, color);
        fprintf(stream, "    %c_c_%d -> %c_%d_%lu [label=\"%d\",style=dotted%s];\n", type, child, type, member, dot_tail, edge_name, color);
        fprintf(stream, "    %c_%d_%lu [shape=box];\n", type, member, dot_head);
        fprintf(stream, "    %c_%d_%lu [shape=box];\n", type, member, dot_tail);
        fprintf(stream, "    %c_c_%d [style=dotted];\n", type, child);
        fprintf(stream, "    %c_p_%d -> %c_c_%d [style=dashed,dir=forward];\n", childType, child, type, child);
    }else{
        if(useElementNames){
            spqr_element element = dec->edges[edge].element;
            if(elementIsRow(element)){
                edge_name = (int) elementToRow(element);
            }else{
                edge_name = (int) elementToColumn(element);
            }
        }

        fprintf(stream, "    %c_%d_%lu -> %c_%d_%lu [label=\"%d \",style=bold%s];\n", type, member, dot_head, type, member, dot_tail,
                edge_name, color);
        fprintf(stream, "    %c_%d_%lu [shape=box];\n", type, member, dot_head);
        fprintf(stream, "    %c_%d_%lu [shape=box];\n", type, member, dot_tail);
    }
}

void decompositionToDot(FILE * stream, const Decomposition *dec, bool useElementNames ){
    fprintf(stream, "//decomposition\ndigraph decomposition{\n   compound = true;\n");
    for (member_id member = 0; member < dec->numMembers; ++member){
        if(!memberIsRepresentative(dec,member)) continue;
        fprintf(stream,"   subgraph member_%d{\n",member);
        switch(getMemberType(dec,member)){
            case RIGID:
            {
                edge_id first_edge = getFirstMemberEdge(dec,member);
                edge_id edge = first_edge;
                do
                {
                    unsigned long edgeHead = (unsigned long) findEdgeHeadNoCompression(dec,edge);
                    unsigned long edgeTail = (unsigned long) findEdgeTailNoCompression(dec,edge);
                    edgeToDot(stream,dec,edge,edgeHead,edgeTail,useElementNames);
                    edge = getNextMemberEdge(dec,edge);
                }
                while(edge != first_edge);
                break;
            }
            case PARALLEL:
            {
                edge_id first_edge = getFirstMemberEdge(dec,member);
                edge_id edge = first_edge;
                do
                {
                    edgeToDot(stream,dec,edge,0,1,useElementNames);
                    edge = getNextMemberEdge(dec,edge);
                }
                while(edge != first_edge);
                break;
            }
            case SERIES:
                {
                unsigned long i = 0;
                unsigned long num_member_edges = (unsigned long) getNumMemberEdges(dec, member);
                edge_id first_edge = getFirstMemberEdge(dec, member);
                edge_id edge = first_edge;
                do {
                    edgeToDot(stream, dec, edge, i, (i + 1) % num_member_edges,useElementNames);
                    edge = getNextMemberEdge(dec, edge);
                    i++;
                } while (edge != first_edge);
                break;
                }
            case UNASSIGNED:
                break;
        }
        fprintf(stream,"   }\n");
    }
    fprintf(stream,"}\n");
}


void updateMemberParentInformation(Decomposition *dec, const member_id newMember,const member_id toRemove){
    assert(memberIsRepresentative(dec,newMember));
    assert(findMemberNoCompression(dec,toRemove) == newMember);

    dec->members[newMember].markerOfParent = dec->members[toRemove].markerOfParent;
    dec->members[newMember].markerToParent = dec->members[toRemove].markerToParent;
    dec->members[newMember].parentMember = dec->members[toRemove].parentMember;

    dec->members[toRemove].markerOfParent = INVALID_EDGE;
    dec->members[toRemove].markerToParent = INVALID_EDGE;
    dec->members[toRemove].parentMember = INVALID_MEMBER;
}
edge_id markerOfParent(const Decomposition *dec, member_id member) {
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].markerOfParent;
}

edge_id getFirstMemberEdge(const Decomposition * dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    return dec->members[member].firstEdge;
}
edge_id getNextMemberEdge(const Decomposition * dec,edge_id edge){
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);
    edge = dec->edges[edge].edgeListNode.next;
    return edge;
}
int getNumMemberEdges(const Decomposition * dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].num_edges;
}

int getNumNodes(const Decomposition *dec){
    assert(dec);
    return dec->numNodes;
}
int getNumMembers(const Decomposition *dec){
    assert(dec);
    return dec->numMembers;
}
edge_id getPreviousMemberEdge(const Decomposition *dec, edge_id edge){
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);
    edge = dec->edges[edge].edgeListNode.previous;
    return edge;
}
SPQR_ERROR createStandaloneParallel(Decomposition *dec, col_idx * columns, int num_columns, row_idx row, member_id * pMember){
    //TODO: fix mappings
    member_id member;
    SPQR_CALL(createMember(dec,PARALLEL,&member));

    edge_id row_edge;
    SPQR_CALL(createRowEdge(dec,member,&row_edge,row));

    edge_id col_edge;
    for (int i = 0; i < num_columns; ++i) {
        SPQR_CALL(createColumnEdge(dec,member,&col_edge,columns[i]));
    }
    *pMember = member;

    ++dec->numConnectedComponents;
    //TODO: double check that this function is not used to create connected parallel (We could also compute this quantity by tracking # of representative members - number of marker pairs
    return SPQR_OKAY;
}

//TODO: fix tracking connectivity more cleanly, should not be left up to the algorithms ideally
SPQR_ERROR createConnectedParallel(Decomposition *dec, col_idx * columns, int num_columns, row_idx row, member_id * pMember){
    member_id member;
    SPQR_CALL(createMember(dec,PARALLEL,&member));

    edge_id row_edge;
    SPQR_CALL(createRowEdge(dec,member,&row_edge,row));

    edge_id col_edge;
    for (int i = 0; i < num_columns; ++i) {
        SPQR_CALL(createColumnEdge(dec,member,&col_edge,columns[i]));
    }
    *pMember = member;

    return SPQR_OKAY;
}

SPQR_ERROR createStandaloneSeries(Decomposition *dec, row_idx * rows, int numRows, col_idx col, member_id * pMember){
    member_id member;
    SPQR_CALL(createMember(dec,SERIES,&member));

    edge_id colEdge;
    SPQR_CALL(createColumnEdge(dec,member,&colEdge,col));

    edge_id rowEdge;
    for (int i = 0; i < numRows; ++i) {
        SPQR_CALL(createRowEdge(dec,member,&rowEdge,rows[i]));
    }
    *pMember = member;
    ++dec->numConnectedComponents;
    return SPQR_OKAY;
}
SPQR_ERROR createConnectedSeries(Decomposition *dec, row_idx * rows, int numRows, col_idx col, member_id * pMember){
    member_id member;
    SPQR_CALL(createMember(dec,SERIES,&member));

    edge_id colEdge;
    SPQR_CALL(createColumnEdge(dec,member,&colEdge,col));

    edge_id rowEdge;
    for (int i = 0; i < numRows; ++i) {
        SPQR_CALL(createRowEdge(dec,member,&rowEdge,rows[i]));
    }
    *pMember = member;
    return SPQR_OKAY;
}
void addEdgeToMemberEdgeList(Decomposition *dec, edge_id edge, member_id member){
    edge_id firstMemberEdge = getFirstMemberEdge(dec,member);

    if(edgeIsValid(firstMemberEdge)){
        edge_id lastMemberEdge = getPreviousMemberEdge(dec,firstMemberEdge);
        dec->edges[edge].edgeListNode.next = firstMemberEdge;
        dec->edges[edge].edgeListNode.previous = lastMemberEdge;
        dec->edges[firstMemberEdge].edgeListNode.previous = edge;
        dec->edges[lastMemberEdge].edgeListNode.next = edge;
    }else{
        assert(dec->members[member].num_edges == 0);
        dec->edges[edge].edgeListNode.next = edge;
        dec->edges[edge].edgeListNode.previous = edge;
    }
    dec->members[member].firstEdge = edge;//TODO: update this in case of row/column edges to make memory ordering nicer?
    ++(dec->members[member].num_edges);
}

void removeEdgeFromMemberEdgeList(Decomposition *dec, edge_id edge, member_id member){
    assert(findEdgeMemberNoCompression(dec,edge) == member);
    assert(memberIsRepresentative(dec,member));

    if(dec->members[member].num_edges == 1){
        dec->members[member].firstEdge = INVALID_EDGE;

        //TODO: also set edgeListNode to invalid, maybe? Not necessary probably
    }else{
        edge_id nextEdge = dec->edges[edge].edgeListNode.next;
        edge_id prevEdge = dec->edges[edge].edgeListNode.previous;

        dec->edges[nextEdge].edgeListNode.previous = prevEdge;
        dec->edges[prevEdge].edgeListNode.next = nextEdge;

        if(dec->members[member].firstEdge == edge){
            dec->members[member].firstEdge = nextEdge; //TODO: fix this if we want fixed ordering for tree/nontree edges in memory
        }
    }


    --(dec->members[member].num_edges);
}

edge_id getFirstNodeEdge(const Decomposition * dec, node_id node){
    assert(dec);
    assert(nodeIsValid(node));
    assert(node < dec->memNodes);
    return dec->nodes[node].firstEdge;
}
edge_id getNextNodeEdge(Decomposition * dec, edge_id edge, node_id node){
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);
    assert(nodeIsRepresentative(dec,node));

    if(findEdgeHead(dec,edge) == node){
        edge = dec->edges[edge].headEdgeListNode.next;
    }else{
        assert(findEdgeTailNoCompression(dec,edge) == node);
        dec->edges[edge].tail = node; //This assignment is not necessary but speeds up future queries.
        edge = dec->edges[edge].tailEdgeListNode.next;
    }
    return edge;
}
edge_id getPreviousNodeEdge(Decomposition *dec, edge_id edge,node_id node){
    assert(dec);
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);
    assert(nodeIsRepresentative(dec,node));

    if(findEdgeHead(dec,edge) == node){
        edge = dec->edges[edge].headEdgeListNode.previous;
    }else{
        assert(findEdgeTailNoCompression(dec,edge) == node);
        dec->edges[edge].tail = node; //This assignment is not necessary but speeds up future queries.
        edge = dec->edges[edge].tailEdgeListNode.previous;
    }
    return edge;
}


void process_edge(row_idx * fundamental_cycle_edges, int * num_cycle_edges, edge_id * callStack, int * callStackSize, edge_id edge,const Decomposition * dec){
    assert(edgeIsTree(dec,edge));
    if(!edgeIsMarker(dec,edge)){
        member_id current_member = findEdgeMemberNoCompression(dec,edge);
        if(markerToParent(dec,current_member) == edge){
            edge_id other_edge = markerOfParent(dec,current_member);
            assert(!edgeIsTree(dec,other_edge));
            callStack[*callStackSize] = other_edge;
            ++(*callStackSize);
        }else{
            spqr_element element = edgeGetElement(dec,edge);
            assert(elementIsRow(element));
            fundamental_cycle_edges[*num_cycle_edges] = elementToRow(element);
            ++(*num_cycle_edges);
        }
    }else{
        member_id child_member = findEdgeChildMemberNoCompression(dec,edge);
        edge_id other_edge = markerToParent(dec,child_member);
        assert(!edgeIsTree(dec,other_edge));
        callStack[*callStackSize] = other_edge;
        ++(*callStackSize);
    }
}

int decompositionGetFundamentalCycleRows(Decomposition *dec, col_idx column, row_idx * output){
    edge_id edge = getDecompositionColumnEdge(dec, column);
    if(edgeIsInvalid(edge)){
        return 0;
    }
    int num_rows = 0;

    edge_id * callStack;
    //TODO: probably an overkill amount of memory allocated here... How can we allocate just enough?
    SPQR_ERROR result = SPQRallocBlockArray(dec->env,&callStack,(size_t) dec->memRows);
    if(result != SPQR_OKAY){
        return -1;
    }
    int callStackSize = 1;
    callStack[0] = edge;

    bool * nodeVisited;
    result = SPQRallocBlockArray(dec->env,&nodeVisited,(size_t) dec->numNodes);
    if(result != SPQR_OKAY){
        return -1;
    }
    for (int i = 0; i < dec->numNodes; ++i) {
        nodeVisited[i] = false;
    }

    typedef struct {
        node_id node;
        edge_id nodeEdge;
    } DFSCallData;
    DFSCallData * pathSearchCallStack;
    result = SPQRallocBlockArray(dec->env,&pathSearchCallStack,(size_t) dec->numNodes);
    if(result != SPQR_OKAY){
        return -1;
    }
    int pathSearchCallStackSize = 0;

    while(callStackSize > 0){
        edge_id column_edge = callStack[callStackSize-1];
        --callStackSize;
        member_id column_edge_member = findEdgeMemberNoCompression(dec,column_edge);
        switch(getMemberType(dec,column_edge_member)){
            case RIGID:
            {

                node_id source = findEdgeHeadNoCompression(dec,column_edge);
                node_id target = findEdgeTailNoCompression(dec,column_edge);

                assert(pathSearchCallStackSize == 0);
                pathSearchCallStack[0].node = source;
                pathSearchCallStack[0].nodeEdge = getFirstNodeEdge(dec,source);
                pathSearchCallStackSize++;
                while(pathSearchCallStackSize > 0){
                    DFSCallData * dfsData  = &pathSearchCallStack[pathSearchCallStackSize-1];
                    nodeVisited[dfsData->node] = true;
                    //cannot be a tree edge which is its parent
                    if(edgeIsTree(dec,dfsData->nodeEdge) &&
                    (pathSearchCallStackSize <= 1 || dfsData->nodeEdge != pathSearchCallStack[pathSearchCallStackSize-2].nodeEdge)){
                        node_id head = findEdgeHeadNoCompression(dec,dfsData->nodeEdge);
                        node_id tail = findEdgeTailNoCompression(dec,dfsData->nodeEdge);
                        node_id other = head == dfsData->node ? tail : head;
                        assert(other != dfsData->node);
                        assert(!nodeVisited[other]);
                        if(other == target){
                            break;
                        }
                        //We go up a level: add new node to the call stack

                        pathSearchCallStack[pathSearchCallStackSize].node = other;
                        pathSearchCallStack[pathSearchCallStackSize].nodeEdge = getFirstNodeEdge(dec,other);
                        ++pathSearchCallStackSize;
                        continue;
                    }
                    do{
                        dfsData->nodeEdge = getNextNodeEdge(dec,dfsData->nodeEdge,dfsData->node);
                        if(dfsData->nodeEdge == getFirstNodeEdge(dec,dfsData->node)){
                            --pathSearchCallStackSize;
                            dfsData = &pathSearchCallStack[pathSearchCallStackSize-1];
                        }else{
                            break;
                        }
                    }while(pathSearchCallStackSize > 0);
                }
                for (int i = 0; i < pathSearchCallStackSize; ++i) {
                    if(edgeIsTree(dec,pathSearchCallStack[i].nodeEdge)){
                        process_edge(output,&num_rows,callStack,&callStackSize,pathSearchCallStack[i].nodeEdge,dec);
                    }
                }

                pathSearchCallStackSize = 0;
                break;
            }

            case PARALLEL:
            {
                edge_id first_edge = getFirstMemberEdge(dec,column_edge_member);
                edge_id iter_edge = first_edge;
                int tree_count = 0;
                do
                {
                    if(edgeIsTree(dec,iter_edge)){
                        process_edge(output,&num_rows,callStack,&callStackSize,iter_edge,dec);
                        tree_count++;
                    }
                    iter_edge = getNextMemberEdge(dec,iter_edge);
                }
                while(iter_edge != first_edge);
                if(tree_count != 1){
                    return -1;
                }
                break;
            }
            case SERIES:
            {
                edge_id first_edge = getFirstMemberEdge(dec,column_edge_member);
                edge_id iter_edge = first_edge;
                int nontree_count = 0;
                do
                {
                    if(edgeIsTree(dec,iter_edge)){
                        process_edge(output,&num_rows,callStack,&callStackSize,iter_edge,dec);
                    }else{
                        nontree_count++;
                    }
                    iter_edge = getNextMemberEdge(dec,iter_edge);
                }
                while(iter_edge != first_edge);
                if(nontree_count != 1){
                    return -1;
                }
                break;
            }
            case UNASSIGNED:
                assert(false);
        }
    }
    SPQRfreeBlockArray(dec->env,&pathSearchCallStack);
    SPQRfreeBlockArray(dec->env,&nodeVisited);
    SPQRfreeBlockArray(dec->env,&callStack);
    return num_rows;
}

int qsort_integer_comparison (const void * a, const void * b)
{
    int *s1 = (int *)a;
    int *s2 = (int *)b;

    if(*s1 > *s2) {
        return 1;
    }
    else if(*s1 == *s2) {
        return 0;
    }
    else {
        return -1;
    }
}

bool checkCorrectnessColumn(Decomposition * dec, col_idx column, row_idx * column_rows, int num_rows, row_idx * computed_column_storage){
    int num_found_rows = decompositionGetFundamentalCycleRows(dec,column,computed_column_storage);

    if(num_found_rows != num_rows){
        return false;
    }
    if(num_rows == 0){
        return true;
    }
    qsort(computed_column_storage,(size_t) num_rows,sizeof(row_idx),qsort_integer_comparison);
    qsort(column_rows            ,(size_t) num_rows,sizeof(row_idx),qsort_integer_comparison);

    for (int i = 0; i < num_rows; ++i) {
        if(column_rows[i] != computed_column_storage[i]){
            return false;
        }
    }
    return true;
}

member_id largestMemberID(const Decomposition *dec){
    return dec->numMembers;
}
edge_id largestEdgeID(const Decomposition *dec){
    return dec->numEdges;
}
node_id largestNodeID(const Decomposition *dec){
    return dec->numNodes;
}
int numConnectedComponents(const Decomposition *dec){
    return dec->numConnectedComponents;
}
SPQR_ERROR createChildMarker(Decomposition *dec, member_id member, member_id child, bool isTree, edge_id * pEdge){
    SPQR_CALL(createEdge(dec,member,pEdge));
    dec->edges[*pEdge].element = isTree ? MARKER_ROW_ELEMENT : MARKER_COLUMN_ELEMENT;
    dec->edges[*pEdge].childMember = child;

    addEdgeToMemberEdgeList(dec,*pEdge,member);
    return SPQR_OKAY;
}
SPQR_ERROR createParentMarker(Decomposition *dec, member_id member, bool isTree, member_id parent, member_id parentMarker
,edge_id * edge){

    SPQR_CALL(createEdge(dec,member,edge));
    dec->edges[*edge].element = isTree ? MARKER_ROW_ELEMENT : MARKER_COLUMN_ELEMENT;

    addEdgeToMemberEdgeList(dec,*edge,member);

    dec->members[member].parentMember = parent;
    dec->members[member].markerOfParent = parentMarker;
    dec->members[member].markerToParent = *edge;
    return SPQR_OKAY;
}
SPQR_ERROR createMarkerPair(Decomposition *dec, member_id parentMember, member_id childMember, bool parentIsTree){
    edge_id parentToChildMarker = INVALID_EDGE;
    SPQR_CALL(createChildMarker(dec,parentMember,childMember,parentIsTree,&parentToChildMarker));

    edge_id childToParentMarker = INVALID_EDGE;
    SPQR_CALL(createParentMarker(dec,childMember,!parentIsTree,parentMember,parentToChildMarker,&childToParentMarker));

    return SPQR_OKAY;
}
SPQR_ERROR createMarkerPairWithReferences(Decomposition *dec, member_id parentMember, member_id childMember, bool parentIsTree,
                            edge_id * parentToChild, edge_id *childToParent){
    SPQR_CALL(createChildMarker(dec,parentMember,childMember,parentIsTree,parentToChild));
    SPQR_CALL(createParentMarker(dec,childMember,!parentIsTree,parentMember,*parentToChild,childToParent));

    return SPQR_OKAY;
}

void moveEdgeToNewMember(Decomposition *dec, edge_id edge, member_id oldMember, member_id newMember){
    assert(edgeIsValid(edge));
    assert(edge < dec->memEdges);
    assert(dec);

    assert(memberIsRepresentative(dec,oldMember));
    assert(memberIsRepresentative(dec,newMember));
    //Need to change the edge's member, remove it from the current member list and add it to the new member list
    assert(findEdgeMemberNoCompression(dec,edge) == oldMember);

    removeEdgeFromMemberEdgeList(dec,edge,oldMember);
    addEdgeToMemberEdgeList(dec,edge,newMember);

    dec->edges[edge].member = newMember;

    //If this edge has a childMember, update the information correctly!
    member_id childMember = dec->edges[edge].childMember;
    if(memberIsValid(childMember)){
        member_id childRepresentative = findEdgeChildMember(dec,edge);
        dec->members[childRepresentative].parentMember = newMember;
    }
    //If this edge is a marker to the parent, update the child edge marker of the parent to reflect the move
    if(dec->members[oldMember].markerToParent == edge){
        dec->members[newMember].markerToParent = edge;
        dec->members[newMember].parentMember = dec->members[oldMember].parentMember;
        dec->members[newMember].markerOfParent = dec->members[oldMember].markerOfParent;

        assert(findEdgeChildMemberNoCompression(dec,dec->members[oldMember].markerOfParent) == oldMember);
        dec->edges[dec->members[oldMember].markerOfParent].childMember = newMember;
    }
}
void mergeMemberEdgeList(Decomposition *dec, member_id toMergeInto, member_id toRemove){
    edge_id firstIntoEdge = getFirstMemberEdge(dec,toMergeInto);
    edge_id firstFromEdge = getFirstMemberEdge(dec,toRemove);
    assert(edgeIsValid(firstIntoEdge));
    assert(edgeIsValid(firstFromEdge));

    edge_id lastIntoEdge = getPreviousMemberEdge(dec,firstIntoEdge);
    edge_id lastFromEdge = getPreviousMemberEdge(dec,firstFromEdge);

    //Relink linked lists to merge them effectively
    dec->edges[firstIntoEdge].edgeListNode.previous = lastFromEdge;
    dec->edges[lastIntoEdge].edgeListNode.next = firstFromEdge;
    dec->edges[firstFromEdge].edgeListNode.previous = lastIntoEdge;
    dec->edges[lastFromEdge].edgeListNode.next = firstIntoEdge;

    //Clean up old
    dec->members[toMergeInto].num_edges += dec->members[toRemove].num_edges;
    dec->members[toRemove].num_edges = 0;
    dec->members[toRemove].firstEdge = INVALID_EDGE;

}
void mergeNodeEdgeList(Decomposition *dec, node_id toMergeInto, node_id toRemove){

    edge_id firstIntoEdge = getFirstNodeEdge(dec,toMergeInto);
    edge_id firstFromEdge = getFirstNodeEdge(dec,toRemove);
    if(edgeIsInvalid(firstIntoEdge)){
        //new node has no edges
        dec->nodes[toMergeInto].numEdges += dec->nodes[toRemove].numEdges;
        dec->nodes[toRemove].numEdges = 0;

        dec->nodes[toMergeInto].firstEdge = dec->nodes[toRemove].firstEdge;
        dec->nodes[toRemove].firstEdge = INVALID_EDGE;

        return;
    }else if (edgeIsInvalid(firstFromEdge)){
        //Old node has no edges; we can just return
        return;
    }

    edge_id lastIntoEdge = getPreviousNodeEdge(dec,firstIntoEdge,toMergeInto);
    assert(edgeIsValid(lastIntoEdge));
    edge_id lastFromEdge = getPreviousNodeEdge(dec,firstFromEdge,toRemove);
    assert(edgeIsValid(lastFromEdge));


    EdgeListNode * firstIntoNode = findEdgeHead(dec,firstIntoEdge) == toMergeInto ?
                                   &dec->edges[firstIntoEdge].headEdgeListNode :
                                   &dec->edges[firstIntoEdge].tailEdgeListNode;
    EdgeListNode * lastIntoNode = findEdgeHead(dec,lastIntoEdge) == toMergeInto ?
                                &dec->edges[lastIntoEdge].headEdgeListNode :
                                &dec->edges[lastIntoEdge].tailEdgeListNode;

    EdgeListNode * firstFromNode = findEdgeHead(dec,firstFromEdge) == toRemove ?
                                   &dec->edges[firstFromEdge].headEdgeListNode :
                                   &dec->edges[firstFromEdge].tailEdgeListNode;
    EdgeListNode * lastFromNode = findEdgeHead(dec,lastFromEdge) == toRemove ?
                                  &dec->edges[lastFromEdge].headEdgeListNode :
                                  &dec->edges[lastFromEdge].tailEdgeListNode;

    firstIntoNode->previous = lastFromEdge;
    lastIntoNode->next = firstFromEdge;
    firstFromNode->previous = lastIntoEdge;
    lastFromNode->next = firstIntoEdge;

    dec->nodes[toMergeInto].numEdges += dec->nodes[toRemove].numEdges;
    dec->nodes[toRemove].numEdges = 0;
    dec->nodes[toRemove].firstEdge = INVALID_EDGE;
}
void changeLoopToSeries(Decomposition * dec, member_id member){
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(dec);
    assert((getMemberType(dec,member) == PARALLEL || getMemberType(dec,member) == SERIES) && getNumMemberEdges(dec,member) == 2);
    assert(memberIsRepresentative(dec,member));
    dec->members[member].type = SERIES;
}
void changeLoopToParallel(Decomposition * dec, member_id member){
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(dec);
    assert((getMemberType(dec,member) == PARALLEL || getMemberType(dec,member) == SERIES) && getNumMemberEdges(dec,member) == 2);
    assert(memberIsRepresentative(dec,member));
    dec->members[member].type = PARALLEL;
}
bool checkDecompositionMinimal(Decomposition * dec){
    //Relies on parents/children etc. being set correctly in the tree
    bool isMinimal = true;
    for (member_id member = 0; member < dec->numMembers; ++member) {
        if (!memberIsRepresentative(dec, member) || getMemberType(dec,member) == UNASSIGNED ) continue; //TODO: fix loop making INVALID members here... this is not a pretty way to do this
        member_id memberParent = findMemberParentNoCompression(dec,member);
        if(memberIsValid(memberParent)){
            MemberType memberType = getMemberType(dec,member);
            MemberType parentType = getMemberType(dec,memberParent);
            if(memberType == parentType && memberType != RIGID){
                isMinimal = false;
                break;
            }
        }

    }
    return isMinimal;
}

void mergeLoop(Decomposition * dec,member_id loopMember){
    assert(getMemberType(dec,loopMember) == SERIES || getMemberType(dec,loopMember) == PARALLEL);
    assert(getNumMemberEdges(dec,loopMember) == 2);
    assert(memberIsRepresentative(dec,loopMember));

#ifndef NDEBUG
    {
        int num_markers = 0;
        edge_id first_edge = getFirstMemberEdge(dec,loopMember);
        edge_id edge = first_edge;
        do {
            if(edgeIsMarker(dec,edge) || (markerToParent(dec,loopMember) == edge)){
                num_markers++;
            }
            edge = getNextMemberEdge(dec, edge);
        } while (edge != first_edge);
        assert(num_markers == 2);
    };
#endif

    member_id childMember = INVALID_MEMBER;
    {
        edge_id first_edge = getFirstMemberEdge(dec, loopMember);
        edge_id edge = first_edge;
        do {
            if (edgeIsMarker(dec, edge)) {
                childMember = findEdgeChildMember(dec, edge);
                break;
            }
            edge = getNextMemberEdge(dec, edge);
        } while (edge != first_edge);
    };
    assert(memberIsValid(childMember));

    member_id loopParentMember = dec->members[loopMember].parentMember;
    member_id loopParentMarkerToLoop = dec->members[loopMember].markerOfParent;


    assert(memberIsValid(loopParentMember));

    edge_id loopChildEdge = dec->members[childMember].markerOfParent;

    dec->members[childMember].markerOfParent = loopParentMarkerToLoop;
    dec->members[childMember].parentMember = loopParentMember;
    dec->edges[loopParentMarkerToLoop].childMember = childMember;

    //TODO: clean up the loopMember
    removeEdgeFromMemberEdgeList(dec,loopChildEdge,loopMember);
    removeEdgeFromMemberEdgeList(dec,dec->members[loopMember].markerToParent,loopMember);
    dec->members[loopMember].type = UNASSIGNED;
    //TODO: probably just use 'merge' functionality twice here instead
}

void decreaseNumConnectedComponents(Decomposition *dec, int by){
    dec->numConnectedComponents-= by;
    assert(dec->numConnectedComponents >= 1);
}

void reorderComponent(Decomposition *dec, member_id newRoot){
    assert(dec);
    assert(memberIsRepresentative(dec,newRoot));
    //If the newRoot has no parent, it is already the root, so then there's no need to reorder.
    if(memberIsValid(dec->members[newRoot].parentMember)){
        member_id member = findMemberParent(dec,newRoot);
        member_id newParent = newRoot;
        edge_id newMarkerToParent = dec->members[newRoot].markerOfParent;
        edge_id markerOfNewParent = dec->members[newRoot].markerToParent;

        //Recursively update the parent
        do{
            assert(memberIsValid(member));
            assert(memberIsValid(newParent));
            member_id oldParent = findMemberParent(dec, member);
            edge_id oldMarkerToParent = dec->members[member].markerToParent;
            edge_id oldMarkerOfParent = dec->members[member].markerOfParent;

            dec->members[member].markerToParent = newMarkerToParent;
            dec->members[member].markerOfParent = markerOfNewParent;
            dec->members[member].parentMember = newParent;
            dec->edges[markerOfNewParent].childMember = member;
            dec->edges[newMarkerToParent].childMember = -1;

            if (memberIsValid(oldParent)){
                newParent = member;
                member = oldParent;
                newMarkerToParent = oldMarkerOfParent;
                markerOfNewParent = oldMarkerToParent;
            }else{
                break;
            }
        }while(true);
    }
}
