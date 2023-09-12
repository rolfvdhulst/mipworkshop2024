//
// Created by rolf on 04-07-22.
//
#include "mipworkshop2024/presolve/SPQRRowAddition.h"
#include <stdlib.h>

static int max(int a, int b){
    return (a > b) ? a : b;
}
static int min(int a, int b){
    return a < b ? a : b;
}

typedef int cut_edge_id;
#define INVALID_CUT_EDGE (-1)

static bool cutEdgeIsInvalid(const cut_edge_id edge){
    return edge < 0;
}
static bool cutEdgeIsValid(const cut_edge_id edge){
    return !cutEdgeIsInvalid(edge);
}

typedef struct { //TODO:test if overhead of pointers is worth it?
    edge_id edge;
    cut_edge_id nextMember;
    cut_edge_id nextOverall;
} CutEdgeListNode;

typedef int reduced_member_id;
#define INVALID_REDUCED_MEMBER (-1)

static bool reducedMemberIsInvalid(const reduced_member_id id){
    return id < 0;
}
static bool reducedMemberIsValid(const reduced_member_id id){
    return !reducedMemberIsInvalid(id);
}

typedef int children_idx;

typedef enum{
    TYPE_UNDETERMINED = 0,
    TYPE_PROPAGATED = 1,
    TYPE_MERGED = 2,
    TYPE_NOT_GRAPHIC = 3
} ReducedMemberType;

/**
 * A 'set' of two nodes. We need this a lot in the used algorithms
 */
typedef struct {
    node_id first;
    node_id second;
}NodePair;

static void NodePairEmptyInitialize(NodePair * pair){
    pair->first = INVALID_NODE;
    pair->second = INVALID_NODE;
}
static void NodePairInitialize(NodePair * pair, const node_id firstNode,const node_id secondNode){
    pair->first = firstNode;
    pair->second = secondNode;
}
static bool NodePairIsEmpty(const NodePair * pair){
    return nodeIsInvalid(pair->first);
}
static bool NodePairHasTwo(const NodePair * pair){
    return nodeIsValid(pair->second);
}
static void NodePairIntersection(NodePair * toChange,const node_id first,const node_id second){
    if (toChange->first!= first &&  toChange->first != second) {
        toChange->first = INVALID_NODE;
    }
    if (toChange->second != first && toChange->second != second) {
        toChange->second= INVALID_NODE;
    }
    if (nodeIsInvalid(toChange->first) && nodeIsValid(toChange->second)) {
        swap_ints(&toChange->first,&toChange->second);
    }
}
static void NodePairInsert(NodePair * pair, const node_id node){
    if(pair->first != INVALID_NODE){
        assert(pair->second == INVALID_NODE);
        pair->second = node;
    }else{
        pair->first = node;
    }
}

typedef struct{
    int low;
    int discoveryTime;
} ArticulationNodeInformation;

//We allocate the callstacks of recursive algorithms (usually DFS, bounded by some linear number of calls)
//If one does not do this, we overflow the stack for large matrices/graphs through the number of recursive function calls
//Then, we can write the recursive algorithms as while loops and allocate the function call data on the heap, preventing
//Stack overflows
typedef struct {
    node_id node;
    edge_id nodeEdge;
} DFSCallData;

typedef struct{
    children_idx currentChild;
    reduced_member_id id;
} MergeTreeCallData;

typedef struct{
    node_id node;
    edge_id edge;
} ColorDFSCallData;

typedef struct{
    edge_id edge;
    node_id node;
    node_id parent;
    bool isAP;
} ArticulationPointCallStack;

typedef struct{
    member_id member;
}  CreateReducedMembersCallstack;

typedef enum{
    UNCOLORED = 0,
    COLOR_FIRST = 1,
    COLOR_SECOND = 2
} COLOR_STATUS;

typedef struct {
    member_id member;
    member_id rootMember;
    int depth;
    ReducedMemberType type;
    reduced_member_id parent;

    children_idx firstChild;
    children_idx numChildren;
    children_idx numPropagatedChildren;

    cut_edge_id firstCutEdge;
    int numCutEdges;

    NodePair splitting_nodes;
    bool allHaveCommonNode;
    edge_id articulationEdge;

    node_id coloredNode; //points to a colored node so that we can efficiently zero out the colors again.
} SPQRRowReducedMember;

typedef struct {
    int rootDepth;
    reduced_member_id root;
} SPQRRowReducedComponent;

typedef struct {
    reduced_member_id reducedMember;
    reduced_member_id rootDepthMinimizer;
} MemberInfo;

struct SPQRNewRowImpl{
    bool remainsGraphic;

    SPQRRowReducedMember* reducedMembers;
    int memReducedMembers;
    int numReducedMembers;

    SPQRRowReducedComponent* reducedComponents;
    int memReducedComponents;
    int numReducedComponents;

    MemberInfo * memberInformation;
    int memMemberInformation;
    int numMemberInformation;

    reduced_member_id * childrenStorage;
    int memChildrenStorage;
    int numChildrenStorage;

    CutEdgeListNode * cutEdges;
    int memCutEdges;
    int numCutEdges;
    cut_edge_id firstOverallCutEdge;

    row_idx newRowIndex;

    col_idx * newColumnEdges;
    int memColumnEdges;
    int numColumnEdges;

    reduced_member_id * leafMembers;
    int numLeafMembers;
    int memLeafMembers;

    edge_id * decompositionColumnEdges;
    int memDecompositionColumnEdges;
    int numDecompositionColumnEdges;

    bool * isEdgeCut;
    int numIsEdgeCut;
    int memIsEdgeCut;

    COLOR_STATUS * nodeColors;
    int memNodeColors;

    node_id * articulationNodes;
    int numArticulationNodes;
    int memArticulationNodes;

    ArticulationNodeInformation * articulationNodeSearchInfo;
    int memNodeSearchInfo;

    int * crossingPathCount;
    int memCrossingPathCount;

    DFSCallData * intersectionDFSData;
    int memIntersectionDFSData;

    ColorDFSCallData * colorDFSData;
    int memColorDFSData;

    ArticulationPointCallStack * artDFSData;
    int memArtDFSData;

    CreateReducedMembersCallstack * createReducedMembersCallstack;
    int memCreateReducedMembersCallstack;

    int * intersectionPathDepth;
    int memIntersectionPathDepth;

    node_id * intersectionPathParent;
    int memIntersectionPathParent;

    MergeTreeCallData * mergeTreeCallData;
    int memMergeTreeCallData;

};

typedef struct {
    member_id member;
    node_id firstNode;
    node_id secondNode;
} NewRowInformation;

static NewRowInformation emptyNewRowInformation(void){
    NewRowInformation information;
    information.member = INVALID_MEMBER;
    information.firstNode = INVALID_NODE;
    information.secondNode = INVALID_NODE;
    return information;
}

/**
 * Saves the information of the current row and partitions it based on whether or not the given columns are
 * already part of the decomposition.
 */
static SPQR_ERROR newRowUpdateRowInformation(const Decomposition *dec, SPQRNewRow *newRow, const row_idx row, const col_idx * columns, const size_t numColumns)
{
    newRow->newRowIndex = row;

    newRow->numDecompositionColumnEdges = 0;
    newRow->numColumnEdges = 0;

    for (size_t i = 0; i < numColumns; ++i) {
        edge_id columnEdge = getDecompositionColumnEdge(dec,columns[i]);
        if(edgeIsValid(columnEdge)){ //If the edge is the current decomposition: save it in the array
            if(newRow->numDecompositionColumnEdges == newRow->memDecompositionColumnEdges){
                int newNumEdges = newRow->memDecompositionColumnEdges == 0 ? 8 : 2*newRow->memDecompositionColumnEdges; //TODO: make reallocation numbers more consistent with rest?
                newRow->memDecompositionColumnEdges = newNumEdges;
                SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->decompositionColumnEdges,
                                                (size_t) newRow->memDecompositionColumnEdges));
            }
            newRow->decompositionColumnEdges[newRow->numDecompositionColumnEdges] = columnEdge;
            ++newRow->numDecompositionColumnEdges;
        }else{
            //Not in the decomposition: add it to the set of edges which are newly added with this row.
            if(newRow->numColumnEdges == newRow->memColumnEdges){
                int newNumEdges = newRow->memColumnEdges == 0 ? 8 : 2*newRow->memColumnEdges; //TODO: make reallocation numbers more consistent with rest?
                newRow->memColumnEdges = newNumEdges;
                SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->newColumnEdges,
                                                (size_t)newRow->memColumnEdges));
            }
            newRow->newColumnEdges[newRow->numColumnEdges] = columns[i];
            newRow->numColumnEdges++;
        }
    }

    return SPQR_OKAY;
}

/**
 * Recursively creates reduced members from this member to the root of the decomposition tree.
 * @param dec
 * @param newRow
 * @param member
 * @return
 */
static reduced_member_id createReducedMembersToRoot(Decomposition *dec, SPQRNewRow * newRow, const member_id firstMember){
    assert(memberIsValid(firstMember));

    CreateReducedMembersCallstack * callstack = newRow->createReducedMembersCallstack;
    callstack[0].member = firstMember;
    int callDepth = 0;

    while(callDepth >= 0){
        member_id member = callstack[callDepth].member;
        reduced_member_id reducedMember = newRow->memberInformation[member].reducedMember;

        bool reducedValid = reducedMemberIsValid(reducedMember);
        if(!reducedValid) {
            //reduced member was not yet created; we create it
            reducedMember = newRow->numReducedMembers;

            SPQRRowReducedMember *reducedMemberData = &newRow->reducedMembers[reducedMember];
            ++newRow->numReducedMembers;

            reducedMemberData->member = member;
            reducedMemberData->numChildren = 0;
            reducedMemberData->numCutEdges = 0;
            reducedMemberData->firstCutEdge = INVALID_CUT_EDGE;
            reducedMemberData->type = TYPE_UNDETERMINED;
            reducedMemberData->numPropagatedChildren = 0;
            reducedMemberData->allHaveCommonNode = false;
            reducedMemberData->articulationEdge = INVALID_EDGE;
            reducedMemberData->coloredNode = INVALID_NODE;
            NodePairEmptyInitialize(&reducedMemberData->splitting_nodes);

            newRow->memberInformation[member].reducedMember = reducedMember;
            assert(memberIsRepresentative(dec, member));
            member_id parentMember = findMemberParent(dec, member);

            if (memberIsValid(parentMember)) {
                //recursive call to parent member
                ++callDepth;
                assert(callDepth < newRow->memCreateReducedMembersCallstack);
                callstack[callDepth].member = parentMember;
                continue;

            } else {
                //we found a new reduced decomposition component

                reducedMemberData->parent = INVALID_REDUCED_MEMBER;
                reducedMemberData->depth = 0;
                reducedMemberData->rootMember = member;

                assert(newRow->numReducedComponents < newRow->memReducedComponents);
                newRow->reducedComponents[newRow->numReducedComponents].root = reducedMember;
                ++newRow->numReducedComponents;
            }
        }
        if(reducedValid){
            assert(reducedMember < newRow->numReducedMembers);
            //Reduced member was already created in earlier call
            //update the depth of the root if appropriate
            reduced_member_id * depthMinimizer = &newRow->memberInformation[newRow->reducedMembers[reducedMember].rootMember].rootDepthMinimizer;
            if(reducedMemberIsInvalid(*depthMinimizer) ||
               newRow->reducedMembers[reducedMember].depth < newRow->reducedMembers[*depthMinimizer].depth){
                *depthMinimizer = reducedMember;
            }
        }
        while(true){
            --callDepth;
            if(callDepth < 0 ) break;
            member_id parentMember = callstack[callDepth+1].member;
            reduced_member_id parentReducedMember = newRow->memberInformation[parentMember].reducedMember;
            member_id currentMember = callstack[callDepth].member;
            reduced_member_id currentReducedMember = newRow->memberInformation[currentMember].reducedMember;

            SPQRRowReducedMember *parentReducedMemberData = &newRow->reducedMembers[parentReducedMember];
            SPQRRowReducedMember *reducedMemberData = &newRow->reducedMembers[currentReducedMember];

            reducedMemberData->parent = parentReducedMember;
            reducedMemberData->depth = parentReducedMemberData->depth + 1;
            reducedMemberData->rootMember = parentReducedMemberData->rootMember;

            newRow->reducedMembers[parentReducedMember].numChildren++;
        }

    }

    reduced_member_id returnedMember = newRow->memberInformation[callstack[0].member].reducedMember;
    return returnedMember;
}


/**
 * Construct a smaller sub tree of the decomposition on which the cut edges lie.
 * @return
 */
static SPQR_ERROR constructReducedDecomposition(Decomposition* dec, SPQRNewRow* newRow){
    //TODO: chop up into more functions
    //TODO: stricter assertions/array bounds checking in this function
#ifndef NDEBUG
    for (int i = 0; i < newRow->memMemberInformation; ++i) {
        assert(reducedMemberIsInvalid(newRow->memberInformation[i].reducedMember));
    }
#endif

    newRow->numReducedComponents = 0;
    newRow->numReducedMembers = 0;
    if(newRow->numDecompositionColumnEdges == 0){ //Early return in case the reduced decomposition will be empty
        return SPQR_OKAY;
    }
    assert(newRow->numReducedMembers == 0);
    assert(newRow->numReducedComponents == 0);

    int newSize = largestMemberID(dec); //Is this sufficient?
    if(newSize > newRow->memReducedMembers){
        newRow->memReducedMembers = max(2*newRow->memReducedMembers,newSize);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->reducedMembers,(size_t) newRow->memReducedMembers));
    }
    if(newSize > newRow->memMemberInformation){
        int updatedSize = max(2*newRow->memMemberInformation,newSize);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->memberInformation,(size_t) updatedSize));
        for (int i = newRow->memMemberInformation; i < updatedSize; ++i) {
            newRow->memberInformation[i].reducedMember = INVALID_REDUCED_MEMBER;
            newRow->memberInformation[i].rootDepthMinimizer = INVALID_REDUCED_MEMBER;
        }
        newRow->memMemberInformation = updatedSize;

    }

    int numComponents = numConnectedComponents(dec);
    if(numComponents > newRow->memReducedComponents){
        newRow->memReducedComponents = max(2*newRow->memReducedComponents,numComponents);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->reducedComponents,(size_t) newRow->memReducedComponents));
    }

    int numMembers = getNumMembers(dec);
    if(newRow->memCreateReducedMembersCallstack < numMembers){
        newRow->memCreateReducedMembersCallstack = max(2*newRow->memCreateReducedMembersCallstack,numMembers);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->createReducedMembersCallstack,(size_t) newRow->memCreateReducedMembersCallstack));
    }

    //Create the reduced members (recursively)
    for (int i = 0; i < newRow->numDecompositionColumnEdges; ++i) {
        assert(i < newRow->memDecompositionColumnEdges);
        edge_id edge = newRow->decompositionColumnEdges[i];
        member_id edgeMember = findEdgeMember(dec,edge);
        reduced_member_id reducedMember = createReducedMembersToRoot(dec,newRow,edgeMember);
        reduced_member_id* depthMinimizer = &newRow->memberInformation[newRow->reducedMembers[reducedMember].rootMember].rootDepthMinimizer;
        if(reducedMemberIsInvalid(*depthMinimizer)){
            *depthMinimizer = reducedMember;
        }
    }

    //Set the reduced roots according to the root depth minimizers
    for (int i = 0; i < newRow->numReducedComponents; ++i) {
        SPQRRowReducedComponent * component = &newRow->reducedComponents[i];
        member_id rootMember = newRow->reducedMembers[component->root].member;
        reduced_member_id reducedMinimizer = newRow->memberInformation[rootMember].rootDepthMinimizer;
        component->rootDepth =  newRow->reducedMembers[reducedMinimizer].depth;
        component->root = reducedMinimizer;

        //This simplifies code further down which does not need to be component-aware; just pretend that the reduced member is the new root.
        newRow->reducedMembers[component->root].parent = INVALID_REDUCED_MEMBER;
        assert(memberIsRepresentative(dec,rootMember));
    }

    //update the children array
    int numTotalChildren = 0;
    for (int i = 0; i < newRow->numReducedMembers; ++i) {
        SPQRRowReducedMember * reducedMember = &newRow->reducedMembers[i];
        reduced_member_id minimizer = newRow->memberInformation[reducedMember->rootMember].rootDepthMinimizer;
        if(reducedMember->depth >= newRow->reducedMembers[minimizer].depth){
            reducedMember->firstChild = numTotalChildren;
            numTotalChildren += reducedMember->numChildren;
            reducedMember->numChildren = 0;
        }
    }

    if(newRow->memChildrenStorage < numTotalChildren){
        int newMemSize = max(newRow->memChildrenStorage*2, numTotalChildren);
        newRow->memChildrenStorage = newMemSize;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->childrenStorage,(size_t) newRow->memChildrenStorage));
    }
    newRow->numChildrenStorage = numTotalChildren;

    //Fill up the children array`
    for(reduced_member_id  reducedMember = 0; reducedMember < newRow->numReducedMembers; ++reducedMember){
        SPQRRowReducedMember * reducedMemberData = &newRow->reducedMembers[reducedMember];
        if(reducedMemberData->depth <= newRow->reducedMembers[newRow->memberInformation[reducedMemberData->rootMember].rootDepthMinimizer].depth){
            continue;
        }
        member_id parentMember = findMemberParent(dec,reducedMemberData->member);
        reduced_member_id parentReducedMember = memberIsValid(parentMember) ? newRow->memberInformation[parentMember].reducedMember : INVALID_REDUCED_MEMBER;
        if(reducedMemberIsValid(parentReducedMember)){ //TODO: probably one of these two checks/branches is unnecessary, as there is a single failure case? (Not sure)
            SPQRRowReducedMember * parentReducedMemberData = &newRow->reducedMembers[parentReducedMember];
            newRow->childrenStorage[parentReducedMemberData->firstChild + parentReducedMemberData->numChildren] = reducedMember;
            ++parentReducedMemberData->numChildren;
        }
    }

    //Clean up the root depth minimizers.
    for (int i = 0; i < newRow->numReducedMembers; ++i) {
        SPQRRowReducedMember * reducedMember = &newRow->reducedMembers[i];
        assert(reducedMember);
        member_id rootMember = reducedMember->rootMember;
        assert(rootMember >= 0);
        assert(rootMember < dec->memMembers);
        newRow->memberInformation[rootMember].rootDepthMinimizer = INVALID_REDUCED_MEMBER;
    }

    return SPQR_OKAY;
}


