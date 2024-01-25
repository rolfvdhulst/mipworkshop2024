//
// Created by rolf on 23-11-23.
//

#include "mipworkshop2024/presolve/NetworkDecomposition.h"


static void swap_ints(int *a, int *b) {
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

bool arcIsInvalid(arc_id arc) {
    return arc < 0;
}
bool arcIsValid(arc_id arc){
    return !arcIsInvalid(arc);
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

bool decompositionHasRow(Decomposition *dec, row_idx row){
    assert(dec);
    assert(rowIsValid(row));
    assert(row < dec->numRows);
    return arcIsValid(dec->rowArcs[row]);
}
void setDecompositionRowArc(Decomposition *dec, row_idx row, arc_id arc){
    assert(dec);
    assert(rowIsValid(row));
    assert(row < dec->numRows);
    dec->rowArcs[row] = arc;

}
arc_id getDecompositionRowArc(const Decomposition *dec, row_idx row){
    assert(dec);
    assert(rowIsValid(row));
    assert(row < dec->numRows);
    return dec->rowArcs[row];
}
bool decompositionHasCol(Decomposition *dec, col_idx col){
    assert(dec);
    assert(colIsValid(col));
    assert(col < dec->numColumns);
    return arcIsValid(dec->columnArcs[col]);
}
void setDecompositionColumnArc(Decomposition *dec, col_idx col, arc_id arc){
    assert(dec);
    assert(colIsValid(col));
    assert(col < dec->numColumns);
    dec->columnArcs[col] = arc;
}

arc_id getDecompositionColumnArc(const Decomposition *dec, col_idx col){
    assert(dec);
    assert(colIsValid(col));
    assert(col < dec->numColumns);
    return dec->columnArcs[col];
}

bool arcIsMarkerToChild(const Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    return memberIsValid(dec->arcs[arc].childMember);
}
bool arcIsTree(const Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    return elementIsRow(dec->arcs[arc].element);
}
spqr_element arcGetElement(const Decomposition * dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    return dec->arcs[arc].element;
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
    //first becomes representative; we merge all of the arcs of second into first
    mergeNodeArcList(dec,first,second);
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

node_id findArcTail(Decomposition *dec, arc_id arc) {
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);

    node_id representative = findNode(dec, dec->arcs[arc].tail);
    dec->arcs[arc].tail = representative; //update the pointer/index to point to the root

    return representative;
}

node_id findArcTailNoCompression(const Decomposition *dec, arc_id arc) {
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);

    node_id representative = findNodeNoCompression(dec, dec->arcs[arc].tail);
    return representative;
}

node_id findArcHead(Decomposition *dec, arc_id arc) {
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);

    node_id representative = findNode(dec, dec->arcs[arc].head);
    dec->arcs[arc].head = representative; //update the pointer/index to point to the root

    return representative;
}

node_id findArcHeadNoCompression(const Decomposition *dec, arc_id arc) {
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);

    node_id representative = findNodeNoCompression(dec, dec->arcs[arc].head);
    return representative;
}


arc_id getFirstMemberArc(const Decomposition * dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    return dec->members[member].firstArc;
}
arc_id getNextMemberArc(const Decomposition * dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    arc = dec->arcs[arc].arcListNode.next;
    return arc;
}
arc_id getPreviousMemberArc(const Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    arc = dec->arcs[arc].arcListNode.previous;
    return arc;
}
int getNumMemberArcs(const Decomposition * dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].numArcs;
}

arc_id getFirstNodeArc(const Decomposition * dec, node_id node){
    assert(dec);
    assert(nodeIsValid(node));
    assert(node < dec->memNodes);
    return dec->nodes[node].firstArc;
}
arc_id getNextNodeArc(Decomposition * dec, arc_id arc, node_id node){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    assert(nodeIsRepresentative(dec,node));

    if(findArcHead(dec,arc) == node){
        arc = dec->arcs[arc].headArcListNode.next;
    }else{
        assert(findArcTailNoCompression(dec,arc) == node);
        dec->arcs[arc].tail = node; //Speed up future queries
        arc = dec->arcs[arc].tailArcListNode.next;
    }
    return arc;
}
arc_id getPreviousNodeArc(Decomposition *dec, arc_id arc,node_id node){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    assert(nodeIsRepresentative(dec,node));

    if(findArcHead(dec,arc) == node){
        arc = dec->arcs[arc].headArcListNode.previous;
    }else{
        assert(findArcTailNoCompression(dec,arc) == node);
        dec->arcs[arc].tail = node; //This assignment is not necessary but speeds up future queries.
        arc = dec->arcs[arc].tailArcListNode.previous;
    }
    return arc;
}