/**
 * Marks an edge as 'cut'. This implies that its cycle in the decomposition must be elongated
 * @param newRow
 * @param edge
 * @param reducedMember
 */
static void createCutEdge(SPQRNewRow* newRow, const edge_id edge, const reduced_member_id reducedMember){
    cut_edge_id cut_edge =  newRow->numCutEdges;

    CutEdgeListNode * listNode = &newRow->cutEdges[cut_edge];
    listNode->edge = edge;

    listNode->nextMember = newRow->reducedMembers[reducedMember].firstCutEdge;
    newRow->reducedMembers[reducedMember].firstCutEdge = cut_edge;

    listNode->nextOverall = newRow->firstOverallCutEdge;
    newRow->firstOverallCutEdge = cut_edge;

    newRow->numCutEdges++;
    newRow->reducedMembers[reducedMember].numCutEdges++;
    assert(newRow->numCutEdges <= newRow->memCutEdges);

    assert(edge < newRow->memIsEdgeCut);
    newRow->isEdgeCut[edge] = true;
}

/**
 * Creates all cut edges within the decomposition for the new row.
 * Note this preallocates memory for cut edges which may be created by propagation.
 */
static SPQR_ERROR createReducedDecompositionCutEdges(Decomposition* dec, SPQRNewRow* newRow){
    //Allocate memory for cut edges
    edge_id maxEdgeID = largestEdgeID(dec);
    if(maxEdgeID > newRow->memIsEdgeCut){
        int newSize = max(maxEdgeID,2*newRow->memIsEdgeCut);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->isEdgeCut,(size_t) newSize));
        for (int i = newRow->memIsEdgeCut; i < newSize ; ++i) {
            newRow->isEdgeCut[i] = false;
        }
        newRow->memIsEdgeCut = newSize;
    }
#ifndef NDEBUG
    for (int i = 0; i < newRow->memIsEdgeCut; ++i) {
        assert(!newRow->isEdgeCut[i]);
    }
#endif

    int numNeededEdges = newRow->numDecompositionColumnEdges*4; //3 Is not enough; see tests. Probably 3 + 12 or so is, but cannot be bothered to work that out for now
    if(numNeededEdges > newRow->memCutEdges){
        int newSize = max(newRow->memCutEdges*2, numNeededEdges);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->cutEdges,(size_t) newSize));
        newRow->memCutEdges = newSize;
    }
    newRow->numCutEdges = 0;
    newRow->firstOverallCutEdge = INVALID_CUT_EDGE;
    for (int i = 0; i < newRow->numDecompositionColumnEdges; ++i) {
        edge_id edge = newRow->decompositionColumnEdges[i];
        member_id member = findEdgeMember(dec,edge);
        reduced_member_id reduced_member = newRow->memberInformation[member].reducedMember;
        assert(reducedMemberIsValid(reduced_member));
        createCutEdge(newRow,edge,reduced_member);
    }

    return SPQR_OKAY;
}

/**
 * Determines the members of the reduced decomposition which are leafs.
 * This is used in propagation to ensure propagation is only checked for components which have at most one neighbour
 * which is not propagated.
 */
static SPQR_ERROR determineLeafReducedMembers(const Decomposition *dec, SPQRNewRow *newRow) {
    if (newRow->numDecompositionColumnEdges > newRow->memLeafMembers) {
        newRow->memLeafMembers = max(newRow->numDecompositionColumnEdges, 2 * newRow->memLeafMembers);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &newRow->leafMembers, (size_t) newRow->memLeafMembers));
    }
    newRow->numLeafMembers = 0;

    for (reduced_member_id reducedMember = 0; reducedMember < newRow->numReducedMembers; ++reducedMember) {
        if (newRow->reducedMembers[reducedMember].numChildren == 0) {
            newRow->leafMembers[newRow->numLeafMembers] = reducedMember;
            ++newRow->numLeafMembers;
        }
    }
    return SPQR_OKAY;
}

/**
 * Preallocates memory arrays necessary for searching rigid components.
 */
static SPQR_ERROR allocateRigidSearchMemory(const Decomposition *dec, SPQRNewRow *newRow){
    int totalNumNodes = getNumNodes(dec);
    if(totalNumNodes > newRow->memNodeColors){
        int newSize = max(2*newRow->memNodeColors,totalNumNodes);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->nodeColors,(size_t) newSize));
        for (int i = newRow->memNodeColors; i < newSize; ++i) {
            newRow->nodeColors[i] = UNCOLORED;
        }
        newRow->memNodeColors = newSize;
    }

    if(totalNumNodes > newRow->memArticulationNodes){
        int newSize = max(2*newRow->memArticulationNodes,totalNumNodes);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->articulationNodes,(size_t) newSize));
        newRow->memArticulationNodes = newSize;
    }
    if(totalNumNodes > newRow->memNodeSearchInfo){
        int newSize = max(2*newRow->memNodeSearchInfo,totalNumNodes);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->articulationNodeSearchInfo,(size_t) newSize));
        newRow->memNodeSearchInfo = newSize;
    }
    if(totalNumNodes > newRow->memCrossingPathCount){
        int newSize = max(2*newRow->memCrossingPathCount,totalNumNodes);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->crossingPathCount,(size_t) newSize));
        newRow->memCrossingPathCount = newSize;
    }

    //TODO: see if tradeoff for performance bound by checking max # of nodes of rigid is worth it to reduce size
    //of the following allocations
    int largestID  = largestNodeID(dec); //TODO: only update the stack sizes of the following when needed? The preallocation might be causing performance problems
    if(largestID > newRow->memIntersectionDFSData){
        int newSize = max(2*newRow->memIntersectionDFSData,largestID);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->intersectionDFSData,(size_t) newSize));
        newRow->memIntersectionDFSData = newSize;
    }
    if(largestID > newRow->memColorDFSData){
        int newSize = max(2*newRow->memColorDFSData, largestID);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->colorDFSData,(size_t) newSize));
        newRow->memColorDFSData = newSize;
    }
    if(largestID > newRow->memArtDFSData){
        int newSize = max(2*newRow->memArtDFSData,largestID);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->artDFSData,(size_t) newSize));
        newRow->memArtDFSData = newSize;
    }

    for (int i = 0; i < newRow->memIntersectionPathDepth; ++i) {
        newRow->intersectionPathDepth[i] = -1;
    }

    if(largestID > newRow->memIntersectionPathDepth){
        int newSize = max(2*newRow->memIntersectionPathDepth,largestID);
        SPQRreallocBlockArray(dec->env, &newRow->intersectionPathDepth, (size_t) newSize);
        for (int i = newRow->memIntersectionPathDepth; i < newSize; ++i) {
            newRow->intersectionPathDepth[i] = -1;
        }
        newRow->memIntersectionPathDepth = newSize;
    }
    for (int i = 0; i < newRow->memIntersectionPathParent; ++i) {
        newRow->intersectionPathParent[i] = INVALID_NODE;
    }
    if(largestID > newRow->memIntersectionPathParent){
        int newSize = max(2*newRow->memIntersectionPathParent,largestID);
        SPQRreallocBlockArray(dec->env, &newRow->intersectionPathParent, (size_t) newSize);
        for (int i = newRow->memIntersectionPathParent; i <newSize; ++i) {
            newRow->intersectionPathParent[i] = INVALID_NODE;
        }
        newRow->memIntersectionPathParent = newSize;
    }

    return SPQR_OKAY;
}
static void zeroOutColors(Decomposition *dec, SPQRNewRow *newRow, const node_id firstRemoveNode){
    assert(firstRemoveNode < newRow->memNodeColors);

    newRow->nodeColors[firstRemoveNode] = UNCOLORED;
    ColorDFSCallData * data = newRow->colorDFSData;

    data[0].node = firstRemoveNode;
    data[0].edge = getFirstNodeEdge(dec,firstRemoveNode);

    int depth = 0;

    while(depth >= 0){
        assert(depth < newRow->memColorDFSData);
        ColorDFSCallData * callData = &data[depth];
        node_id head = findEdgeHead(dec, callData->edge);
        node_id tail = findEdgeTail(dec, callData->edge);
        node_id otherNode = callData->node == head ? tail : head;
        assert(otherNode < newRow->memNodeColors);
        if(newRow->nodeColors[otherNode] != UNCOLORED){
            callData->edge = getNextNodeEdge(dec,callData->edge,callData->node);

            newRow->nodeColors[otherNode] = UNCOLORED;
            ++depth;
            data[depth].node = otherNode;
            data[depth].edge = getFirstNodeEdge(dec,otherNode);
            continue;
        }

        callData->edge = getNextNodeEdge(dec,callData->edge,callData->node);
        while(depth >= 0 && data[depth].edge == getFirstNodeEdge(dec,data[depth].node)){
            --depth;
        }
    }


}

static void cleanUpPreviousIteration(Decomposition * dec, SPQRNewRow * newRow){
    //zero out coloring information from previous check
    for (int i = 0; i < newRow->numReducedMembers; ++i) {
        if(nodeIsValid(newRow->reducedMembers[i].coloredNode)){
            zeroOutColors(dec,newRow,newRow->reducedMembers[i].coloredNode);
        }
    }
#ifndef NDEBUG
    for (int i = 0; i < newRow->memNodeColors; ++i) {
        assert(newRow->nodeColors[i] == UNCOLORED);
    }
#endif

    //For cut edges: clear them from the array from previous iteration
    cut_edge_id cutEdgeIdx = newRow->firstOverallCutEdge;
    while(cutEdgeIsValid(cutEdgeIdx)){
        edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
        cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextOverall;
        newRow->isEdgeCut[cutEdge] = false;
    }
}

static NodePair rigidDetermineAllAdjacentSplittableNodes(Decomposition * dec, SPQRNewRow * newRow,
                                                  const reduced_member_id toCheck, edge_id * secondArticulationEdge){
    NodePair pair;
    NodePairEmptyInitialize(&pair);
    NodePair * nodePair = &pair;
    assert(NodePairIsEmpty(nodePair));
    assert(newRow->reducedMembers[toCheck].numCutEdges > 0);//calling this function otherwise is nonsensical
    assert(*secondArticulationEdge == INVALID_EDGE);

    cut_edge_id cutEdgeIdx = newRow->reducedMembers[toCheck].firstCutEdge;
    edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
    node_id head = findEdgeHead(dec, cutEdge);
    node_id tail = findEdgeTail(dec, cutEdge);
    NodePairInitialize(nodePair, head, tail);

    while (cutEdgeIsValid(newRow->cutEdges[cutEdgeIdx].nextMember)) {
        cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
        cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
        head = findEdgeHead(dec, cutEdge);
        tail = findEdgeTail(dec, cutEdge);
        NodePairIntersection(nodePair, head, tail);

        if (NodePairIsEmpty(nodePair)) {
            break;
        }
    }

    if(!NodePairIsEmpty(nodePair)){
        //Check if the cut edges are n-1 of the n edges incident at this node; if so, that point is an articulation point
        //as it disconnects the first splitnode from the rest of the tree
        node_id splitNode = nodePair->first;
        if(!NodePairHasTwo(nodePair) &&newRow->reducedMembers[toCheck].numCutEdges == nodeDegree(dec,splitNode) -1 ){
            edge_id firstNodeEdge = getFirstNodeEdge(dec,splitNode);
            edge_id neighbourEdge = firstNodeEdge;
            do{
                if(edgeIsTree(dec,neighbourEdge)){
                    break;
                }
                neighbourEdge = getNextNodeEdge(dec,neighbourEdge,splitNode);
            }while(neighbourEdge != firstNodeEdge);
            node_id otherHead = findEdgeHead(dec,neighbourEdge);
            node_id otherTail = findEdgeTail(dec,neighbourEdge);
            node_id otherNode = otherHead == splitNode ? otherTail : otherHead;
            NodePairInsert(nodePair,otherNode);
            *secondArticulationEdge = neighbourEdge;
        }
    }
    return pair;
}

//TODO: remove SPQR_ERROR from below functions (until propagation function, basically) and refactor memory allocation
static SPQR_ERROR zeroOutColorsExceptNeighbourhood(Decomposition *dec, SPQRNewRow *newRow,
                                                   const node_id articulationNode,const node_id startRemoveNode){
    COLOR_STATUS * neighbourColors;
    int degree = nodeDegree(dec,articulationNode);
    SPQR_CALL(SPQRallocBlockArray(dec->env,&neighbourColors,(size_t) degree));

    {
        int i = 0;
        edge_id artFirstEdge = getFirstNodeEdge(dec,articulationNode);
        edge_id artItEdge = artFirstEdge;
        do{
            node_id head = findEdgeHead(dec, artItEdge);
            node_id tail = findEdgeTail(dec, artItEdge);
            node_id otherNode = articulationNode == head ? tail : head;
            neighbourColors[i] = newRow->nodeColors[otherNode];
            i++;
            assert(i <= degree);
            artItEdge = getNextNodeEdge(dec,artItEdge,articulationNode);
        }while(artItEdge != artFirstEdge);
    }
    zeroOutColors(dec,newRow,startRemoveNode);

    {
        int i = 0;
        edge_id artFirstEdge = getFirstNodeEdge(dec,articulationNode);
        edge_id artItEdge = artFirstEdge;
        do{
            node_id head = findEdgeHead(dec, artItEdge);
            node_id tail = findEdgeTail(dec, artItEdge);
            node_id otherNode = articulationNode == head ? tail : head;
            newRow->nodeColors[otherNode] = neighbourColors[i];
            i++;
            assert(i <= degree);
            artItEdge = getNextNodeEdge(dec,artItEdge,articulationNode);
        }while(artItEdge != artFirstEdge);
    }

    SPQRfreeBlockArray(dec->env,&neighbourColors);
    return SPQR_OKAY;
}

static void intersectionOfAllPaths(Decomposition * dec, SPQRNewRow *newRow,
                                   const reduced_member_id toCheck, int * const nodeNumPaths){
    int * intersectionPathDepth = newRow->intersectionPathDepth;
    node_id * intersectionPathParent = newRow->intersectionPathParent;

    //First do a dfs over the tree, storing all the tree-parents and depths for each node
    //TODO: maybe cache this tree and also update it so we can prevent this DFS call?


    //pick an arbitrary node as root; we just use the first cutEdge here
    {
        node_id root = findEdgeHead(dec,newRow->cutEdges[newRow->reducedMembers[toCheck].firstCutEdge].edge);
        DFSCallData *pathSearchCallStack = newRow->intersectionDFSData;

        assert(intersectionPathDepth[root] == -1);
        assert(intersectionPathParent[root] == INVALID_NODE);

        int pathSearchCallStackSize = 0;

        intersectionPathDepth[root] = 0;
        intersectionPathParent[root] = INVALID_NODE;

        pathSearchCallStack[0].node = root;
        pathSearchCallStack[0].nodeEdge = getFirstNodeEdge(dec, root);
        pathSearchCallStackSize++;
        while (pathSearchCallStackSize > 0) {
            assert(pathSearchCallStackSize <= newRow->memIntersectionDFSData);
            DFSCallData *dfsData = &pathSearchCallStack[pathSearchCallStackSize - 1];
            //cannot be a tree edge which is its parent
            if (edgeIsTree(dec, dfsData->nodeEdge) &&
                (pathSearchCallStackSize <= 1 ||
                 dfsData->nodeEdge != pathSearchCallStack[pathSearchCallStackSize - 2].nodeEdge)) {
                node_id head = findEdgeHeadNoCompression(dec, dfsData->nodeEdge);
                node_id tail = findEdgeTailNoCompression(dec, dfsData->nodeEdge);
                node_id other = head == dfsData->node ? tail : head;
                assert(other != dfsData->node);

                //We go up a level: add new node to the call stack
                pathSearchCallStack[pathSearchCallStackSize].node = other;
                pathSearchCallStack[pathSearchCallStackSize].nodeEdge = getFirstNodeEdge(dec, other);
                //Every time a new node is discovered/added, we update its parent and depth information
                assert(intersectionPathDepth[other] == -1);
                assert(intersectionPathParent[other] == INVALID_NODE);
                intersectionPathParent[other] = dfsData->node;
                intersectionPathDepth[other] = pathSearchCallStackSize;
                ++pathSearchCallStackSize;
                continue;
            }
            do {
                dfsData->nodeEdge = getNextNodeEdge(dec, dfsData->nodeEdge, dfsData->node);
                if (dfsData->nodeEdge == getFirstNodeEdge(dec, dfsData->node)) {
                    --pathSearchCallStackSize;
                    dfsData = &pathSearchCallStack[pathSearchCallStackSize - 1];
                } else {
                    break;
                }
            } while (pathSearchCallStackSize > 0);
        }
    }

    //For each cut edge, trace back both ends until they meet
    cut_edge_id cutEdge = newRow->reducedMembers[toCheck].firstCutEdge;
    do{
        edge_id edge = newRow->cutEdges[cutEdge].edge;
        cutEdge = newRow->cutEdges[cutEdge].nextMember;

        //Iteratively jump up to the parents until they reach a common parent
        node_id source = findEdgeHead(dec,edge);
        node_id target = findEdgeTail(dec,edge);
        int sourceDepth = intersectionPathDepth[source];
        int targetDepth = intersectionPathDepth[target];
        nodeNumPaths[source]++;
        nodeNumPaths[target]++;

        while (sourceDepth > targetDepth){
            assert(source != target);
            source = intersectionPathParent[source];
            nodeNumPaths[source]++;
            --sourceDepth;
        }
        while(targetDepth > sourceDepth){
            assert(source != target);
            target = intersectionPathParent[target];
            nodeNumPaths[target]++;
            --targetDepth;
        }
        while(source != target && targetDepth >= 0){
            source = intersectionPathParent[source];
            target = intersectionPathParent[target];
            nodeNumPaths[source]++;
            nodeNumPaths[target]++;
            --targetDepth;
        }
        //In all the above, the lowest common ancestor is increased twice, so we correct for it ad-hoc
        nodeNumPaths[source]--;
        assert(nodeIsValid(source) && nodeIsValid(target));
        assert(source == target);

    }while (cutEdgeIsValid(cutEdge));

}

void addArticulationNode(SPQRNewRow *newRow, node_id articulationNode){
#ifndef NDEBUG
    for (int i = 0; i < newRow->numArticulationNodes; ++i) {
        assert(newRow->articulationNodes[i] != articulationNode);
    }
#endif
    newRow->articulationNodes[newRow->numArticulationNodes] = articulationNode;
    ++newRow->numArticulationNodes;
}
static void articulationPoints(Decomposition *dec, SPQRNewRow * newRow, ArticulationNodeInformation *nodeInfo, reduced_member_id reducedMember){
    const bool * edgeRemoved = newRow->isEdgeCut;

    int rootChildren = 0;
    node_id root_node = findEdgeHead(dec,getFirstMemberEdge(dec,newRow->reducedMembers[reducedMember].member));;

    ArticulationPointCallStack * callStack = newRow->artDFSData;

    int depth = 0;
    int time = 1;

    callStack[depth].edge = getFirstNodeEdge(dec,root_node);
    callStack[depth].node = root_node;
    callStack[depth].parent = INVALID_NODE;
    callStack[depth].isAP = false;

    nodeInfo[root_node].low = time;
    nodeInfo[root_node].discoveryTime = time;

    while(depth >= 0){
        if(!edgeRemoved[callStack[depth].edge]){
            node_id node = callStack[depth].node;
            node_id head = findEdgeHead(dec, callStack[depth].edge);
            node_id tail = findEdgeTail(dec, callStack[depth].edge);
            node_id otherNode = node == head ? tail : head;
            if(otherNode != callStack[depth].parent){
                if(nodeInfo[otherNode].discoveryTime == 0){
                    if(depth == 0){
                        rootChildren++;
                    }
                    //recursive call
                    ++depth;
                    assert(depth < newRow->memArtDFSData);
                    callStack[depth].parent = node;
                    callStack[depth].node = otherNode;
                    callStack[depth].edge = getFirstNodeEdge(dec,otherNode);
                    callStack[depth].isAP = false;

                    ++time;
                    nodeInfo[otherNode].low = time;
                    nodeInfo[otherNode].discoveryTime = time;
                    continue;

                }else{
                    nodeInfo[node].low = min(nodeInfo[node].low, nodeInfo[otherNode].discoveryTime);
                }
            }
        }

        while(true){
            callStack[depth].edge = getNextNodeEdge(dec,callStack[depth].edge,callStack[depth].node);
            if(callStack[depth].edge != getFirstNodeEdge(dec,callStack[depth].node)) break;
            --depth;
            if (depth < 0) break;

            node_id current_node = callStack[depth].node;
            node_id other_node = callStack[depth + 1].node;
            nodeInfo[current_node].low = min(nodeInfo[current_node].low,
                                                      nodeInfo[other_node].low);
            if (depth != 0 &&
                !callStack[depth].isAP &&
                nodeInfo[current_node].discoveryTime <= nodeInfo[other_node].low) {
                addArticulationNode(newRow, current_node);
                callStack[depth].isAP = true;
            }
        }

    }
    if(rootChildren > 1 ){
        addArticulationNode(newRow,root_node);
    }
}

static void rigidConnectedColoringRecursive(Decomposition *dec, SPQRNewRow * newRow, node_id articulationNode,
                                             node_id firstProcessNode,bool *isGood){
    const bool * isEdgeCut = newRow->isEdgeCut;
    COLOR_STATUS * nodeColors = newRow->nodeColors;
    ColorDFSCallData * data = newRow->colorDFSData;

    data[0].node = firstProcessNode;
    data[0].edge = getFirstNodeEdge(dec,firstProcessNode);
    newRow->nodeColors[firstProcessNode] = COLOR_FIRST;

    int depth = 0;
    while(depth >= 0){
        assert(depth < newRow->memColorDFSData);
        ColorDFSCallData * callData = &data[depth];
        node_id head = findEdgeHead(dec, callData->edge);
        node_id tail = findEdgeTail(dec, callData->edge);
        node_id otherNode = callData->node == head ? tail : head;
        if(otherNode != articulationNode){
            COLOR_STATUS currentColor = nodeColors[callData->node];
            COLOR_STATUS otherColor = nodeColors[otherNode];
            if(otherColor == UNCOLORED){
                if(isEdgeCut[callData->edge]){
                    nodeColors[otherNode] = currentColor == COLOR_FIRST ? COLOR_SECOND : COLOR_FIRST; //reverse the colors
                }else{
                    nodeColors[otherNode] = currentColor;
                }
                callData->edge = getNextNodeEdge(dec,callData->edge,callData->node);

                depth++;
                assert(depth < newRow->memColorDFSData);
                data[depth].node = otherNode;
                data[depth].edge = getFirstNodeEdge(dec,otherNode);
                continue;
            }else if(isEdgeCut[callData->edge] ^ (otherColor != currentColor)){
                *isGood = false;
                break;
            }
        }
        callData->edge = getNextNodeEdge(dec,callData->edge,callData->node);
        while(depth >= 0 && data[depth].edge == getFirstNodeEdge(dec,data[depth].node)){
            --depth;
        }
    }
}

static void rigidConnectedColoring(Decomposition *dec, SPQRNewRow *newRow,
                                   const reduced_member_id reducedMember,const node_id node, bool * const isGood){

    node_id firstProcessNode;
    {
        edge_id edge = getFirstNodeEdge(dec,node);
        assert(edgeIsValid(edge));
        node_id head = findEdgeHead(dec,edge);
        node_id tail = findEdgeTail(dec,edge);
        //arbitrary way to select the first node
        firstProcessNode = head == node ? tail : head;

#ifndef NDEBUG
        {

            member_id member = findEdgeMemberNoCompression(dec,edge);
            edge_id firstEdge = getFirstMemberEdge(dec,member);
            edge_id memberEdge = firstEdge;
            do{
                assert(newRow->nodeColors[findEdgeHeadNoCompression(dec,memberEdge)] == UNCOLORED);
                assert(newRow->nodeColors[findEdgeTailNoCompression(dec,memberEdge)] == UNCOLORED);
                memberEdge = getNextMemberEdge(dec,memberEdge);
            }while(firstEdge != memberEdge);
        }
#endif
    }
    assert(nodeIsValid(firstProcessNode) && firstProcessNode != node);
    *isGood = true;
    newRow->reducedMembers[reducedMember].coloredNode = firstProcessNode;
    rigidConnectedColoringRecursive(dec,newRow,node,firstProcessNode,isGood);

    // Need to zero all colors for next attempts if we failed
    if(!(*isGood)){
        zeroOutColors(dec,newRow,firstProcessNode);
        newRow->reducedMembers[reducedMember].coloredNode = INVALID_NODE;
    }else{
        zeroOutColorsExceptNeighbourhood(dec,newRow,node, firstProcessNode);
        newRow->reducedMembers[reducedMember].coloredNode = node;
    }
}

static node_id checkNeighbourColoringArticulationNode(Decomposition * dec, SPQRNewRow *newRow,
                                                      const node_id articulationNode, edge_id * const adjacentSplittingEdge){
    node_id firstSideCandidate = INVALID_NODE;
    node_id secondSideCandidate = INVALID_NODE;
    edge_id firstSideEdge = INVALID_EDGE;
    edge_id secondSideEdge = INVALID_EDGE;
    int numFirstSide = 0;
    int numSecondSide = 0;

    edge_id firstEdge = getFirstNodeEdge(dec,articulationNode);
    edge_id moveEdge = firstEdge;
    do{
        node_id head = findEdgeHead(dec, moveEdge);
        node_id tail = findEdgeTail(dec, moveEdge);
        node_id otherNode = articulationNode == head ? tail : head;
        assert(newRow->nodeColors[otherNode] != UNCOLORED);
        //TODO: bit duplicate logic here? Maybe a nice way to fix?
        if((newRow->nodeColors[otherNode] == COLOR_FIRST) ^ newRow->isEdgeCut[moveEdge] ){
            if(numFirstSide == 0 && edgeIsTree(dec,moveEdge)){
                firstSideCandidate = otherNode;
                firstSideEdge = moveEdge;
            }else if (numFirstSide > 0){
                firstSideCandidate = INVALID_NODE;
            }
            ++numFirstSide;
        }else{
            if(numSecondSide == 0 && edgeIsTree(dec,moveEdge)){
                secondSideCandidate = otherNode;
                secondSideEdge = moveEdge;
            }else if (numFirstSide > 0){
                secondSideCandidate = INVALID_NODE;
            }
            ++numSecondSide;
        }
        moveEdge = getNextNodeEdge(dec,moveEdge,articulationNode);
    }while(moveEdge != firstEdge);

    if(numFirstSide == 1){
        *adjacentSplittingEdge = firstSideEdge;
        return firstSideCandidate;
    }else if (numSecondSide == 1){
        *adjacentSplittingEdge = secondSideEdge;
        return secondSideCandidate;
    }
    return INVALID_NODE;
}