int getNumNodes(const Decomposition *dec){
    assert(dec);
    return dec->numNodes;
}
int getNumMembers(const Decomposition *dec){
    assert(dec);
    return dec->numMembers;
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

member_id findArcChildMember(Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);

    member_id representative = findMember(dec, dec->arcs[arc].childMember);
    dec->arcs[arc].childMember = representative;
    return representative;
}
member_id findArcChildMemberNoCompression(const Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);

    member_id representative = findMemberNoCompression(dec, dec->arcs[arc].childMember);
    return representative;
}


MemberType getMemberType(const Decomposition *dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);

    return dec->members[member].type;
}
void updateMemberType(const Decomposition *dec, member_id member, MemberType type){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    dec->members[member].type = type;
}

member_id findArcMember(Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    return findMember(dec,dec->arcs[arc].member);
}
member_id findArcMemberNoCompression(const Decomposition *dec, arc_id arc){
    assert(dec);
    assert(arcIsValid(arc));
    assert(arc < dec->memArcs);
    return findMemberNoCompression(dec,dec->arcs[arc].member);
}

arc_id markerToParent(const Decomposition *dec, member_id member){
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].markerToParent;
}
arc_id markerOfParent(const Decomposition *dec, member_id member) {
    assert(dec);
    assert(memberIsValid(member));
    assert(member < dec->memMembers);
    assert(memberIsRepresentative(dec,member));
    return dec->members[member].markerOfParent;
}
char memberTypeToChar(MemberType type){
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

member_id largestMemberID(const Decomposition *dec){
    assert(dec);
    return dec->memMembers;
}
arc_id largestArcID(const Decomposition *dec){
    assert(dec);
    return dec->numArcs;
}
node_id largestNodeID(const Decomposition *dec){
    assert(dec);
    return dec->memNodes;
}
int numConnectedComponents(const Decomposition *dec){
    assert(dec);
    return dec->numConnectedComponents;
}

void updateMemberParentInformation(Decomposition *dec, const member_id newMember,const member_id toRemove){
    assert(dec);
    assert(memberIsRepresentative(dec,newMember));
    assert(findMemberNoCompression(dec,toRemove) == newMember);

    dec->members[newMember].markerOfParent = dec->members[toRemove].markerOfParent;
    dec->members[newMember].markerToParent = dec->members[toRemove].markerToParent;
    dec->members[newMember].parentMember = dec->members[toRemove].parentMember;

    dec->members[toRemove].markerOfParent = INVALID_ARC;
    dec->members[toRemove].markerToParent = INVALID_ARC;
    dec->members[toRemove].parentMember = INVALID_MEMBER;
}

SPQR_ERROR createDecomposition(SPQR * env, Decomposition ** pDecomposition, int numRows,int numColumns){
    assert(env);
    assert(pDecomposition);

    SPQR_CALL(SPQRallocBlock(env, pDecomposition));
    Decomposition *dec = *pDecomposition;
    dec->env = env;

    //Initialize arc array data
    int initialMemArcs = 8;
    {
        assert(initialMemArcs > 0);
        dec->memArcs = initialMemArcs;
        dec->numArcs = 0;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->arcs, (size_t) dec->memArcs));
        for (arc_id i = 0; i < dec->memArcs; ++i) {
            dec->arcs[i].arcListNode.next = i + 1;
            dec->arcs[i].member = INVALID_MEMBER;
        }
        dec->arcs[dec->memArcs - 1].arcListNode.next = INVALID_ARC;
        dec->firstFreeArc = 0;
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
        SPQR_CALL(SPQRallocBlockArray(env, &dec->rowArcs, (size_t) dec->memRows));
        for (int i = 0; i < dec->memRows; ++i) {
            dec->rowArcs[i] = INVALID_ARC;
        }
    }
    //Initialize mappings for columns
    {
        dec->memColumns = numColumns;
        dec->numColumns = 0;
        SPQR_CALL(SPQRallocBlockArray(env, &dec->columnArcs, (size_t) dec->memColumns));
        for (int i = 0; i < dec->memColumns; ++i) {
            dec->columnArcs[i] = INVALID_ARC;
        }
    }

    dec->numConnectedComponents = 0;
    return SPQR_OKAY;
}

void freeDecomposition(Decomposition ** pDec){
    assert(pDec);
    assert(*pDec);

    Decomposition *dec = *pDec;
    SPQRfreeBlockArray(dec->env, &dec->columnArcs);
    SPQRfreeBlockArray(dec->env, &dec->rowArcs);
    SPQRfreeBlockArray(dec->env, &dec->nodes);

    SPQRfreeBlockArray(dec->env, &dec->members);
    SPQRfreeBlockArray(dec->env, &dec->arcs);

    SPQRfreeBlock(dec->env, pDec);
}