static void rigidGetSplittableArticulationPointsOnPath(Decomposition *dec, SPQRNewRow *newRow,
                                                      const reduced_member_id toCheck, NodePair * const pair){
    int totalNumNodes = getNumNodes(dec);
    int * nodeNumPaths = newRow->crossingPathCount;

    for (int i = 0; i < totalNumNodes; ++i) {
        nodeNumPaths[i] = 0;
    }

    intersectionOfAllPaths(dec,newRow,toCheck,nodeNumPaths);

    newRow->numArticulationNodes = 0;

    ArticulationNodeInformation * artNodeInfo = newRow->articulationNodeSearchInfo;
    for (int i = 0; i < totalNumNodes; ++i) { //clean up can not easily be done in the search, unfortunately
       artNodeInfo[i].low = 0 ;
       artNodeInfo[i].discoveryTime = 0;
    }

    articulationPoints(dec,newRow,artNodeInfo,toCheck);

    int numCutEdges = newRow->reducedMembers[toCheck].numCutEdges;
    NodePairEmptyInitialize(&newRow->reducedMembers[toCheck].splitting_nodes);
    for (int i = 0; i < newRow->numArticulationNodes; i++) {
        node_id articulationNode = newRow->articulationNodes[i];
        assert(nodeIsRepresentative(dec, articulationNode));
        bool isOnPath = nodeNumPaths[articulationNode] == numCutEdges;
        if (isOnPath &&
            (NodePairIsEmpty(pair) || pair->first == articulationNode || pair->second == articulationNode)) {
            bool isGood = true;
            rigidConnectedColoring(dec, newRow, toCheck, articulationNode, &isGood);
            if (isGood) {
                NodePairInsert(&newRow->reducedMembers[toCheck].splitting_nodes, articulationNode);

                edge_id adjacentSplittingEdge = INVALID_EDGE;
                node_id adjacentSplittingNode = checkNeighbourColoringArticulationNode(dec, newRow, articulationNode,&adjacentSplittingEdge);
                if (nodeIsValid(adjacentSplittingNode) &&
                    (NodePairIsEmpty(pair) || pair->first == adjacentSplittingNode ||
                     pair->second == adjacentSplittingNode)) {
                    bool isArticulationNode = false;
                    for (int j = 0; j < newRow->numArticulationNodes; ++j) {
                        if (newRow->articulationNodes[j] == adjacentSplittingNode) {
                            isArticulationNode = true;
                            break;
                        }
                    }
                    if (isArticulationNode) {
                        NodePairInsert(&newRow->reducedMembers[toCheck].splitting_nodes, adjacentSplittingNode);
                        assert(NodePairHasTwo(&newRow->reducedMembers[toCheck].splitting_nodes));
                        newRow->reducedMembers[toCheck].articulationEdge = adjacentSplittingEdge;
                        //Cleaning up the colors for next iterations...
                        {
                            edge_id firstNodeEdge = getFirstNodeEdge(dec, articulationNode);
                            edge_id itEdge = firstNodeEdge;
                            do {
                                node_id head = findEdgeHead(dec, itEdge);
                                node_id tail = findEdgeTail(dec, itEdge);
                                node_id otherNode = articulationNode == head ? tail : head;
                                newRow->nodeColors[otherNode] = UNCOLORED;
                                itEdge = getNextNodeEdge(dec, itEdge, articulationNode);
                            } while (itEdge != firstNodeEdge);

                        }
                    }
                }
                break;
            }
        }
    }

}

static void determineTypeRigid(Decomposition *dec, SPQRNewRow *newRow,
                        const reduced_member_id toCheckMember, const edge_id markerToOther,
                        const reduced_member_id otherMember, const edge_id markerToCheck){
    assert(newRow->reducedMembers[toCheckMember].numCutEdges > 0);//Checking for propagation only makes sense if there is at least one cut edge
    NodePair * nodePair = &newRow->reducedMembers[toCheckMember].splitting_nodes;
    {
        node_id head = findEdgeHead(dec, markerToOther);
        node_id tail = findEdgeTail(dec, markerToOther);
        NodePairInitialize(nodePair,head,tail);
    }
    if(newRow->reducedMembers[toCheckMember].numCutEdges == 1){
        //If there is a single cut edge, the two splitting points are its ends and thus there is no propagation

        edge_id cutEdge = newRow->cutEdges[newRow->reducedMembers[toCheckMember].firstCutEdge].edge;
        node_id head = findEdgeHead(dec,cutEdge);
        node_id tail = findEdgeTail(dec,cutEdge);
        NodePairIntersection(nodePair, head, tail);

        if(!NodePairIsEmpty(nodePair)){
            newRow->reducedMembers[toCheckMember].type = TYPE_MERGED;
            newRow->reducedMembers[toCheckMember].allHaveCommonNode = true;
        }else{
            newRow->reducedMembers[toCheckMember].type = TYPE_NOT_GRAPHIC;
            newRow->remainsGraphic = false;
        }
        assert(!NodePairHasTwo(nodePair)); //if this is not the case, there is a parallel edge
        return;
    }
    assert(newRow->reducedMembers[toCheckMember].numCutEdges > 1);
    edge_id articulationEdge = INVALID_EDGE;
    NodePair pair = rigidDetermineAllAdjacentSplittableNodes(dec,newRow,toCheckMember,&articulationEdge);
    if(!NodePairIsEmpty(&pair)){
        if(edgeIsValid(articulationEdge)){
            if(articulationEdge == markerToOther){
                assert(edgeIsTree(dec,markerToOther));
                assert(NodePairHasTwo(&pair));
                newRow->reducedMembers[toCheckMember].type = TYPE_PROPAGATED;
                createCutEdge(newRow,markerToCheck,otherMember);
                return;
            }
            NodePairIntersection(nodePair,pair.first,pair.second);
            assert(!NodePairHasTwo(nodePair)); // graph was not simple
            if(!NodePairIsEmpty(nodePair)){
                newRow->reducedMembers[toCheckMember].allHaveCommonNode = true;
                if(nodePair->first == pair.second){
                    newRow->reducedMembers[toCheckMember].articulationEdge = articulationEdge;
                }

                newRow->reducedMembers[toCheckMember].type = TYPE_MERGED;
            }else{
                newRow->reducedMembers[toCheckMember].type = TYPE_NOT_GRAPHIC;
                newRow->remainsGraphic = false;
            }

            return;
        }
        assert(!NodePairHasTwo(&pair));
        NodePairIntersection(nodePair,pair.first,pair.second);
        if(NodePairIsEmpty(nodePair)){
            newRow->reducedMembers[toCheckMember].type = TYPE_NOT_GRAPHIC;
            newRow->remainsGraphic = false;
        }else{
            newRow->reducedMembers[toCheckMember].allHaveCommonNode = true;
            newRow->reducedMembers[toCheckMember].type = TYPE_MERGED;
        }
        return;
    }

    NodePair copy = newRow->reducedMembers[toCheckMember].splitting_nodes; ; //At this point, this is simply the two nodes which were adjacent
    NodePairEmptyInitialize(nodePair);
    //TODO: below function mutates this internally... use a cleaner interface
    rigidGetSplittableArticulationPointsOnPath(dec,newRow,toCheckMember,&copy);

    if(NodePairIsEmpty(nodePair)){
        newRow->reducedMembers[toCheckMember].type = TYPE_NOT_GRAPHIC;
        newRow->remainsGraphic = false;
        return;
    }else if (NodePairHasTwo(nodePair)){
        assert(findEdgeHead(dec,markerToOther) == nodePair->first || findEdgeHead(dec,markerToOther) == nodePair->second);
        assert(findEdgeTail(dec,markerToOther) == nodePair->first || findEdgeTail(dec,markerToOther) == nodePair->second);
        newRow->reducedMembers[toCheckMember].type = TYPE_PROPAGATED;
        createCutEdge(newRow,markerToCheck,otherMember);
        return;
    }else{
        newRow->reducedMembers[toCheckMember].type = TYPE_MERGED;
        return;
    }
}

static ReducedMemberType determineType(Decomposition *dec, SPQRNewRow *newRow,
                                const reduced_member_id toCheckMember,const edge_id markerToOther,
                                const reduced_member_id otherMember,const edge_id markerToCheck){
    assert(newRow->reducedMembers[toCheckMember].type == TYPE_UNDETERMINED);
    switch (getMemberType(dec,newRow->reducedMembers[toCheckMember].member)) {
        case RIGID:
        {
            determineTypeRigid(dec,newRow,toCheckMember,markerToOther,otherMember,markerToCheck);
            break;

        }
        case PARALLEL:
        {
            //we can only propagate if the marker edge is a tree edge and all other edges are cut
            if(edgeIsTree(dec,markerToOther) &&
               newRow->reducedMembers[toCheckMember].numCutEdges == (getNumMemberEdges(dec,newRow->reducedMembers[toCheckMember].member) - 1)){
                newRow->reducedMembers[toCheckMember].type = TYPE_PROPAGATED;
                createCutEdge(newRow,markerToCheck,otherMember); //TODO: remove old cut edges? Or reuse memory maybe?
            }else{
                //In all other cases, the bond can be split so that the result will be okay!
                newRow->reducedMembers[toCheckMember].type = TYPE_MERGED;
            }
            break;
        }
        case SERIES:
        {
            //Propagation only calls this function if the edge is tree already, so we do not check it here.
            assert(edgeIsTree(dec,markerToOther));
            assert(newRow->reducedMembers[toCheckMember].numCutEdges == 1);
            newRow->reducedMembers[toCheckMember].type = TYPE_PROPAGATED;
            createCutEdge(newRow,markerToCheck,otherMember); //TODO: remove old cut edges? Or reuse memory maybe?
            break;
        }
        default:
            assert(false);
            newRow->reducedMembers[toCheckMember].type = TYPE_NOT_GRAPHIC;
    }

    return newRow->reducedMembers[toCheckMember].type;
}

static void propagateComponents(Decomposition *dec, SPQRNewRow *newRow){
    int leafArrayIndex = 0;

    reduced_member_id leaf;
    reduced_member_id next;

    while(leafArrayIndex != newRow->numLeafMembers){
        leaf = newRow->leafMembers[leafArrayIndex];
        //next is invalid if the member is not in the reduced decomposition.
        next = newRow->reducedMembers[leaf].parent;
        edge_id parentMarker = markerToParent(dec,newRow->reducedMembers[leaf].member);
        if(next != INVALID_REDUCED_MEMBER && edgeIsTree(dec,parentMarker)){
            assert(reducedMemberIsValid(next));
            assert(edgeIsValid(parentMarker));
            ReducedMemberType type = determineType(dec,newRow,leaf,parentMarker,next,markerOfParent(dec,newRow->reducedMembers[leaf].member));
            if(type == TYPE_PROPAGATED){
                ++newRow->reducedMembers[next].numPropagatedChildren;
                if(newRow->reducedMembers[next].numPropagatedChildren == newRow->reducedMembers[next].numChildren){
                    newRow->leafMembers[leafArrayIndex] = next;
                }else{
                    ++leafArrayIndex;
                }
            }else if(type == TYPE_NOT_GRAPHIC){
                return;
            }else{
                assert(type == TYPE_MERGED);
                ++leafArrayIndex;
            }
        }else{
            ++leafArrayIndex;
        }

    }

    for (int j = 0; j < newRow->numReducedComponents; ++j) {
        //The reduced root might be a leaf as well: we propagate it last
        reduced_member_id root = newRow->reducedComponents[j].root;

        while(true){
            if(newRow->reducedMembers[root].numPropagatedChildren == newRow->reducedMembers[root].numChildren -1){
                //TODO: bit ugly, have to do a linear search for the child
                reduced_member_id child = INVALID_REDUCED_MEMBER;
                edge_id markerToChild = INVALID_EDGE;
                for (children_idx i = newRow->reducedMembers[root].firstChild; i < newRow->reducedMembers[root].firstChild + newRow->reducedMembers[root].numChildren; ++i) {
                    reduced_member_id childReduced = newRow->childrenStorage[i];
                    if(newRow->reducedMembers[childReduced].type != TYPE_PROPAGATED){
                        child = childReduced;
                        markerToChild = markerOfParent(dec,newRow->reducedMembers[child].member);
                        break;
                    }
                }
                assert(memberIsValid(newRow->reducedMembers[child].member));
                assert(edgeIsValid(markerToChild));
                if(!edgeIsTree(dec,markerToChild)){
                    break;
                }
                ReducedMemberType type = determineType(dec,newRow,root,markerToChild,child,markerToParent(dec,newRow->reducedMembers[child].member));
                if(type == TYPE_PROPAGATED){
                    root = child;
                }else if(type == TYPE_NOT_GRAPHIC){
                    return;
                }else{
                    break;
                }
            }else{
                break;
            }
        }
        newRow->reducedComponents[j].root = root;
        newRow->reducedMembers[root].parent = INVALID_REDUCED_MEMBER;
    }
}

static NodePair
rigidDetermineCandidateNodesFromAdjacentComponents(Decomposition *dec, SPQRNewRow *newRow, const reduced_member_id toCheck) {

    NodePair pair;
    NodePairEmptyInitialize(&pair);
    NodePair * nodePair = &pair;
    assert(NodePairIsEmpty(nodePair));

    //take union of children's edges nodes to find one or two candidates
    for (children_idx i = newRow->reducedMembers[toCheck].firstChild;
         i < newRow->reducedMembers[toCheck].firstChild + newRow->reducedMembers[toCheck].numChildren; ++i) {
        reduced_member_id reducedChild = newRow->childrenStorage[i];
        if (newRow->reducedMembers[reducedChild].type != TYPE_PROPAGATED) {
            edge_id edge = markerOfParent(dec, newRow->reducedMembers[reducedChild].member);
            node_id head = findEdgeHead(dec, edge);
            node_id tail = findEdgeTail(dec, edge);
            if(NodePairIsEmpty(nodePair)){
                NodePairInitialize(nodePair,head,tail);
            }else{
                NodePairIntersection(nodePair, head, tail);
            }
            if (NodePairIsEmpty(nodePair)) {
                return pair;
            }
        }
    }
    if (reducedMemberIsValid(newRow->reducedMembers[toCheck].parent) &&
        newRow->reducedMembers[newRow->reducedMembers[toCheck].parent].type != TYPE_PROPAGATED) {

        edge_id edge = markerToParent(dec, newRow->reducedMembers[toCheck].member);
        node_id head = findEdgeHead(dec, edge);
        node_id tail = findEdgeTail(dec, edge);
        if(NodePairIsEmpty(nodePair)){
            NodePairInitialize(nodePair,head,tail);
        }else{
            NodePairIntersection(nodePair, head, tail);
        }
    }
    return pair;
}

static void determineTypeRigidMerging(Decomposition *dec, SPQRNewRow *newRow, const reduced_member_id toCheck){
    NodePair * nodePair = &newRow->reducedMembers[toCheck].splitting_nodes;
    bool hasNoAdjacentMarkers = (newRow->reducedMembers[toCheck].numChildren - newRow->reducedMembers[toCheck].numPropagatedChildren) == 0 &&
                                (reducedMemberIsInvalid(newRow->reducedMembers[toCheck].parent) ||
                                 newRow->reducedMembers[newRow->reducedMembers[toCheck].parent].type == TYPE_PROPAGATED);
    //if the component is free standing
    if(hasNoAdjacentMarkers){
        assert(newRow->reducedMembers[toCheck].numCutEdges> 0);
        if(newRow->reducedMembers[toCheck].numCutEdges == 1){
            //TODO: with two splitting nodes maybe store the cut edge?
            newRow->reducedMembers[toCheck].type = TYPE_MERGED;
        }else{

            //take union of edge ends
            cut_edge_id cutEdgeIdx = newRow->reducedMembers[toCheck].firstCutEdge;
            edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
            node_id head = findEdgeHead(dec, cutEdge);
            node_id tail = findEdgeTail(dec, cutEdge);
            NodePairInitialize(nodePair,head,tail);

            while(cutEdgeIsValid(newRow->cutEdges[cutEdgeIdx].nextMember)){
                cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
                cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
                head = findEdgeHead(dec, cutEdge);
                tail = findEdgeTail(dec, cutEdge);
                NodePairIntersection(nodePair, head, tail);

                if(NodePairIsEmpty(nodePair)){
                    break;
                }
            }
            //Since there is two ore more edges, there can be at most one splitting node:
            assert(!NodePairHasTwo(nodePair));
            if(!NodePairIsEmpty(nodePair)){
                //All cut edges are adjacent to one node; either this node can be 'extended' or if the number of cut edges == degree of node -1,
                //the other edge is placed in series with the new row edge
                newRow->reducedMembers[toCheck].allHaveCommonNode = true;
                newRow->reducedMembers[toCheck].type = TYPE_MERGED;
                return;
            }
            NodePair emptyPair;
            NodePairEmptyInitialize(&emptyPair);
            rigidGetSplittableArticulationPointsOnPath(dec,newRow,toCheck,&emptyPair);
            if(NodePairIsEmpty(nodePair)){
                newRow->reducedMembers[toCheck].type = TYPE_NOT_GRAPHIC;
                newRow->remainsGraphic = false;
                return;
            }else{
                newRow->reducedMembers[toCheck].type = TYPE_MERGED;
                return;
            }
        }
        return;
    }

    NodePair adjacentMarkers = rigidDetermineCandidateNodesFromAdjacentComponents(dec,newRow,toCheck);

    if(NodePairIsEmpty(&adjacentMarkers)){
        NodePairEmptyInitialize(&newRow->reducedMembers[toCheck].splitting_nodes);
        newRow->reducedMembers[toCheck].type = TYPE_NOT_GRAPHIC;
        newRow->remainsGraphic = false;
        return;
    }
    //Check if all edges are adjacent or have an articulation point
    if(newRow->reducedMembers[toCheck].numCutEdges > 0){
        edge_id articulationEdgeToSecond = INVALID_EDGE;
        NodePair edgeAdjacentPair = rigidDetermineAllAdjacentSplittableNodes(dec,newRow,toCheck,&articulationEdgeToSecond);
        if(!NodePairIsEmpty(&edgeAdjacentPair)){
            NodePairIntersection(&adjacentMarkers,edgeAdjacentPair.first,edgeAdjacentPair.second);
            if(NodePairIsEmpty(&adjacentMarkers)){
                NodePairEmptyInitialize(&newRow->reducedMembers[toCheck].splitting_nodes);
                newRow->reducedMembers[toCheck].type = TYPE_NOT_GRAPHIC;
                newRow->remainsGraphic = false;
            }else{
                newRow->reducedMembers[toCheck].allHaveCommonNode = true;
                assert(!NodePairHasTwo(&adjacentMarkers)); //graph should have been simple
                if(edgeIsValid(articulationEdgeToSecond) && adjacentMarkers.first == edgeAdjacentPair.second){
                    newRow->reducedMembers[toCheck].articulationEdge = articulationEdgeToSecond;
                }
                newRow->reducedMembers[toCheck].type = TYPE_MERGED;
                NodePairEmptyInitialize(&newRow->reducedMembers[toCheck].splitting_nodes);
                NodePairInsert(&newRow->reducedMembers[toCheck].splitting_nodes,adjacentMarkers.first);
            }
            return;
        }


        assert(NodePairIsEmpty(nodePair));
        rigidGetSplittableArticulationPointsOnPath(dec,newRow,toCheck,&adjacentMarkers);
        if(NodePairIsEmpty(nodePair)){
            newRow->reducedMembers[toCheck].type = TYPE_NOT_GRAPHIC;
            newRow->remainsGraphic = false;
            return;
        }
        assert(!NodePairHasTwo(nodePair)); //Graph was not simple
        newRow->reducedMembers[toCheck].type = TYPE_MERGED;
        return;
    }

    //No cut edges: simply take the point of the adjacent markers
    assert(!NodePairHasTwo(&adjacentMarkers));
    NodePairEmptyInitialize(&newRow->reducedMembers[toCheck].splitting_nodes);
    NodePairInsert(&newRow->reducedMembers[toCheck].splitting_nodes,adjacentMarkers.first);
    newRow->reducedMembers[toCheck].type = TYPE_MERGED;
}

static ReducedMemberType determineTypeMerging(Decomposition *dec, SPQRNewRow *newRow, const reduced_member_id toCheck){
    switch(getMemberType(dec,newRow->reducedMembers[toCheck].member)){
        case RIGID:
        {
            determineTypeRigidMerging(dec,newRow,toCheck);
            break;
        }
        case PARALLEL:
        {
            newRow->reducedMembers[toCheck].type = TYPE_MERGED;
            break;
        }
        case SERIES:
        {
            int numNonPropagatedAdjacent = newRow->reducedMembers[toCheck].numChildren-newRow->reducedMembers[toCheck].numPropagatedChildren;
            if(reducedMemberIsValid(newRow->reducedMembers[toCheck].parent) &&
               newRow->reducedMembers[newRow->reducedMembers[toCheck].parent].type != TYPE_PROPAGATED){
                ++numNonPropagatedAdjacent;
            }

            assert(numNonPropagatedAdjacent != 1);
            if(numNonPropagatedAdjacent > 2){
                newRow->reducedMembers[toCheck].type = TYPE_NOT_GRAPHIC;
                newRow->remainsGraphic = false;
            }else{
                newRow->reducedMembers[toCheck].type = TYPE_MERGED;
            }
            break;
        }
        default:
        {
            assert(false);
            newRow->reducedMembers[toCheck].type = TYPE_NOT_GRAPHIC;
            break;
        }
    }
    return newRow->reducedMembers[toCheck].type;
}

static SPQR_ERROR allocateTreeSearchMemory(Decomposition *dec, SPQRNewRow *newRow){
    int necessarySpace = newRow->numReducedMembers;
    if( necessarySpace > newRow->memMergeTreeCallData ){
        newRow->memMergeTreeCallData = max(2*newRow->memMergeTreeCallData,necessarySpace);
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newRow->mergeTreeCallData,(size_t) newRow->memMergeTreeCallData));
    }
    return SPQR_OKAY;
}

static void determineMergeableTypes(Decomposition *dec, SPQRNewRow *newRow, reduced_member_id root){
    assert(newRow->numReducedMembers <= newRow->memMergeTreeCallData);

    int depth = 0;
    MergeTreeCallData * stack = newRow->mergeTreeCallData;

    stack[0].currentChild = newRow->reducedMembers[root].firstChild;
    stack[0].id = root;

    //First determine type of all children, then determine type of the node itself
    do{
        if(stack[depth].currentChild == newRow->reducedMembers[stack[depth].id].firstChild +
                                        newRow->reducedMembers[stack[depth].id].numChildren){
            if(newRow->reducedMembers[stack[depth].id].type == TYPE_UNDETERMINED){
                ReducedMemberType type = determineTypeMerging(dec,newRow,stack[depth].id);
                if(type == TYPE_NOT_GRAPHIC){
                    assert(!newRow->remainsGraphic);
                    break;
                }

            }
            --depth;
            continue; //does this break when necessary?
        }
        reduced_member_id reducedChild = newRow->childrenStorage[stack[depth].currentChild];
        ++stack[depth].currentChild;
        if (newRow->reducedMembers[reducedChild].type != TYPE_PROPAGATED) {
            ++depth;
            assert(depth < newRow->memMergeTreeCallData);
            stack[depth].id = reducedChild;
            stack[depth].currentChild = newRow->reducedMembers[reducedChild].firstChild;
        }
    }while(depth >= 0);
}

static void cleanUpMemberInformation(SPQRNewRow * newRow){
    //This loop is at the end as memberInformation is also used to assign the cut edges during propagation
    //Clean up the memberInformation array
    for (int i = 0; i < newRow->numReducedMembers; ++i) {
        newRow->memberInformation[newRow->reducedMembers[i].member].reducedMember = INVALID_REDUCED_MEMBER;
    }
#ifndef NDEBUG
    for (int i = 0; i < newRow->memMemberInformation; ++i) {
        assert(reducedMemberIsInvalid(newRow->memberInformation[i].reducedMember));
    }
#endif
}

static SPQR_ERROR rigidTransformEdgeIntoCycle(Decomposition *dec,
                                              const member_id member,
                                              const edge_id edge,
                                              NewRowInformation * const newRowInformation){
    //If a cycle already exists, just expand it with the new edge.
    member_id markerCycleMember = INVALID_MEMBER;
    if (edge == markerToParent(dec, member)) {
        member_id parentMember = findMemberParent(dec, member);
        if (getMemberType(dec, parentMember) == SERIES) {
            markerCycleMember = parentMember;
        }
    } else if (edgeIsMarker(dec, edge)) {
        member_id childMember = findEdgeChildMember(dec, edge);
        if (getMemberType(dec, childMember) == SERIES) {
            markerCycleMember = childMember;
        }
    }
    if (markerCycleMember != INVALID_MEMBER) {
        newRowInformation->member = markerCycleMember;
        return SPQR_OKAY;
    }

    //create a new cycle
    member_id new_cycle;
    SPQR_CALL(createMember(dec,SERIES,&new_cycle));
    bool isTreeEdge = edgeIsTree(dec,edge);
    bool parentMoved = markerToParent(dec,member) == edge;
    moveEdgeToNewMember(dec,edge,member,new_cycle);
    node_id cutHead = findEdgeHead(dec,edge);
    node_id cutTail = findEdgeTail(dec,edge);
    clearEdgeHeadAndTail(dec,edge);

    edge_id ignore = INVALID_EDGE;
    edge_id rigidMarker = INVALID_EDGE;

    if(parentMoved){
        SPQR_CALL(createMarkerPairWithReferences(dec,new_cycle,member,!isTreeEdge,&ignore,&rigidMarker));
    }else{
        SPQR_CALL(createMarkerPairWithReferences(dec,member,new_cycle,isTreeEdge,&rigidMarker,&ignore));

    }
    setEdgeHeadAndTail(dec,rigidMarker,cutHead,cutTail);

    newRowInformation->member = new_cycle;

    return SPQR_OKAY;
}

static SPQR_ERROR transformSingleRigid(Decomposition *dec, SPQRNewRow *newRow,
                                       const reduced_member_id reducedMember,
                                       const member_id member,
                                       NewRowInformation * const newRowInformation){
    if(newRow->reducedMembers[reducedMember].numCutEdges == 1){
        //Cut edge is propagated into a cycle with new edge
        edge_id cutEdge = newRow->cutEdges[newRow->reducedMembers[reducedMember].firstCutEdge].edge;
        SPQR_CALL(rigidTransformEdgeIntoCycle(dec,member,cutEdge,newRowInformation));
        return SPQR_OKAY;
    }
    assert(newRow->reducedMembers[reducedMember].numCutEdges > 1);
    assert(!NodePairIsEmpty(&newRow->reducedMembers[reducedMember].splitting_nodes));

    node_id splitNode = newRow->reducedMembers[reducedMember].splitting_nodes.first;

    //For now this code assumes that all nodes are adjacent to the split node!!
    if(newRow->reducedMembers[reducedMember].allHaveCommonNode){
        if (newRow->reducedMembers[reducedMember].numCutEdges != nodeDegree(dec, splitNode) - 1) {
            //Create a new node; move all cut edges end of split node to it and add new edge between new node and split node
            node_id newNode = INVALID_NODE;
            SPQR_CALL(createNode(dec, &newNode));
            newRowInformation->member = member;
            newRowInformation->firstNode = newNode;
            newRowInformation->secondNode = splitNode;

            cut_edge_id cutEdgeIdx = newRow->reducedMembers[reducedMember].firstCutEdge;
            do {
                edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
                node_id edgeHead = findEdgeHead(dec, cutEdge);
                if (edgeHead == splitNode) {
                    changeEdgeHead(dec, cutEdge, edgeHead, newNode);
                } else {
                    changeEdgeTail(dec, cutEdge, findEdgeTail(dec, cutEdge), newNode);
                }

                cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
            } while (cutEdgeIsValid(cutEdgeIdx));

            return SPQR_OKAY;
        } else {
            //We need to find the non-cut edge. By definition, this must be a tree edge
            //TODO: searching for the edge is a bit ugly, but we need to do it somewhere

            edge_id firstEdge = getFirstNodeEdge(dec, splitNode);
            edge_id moveEdge = firstEdge;
            do {
                if (edgeIsTree(dec, moveEdge)) {
                    break;
                }
                moveEdge = getNextNodeEdge(dec, moveEdge, splitNode);
            } while (moveEdge != firstEdge);
            assert(edgeIsTree(dec, moveEdge));

            SPQR_CALL(rigidTransformEdgeIntoCycle(dec,member,moveEdge,newRowInformation));
            return SPQR_OKAY;
        }
    }


    if(NodePairHasTwo(&newRow->reducedMembers[reducedMember].splitting_nodes)){
        //Two splitting nodes: identify the edge and either make it a cycle or prolong it, if it is already a marker to a cycle
        edge_id makeCycleEdge = newRow->reducedMembers[reducedMember].articulationEdge;

        assert(edgeIsTree(dec,makeCycleEdge));
        assert(edgeIsValid(makeCycleEdge));

        SPQR_CALL(rigidTransformEdgeIntoCycle(dec,member,makeCycleEdge,newRowInformation));
        return SPQR_OKAY;
    }
    assert(!NodePairIsEmpty(&newRow->reducedMembers[reducedMember].splitting_nodes));
    //Multiple edges without a common end: need to use coloring information
    int numFirstColor = 0;
    int numSecondColor = 0;

    edge_id firstNodeEdge = getFirstNodeEdge(dec,splitNode);
    edge_id iterEdge = firstNodeEdge;
    do{
        node_id head = findEdgeHead(dec,iterEdge);
        node_id tail = findEdgeTail(dec,iterEdge);
        node_id other = head == splitNode ? tail : head;
        if(newRow->nodeColors[other] == COLOR_FIRST){
            numFirstColor++;
        }else{
            numSecondColor++;
        }
        iterEdge = getNextNodeEdge(dec,iterEdge,splitNode);
    }while(iterEdge != firstNodeEdge);

    COLOR_STATUS toNewNodeColor = numFirstColor < numSecondColor ? COLOR_FIRST : COLOR_SECOND; //TODO: sanity check this?

    node_id newNode = INVALID_NODE;
    SPQR_CALL(createNode(dec, &newNode));


    //TODO: fix duplication? (with where?)
    {
        firstNodeEdge = getFirstNodeEdge(dec,splitNode);
        iterEdge = firstNodeEdge;
        do{
            bool isCut = newRow->isEdgeCut[iterEdge];
            node_id otherHead = findEdgeHead(dec,iterEdge);
            node_id otherTail = findEdgeTail(dec,iterEdge);
            node_id otherEnd = otherHead == splitNode ? otherTail : otherHead;
            bool isMoveColor = newRow->nodeColors[otherEnd] == toNewNodeColor;
            edge_id nextEdge = getNextNodeEdge(dec,iterEdge,splitNode); //Need to do this before we modify the edge :)

            bool changeEdgeEnd = (isCut && isMoveColor) || (!isCut && !isMoveColor);
            if(changeEdgeEnd){
                if(otherHead == splitNode){
                    changeEdgeHead(dec,iterEdge,otherHead,newNode);
                }else{
                    changeEdgeTail(dec,iterEdge,otherTail,newNode);
                }
            }
            newRow->nodeColors[otherEnd] = UNCOLORED; //Clean up

            //Ugly hack to make sure we can iterate neighbourhood whilst changing edge ends.
            edge_id previousEdge = iterEdge;
            iterEdge = nextEdge;
            if(iterEdge == firstNodeEdge){
                break;
            }
            if(changeEdgeEnd && previousEdge == firstNodeEdge){
                firstNodeEdge = iterEdge;
            }
        }while(true);
        newRow->reducedMembers[reducedMember].coloredNode = INVALID_NODE;
    }
    newRowInformation->member = member;
    newRowInformation->firstNode = splitNode;
    newRowInformation->secondNode = newNode;
    return SPQR_OKAY;
}