SPQR_ERROR createArc(Decomposition *dec, member_id member, arc_id *pArc, bool reverse) {
    assert(dec);
    assert(pArc);
    assert(memberIsInvalid(member) || memberIsRepresentative(dec, member));

    arc_id index = dec->firstFreeArc;
    //Check if we have some allocated space available
    if (arcIsValid(index)) {
        dec->firstFreeArc = dec->arcs[index].arcListNode.next;
    } else {
        //If not, enlarge the array
        int newSize = 2 * dec->memArcs;
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &dec->arcs, (size_t) newSize));
        for (int i = dec->memArcs + 1; i < newSize; ++i) {
            dec->arcs[i].arcListNode.next = i + 1;
            dec->arcs[i].member = INVALID_MEMBER;
        }
        dec->arcs[newSize - 1].arcListNode.next = INVALID_ARC;
        dec->firstFreeArc = dec->memArcs + 1;
        index = dec->memArcs;
        dec->memArcs = newSize;
    }
    dec->arcs[index].tail = INVALID_NODE;
    dec->arcs[index].head = INVALID_NODE;
    dec->arcs[index].member = member;
    dec->arcs[index].childMember = INVALID_MEMBER;

    dec->arcs[index].headArcListNode.next = INVALID_ARC;
    dec->arcs[index].headArcListNode.previous = INVALID_ARC;
    dec->arcs[index].tailArcListNode.next = INVALID_ARC;
    dec->arcs[index].tailArcListNode.previous = INVALID_ARC;

    dec->arcs[index].reversed = reverse;

    dec->numArcs++;

    *pArc = index;

    return SPQR_OKAY;
}

SPQR_ERROR createRowArc(Decomposition *dec, member_id member, arc_id *pArc, row_idx row, bool reverse){
    SPQR_CALL(createArc(dec,member,pArc,reverse));
    setDecompositionRowArc(dec,row,*pArc);
    addArcToMemberArcList(dec,*pArc,member);
    dec->arcs[*pArc].element = rowToElement(row);
    return SPQR_OKAY;
}
SPQR_ERROR createColumnArc(Decomposition *dec, member_id member, arc_id *pArc, col_idx column, bool reverse){
    SPQR_CALL(createArc(dec,member,pArc,reverse));
    setDecompositionColumnArc(dec,column,*pArc);
    addArcToMemberArcList(dec,*pArc,member);
    dec->arcs[*pArc].element = columnToElement(column);

    return SPQR_OKAY;
}

void addArcToMemberArcList(Decomposition *dec, arc_id arc, member_id member){
    arc_id firstMemberArc = getFirstMemberArc(dec,member);

    if(arcIsValid(firstMemberArc)){
        arc_id lastMemberArc = getPreviousMemberArc(dec,firstMemberArc);
        dec->arcs[arc].arcListNode.next = firstMemberArc;
        dec->arcs[arc].arcListNode.previous = lastMemberArc;
        dec->arcs[firstMemberArc].arcListNode.previous = arc;
        dec->arcs[lastMemberArc].arcListNode.next = arc;
    }else{
        assert(dec->members[member].numArcs == 0);
        dec->arcs[arc].arcListNode.next = arc;
        dec->arcs[arc].arcListNode.previous = arc;
    }
    dec->members[member].firstArc = arc;//TODO: update this in case of row/column arcs to make memory ordering nicer?
    ++(dec->members[member].numArcs);
}

void removeArcFromMemberArcList(Decomposition *dec, arc_id arc, member_id member){
    assert(findArcMemberNoCompression(dec,arc) == member);
    assert(memberIsRepresentative(dec,member));

    if(dec->members[member].numArcs == 1){
        dec->members[member].firstArc = INVALID_ARC;
    }else{
        arc_id nextArc = dec->arcs[arc].arcListNode.next;
        arc_id prevArc = dec->arcs[arc].arcListNode.previous;

        dec->arcs[nextArc].arcListNode.previous = prevArc;
        dec->arcs[prevArc].arcListNode.next = nextArc;

        if(dec->members[member].firstArc == arc){
            dec->members[member].firstArc = nextArc;
        }
    }

    --(dec->members[member].numArcs);
}
void mergeMemberArcList(Decomposition *dec, member_id toMergeInto, member_id toRemove){
    arc_id firstIntoArc = getFirstMemberArc(dec,toMergeInto);
    arc_id firstFromArc = getFirstMemberArc(dec,toRemove);
    assert(arcIsValid(firstIntoArc));
    assert(arcIsValid(firstFromArc));

    arc_id lastIntoArc = getPreviousMemberArc(dec,firstIntoArc);
    arc_id lastFromArc = getPreviousMemberArc(dec,firstFromArc);

    //Relink linked lists to merge them effectively
    dec->arcs[firstIntoArc].arcListNode.previous = lastFromArc;
    dec->arcs[lastIntoArc].arcListNode.next = firstFromArc;
    dec->arcs[firstFromArc].arcListNode.previous = lastIntoArc;
    dec->arcs[lastFromArc].arcListNode.next = firstIntoArc;

    //Clean up old
    dec->members[toMergeInto].numArcs += dec->members[toRemove].numArcs;
    dec->members[toRemove].numArcs = 0;
    dec->members[toRemove].firstArc = INVALID_ARC;

}