static SPQR_ERROR splitParallel(Decomposition *dec, SPQRNewRow *newRow,
                                const reduced_member_id reducedMember,
                                const member_id member,
                                member_id * const loopMember){
    assert(newRow->reducedMembers[reducedMember].numCutEdges > 0);

    int numCutEdges = newRow->reducedMembers[reducedMember].numCutEdges;
    int numParallelEdges = getNumMemberEdges(dec,member);

    bool createCutParallel = numCutEdges > 1;
    bool convertOriginalParallel = (numCutEdges + 1) == numParallelEdges;

    if(!createCutParallel && convertOriginalParallel){
        assert(getNumMemberEdges(dec,member) == 2);
        assert(newRow->reducedMembers[reducedMember].numCutEdges == 1);
        *loopMember = member;
        return SPQR_OKAY;
    }

    member_id cutMember = INVALID_MEMBER;
    SPQR_CALL(createMember(dec,PARALLEL,&cutMember));

    cut_edge_id cutEdgeIdx = newRow->reducedMembers[reducedMember].firstCutEdge;
    assert(cutEdgeIsValid(cutEdgeIdx));
    bool parentCut = false;

    while(cutEdgeIsValid(cutEdgeIdx)){
        edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
        cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
        moveEdgeToNewMember(dec,cutEdge,member,cutMember);
        if (cutEdge == markerToParent(dec,member)){
            parentCut = true;
        }
    }
    if(convertOriginalParallel == createCutParallel){
        if(parentCut){
            SPQR_CALL(createMarkerPair(dec,cutMember,member,true));
        }else{
            SPQR_CALL(createMarkerPair(dec,member,cutMember,false));
        }
        *loopMember = convertOriginalParallel ? member : cutMember;
        return SPQR_OKAY;
    }

    SPQR_CALL(createMember(dec,PARALLEL,loopMember));

    //TODO: check tree directions here...
    //TODO Can probably eliminate some branches here
    if(parentCut){
        SPQR_CALL(createMarkerPair(dec,cutMember,*loopMember,true));
        SPQR_CALL(createMarkerPair(dec,*loopMember,member,true));
    }else{
        SPQR_CALL(createMarkerPair(dec,member,*loopMember,false));
        SPQR_CALL(createMarkerPair(dec,*loopMember,cutMember,false));
    }
    return SPQR_OKAY;
}

static SPQR_ERROR transformSingleParallel(Decomposition *dec, SPQRNewRow *newRow,
                                          const reduced_member_id reducedMember,
                                          const member_id member,
                                          member_id * const newRowMember){
    member_id loopMember = INVALID_MEMBER;
    SPQR_CALL(splitParallel(dec,newRow,reducedMember,member,&loopMember));

    //TODO: fix with adjacent cycle
    //Make loop member a series and elongate it with the new row,
    //The only exception is if the non-cut edge is a child or parent marker to a series. In that case, the series member is elongated and the loop is merged
    //TODO: now we iterate to find the non-cut edge, with better memory layout this would be much simpler

    bool adjacent_series_marker = false;
    {
        edge_id first_edge = getFirstMemberEdge(dec, loopMember);
        edge_id edge = first_edge;

        do {
            if(edgeIsMarker(dec,edge)){
                member_id child = findEdgeChildMember(dec,edge);
                if(getMemberType(dec,child) == SERIES){
                    adjacent_series_marker = true;
                    *newRowMember = child;
                    break;
                }
            }else if(markerToParent(dec,member) == edge) {
                member_id parent = findMemberParent(dec, loopMember);
                if (getMemberType(dec, parent) == SERIES) {
                    adjacent_series_marker = true;
                    *newRowMember = parent;
                    break;
                }
            }
            edge = getNextMemberEdge(dec, edge);
        } while (edge != first_edge);
    }
    if(!adjacent_series_marker){
        changeLoopToSeries(dec,loopMember);
        *newRowMember = loopMember;
    }else{
        mergeLoop(dec, loopMember);
    }



    return SPQR_OKAY;
}

/**
 * Splits a parallel member into two parallel members connected by a loop, based on which edges are cut.
 * For both of the bonds if they would have only 2 edges, they are merged into the middle bond
 * @return
 */
static SPQR_ERROR splitParallelMerging(Decomposition *dec, SPQRNewRow *newRow,
                                       const reduced_member_id reducedMember,
                                       const member_id member,
                                       member_id * const pMergeMember,
                                       edge_id * const cutRepresentative){
    //When merging, we cannot have propagated members;
    assert(newRow->reducedMembers[reducedMember].numCutEdges < (getNumMemberEdges(dec,member)-1));

    int numMergeableAdjacent = newRow->reducedMembers[reducedMember].numChildren - newRow->reducedMembers[reducedMember].numPropagatedChildren;
    if(reducedMemberIsValid(newRow->reducedMembers[reducedMember].parent) &&
       newRow->reducedMembers[newRow->reducedMembers[reducedMember].parent].type == TYPE_MERGED){
        numMergeableAdjacent++;
    }

    int numCutEdges = newRow->reducedMembers[reducedMember].numCutEdges;
    //All edges which are not in the mergeable decomposition or cut
    int numBaseSplitAwayEdges = getNumMemberEdges(dec,member) - numMergeableAdjacent - numCutEdges ;

    bool createCutParallel = numCutEdges > 1;
    bool keepOriginalParallel = numBaseSplitAwayEdges  <= 1;

    bool parentCut = false;

    member_id cutMember = INVALID_MEMBER;
    if(createCutParallel){
        SPQR_CALL(createMember(dec,PARALLEL,&cutMember));

        cut_edge_id cutEdgeIdx = newRow->reducedMembers[reducedMember].firstCutEdge;
        assert(cutEdgeIsValid(cutEdgeIdx));

        while(cutEdgeIsValid(cutEdgeIdx)){
            edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
            cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
            moveEdgeToNewMember(dec,cutEdge,member,cutMember);
            if (cutEdge == markerToParent(dec,member)){
                parentCut = true;
            }
        }

    }else if(numCutEdges == 1){
        *cutRepresentative = newRow->cutEdges[newRow->reducedMembers[reducedMember].firstCutEdge].edge;
    }

    edge_id noCutRepresentative = INVALID_EDGE;
    member_id mergingMember = member;
    bool parentToMergingMember = false;
    bool treeToMergingMember = false;
    if(!keepOriginalParallel){
        SPQR_CALL(createMember(dec,PARALLEL,&mergingMember));
        //move all mergeable children and parent edges to the mergingMember
        for (children_idx i = newRow->reducedMembers[reducedMember].firstChild;
             i < newRow->reducedMembers[reducedMember].firstChild + newRow->reducedMembers[reducedMember].numChildren; ++i) {
            reduced_member_id child = newRow->childrenStorage[i];
            if(newRow->reducedMembers[child].type == TYPE_MERGED){
                edge_id moveEdge = markerOfParent(dec,newRow->reducedMembers[child].member);
                moveEdgeToNewMember(dec,moveEdge,member,mergingMember);
                if(edgeIsTree(dec,moveEdge)){
                    treeToMergingMember = true;
                }
            }
        }
        reduced_member_id parent = newRow->reducedMembers[reducedMember].parent;
        if(reducedMemberIsValid(parent) &&
           newRow->reducedMembers[parent].type == TYPE_MERGED && !parentCut){
            edge_id moveEdge = markerToParent(dec,member);
            moveEdgeToNewMember(dec,moveEdge,member,mergingMember);
            parentToMergingMember = true;
            if(edgeIsTree(dec,moveEdge)){
                treeToMergingMember = true;
            }
        }
        //If there is only one cut edge, we also move it.
        if(edgeIsValid(*cutRepresentative)){
            if(*cutRepresentative == markerToParent(dec,member)){
                parentToMergingMember = true;
            }
            moveEdgeToNewMember(dec,*cutRepresentative,member,mergingMember);
        }
    }
    //TODO: can probably reduce branching a bit here.
    if(createCutParallel){
        edge_id ignoreArgument = INVALID_EDGE;
        if(parentCut){
            SPQR_CALL(createMarkerPairWithReferences(dec,cutMember,mergingMember,true,&ignoreArgument,cutRepresentative));
        }else{
            SPQR_CALL(createMarkerPairWithReferences(dec,mergingMember,cutMember,false,cutRepresentative,&ignoreArgument));
        }
    }
    if(!keepOriginalParallel){
        edge_id ignoreArgument = INVALID_EDGE;
        if(parentToMergingMember){
            SPQR_CALL(createMarkerPairWithReferences(dec,mergingMember,member,!treeToMergingMember,&ignoreArgument,&noCutRepresentative));
        }else{
            if(parentCut){
                SPQR_CALL(createMarkerPairWithReferences(dec,mergingMember,member,!treeToMergingMember,&ignoreArgument,&noCutRepresentative));
            }else{
                SPQR_CALL(createMarkerPairWithReferences(dec,member,mergingMember,treeToMergingMember,&noCutRepresentative,&ignoreArgument));
            }
        }
    }
    *pMergeMember = mergingMember;
    return SPQR_OKAY;
}

static SPQR_ERROR splitSeriesMerging(Decomposition *dec, SPQRNewRow *newRow,
                                     const reduced_member_id reducedMember,
                                     const member_id member,
                                     member_id * const mergingMember, bool * const isCut){
    assert(getNumMemberEdges(dec,member) >= 3);
    * isCut = newRow->reducedMembers[reducedMember].numCutEdges > 0;

    if(getNumMemberEdges(dec,member) == 3){

        *mergingMember = member;
        return SPQR_OKAY;
    }
    //Split off the relevant part of the series member
    member_id mergingSeries = INVALID_MEMBER;
    SPQR_CALL(createMember(dec,SERIES,&mergingSeries));
    //Move all marker edges which point to another component in the reduced decomposition to the new member
    //This should be exactly 2, as with 3 the result is not graphic anymore
    //move all mergeable children and parent edges to the mergingMember

    bool coTreeToMergingMember = false;
    bool parentToMergingMember = false;
    for (children_idx i = newRow->reducedMembers[reducedMember].firstChild;
         i < newRow->reducedMembers[reducedMember].firstChild + newRow->reducedMembers[reducedMember].numChildren; ++i) {
        reduced_member_id child = newRow->childrenStorage[i];
        if(newRow->reducedMembers[child].type == TYPE_MERGED){
            edge_id moveEdge = markerOfParent(dec,newRow->reducedMembers[child].member);
            moveEdgeToNewMember(dec,moveEdge,member,mergingSeries);
            if(!edgeIsTree(dec,moveEdge)){
                coTreeToMergingMember = true;
            }
        }
    }

    reduced_member_id parent = newRow->reducedMembers[reducedMember].parent;
    if(reducedMemberIsValid(parent) &&
       newRow->reducedMembers[parent].type == TYPE_MERGED ){
        edge_id moveEdge = markerToParent(dec,member);
        moveEdgeToNewMember(dec,moveEdge,member,mergingSeries);
        parentToMergingMember = true;
        if(!edgeIsTree(dec,moveEdge)){
            coTreeToMergingMember = true;
        }
    }
    if(parentToMergingMember){
        SPQR_CALL(createMarkerPair(dec,mergingSeries,member,coTreeToMergingMember));
    }else{
        SPQR_CALL(createMarkerPair(dec,member,mergingSeries,!coTreeToMergingMember));
    }

    *mergingMember = mergingSeries;
    assert(getNumMemberEdges(dec,mergingSeries) == 3 );
    assert(getNumMemberEdges(dec,member) >= 3);
    return SPQR_OKAY;
}

static SPQR_ERROR transformRoot(Decomposition *dec, SPQRNewRow *newRow,
                                const reduced_member_id reducedMember,
                                NewRowInformation * const newRowInfo){
    assert(newRow->reducedMembers[reducedMember].type == TYPE_MERGED);

    member_id member = newRow->reducedMembers[reducedMember].member;

    //For any series or parallel member, 'irrelevant parts' are first split off into separate parallel and series members
    //For series and parallel members, we need to create nodes and correctly split them

    //TODO: split up into functions
    switch(getMemberType(dec,member)){
        case RIGID: {
            node_id newNode = INVALID_NODE;
            SPQR_CALL(createNode(dec,&newNode));
            //we should have identified a unique splitting node
            assert(!NodePairIsEmpty(&newRow->reducedMembers[reducedMember].splitting_nodes)
                   && !NodePairHasTwo(&newRow->reducedMembers[reducedMember].splitting_nodes));
            node_id splitNode = newRow->reducedMembers[reducedMember].splitting_nodes.first;

            newRowInfo->firstNode = splitNode;
            newRowInfo->secondNode = newNode;
            newRowInfo->member = member;

            if(newRow->reducedMembers[reducedMember].numCutEdges == 0){
                break;
            }
            if (newRow->reducedMembers[reducedMember].allHaveCommonNode) {
                if(edgeIsValid(newRow->reducedMembers[reducedMember].articulationEdge)){
                    edge_id articulationEdge =newRow->reducedMembers[reducedMember].articulationEdge;
                    node_id cutHead = findEdgeHead(dec,articulationEdge);
                    node_id cutTail = findEdgeTail(dec,articulationEdge);
                    if(cutHead == splitNode){
                        changeEdgeHead(dec,articulationEdge,cutHead,newNode);
                    }else{
                        assert(cutTail == splitNode);
                        changeEdgeTail(dec,articulationEdge,cutTail,newNode);
                    }
                }else{

                    cut_edge_id cutEdgeIdx = newRow->reducedMembers[reducedMember].firstCutEdge;
                    do{
                        edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
                        node_id cutHead = findEdgeHead(dec,cutEdge);
                        node_id cutTail = findEdgeTail(dec,cutEdge);
                        bool moveHead = cutHead == splitNode;
                        assert((moveHead && cutHead == splitNode) || (!moveHead && cutTail == splitNode));
                        if(moveHead){
                            changeEdgeHead(dec,cutEdge,cutHead,newNode);
                        }else{
                            changeEdgeTail(dec,cutEdge,cutTail,newNode);
                        }
                        cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
                    }while(cutEdgeIsValid(cutEdgeIdx));
                }

            } else {
                //TODO: fix duplication here.
                int numFirstColor = 0;
                int numSecondColor = 0;

                edge_id firstNodeEdge = getFirstNodeEdge(dec,splitNode);
                edge_id iterEdge = firstNodeEdge;
                do{
                    node_id head = findEdgeHead(dec,iterEdge);
                    node_id tail = findEdgeTail(dec,iterEdge);
                    node_id other = head == splitNode ? tail : head;
                    if(newRow->nodeColors[other] == COLOR_FIRST){
                        numFirstColor++;
                    }else{
                        numSecondColor++;
                    }
                    iterEdge = getNextNodeEdge(dec,iterEdge,splitNode);
                }while(iterEdge != firstNodeEdge);

                COLOR_STATUS toNewNodeColor = numFirstColor < numSecondColor ? COLOR_FIRST : COLOR_SECOND;

                {

                    firstNodeEdge = getFirstNodeEdge(dec,splitNode);
                    iterEdge = firstNodeEdge;
                    do{
                        bool isCut = newRow->isEdgeCut[iterEdge];
                        node_id otherHead = findEdgeHead(dec,iterEdge);
                        node_id otherTail = findEdgeTail(dec,iterEdge);
                        node_id otherEnd = otherHead == splitNode ? otherTail : otherHead;
                        bool isMoveColor = newRow->nodeColors[otherEnd] == toNewNodeColor;
                        edge_id nextEdge = getNextNodeEdge(dec,iterEdge,splitNode); //Need to do this before we modify the edge :)

                        bool changeEdgeEnd = (isCut && isMoveColor) || (!isCut && !isMoveColor);
                        if(changeEdgeEnd){
                            if(otherHead == splitNode){
                                changeEdgeHead(dec,iterEdge,otherHead,newNode);
                            }else{
                                changeEdgeTail(dec,iterEdge,otherTail,newNode);
                            }
                        }
                        newRow->nodeColors[otherEnd] = UNCOLORED; //Clean up coloring information
                        //Ugly hack to make sure we can iterate neighbourhood whilst changing edge ends.
                        edge_id previousEdge = iterEdge;
                        iterEdge = nextEdge;
                        if(iterEdge == firstNodeEdge){
                            break;
                        }
                        if(changeEdgeEnd && previousEdge == firstNodeEdge){
                            firstNodeEdge = iterEdge;
                        }
                    }while(true);
                    newRow->reducedMembers[reducedMember].coloredNode = INVALID_NODE;
                }

            }
            break;
        }
        case PARALLEL:{
            edge_id cutRepresentative = INVALID_EDGE;
            SPQR_CALL(splitParallelMerging(dec,newRow,reducedMember,member,&newRowInfo->member,&cutRepresentative));
            node_id firstNode = INVALID_NODE;
            node_id secondNode = INVALID_NODE;
            node_id thirdNode = INVALID_NODE;
            SPQR_CALL(createNode(dec,&firstNode));
            SPQR_CALL(createNode(dec,&secondNode));
            SPQR_CALL(createNode(dec,&thirdNode));
            edge_id first_edge = getFirstMemberEdge(dec, newRowInfo->member);
            edge_id edge = first_edge;

            do {
                if(edge != cutRepresentative){
                    setEdgeHeadAndTail(dec,edge,firstNode,secondNode);
                }else{
                    setEdgeHeadAndTail(dec,edge,firstNode,thirdNode);
                }
                edge = getNextMemberEdge(dec, edge);
            } while (edge != first_edge);

            newRowInfo->firstNode = secondNode;
            newRowInfo->secondNode = thirdNode;
            updateMemberType(dec,newRowInfo->member,RIGID);

            break;
        }
        case SERIES:{
            bool isCut = false;
            SPQR_CALL(splitSeriesMerging(dec,newRow,reducedMember,member,&newRowInfo->member,&isCut));
            assert((newRow->reducedMembers[reducedMember].numChildren - newRow->reducedMembers[reducedMember].numPropagatedChildren) == 2);
            node_id firstNode = INVALID_NODE;
            node_id secondNode = INVALID_NODE;
            node_id thirdNode = INVALID_NODE;
            node_id fourthNode = INVALID_NODE;
            SPQR_CALL(createNode(dec,&firstNode));
            SPQR_CALL(createNode(dec,&secondNode));
            SPQR_CALL(createNode(dec,&thirdNode));
            SPQR_CALL(createNode(dec,&fourthNode));

            int reducedChildIndex = 0;

            edge_id reducedEdges[2];
            for (children_idx i = newRow->reducedMembers[reducedMember].firstChild;
                 i < newRow->reducedMembers[reducedMember].firstChild + newRow->reducedMembers[reducedMember].numChildren; ++i) {
                reduced_member_id child = newRow->childrenStorage[i];
                if(newRow->reducedMembers[child].type == TYPE_MERGED){
                    assert(reducedChildIndex < 2);
                    reducedEdges[reducedChildIndex] = markerOfParent(dec,newRow->reducedMembers[child].member);
                    reducedChildIndex++;
                }
            }

            edge_id first_edge = getFirstMemberEdge(dec, newRowInfo->member);
            edge_id edge = first_edge;


            do {

                if(edge == reducedEdges[0]){
                    setEdgeHeadAndTail(dec,edge,thirdNode,firstNode);
                }else if (edge == reducedEdges[1]){
                    if(isCut){
                        setEdgeHeadAndTail(dec,edge,fourthNode,secondNode);
                    }else{
                        setEdgeHeadAndTail(dec,edge,thirdNode,secondNode);
                    }
                }else{
                    setEdgeHeadAndTail(dec,edge,firstNode,secondNode);
                }
                edge = getNextMemberEdge(dec, edge);
            } while (edge != first_edge);

            newRowInfo->firstNode = thirdNode;
            newRowInfo->secondNode = fourthNode;

            updateMemberType(dec,newRowInfo->member,RIGID);
            break;
        }
        default:
            assert(false);
            break;
    }
    return SPQR_OKAY;
}
static node_id getAdjacentNode(const NewRowInformation * const information,
                               const node_id node1,const node_id node2){
    //Return the node which was passed which is one of the nodes in information
    if(node1 == information->firstNode || node1 == information->secondNode){
        return node1;
    }
    assert(node2 == information->firstNode || node2 == information->secondNode);
    return node2;
}