void mergeNodeArcList(Decomposition *dec, node_id toMergeInto, node_id toRemove){

    arc_id firstIntoArc = getFirstNodeArc(dec,toMergeInto);
    arc_id firstFromArc = getFirstNodeArc(dec,toRemove);
    if(arcIsInvalid(firstIntoArc)){
        //new node has no arcs
        dec->nodes[toMergeInto].numArcs += dec->nodes[toRemove].numArcs;
        dec->nodes[toRemove].numArcs = 0;

        dec->nodes[toMergeInto].firstArc = dec->nodes[toRemove].firstArc;
        dec->nodes[toRemove].firstArc = INVALID_ARC;

        return;
    }else if (arcIsInvalid(firstFromArc)){
        //Old node has no arcs; we can just return
        return;
    }

    arc_id lastIntoArc = getPreviousNodeArc(dec,firstIntoArc,toMergeInto);
    assert(arcIsValid(lastIntoArc));
    arc_id lastFromArc = getPreviousNodeArc(dec,firstFromArc,toRemove);
    assert(arcIsValid(lastFromArc));



    ArcListNode * firstIntoNode = findArcHead(dec,firstIntoArc) == toMergeInto ?
                                   &dec->arcs[firstIntoArc].headArcListNode :
                                   &dec->arcs[firstIntoArc].tailArcListNode;
    ArcListNode * lastIntoNode = findArcHead(dec,lastIntoArc) == toMergeInto ?
                                  &dec->arcs[lastIntoArc].headArcListNode :
                                  &dec->arcs[lastIntoArc].tailArcListNode;

    ArcListNode * firstFromNode = findArcHead(dec,firstFromArc) == toRemove ?
                                   &dec->arcs[firstFromArc].headArcListNode :
                                   &dec->arcs[firstFromArc].tailArcListNode;
    ArcListNode * lastFromNode = findArcHead(dec,lastFromArc) == toRemove ?
                                  &dec->arcs[lastFromArc].headArcListNode :
                                  &dec->arcs[lastFromArc].tailArcListNode;

    firstIntoNode->previous = lastFromArc;
    lastIntoNode->next = firstFromArc;
    firstFromNode->previous = lastIntoArc;
    lastFromNode->next = firstIntoArc;

    dec->nodes[toMergeInto].numArcs += dec->nodes[toRemove].numArcs;
    dec->nodes[toRemove].numArcs = 0;
    dec->nodes[toRemove].firstArc = INVALID_ARC;
}

SPQR_ERROR createMember(Decomposition *dec, MemberType type, member_id * pMember){
    assert(dec);
    assert(pMember);

    if(dec->numMembers == dec->memMembers){
        dec->memMembers *= 2;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&dec->members,(size_t) dec->memMembers));
    }
    DecompositionMemberData *data = &dec->members[dec->numMembers];
    data->markerOfParent = INVALID_ARC;
    data->markerToParent = INVALID_ARC;
    data->firstArc = INVALID_ARC;
    data->representativeMember = INVALID_MEMBER;
    data->numArcs = 0;
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
    dec->nodes[dec->numNodes].firstArc = INVALID_ARC;
    dec->nodes[dec->numNodes].numArcs = 0;
    dec->numNodes++;

    return SPQR_OKAY;
}



SPQR_ERROR createStandaloneSeries(Decomposition *dec,const row_idx * rows,const bool * reflected, int numRows, col_idx col, member_id * pMember){
    member_id member;
    MemberType type = numRows <= 1 ? LOOP : SERIES;
    SPQR_CALL(createMember(dec,type,&member));

    arc_id colArc;
    SPQR_CALL(createColumnArc(dec,member,&colArc,col,false));

    arc_id rowArc;
    for (int i = 0; i < numRows; ++i) {
        SPQR_CALL(createRowArc(dec,member,&rowArc,rows[i],reflected[i]));
    }
    *pMember = member;
    ++dec->numConnectedComponents;
    return SPQR_OKAY;
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