static SPQR_ERROR mergeRigidIntoParent(Decomposition *dec, SPQRNewRow *newRow,
                                       const reduced_member_id reducedRigid,const member_id rigid,
                                       NewRowInformation * const information){

    //TODO: split up into smaller functions and re-use parts of series/parallel splitting
    member_id parent = information->member;
    edge_id parentToChild = markerOfParent(dec,rigid);
    edge_id childToParent = markerToParent(dec,rigid);

    node_id toParentHead = findEdgeHead(dec,childToParent);
    node_id toParentTail = findEdgeTail(dec,childToParent);
    assert(nodeIsValid(toParentHead) && nodeIsValid(toParentTail));
    assert(!NodePairHasTwo(&newRow->reducedMembers[reducedRigid].splitting_nodes) &&
           !NodePairIsEmpty(&newRow->reducedMembers[reducedRigid].splitting_nodes));
    node_id rigidSplit = newRow->reducedMembers[reducedRigid].splitting_nodes.first;
    node_id otherRigid = toParentHead == rigidSplit ? toParentTail : toParentHead;
    assert(otherRigid!= rigidSplit);

    node_id firstAdjacentNode = findEdgeHead(dec, parentToChild);
    node_id secondAdjacentNode = findEdgeTail(dec,parentToChild);
    assert(nodeIsValid(firstAdjacentNode) && nodeIsValid(secondAdjacentNode));
    node_id adjacentSplitNode = getAdjacentNode(information, firstAdjacentNode, secondAdjacentNode);
    node_id otherNode = adjacentSplitNode == firstAdjacentNode ? secondAdjacentNode : firstAdjacentNode;
    node_id otherSplitNode = adjacentSplitNode == information->firstNode ? information->secondNode : information->firstNode;
    assert(otherNode != information->firstNode && otherNode != information->secondNode);
    assert(otherSplitNode != firstAdjacentNode && otherSplitNode != secondAdjacentNode);

    node_id articulationEdgeHead = INVALID_NODE;
#ifndef NDEBUG
    node_id articulationEdgeTail = INVALID_NODE;
#endif
    if(newRow->reducedMembers[reducedRigid].allHaveCommonNode && (edgeIsValid(newRow->reducedMembers[reducedRigid].articulationEdge))){
        edge_id articulationEdge = newRow->reducedMembers[reducedRigid].articulationEdge;
        articulationEdgeHead = findEdgeHead(dec,articulationEdge);
#ifndef NDEBUG
        articulationEdgeTail = findEdgeTail(dec, articulationEdge);
#endif
        if(articulationEdge == childToParent ){
            swap_ints(&adjacentSplitNode,&otherSplitNode);
        }
    }

    COLOR_STATUS moveColor = newRow->nodeColors[otherRigid];
    //Remove the two marker edges which are merged (TODO: reuse edge memory?)
    {
        if(findEdgeHead(dec,childToParent) == rigidSplit){
            newRow->nodeColors[findEdgeTail(dec,childToParent)] = UNCOLORED;
        }else if (findEdgeTail(dec,childToParent) == rigidSplit){
            newRow->nodeColors[findEdgeHead(dec,childToParent)] = UNCOLORED;
        }
        removeEdgeFromMemberEdgeList(dec,childToParent,rigid);
        clearEdgeHeadAndTail(dec,childToParent);


        removeEdgeFromMemberEdgeList(dec,parentToChild,parent);
        clearEdgeHeadAndTail(dec,parentToChild); //TODO These functions call redundant finds

    }
    if(!newRow->reducedMembers[reducedRigid].allHaveCommonNode && newRow->reducedMembers[reducedRigid].numCutEdges > 0)  //articulation node splitting is easier to do before the merging
    {
        assert(moveColor == COLOR_FIRST || moveColor == COLOR_SECOND);
        //for each edge adjacent to the old rigid
        edge_id firstEdge = getFirstNodeEdge(dec,rigidSplit);
        edge_id iterEdge = firstEdge;
        do{
            bool isCut = newRow->isEdgeCut[iterEdge];
            node_id otherHead = findEdgeHead(dec,iterEdge);
            node_id otherTail = findEdgeTail(dec,iterEdge);
            node_id otherEnd = otherHead == rigidSplit ? otherTail : otherHead;
            bool isMoveColor = newRow->nodeColors[otherEnd] == moveColor;
            edge_id nextEdge = getNextNodeEdge(dec,iterEdge,rigidSplit); //Need to do this before we modify the edge :)

            bool changeEdgeEnd = (isCut && isMoveColor) || (!isCut && !isMoveColor);
            if(changeEdgeEnd){
                if(otherHead == rigidSplit){
                    changeEdgeHead(dec,iterEdge,otherHead,otherSplitNode);
                }else{
                    changeEdgeTail(dec,iterEdge,otherTail,otherSplitNode);
                }
            }
            newRow->nodeColors[otherEnd] = UNCOLORED;

            //Ugly hack to make sure we can iterate neighbourhood whilst changing edge ends.
            edge_id previousEdge = iterEdge;
            iterEdge = nextEdge;
            if(iterEdge == firstEdge){
                break;
            }
            if(changeEdgeEnd && previousEdge == firstEdge){
                firstEdge = iterEdge;
            }
        }while(true);
        newRow->reducedMembers[reducedRigid].coloredNode = INVALID_NODE;
    }

    //Identify the members with each other
    {
        member_id newMember = mergeMembers(dec, rigid, parent);
        member_id toRemoveFrom = newMember == rigid ? parent : rigid;

        mergeMemberEdgeList(dec, newMember, toRemoveFrom);
        if (toRemoveFrom == parent) { //Correctly update the parent information
            updateMemberParentInformation(dec,newMember,toRemoveFrom);
        }
        updateMemberType(dec, newMember, RIGID);
        information->member = newMember;
    }

    //identify rigid_split with adjacent_split and other_rigid with other_node
    node_id mergedSplit = mergeNodes(dec,rigidSplit,adjacentSplitNode);
    node_id splitToRemove = mergedSplit == rigidSplit ? adjacentSplitNode : rigidSplit;
    if(splitToRemove == information->firstNode){
        information->firstNode = mergedSplit;
    }else if (splitToRemove == information->secondNode){
        information->secondNode = mergedSplit;
    }
    mergeNodes(dec,otherRigid,otherNode); //Returns the update node ID, but we do not need this here.

    if(newRow->reducedMembers[reducedRigid].allHaveCommonNode){
        if(edgeIsValid(newRow->reducedMembers[reducedRigid].articulationEdge)) {
            if(newRow->reducedMembers[reducedRigid].articulationEdge != childToParent) {
                edge_id articulationEdge = newRow->reducedMembers[reducedRigid].articulationEdge;
                if (articulationEdgeHead == rigidSplit) {
                    changeEdgeHead(dec, articulationEdge, mergedSplit, otherSplitNode);
                } else {
                    assert(articulationEdgeTail == rigidSplit);
                    changeEdgeTail(dec, articulationEdge, mergedSplit, otherSplitNode);
                }
            }
        }else{
            cut_edge_id cutEdgeIdx = newRow->reducedMembers[reducedRigid].firstCutEdge;
            do {
                edge_id cutEdge = newRow->cutEdges[cutEdgeIdx].edge;
                node_id edgeHead = findEdgeHead(dec, cutEdge);
                if (edgeHead == mergedSplit) {
                    changeEdgeHead(dec, cutEdge, edgeHead, otherSplitNode);
                } else {
                    changeEdgeTail(dec, cutEdge, findEdgeTail(dec, cutEdge), otherSplitNode);
                }

                cutEdgeIdx = newRow->cutEdges[cutEdgeIdx].nextMember;
            } while (cutEdgeIsValid(cutEdgeIdx));
        }
        return SPQR_OKAY;
    }
    return SPQR_OKAY;
}

static member_id mergeParallelIntoParent(Decomposition * dec,
                                         const member_id member,const member_id parent,
                                         NewRowInformation * const information,
                                         const edge_id cutRepresentative){

    edge_id parentToChild = markerOfParent(dec,member);
    assert(findEdgeMemberNoCompression(dec,parentToChild) == parent);

    //Remove the marker edges which are merged
    //TODO: reuse edge memory?
    removeEdgeFromMemberEdgeList(dec,parentToChild,parent);

    edge_id childToParent = markerToParent(dec,member);
    removeEdgeFromMemberEdgeList(dec,childToParent,member);

    {
        node_id firstAdjacentNode = findEdgeHead(dec, parentToChild);
        node_id secondAdjacentNode = findEdgeTail(dec,parentToChild);
        assert(nodeIsValid(firstAdjacentNode) && nodeIsValid(secondAdjacentNode));
        clearEdgeHeadAndTail(dec,parentToChild); //By the merging procedure,the parent is always a rigid member.

        node_id adjacentSplitNode = getAdjacentNode(information, firstAdjacentNode, secondAdjacentNode);
        node_id otherNode = adjacentSplitNode == firstAdjacentNode ? secondAdjacentNode : firstAdjacentNode;
        node_id otherSplitNode = adjacentSplitNode == information->firstNode ? information->secondNode : information->firstNode;

        assert(otherNode != information->firstNode && otherNode != information->secondNode);
        assert(otherSplitNode != firstAdjacentNode && otherSplitNode != secondAdjacentNode);

        edge_id first_edge = getFirstMemberEdge(dec, member);
        edge_id edge = first_edge;

        do {
            if(edge == cutRepresentative){
                setEdgeHeadAndTail(dec,edge,otherSplitNode,otherNode);
            }else{
                setEdgeHeadAndTail(dec,edge,adjacentSplitNode,otherNode);
            }
            edge = getNextMemberEdge(dec, edge);
        } while (edge != first_edge);

    }

    member_id newMember = mergeMembers(dec,member,parent);
    member_id toRemoveFrom = newMember == member ? parent : member;

    mergeMemberEdgeList(dec,newMember,toRemoveFrom);
    if(toRemoveFrom == parent){ //Correctly update the parent information
        updateMemberParentInformation(dec,newMember,toRemoveFrom);
    }
    updateMemberType(dec,newMember,RIGID);

    return newMember;
}

static edge_id seriesGetOtherEdge(const Decomposition * dec,const SPQRNewRow * newRow,
                                  const reduced_member_id reducedMember){
    for (children_idx i = newRow->reducedMembers[reducedMember].firstChild;
         i < newRow->reducedMembers[reducedMember].firstChild +
             newRow->reducedMembers[reducedMember].numChildren; ++i) {
        reduced_member_id child = newRow->childrenStorage[i];
        if (newRow->reducedMembers[child].type == TYPE_MERGED) {
            return markerOfParent(dec, newRow->reducedMembers[child].member);
        }
    }
    assert(false);
    return INVALID_EDGE;
}
static SPQR_ERROR mergeSeriesIntoParent(Decomposition * dec,
                                        const member_id member, member_id * const parent,
                                 NewRowInformation * const information, const bool isCut,const edge_id childDecompositionEdge){
    assert(getNumMemberEdges(dec,member) == 3);
    edge_id parentToChild = markerOfParent(dec,member);
    assert(findEdgeMemberNoCompression(dec,parentToChild) == *parent);

    //Remove the marker edges which are merged
    removeEdgeFromMemberEdgeList(dec,parentToChild,*parent);
    edge_id childToParent = markerToParent(dec,member);
    removeEdgeFromMemberEdgeList(dec,childToParent,member);

    {
        node_id newNode = INVALID_NODE;
        SPQR_CALL(createNode(dec,&newNode));

        node_id firstAdjacentNode = findEdgeHead(dec, parentToChild);
        node_id secondAdjacentNode = findEdgeTail(dec,parentToChild);
        assert(nodeIsValid(firstAdjacentNode) && nodeIsValid(secondAdjacentNode));
        clearEdgeHeadAndTail(dec,parentToChild);

        node_id adjacentSplitNode = getAdjacentNode(information, firstAdjacentNode, secondAdjacentNode);
        node_id otherNode = adjacentSplitNode == firstAdjacentNode ? secondAdjacentNode : firstAdjacentNode;
        node_id otherSplitNode = adjacentSplitNode == information->firstNode ? information->secondNode : information->firstNode;

        assert(otherNode != information->firstNode && otherNode != information->secondNode);
        assert(otherSplitNode != firstAdjacentNode && otherSplitNode != secondAdjacentNode);

        edge_id first_edge = getFirstMemberEdge(dec, member);
        edge_id edge = first_edge;

        do {
            if(edge == childDecompositionEdge){
                if(isCut){
                    setEdgeHeadAndTail(dec,edge,otherSplitNode,newNode);
                }else{
                    setEdgeHeadAndTail(dec,edge,adjacentSplitNode,newNode);
                }
            }else{
                setEdgeHeadAndTail(dec,edge,otherNode,newNode);
            }

            edge = getNextMemberEdge(dec, edge);
        } while (edge != first_edge);

    }

    member_id newMember = mergeMembers(dec,member,*parent);
    member_id toRemoveFrom = newMember == member ? *parent : member;

    mergeMemberEdgeList(dec,newMember,toRemoveFrom);
    if(toRemoveFrom == *parent){ //Correctly update the parent information
        updateMemberParentInformation(dec,newMember,toRemoveFrom);
    }
    updateMemberType(dec,newMember,RIGID);
    *parent = newMember;
    return SPQR_OKAY;
}

static SPQR_ERROR mergeIntoLargeComponent(Decomposition *dec, SPQRNewRow * newRow,
                                          const reduced_member_id reducedMember, NewRowInformation * const newRowInformation){
    member_id member = newRow->reducedMembers[reducedMember].member;
    assert(findMemberParentNoCompression(dec,member) == newRowInformation->member);
    member_id memberForMerging = member;

    switch(getMemberType(dec,member)){
        case RIGID:{
            SPQR_CALL(mergeRigidIntoParent(dec, newRow, reducedMember, member, newRowInformation));
            break;
        }
        case PARALLEL:{
            edge_id cutRepresentative = INVALID_EDGE;
            SPQR_CALL(splitParallelMerging(dec,newRow,reducedMember,member,&memberForMerging,&cutRepresentative));
            assert(findMemberParentNoCompression(dec,memberForMerging) == newRowInformation->member);
            member_id newID = mergeParallelIntoParent(dec,memberForMerging,newRowInformation->member,newRowInformation,cutRepresentative);
            newRowInformation->member = newID;
            break;
        }
        case SERIES:
        {
            bool isCut = false;
            SPQR_CALL(splitSeriesMerging(dec,newRow,reducedMember,member,&memberForMerging,&isCut));
            assert(findMemberParentNoCompression(dec,memberForMerging) == newRowInformation->member);

            edge_id otherRepresentative = seriesGetOtherEdge(dec,newRow,reducedMember);
            SPQR_CALL(mergeSeriesIntoParent(dec,memberForMerging,&newRowInformation->member,newRowInformation, isCut,otherRepresentative));
            break;
        }

        default:
            break;
    }
    return SPQR_OKAY;
}

static SPQR_ERROR mergeTree(Decomposition *dec, SPQRNewRow *newRow, reduced_member_id root,
                            NewRowInformation * const newRowInformation){

    //Multiple members: we need to merge them together
    //We start by transforming the root, and then iteratively merging any of the relevant children into it
    SPQR_CALL(transformRoot(dec, newRow, root,newRowInformation));

    //Iteratively merge into root component using DFS

    //We reuse the data for determining the types, which has similar call stack data and uses more memories
    assert(newRow->memMergeTreeCallData >= newRow->numReducedMembers);

    int depth = 0;
    MergeTreeCallData * stack = newRow->mergeTreeCallData;

    stack[0].currentChild = newRow->reducedMembers[root].firstChild;
    stack[0].id = root;
    do{
        if(stack[depth].currentChild == newRow->reducedMembers[stack[depth].id].firstChild +
                                        newRow->reducedMembers[stack[depth].id].numChildren){
            --depth;
            continue; //does this break when necessary?
        }
        reduced_member_id reducedChild = newRow->childrenStorage[stack[depth].currentChild];
        ++stack[depth].currentChild;
        if(newRow->reducedMembers[reducedChild].type == TYPE_MERGED){
            SPQR_CALL(mergeIntoLargeComponent(dec,newRow, reducedChild, newRowInformation));
            ++depth;
            assert(depth < newRow->memMergeTreeCallData);
            stack[depth].id = reducedChild;
            stack[depth].currentChild = newRow->reducedMembers[reducedChild].firstChild;
        }

    }while(depth >= 0);
    return SPQR_OKAY;
}
static SPQR_ERROR transformComponent(Decomposition *dec, SPQRNewRow *newRow,
                                     SPQRRowReducedComponent* component, NewRowInformation * const newRowInformation){
    assert(component);
    if(newRow->reducedMembers[component->root].numChildren == newRow->reducedMembers[component->root].numPropagatedChildren){
        //No merging necessary, only a single component
        reduced_member_id reducedMember = component->root;
        assert(reducedMemberIsValid(reducedMember));
        member_id member = newRow->reducedMembers[reducedMember].member;
        MemberType type = getMemberType(dec,member);

        switch(type){
            case RIGID:
                SPQR_CALL(transformSingleRigid(dec,newRow,reducedMember,member,newRowInformation));
                break;
            case PARALLEL: {
                SPQR_CALL(transformSingleParallel(dec,newRow,reducedMember,member,&newRowInformation->member));
                break;
            }
            case SERIES: {
                newRowInformation->member = member;
                break;
            }
            default:
                assert(false);
                break;
        }

        return SPQR_OKAY;
    }

    SPQR_CALL(mergeTree(dec,newRow,component->root,newRowInformation));

    return SPQR_OKAY;
}


SPQR_ERROR createNewRow(SPQR* env, SPQRNewRow** pNewRow ){
    assert(env);
    SPQR_CALL(SPQRallocBlock(env,pNewRow));
    SPQRNewRow * newRow = *pNewRow;

    newRow->remainsGraphic = true;

    newRow->reducedMembers = NULL;
    newRow->memReducedMembers = 0;
    newRow->numReducedMembers = 0;

    newRow->reducedComponents = NULL;
    newRow->memReducedComponents = 0;
    newRow->numReducedComponents = 0;

    newRow->memberInformation = NULL;
    newRow->memMemberInformation = 0;
    newRow->numMemberInformation = 0;

    newRow->cutEdges = NULL;
    newRow->memCutEdges = 0;
    newRow->numCutEdges = 0;
    newRow->firstOverallCutEdge = INVALID_CUT_EDGE;

    newRow->childrenStorage = NULL;
    newRow->memChildrenStorage = 0;
    newRow->numChildrenStorage = 0;

    newRow->newRowIndex = INVALID_ROW;

    newRow->newColumnEdges = NULL;
    newRow->memColumnEdges = 0;
    newRow->numColumnEdges = 0;

    newRow->leafMembers = NULL;
    newRow->numLeafMembers = 0;
    newRow->memLeafMembers = 0;

    newRow->decompositionColumnEdges = NULL;
    newRow->memDecompositionColumnEdges = 0;
    newRow->numDecompositionColumnEdges = 0;

    newRow->isEdgeCut = NULL;
    newRow->memIsEdgeCut = 0;
    newRow->numIsEdgeCut = 0;

    newRow->nodeColors = NULL;
    newRow->memNodeColors = 0;

    newRow->articulationNodes = NULL;
    newRow->memArticulationNodes = 0;
    newRow->numArticulationNodes = 0;

    newRow->articulationNodeSearchInfo = NULL;
    newRow->memNodeSearchInfo = 0;

    newRow->crossingPathCount = NULL;
    newRow->memCrossingPathCount = 0;

    newRow->intersectionDFSData = NULL;
    newRow->memIntersectionDFSData = 0;

    newRow->colorDFSData = NULL;
    newRow->memColorDFSData = 0;

    newRow->artDFSData = NULL;
    newRow->memArtDFSData = 0;

    newRow->createReducedMembersCallstack = NULL;
    newRow->memCreateReducedMembersCallstack = 0;

    newRow->intersectionPathDepth = NULL;
    newRow->memIntersectionPathDepth = 0;

    newRow->intersectionPathParent = NULL;
    newRow->memIntersectionPathParent = 0;

    newRow->mergeTreeCallData = NULL;
    newRow->memMergeTreeCallData = 0;

    return SPQR_OKAY;
}

void freeNewRow(SPQR* env, SPQRNewRow ** pNewRow){
    assert(*pNewRow);

    SPQRNewRow * newRow = *pNewRow;
    //TODO: check if everything is truly freed in reverse order

    SPQRfreeBlockArray(env,&newRow->createReducedMembersCallstack);
    SPQRfreeBlockArray(env,&newRow->artDFSData);
    SPQRfreeBlockArray(env,&newRow->colorDFSData);
    SPQRfreeBlockArray(env,&newRow->mergeTreeCallData);
    SPQRfreeBlockArray(env,&newRow->intersectionDFSData);
    SPQRfreeBlockArray(env,&newRow->intersectionPathParent);
    SPQRfreeBlockArray(env,&newRow->intersectionPathDepth);
    SPQRfreeBlockArray(env,&newRow->crossingPathCount);
    SPQRfreeBlockArray(env,&newRow->articulationNodeSearchInfo);
    SPQRfreeBlockArray(env,&newRow->articulationNodes);
    SPQRfreeBlockArray(env,&newRow->nodeColors);
    SPQRfreeBlockArray(env,&newRow->isEdgeCut);
    if(newRow->decompositionColumnEdges){
        SPQRfreeBlockArray(env,&newRow->decompositionColumnEdges);
    }
    if(newRow->newColumnEdges){
        SPQRfreeBlockArray(env,&newRow->newColumnEdges);
    }
    if(newRow->childrenStorage){
        SPQRfreeBlockArray(env,&newRow->childrenStorage);
    }
    if(newRow->cutEdges){
        SPQRfreeBlockArray(env,&newRow->cutEdges);
    }
    if(newRow->memberInformation){
        SPQRfreeBlockArray(env,&newRow->memberInformation);
    }
    if(newRow->reducedComponents){
        SPQRfreeBlockArray(env,&newRow->reducedComponents);
    }
    if(newRow->reducedMembers){
        SPQRfreeBlockArray(env,&newRow->reducedMembers);
    }
    SPQRfreeBlockArray(env,&newRow->leafMembers);
    SPQRfreeBlock(env,pNewRow);
}

SPQR_ERROR checkNewRow(Decomposition * dec, SPQRNewRow * newRow, const row_idx row, const col_idx * columns, size_t numColumns){
    assert(dec);
    assert(newRow);
    assert(numColumns == 0 || columns );

    newRow->remainsGraphic = true;
    cleanUpPreviousIteration(dec,newRow);

    SPQR_CALL(newRowUpdateRowInformation(dec,newRow,row,columns,numColumns));
    SPQR_CALL(constructReducedDecomposition(dec,newRow));
    SPQR_CALL(createReducedDecompositionCutEdges(dec,newRow));

    SPQR_CALL(determineLeafReducedMembers(dec,newRow));
    SPQR_CALL(allocateRigidSearchMemory(dec,newRow));
    SPQR_CALL(allocateTreeSearchMemory(dec,newRow));
    //Check for each component if the cut edges propagate through a row tree marker to a cut edge in another component
    //From the leafs inward.
    propagateComponents(dec,newRow);
    //It can happen that we are not graphic by some of the checked components.
    //In that case, further checking may lead to errors as some invariants that the code assumes will be broken.
    if(newRow->remainsGraphic){
        for (int i = 0; i < newRow->numReducedComponents; ++i) {
            determineMergeableTypes(dec,newRow,newRow->reducedComponents[i].root);
            //exit early if one is not graphic
            if(!newRow->remainsGraphic){
                break;
            }
        }
    }

    cleanUpMemberInformation(newRow);

    return SPQR_OKAY;
}

SPQR_ERROR addNewRow(Decomposition *dec, SPQRNewRow *newRow){
    assert(newRow->remainsGraphic);
    if(newRow->numReducedComponents == 0){
        member_id newMember = INVALID_MEMBER;
        SPQR_CALL(createStandaloneParallel(dec,newRow->newColumnEdges,newRow->numColumnEdges,newRow->newRowIndex,&newMember));
    }else if (newRow->numReducedComponents == 1){
        NewRowInformation information = emptyNewRowInformation();
        SPQR_CALL(transformComponent(dec,newRow,&newRow->reducedComponents[0],&information));

        if(newRow->numColumnEdges == 0){
            edge_id row_edge = INVALID_EDGE;
            SPQR_CALL(createRowEdge(dec,information.member,&row_edge,newRow->newRowIndex));
            if(nodeIsValid(information.firstNode)){
                setEdgeHeadAndTail(dec,row_edge,information.firstNode,information.secondNode);
            }
        }else{
            member_id new_row_parallel = INVALID_MEMBER;
            SPQR_CALL(createConnectedParallel(dec,newRow->newColumnEdges,newRow->numColumnEdges,newRow->newRowIndex,&new_row_parallel));
            edge_id markerEdge = INVALID_EDGE;
            edge_id ignore = INVALID_EDGE;
            SPQR_CALL(createMarkerPairWithReferences(dec,information.member,new_row_parallel,true,&markerEdge,&ignore));
            if(nodeIsValid(information.firstNode)){
                setEdgeHeadAndTail(dec,markerEdge,information.firstNode,information.secondNode);
            }
        }
    }else{

#ifndef NDEBUG
        int numDecComponentsBefore = numConnectedComponents(dec);
#endif
        member_id new_row_parallel = INVALID_MEMBER;
        SPQR_CALL(createConnectedParallel(dec,newRow->newColumnEdges,newRow->numColumnEdges,newRow->newRowIndex,&new_row_parallel));
        for (int i = 0; i < newRow->numReducedComponents; ++i) {
            NewRowInformation information = emptyNewRowInformation();
            SPQR_CALL(transformComponent(dec,newRow,&newRow->reducedComponents[i],&information));
            reorderComponent(dec,information.member); //Make sure the new component is the root of the local decomposition tree
            edge_id markerEdge = INVALID_EDGE;
            edge_id ignore = INVALID_EDGE;
            SPQR_CALL(createMarkerPairWithReferences(dec,new_row_parallel,information.member,false,&ignore,&markerEdge));
            if(nodeIsValid(information.firstNode)){
                setEdgeHeadAndTail(dec,markerEdge,information.firstNode,information.secondNode);
            }
        }
        decreaseNumConnectedComponents(dec,newRow->numReducedComponents-1);
        assert(numConnectedComponents(dec) == (numDecComponentsBefore - newRow->numReducedComponents + 1));
    }
    return SPQR_OKAY;
}

bool rowAdditionRemainsGraphic(SPQRNewRow *newRow){
    return newRow->remainsGraphic;
}
