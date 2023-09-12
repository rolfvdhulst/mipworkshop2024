//
// Created by rolf on 5-6-23.
//

#include "mipworkshop2024/presolve/SPQRColumnAddition.h"

static int max(int a, int b){
    return (a > b) ? a : b;
}

typedef int path_edge_id;
#define INVALID_PATH_EDGE (-1)

static bool pathEdgeIsInvalid(const path_edge_id edge) {
    return edge < 0;
}

static bool pathEdgeIsValid(const path_edge_id edge) {
    return !pathEdgeIsInvalid(edge);
}

typedef struct {
    edge_id edge;
    node_id edgeHead; //These can be used in various places to prevent additional find()'s
    node_id edgeTail;
    path_edge_id nextMember;
    path_edge_id nextOverall;
} PathEdgeListNode;

typedef int reduced_member_id;
#define INVALID_REDUCED_MEMBER (-1)

static bool reducedMemberIsInvalid(const reduced_member_id id) {
    return id < 0;
}
static bool reducedMemberIsValid(const reduced_member_id id){
    return !reducedMemberIsInvalid(id);
}

typedef int children_idx;

typedef enum {
    TYPE_INVALID = 1,
    TYPE_SINGLE_CHILD = 2,
    TYPE_DOUBLE_CHILD = 3,
    TYPE_CYCLE_CHILD = 4,
    TYPE_ROOT = 5
    //TODO fix
} ReducedMemberType;

typedef struct {
    member_id member;
    member_id rootMember;
    int depth;
    ReducedMemberType type;
    reduced_member_id parent;

    children_idx firstChild;
    children_idx numChildren;

    path_edge_id firstPathEdge;
    int numPathEdges;

    int numOneEnd;
    int numTwoEnds;
    edge_id childMarkerEdges[2];
    node_id rigidEndNodes[4];
} SPQRColReducedMember;


typedef struct {
    int rootDepth;
    reduced_member_id root;
} SPQRColReducedComponent;

typedef struct {
    reduced_member_id reducedMember;
    reduced_member_id rootDepthMinimizer;
} MemberInfo;

typedef struct {
    member_id member;
} CreateReducedMembersCallstack;

struct SPQRNewColumnImpl {
    bool remainsGraphic;

    SPQRColReducedMember *reducedMembers;
    int memReducedMembers;
    int numReducedMembers;

    SPQRColReducedComponent *reducedComponents;
    int memReducedComponents;
    int numReducedComponents;

    MemberInfo *memberInformation;
    int memMemberInformation;
    int numMemberInformation;

    reduced_member_id *childrenStorage;
    int memChildrenStorage;
    int numChildrenStorage;

    PathEdgeListNode *pathEdges;
    int memPathEdges;
    int numPathEdges;
    path_edge_id firstOverallPathEdge;

    int *nodePathDegree;
    int memNodePathDegree;

    bool *edgeInPath;
    int memEdgesInPath;

    CreateReducedMembersCallstack * createReducedMembersCallStack;
    int memCreateReducedMembersCallStack;

    col_idx newColIndex;

    row_idx *newRowEdges;
    int memNewRowEdges;
    int numNewRowEdges;

    edge_id *decompositionRowEdges;
    int memDecompositionRowEdges;
    int numDecompositionRowEdges;
};

static void cleanupPreviousIteration(Decomposition *dec, SPQRNewColumn *newCol) {
    assert(dec);
    assert(newCol);

    path_edge_id pathEdge = newCol->firstOverallPathEdge;
    while (pathEdgeIsValid(pathEdge)) {
        node_id head = newCol->pathEdges[pathEdge].edgeHead;
        node_id tail = newCol->pathEdges[pathEdge].edgeTail;
        if(nodeIsValid(head)){
            newCol->nodePathDegree[head] = 0;
        }
        if(nodeIsValid(tail)){
            newCol->nodePathDegree[tail] = 0;
        }

        edge_id edge = newCol->pathEdges[pathEdge].edge;
        if(edge < newCol->memEdgesInPath){
            newCol->edgeInPath[edge] = false;
        }
        pathEdge = newCol->pathEdges[pathEdge].nextOverall;
    }
#ifndef NDEBUG
    for (int i = 0; i < newCol->memEdgesInPath; ++i) {
        assert(newCol->edgeInPath[i] == false);
    }

    for (int i = 0; i < newCol->memNodePathDegree; ++i) {
        assert(newCol->nodePathDegree[i] == 0);
    }
#endif

    newCol->firstOverallPathEdge = INVALID_PATH_EDGE;
    newCol->numPathEdges = 0;
}

SPQR_ERROR createNewColumn(SPQR *env, SPQRNewColumn **pNewCol) {
    assert(env);

    SPQR_CALL(SPQRallocBlock(env, pNewCol));
    SPQRNewColumn *newCol = *pNewCol;

    newCol->remainsGraphic = false;
    newCol->reducedMembers = NULL;
    newCol->memReducedMembers = 0;
    newCol->numReducedMembers = 0;

    newCol->reducedComponents = NULL;
    newCol->memReducedComponents = 0;
    newCol->numReducedComponents = 0;

    newCol->memberInformation = NULL;
    newCol->memMemberInformation = 0;
    newCol->numMemberInformation = 0;

    newCol->childrenStorage = NULL;
    newCol->memChildrenStorage = 0;
    newCol->numChildrenStorage = 0;

    newCol->pathEdges = NULL;
    newCol->memPathEdges = 0;
    newCol->numPathEdges = 0;
    newCol->firstOverallPathEdge = INVALID_PATH_EDGE;

    newCol->nodePathDegree = NULL;
    newCol->memNodePathDegree = 0;

    newCol->edgeInPath = NULL;
    newCol->memEdgesInPath = 0;

    newCol->createReducedMembersCallStack = NULL;
    newCol->memCreateReducedMembersCallStack = 0;

    newCol->newColIndex = INVALID_COL;

    newCol->newRowEdges = NULL;
    newCol->memNewRowEdges = 0;
    newCol->numNewRowEdges = 0;

    newCol->decompositionRowEdges = NULL;
    newCol->memDecompositionRowEdges = 0;
    newCol->numDecompositionRowEdges = 0;

    return SPQR_OKAY;
}

void freeNewColumn(SPQR *env, SPQRNewColumn **pNewCol) {
    assert(env);
    SPQRNewColumn *newCol = *pNewCol;
    SPQRfreeBlockArray(env, &newCol->decompositionRowEdges);
    SPQRfreeBlockArray(env, &newCol->newRowEdges);
    SPQRfreeBlockArray(env, &newCol->createReducedMembersCallStack);
    SPQRfreeBlockArray(env, &newCol->edgeInPath);
    SPQRfreeBlockArray(env, &newCol->nodePathDegree);
    SPQRfreeBlockArray(env, &newCol->pathEdges);
    SPQRfreeBlockArray(env, &newCol->childrenStorage);
    SPQRfreeBlockArray(env, &newCol->memberInformation);
    SPQRfreeBlockArray(env, &newCol->reducedComponents);
    SPQRfreeBlockArray(env, &newCol->reducedMembers);

    SPQRfreeBlock(env, pNewCol);
}


static reduced_member_id createReducedMembersToRoot(Decomposition *dec,SPQRNewColumn * newCol,const member_id firstMember ){
    assert(memberIsValid(firstMember));

    CreateReducedMembersCallstack * callstack = newCol->createReducedMembersCallStack;
    callstack[0].member = firstMember;
    int callDepth = 0;

    while(callDepth >= 0){
        member_id member = callstack[callDepth].member;
        reduced_member_id reducedMember = newCol->memberInformation[member].reducedMember;

        bool reducedValid = reducedMemberIsValid(reducedMember);
        if(!reducedValid) {
            //reduced member was not yet created; we create it
            reducedMember = newCol->numReducedMembers;

            SPQRColReducedMember *reducedMemberData = &newCol->reducedMembers[reducedMember];
            ++newCol->numReducedMembers;

            reducedMemberData->member = member;
            reducedMemberData->numChildren = 0;

            reducedMemberData->type = TYPE_INVALID;
            reducedMemberData->firstPathEdge = INVALID_PATH_EDGE;
            reducedMemberData->numPathEdges = 0;
            for (int i = 0; i < 4; ++i) {
                reducedMemberData->rigidEndNodes[i] = INVALID_NODE;
            }
            //The children are set later

            newCol->memberInformation[member].reducedMember = reducedMember;
            assert(memberIsRepresentative(dec, member));
            member_id parentMember = findMemberParent(dec, member);

            if (memberIsValid(parentMember)) {
                //recursive call to parent member
                ++callDepth;
                assert(callDepth < newCol->memCreateReducedMembersCallStack);
                callstack[callDepth].member = parentMember;
                continue;

            } else {
                //we found a new reduced decomposition component

                reducedMemberData->parent = INVALID_REDUCED_MEMBER;
                reducedMemberData->depth = 0;
                reducedMemberData->rootMember = member;

                assert(newCol->numReducedComponents < newCol->memReducedComponents);
                newCol->reducedComponents[newCol->numReducedComponents].root = reducedMember;
                ++newCol->numReducedComponents;
            }
        }
        if(reducedValid){
            assert(reducedMember < newCol->numReducedMembers);
            //Reduced member was already created in earlier call
            //update the depth of the root if appropriate
            reduced_member_id * depthMinimizer = &newCol->memberInformation[newCol->reducedMembers[reducedMember].rootMember].rootDepthMinimizer;
            if(reducedMemberIsInvalid(*depthMinimizer) ||
               newCol->reducedMembers[reducedMember].depth < newCol->reducedMembers[*depthMinimizer].depth){
                *depthMinimizer = reducedMember;
            }
        }
        while(true){
            --callDepth;
            if(callDepth < 0 ) break;
            member_id parentMember = callstack[callDepth+1].member;
            reduced_member_id parentReducedMember = newCol->memberInformation[parentMember].reducedMember;
            member_id currentMember = callstack[callDepth].member;
            reduced_member_id currentReducedMember = newCol->memberInformation[currentMember].reducedMember;

            SPQRColReducedMember *parentReducedMemberData = &newCol->reducedMembers[parentReducedMember];
            SPQRColReducedMember *reducedMemberData = &newCol->reducedMembers[currentReducedMember];

            reducedMemberData->parent = parentReducedMember;
            reducedMemberData->depth = parentReducedMemberData->depth + 1;
            reducedMemberData->rootMember = parentReducedMemberData->rootMember;

            newCol->reducedMembers[parentReducedMember].numChildren++;
        }

    }

    reduced_member_id returnedMember = newCol->memberInformation[callstack[0].member].reducedMember;
    return returnedMember;
}

static SPQR_ERROR constructReducedDecomposition(Decomposition *dec, SPQRNewColumn *newCol) {
    assert(dec);
    assert(newCol);
#ifndef NDEBUG
    for (int i = 0; i < newCol->memMemberInformation; ++i) {
        assert(reducedMemberIsInvalid(newCol->memberInformation[i].reducedMember));
    }
#endif
    newCol->numReducedComponents = 0;
    newCol->numReducedMembers = 0;
    if (newCol->numDecompositionRowEdges == 0) { //Early return in case the reduced decomposition will be empty
        return SPQR_OKAY;
    }
    assert(newCol->numReducedMembers == 0);
    assert(newCol->numReducedComponents == 0);

    int newSize = largestMemberID(dec); //Is this sufficient?
    if (newSize > newCol->memReducedMembers) {
        newCol->memReducedMembers = max(2 * newCol->memReducedMembers, newSize);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->reducedMembers, (size_t) newCol->memReducedMembers));
    }
    if (newSize > newCol->memMemberInformation) {
        int updatedSize = max(2 * newCol->memMemberInformation, newSize);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->memberInformation, (size_t) updatedSize));
        for (int i = newCol->memMemberInformation; i < updatedSize; ++i) {
            newCol->memberInformation[i].reducedMember = INVALID_REDUCED_MEMBER;
            newCol->memberInformation[i].rootDepthMinimizer = INVALID_REDUCED_MEMBER;
        }
        newCol->memMemberInformation = updatedSize;

    }

    int numComponents = numConnectedComponents(dec);
    if (numComponents > newCol->memReducedComponents) {
        newCol->memReducedComponents = max(2 * newCol->memReducedComponents, numComponents);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->reducedComponents, (size_t) newCol->memReducedComponents));
    }

    int numMembers = getNumMembers(dec);
    if (newCol->memCreateReducedMembersCallStack < numMembers) {
        newCol->memCreateReducedMembersCallStack = max(2 * newCol->memCreateReducedMembersCallStack, numMembers);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->createReducedMembersCallStack,
                                        (size_t) newCol->memCreateReducedMembersCallStack));
    }

    //Create the reduced members (recursively)
    for (int i = 0; i < newCol->numDecompositionRowEdges; ++i) {
        assert(i < newCol->memDecompositionRowEdges);
        edge_id edge = newCol->decompositionRowEdges[i];
        member_id edgeMember = findEdgeMember(dec, edge);
        reduced_member_id reducedMember = createReducedMembersToRoot(dec, newCol, edgeMember);
        reduced_member_id *depthMinimizer = &newCol->memberInformation[newCol->reducedMembers[reducedMember].rootMember].rootDepthMinimizer;
        if (reducedMemberIsInvalid(*depthMinimizer)) {
            *depthMinimizer = reducedMember;
        }
    }

    //Set the reduced roots according to the root depth minimizers
    for (int i = 0; i < newCol->numReducedComponents; ++i) {
        SPQRColReducedComponent *component = &newCol->reducedComponents[i];
        member_id rootMember = newCol->reducedMembers[component->root].member;
        reduced_member_id reducedMinimizer = newCol->memberInformation[rootMember].rootDepthMinimizer;
        component->rootDepth = newCol->reducedMembers[reducedMinimizer].depth;
        component->root = reducedMinimizer;

        //This simplifies code further down which does not need to be component-aware; just pretend that the reduced member is the new root.
        newCol->reducedMembers[component->root].parent = INVALID_REDUCED_MEMBER;
        assert(memberIsRepresentative(dec, rootMember));
    }

    //update the children array
    int numTotalChildren = 0;
    for (int i = 0; i < newCol->numReducedMembers; ++i) {
        SPQRColReducedMember *reducedMember = &newCol->reducedMembers[i];
        reduced_member_id minimizer = newCol->memberInformation[reducedMember->rootMember].rootDepthMinimizer;
        if (reducedMember->depth >= newCol->reducedMembers[minimizer].depth) {
            reducedMember->firstChild = numTotalChildren;
            numTotalChildren += reducedMember->numChildren;
            reducedMember->numChildren = 0;
        }
    }

    if (newCol->memChildrenStorage < numTotalChildren) {
        int newMemSize = max(newCol->memChildrenStorage * 2, numTotalChildren);
        newCol->memChildrenStorage = newMemSize;
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->childrenStorage, (size_t) newCol->memChildrenStorage));
    }
    newCol->numChildrenStorage = numTotalChildren;

    //Fill up the children array`
    for (reduced_member_id reducedMember = 0; reducedMember < newCol->numReducedMembers; ++reducedMember) {
        SPQRColReducedMember *reducedMemberData = &newCol->reducedMembers[reducedMember];
        if (reducedMemberData->depth <=
            newCol->reducedMembers[newCol->memberInformation[reducedMemberData->rootMember].rootDepthMinimizer].depth) {
            continue;
        }
        member_id parentMember = findMemberParent(dec, reducedMemberData->member);
        reduced_member_id parentReducedMember = memberIsValid(parentMember)
                                                ? newCol->memberInformation[parentMember].reducedMember
                                                : INVALID_REDUCED_MEMBER;
        if (reducedMemberIsValid(parentReducedMember)) {
            //TODO: probably one of these two checks/branches is unnecessary, as there is a single failure case? (Not sure)
            SPQRColReducedMember *parentReducedMemberData = &newCol->reducedMembers[parentReducedMember];
            newCol->childrenStorage[parentReducedMemberData->firstChild +
                                    parentReducedMemberData->numChildren] = reducedMember;
            ++parentReducedMemberData->numChildren;
        }
    }

    //Clean up the root depth minimizers.
    for (int i = 0; i < newCol->numReducedMembers; ++i) {
        SPQRColReducedMember *reducedMember = &newCol->reducedMembers[i];
        assert(reducedMember);
        member_id rootMember = reducedMember->rootMember;
        assert(rootMember >= 0);
        assert(rootMember < dec->memMembers);
        newCol->memberInformation[rootMember].rootDepthMinimizer = INVALID_REDUCED_MEMBER;
    }

    return SPQR_OKAY;
}

static void cleanUpMemberInformation(SPQRNewColumn * newCol){
    //This loop is at the end as memberInformation is also used to assign the cut edges during propagation
    //Clean up the memberInformation array
    for (int i = 0; i < newCol->numReducedMembers; ++i) {
        newCol->memberInformation[newCol->reducedMembers[i].member].reducedMember = INVALID_REDUCED_MEMBER;
    }
#ifndef NDEBUG
    for (int i = 0; i < newCol->memMemberInformation; ++i) {
        assert(reducedMemberIsInvalid(newCol->memberInformation[i].reducedMember));
    }
#endif
}

static void createPathEdge(
        Decomposition * dec, SPQRNewColumn * newCol,
        const edge_id edge, const reduced_member_id reducedMember){
    assert(dec);
    assert(newCol);

    path_edge_id path_edge = newCol->numPathEdges;
    PathEdgeListNode * listNode = &newCol->pathEdges[path_edge];
    listNode->edge = edge;

    listNode->nextMember = newCol->reducedMembers[reducedMember].firstPathEdge;
    newCol->reducedMembers[reducedMember].firstPathEdge = path_edge;
    newCol->reducedMembers[reducedMember].numPathEdges += 1;

    listNode->nextOverall = newCol->firstOverallPathEdge;
    newCol->firstOverallPathEdge = path_edge;

    ++newCol->numPathEdges;
    assert(newCol->numPathEdges <= newCol->memPathEdges);

    assert(edge < newCol->memEdgesInPath);
    newCol->edgeInPath[edge] = true;
    if(getMemberType(dec,newCol->reducedMembers[reducedMember].member) == RIGID){

        listNode->edgeHead = findEdgeHead(dec,edge);
        listNode->edgeTail = findEdgeTail(dec,edge);
        assert(nodeIsValid(listNode->edgeHead) && nodeIsValid(listNode->edgeTail));
        assert(listNode->edgeHead < newCol->memNodePathDegree && listNode->edgeTail < newCol->memNodePathDegree);
        ++newCol->nodePathDegree[listNode->edgeHead];
        ++newCol->nodePathDegree[listNode->edgeTail];
    }else{
        listNode->edgeHead = INVALID_NODE;
        listNode->edgeTail = INVALID_NODE;
    }

}

static SPQR_ERROR createPathEdges(Decomposition *dec, SPQRNewColumn *newCol){
    int maxNumPathEdges = newCol->numDecompositionRowEdges + getNumMembers(dec);
    if(newCol->memPathEdges < maxNumPathEdges){
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->pathEdges,(size_t) maxNumPathEdges)); //TODO: fix reallocation strategy
        newCol->memPathEdges = maxNumPathEdges;
    }
    int maxPathEdgeIndex = largestEdgeID(dec);
    if(newCol->memEdgesInPath < maxPathEdgeIndex){
        int newSize = maxPathEdgeIndex;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->edgeInPath,(size_t) newSize));//TODO: fix reallocation strategy
        for (int i = newCol->memEdgesInPath; i < newSize; ++i) {
            newCol->edgeInPath[i] = false;
        }
        newCol->memEdgesInPath = newSize;
    }
    int maxNumNodes = largestNodeID(dec);
    if(newCol->memNodePathDegree < maxNumNodes){
        int newSize = maxNumNodes;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->nodePathDegree,(size_t) newSize));
        for (int i = newCol->memNodePathDegree; i < newSize; ++i) {
            newCol->nodePathDegree[i] = 0;
        }
        newCol->memNodePathDegree = newSize;
    }
    for (int i = 0; i < newCol->numDecompositionRowEdges; ++i) {
        edge_id edge = newCol->decompositionRowEdges[i];
        member_id member = findEdgeMember(dec,edge);
        reduced_member_id reducedMember = newCol->memberInformation[member].reducedMember;
        createPathEdge(dec,newCol,edge,reducedMember);
    }

    return SPQR_OKAY;
}


/**
 * Saves the information of the current row and partitions it based on whether or not the given columns are
 * already part of the decomposition.
 */
static SPQR_ERROR
newColUpdateColInformation(Decomposition *dec, SPQRNewColumn *newCol, col_idx column, const row_idx *rows,
                           size_t numRows) {
    newCol->newColIndex = column;

    newCol->numDecompositionRowEdges = 0;
    newCol->numNewRowEdges = 0;

    for (size_t i = 0; i < numRows; ++i) {
        edge_id rowEdge = getDecompositionRowEdge(dec, rows[i]);
        if (edgeIsValid(rowEdge)) { //If the edge is the current decomposition: save it in the array
            if (newCol->numDecompositionRowEdges == newCol->memDecompositionRowEdges) {
                int newNumEdges = newCol->memDecompositionRowEdges == 0 ? 8 : 2 *
                                                                              newCol->memDecompositionRowEdges; //TODO: make reallocation numbers more consistent with rest?
                newCol->memDecompositionRowEdges = newNumEdges;
                SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->decompositionRowEdges,
                                                (size_t) newCol->memDecompositionRowEdges));
            }
            newCol->decompositionRowEdges[newCol->numDecompositionRowEdges] = rowEdge;
            ++newCol->numDecompositionRowEdges;
        } else {
            //Not in the decomposition: add it to the set of edges which are newly added with this row.
            if (newCol->numNewRowEdges == newCol->memNewRowEdges) {
                int newNumEdges = newCol->memNewRowEdges == 0 ? 8 : 2 *
                                                                    newCol->memNewRowEdges; //TODO: make reallocation numbers more consistent with rest?
                newCol->memNewRowEdges = newNumEdges;
                SPQR_CALL(SPQRreallocBlockArray(dec->env, &newCol->newRowEdges,
                                                (size_t) newCol->memNewRowEdges));
            }
            newCol->newRowEdges[newCol->numNewRowEdges] = rows[i];
            newCol->numNewRowEdges++;
        }
    }

    return SPQR_OKAY;
}

static void countChildrenTypes(Decomposition* dec, SPQRNewColumn * newCol, reduced_member_id reducedMember){
    newCol->reducedMembers[reducedMember].numOneEnd = 0;
    newCol->reducedMembers[reducedMember].numTwoEnds = 0;
    newCol->reducedMembers[reducedMember].childMarkerEdges[0] = INVALID_EDGE;
    newCol->reducedMembers[reducedMember].childMarkerEdges[1] = INVALID_EDGE;

    int nextChildMarker = 0;
    for (children_idx idx = newCol->reducedMembers[reducedMember].firstChild;
         idx < newCol->reducedMembers[reducedMember].firstChild
               + newCol->reducedMembers[reducedMember].numChildren;
         ++idx) {
        reduced_member_id reducedChild = newCol->childrenStorage[idx];
        assert(reducedMemberIsValid(reducedChild));
        if(newCol->reducedMembers[reducedChild].type == TYPE_SINGLE_CHILD){
            if(nextChildMarker < 2){
                newCol->reducedMembers[reducedMember].childMarkerEdges[nextChildMarker] = markerOfParent(dec, findMember(dec,newCol->reducedMembers[reducedChild].member)); //TODO: check if find is necessary
                ++nextChildMarker;
            }
            newCol->reducedMembers[reducedMember].numOneEnd++;
        }else if(newCol->reducedMembers[reducedChild].type == TYPE_DOUBLE_CHILD){
            if(nextChildMarker < 2){
                newCol->reducedMembers[reducedMember].childMarkerEdges[nextChildMarker] = markerOfParent(dec, findMember(dec,newCol->reducedMembers[reducedChild].member)); //TODO: check if find is necessary
                ++nextChildMarker;
            }
            newCol->reducedMembers[reducedMember].numTwoEnds++;
        }
    }
}

void determineTypeParallel(
        SPQRNewColumn *newCol, reduced_member_id reducedMemberId, int depth){
    const int numOneEnd = newCol->reducedMembers[reducedMemberId].numOneEnd;
    const int numTwoEnds = newCol->reducedMembers[reducedMemberId].numTwoEnds;
    assert(numOneEnd >= 0);
    assert(numTwoEnds >= 0);
    assert(numOneEnd + 2*numTwoEnds <= 2);
    SPQRColReducedMember * reducedMember = &newCol->reducedMembers[reducedMemberId];
    if( depth == 0){
        reducedMember->type = TYPE_ROOT;
        return;
    }
    int numEnds = numOneEnd + 2*numTwoEnds;

    if(numEnds == 0 && pathEdgeIsValid(reducedMember->firstPathEdge)){
        reducedMember->type = TYPE_CYCLE_CHILD;
    }else if(numEnds == 1){
        reducedMember->type = TYPE_SINGLE_CHILD;
    }else if(numEnds == 2){
        if(pathEdgeIsValid(reducedMember->firstPathEdge)){
            newCol->remainsGraphic = false;
            reducedMember->type = TYPE_INVALID;
        }else{
            reducedMember->type = TYPE_DOUBLE_CHILD;
        }
    }else{
        //no child contains path edges, so we are a leaf of the reduced decomposition
        assert(pathEdgeIsValid(reducedMember->firstPathEdge));
        reducedMember->type = TYPE_CYCLE_CHILD; //TODO: is this not duplicate with first case? Should be able to turn into a switch case
    }
}
void determineTypeSeries(Decomposition* dec, SPQRNewColumn* newCol, reduced_member_id reducedMemberId,
                         int depth){
    const int numOneEnd = newCol->reducedMembers[reducedMemberId].numOneEnd;
    const int numTwoEnds = newCol->reducedMembers[reducedMemberId].numTwoEnds;
    assert(dec);
    assert(newCol);
    assert(numOneEnd >= 0);
    assert(numTwoEnds >= 0);
    assert(numOneEnd + 2*numTwoEnds <= 2);
    assert(getMemberType(dec, findMemberNoCompression(dec,newCol->reducedMembers[reducedMemberId].member)) == SERIES);

    SPQRColReducedMember *reducedMember =&newCol->reducedMembers[reducedMemberId];
    member_id member = findMember(dec,reducedMember->member); //We could also pass this as function argument
    int countedPathEdges = 0;
    for(path_edge_id pathEdge = reducedMember->firstPathEdge; pathEdgeIsValid(pathEdge);
    pathEdge = newCol->pathEdges[pathEdge].nextMember){
        ++countedPathEdges;
    } //TODO: replace loop by count
    int numMemberEdges = getNumMemberEdges(dec,member);
    if(depth == 0){
        if(numTwoEnds != 0){
            reducedMember->type = TYPE_INVALID;
            newCol->remainsGraphic = false;
        }else{
            reducedMember->type = countedPathEdges == numMemberEdges-1 ? TYPE_CYCLE_CHILD : TYPE_ROOT;
        }
        return;
    }

    if(countedPathEdges == numMemberEdges-1){
        reducedMember->type = TYPE_CYCLE_CHILD;
    }else if(countedPathEdges + numTwoEnds == numMemberEdges-1){ //TODO: shouldn't this be numMemberEdges?
        assert(numTwoEnds == 1);
        reducedMember->type = TYPE_DOUBLE_CHILD;
    }else if(numTwoEnds == 1){
        newCol->remainsGraphic = false;
        reducedMember->type = TYPE_INVALID;
    }else{
        assert(numTwoEnds == 0);
        reducedMember->type = numOneEnd == 2 ? TYPE_DOUBLE_CHILD : TYPE_SINGLE_CHILD;
    }
}

void determineTypeRigid(Decomposition* dec, SPQRNewColumn* newCol, reduced_member_id reducedMemberId, int depth){
    //Rough idea; first, we find the
    const int numOneEnd = newCol->reducedMembers[reducedMemberId].numOneEnd;
    const int numTwoEnds = newCol->reducedMembers[reducedMemberId].numTwoEnds;
    assert(dec);
    assert(newCol);
    assert(numOneEnd >= 0);
    assert(numTwoEnds >= 0);
    assert(numOneEnd + 2*numTwoEnds <= 2);
    assert(getMemberType(dec, findMemberNoCompression(dec,newCol->reducedMembers[reducedMemberId].member)) == RIGID);
    member_id member = findMember(dec,newCol->reducedMembers[reducedMemberId].member);

    node_id parentMarkerNodes[2] = {
            depth == 0 ? INVALID_NODE : findEdgeHead(dec, markerToParent(dec,member)),
            depth == 0 ? INVALID_NODE : findEdgeTail(dec, markerToParent(dec,member)),
    };

    edge_id * childMarkerEdges = newCol->reducedMembers[reducedMemberId].childMarkerEdges;
    node_id childMarkerNodes[4] = {
            childMarkerEdges[0] == INVALID_EDGE ? INVALID_NODE : findEdgeHead(dec,childMarkerEdges[0]),
            childMarkerEdges[0] == INVALID_EDGE ? INVALID_NODE : findEdgeTail(dec,childMarkerEdges[0]),
            childMarkerEdges[1] == INVALID_EDGE ? INVALID_NODE : findEdgeHead(dec,childMarkerEdges[1]),
            childMarkerEdges[1] == INVALID_EDGE ? INVALID_NODE : findEdgeTail(dec,childMarkerEdges[1]),
    };

    //First, find the end nodes of the path.
    //If there are too many (>4) or there is some node with degree > 2, we terminate
    node_id * pathEndNodes = newCol->reducedMembers[reducedMemberId].rigidEndNodes;
    int numPathEndNodes = 0;
    for (path_edge_id pathEdge = newCol->reducedMembers[reducedMemberId].firstPathEdge; pathEdgeIsValid(pathEdge);
    pathEdge = newCol->pathEdges[pathEdge].nextMember) {
        edge_id edge = newCol->pathEdges[pathEdge].edge;
        node_id nodes[2] = {findEdgeHead(dec,edge), findEdgeTail(dec,edge)};
        for (int i = 0; i < 2; ++i) {
            node_id node = nodes[i];
            assert(newCol->nodePathDegree[node] > 0);
            if(newCol->nodePathDegree[node] > 2){
                //Node of degree 3 or higher implies that the given edges cannot form a path.
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
            if(newCol->nodePathDegree[node] == 1){
                //If we have 5 or more end nodes, stop
                if(numPathEndNodes >= 4){
                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                    newCol->remainsGraphic = false;
                    return;
                }
                pathEndNodes[numPathEndNodes] = node;
                ++numPathEndNodes;
            }


        }
    }

    //Exchange end nodes in the end nodes so that if there are 4 edges, 0 is connected with 1 and 2 is connected with 3
    //Parent marker should come first for each path
    if(numPathEndNodes == 4){
        //We try to follow the path
        edge_id * nodeEdges;
        SPQRallocBlockArray(dec->env,&nodeEdges, (size_t) (2* largestNodeID(dec))); //TODO: move to struct

        //initialize for all relevant nodes
        for (path_edge_id pathEdge = newCol->reducedMembers[reducedMemberId].firstPathEdge; pathEdgeIsValid(pathEdge);
             pathEdge = newCol->pathEdges[pathEdge].nextMember){
            edge_id edge = newCol->pathEdges[pathEdge].edge;
            node_id nodes[2] = {findEdgeHead(dec,edge),findEdgeTail(dec,edge)};
            for (int i = 0; i < 2; ++i) {
                node_id node = nodes[i];
                nodeEdges[2*node] = INVALID_EDGE;
                nodeEdges[2*node + 1] = INVALID_EDGE;
            }
        }
        //store incident edges for every node
        for (path_edge_id pathEdge = newCol->reducedMembers[reducedMemberId].firstPathEdge; pathEdgeIsValid(pathEdge);
             pathEdge = newCol->pathEdges[pathEdge].nextMember){
            edge_id edge = newCol->pathEdges[pathEdge].edge;
            node_id nodes[2] = {findEdgeHead(dec,edge),findEdgeTail(dec,edge)};
            for (int i = 0; i < 2; ++i) {
                node_id node = nodes[i];
                node_id index = 2*node;
                if(nodeEdges[index] != INVALID_EDGE){
                    ++index;
                }
                nodeEdges[index] = edge;
            }
        }
        //Now follow the path starting from end node 0 to see where we end
        edge_id previousEdge = INVALID_EDGE;
        node_id currentNode = pathEndNodes[0];
        while(true){
            edge_id edge = nodeEdges[2*currentNode];
            if(edge == previousEdge){
                edge = nodeEdges[2*currentNode+1];
            }
            if(edge == INVALID_EDGE){
                break;
            }
            previousEdge = edge;
            node_id node = findEdgeHead(dec,edge);
            currentNode = (node != currentNode) ? node : findEdgeTail(dec,edge);
        }
        SPQRfreeBlockArray(dec->env,&nodeEdges);

        if(currentNode == pathEndNodes[2]){
            pathEndNodes[2] = pathEndNodes[1];
            pathEndNodes[1] = currentNode;
        }else if(currentNode == pathEndNodes[3]){
            pathEndNodes[3] = pathEndNodes[1];
            pathEndNodes[1] = currentNode;
        }
        //make sure node 2 is the parent marker. We can assume that 2 and 3 now also form a nice path
        if(pathEndNodes[2] != parentMarkerNodes[0] && pathEndNodes[2] != parentMarkerNodes[1]){
            node_id temp = pathEndNodes[2];
            pathEndNodes[2] = pathEndNodes[3];
            pathEndNodes[3] = temp;
        }
    }
    //make sure node 0 is the parent marker node
    if(numPathEndNodes >= 2 && pathEndNodes[0] != parentMarkerNodes[0] && pathEndNodes[0] != parentMarkerNodes[1]){
        node_id temp = pathEndNodes[0];
        pathEndNodes[0] = pathEndNodes[1];
        pathEndNodes[1] = temp;
    }

    //Finally, we determine the type of the rigid node
    if(depth == 0){
        if(numPathEndNodes == 0){
            if(numOneEnd == 2 && (
                    childMarkerNodes[0] == childMarkerNodes[2] ||
                    childMarkerNodes[0] == childMarkerNodes[3] ||
                    childMarkerNodes[1] == childMarkerNodes[2] ||
                    childMarkerNodes[1] == childMarkerNodes[3])){
                newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
                return;
            }else{
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
        }else if(numPathEndNodes == 2){
            if(numOneEnd == 1){
                bool pathAdjacentToChildMarker = false;
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        if(pathEndNodes[i] == childMarkerNodes[j]){
                            pathAdjacentToChildMarker = true;
                        }
                    }
                }

                if(pathAdjacentToChildMarker){
                    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
                    return;
                }else{
                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                    newCol->remainsGraphic = false;
                    return;
                }
            }else if(numOneEnd == 2){
                bool childMarkerNodesMatched[2] = {false,false};
                bool endNodesMatched[2] ={false, false};
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        if(pathEndNodes[i] == childMarkerNodes[j]){
                            endNodesMatched[i] = true;
                            childMarkerNodesMatched[j/2] = true;
                        }
                    }
                }

                if(childMarkerNodesMatched[0] && childMarkerNodesMatched[1] && endNodesMatched[0] && endNodesMatched[1]){
                    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
                    return;
                }else{
                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                    newCol->remainsGraphic = false;
                    return;
                }
            }else if(numTwoEnds == 0){
                assert(numOneEnd == 0);
                newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
                return;
            }else{
                assert(numOneEnd == 0);
                assert(numTwoEnds == 1);
                if((childMarkerNodes[0] == pathEndNodes[0] && childMarkerNodes[1] == pathEndNodes[1]) ||
                   (childMarkerNodes[0] == pathEndNodes[1] && childMarkerNodes[1] == pathEndNodes[0])
                   ){
                    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
                    return;
                }else{
                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                    newCol->remainsGraphic = false;
                    return;
                }
            }
        }else{
            assert(numPathEndNodes == 4);
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
    }
    int parentMarkerDegrees[2] = {
            newCol->nodePathDegree[parentMarkerNodes[0]],
            newCol->nodePathDegree[parentMarkerNodes[1]]
    };
    //Non-root rigid member
    if(numPathEndNodes == 0){
        // We have no path edges, so there must be at least one child containing one/two path ends.
        assert(numOneEnd + numTwoEnds > 0);
        // We should not have a child marker edge parallel to the parent marker edge!
        assert(!(parentMarkerNodes[0] == childMarkerNodes[0] && parentMarkerNodes[1] == childMarkerNodes[1])
               && !(parentMarkerNodes[0] == childMarkerNodes[1] && parentMarkerNodes[1] == childMarkerNodes[0]));
        if(numOneEnd == 0){
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }else if (numOneEnd == 1){
            if (childMarkerNodes[0] == parentMarkerNodes[0] || childMarkerNodes[0] == parentMarkerNodes[1]
                || childMarkerNodes[1] == parentMarkerNodes[0] || childMarkerNodes[1] == parentMarkerNodes[1]){
                newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
                return;
            }else{
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
        }else{
            assert(numOneEnd == 2);

            int childMarkerParentNode[2] = {-1,-1};
            bool isParallel = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 2; ++j) {
                    if(childMarkerNodes[i] == parentMarkerNodes[j]){
                        if(childMarkerParentNode[i/2] >= 0){
                            isParallel = true;
                        }
                        childMarkerParentNode[i/2] = j;
                    }
                }
            }
            if(!isParallel && childMarkerParentNode[0] != -1 && childMarkerParentNode[1] != -1 &&
            childMarkerParentNode[0] != childMarkerParentNode[1]){
                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                return;
            }else{
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
        }
    }
    else if(numPathEndNodes == 2){
        if(numOneEnd == 1){
            //TODO: is the below check necessary?
            if(parentMarkerNodes[0] != pathEndNodes[0]){
                node_id tempMarker = parentMarkerNodes[0];
                parentMarkerNodes[0] = parentMarkerNodes[1];
                parentMarkerNodes[1] = tempMarker;

                int tempDegree = parentMarkerDegrees[0];
                parentMarkerDegrees[0] = parentMarkerDegrees[1];
                parentMarkerDegrees[1] = tempDegree;
            }
            if(parentMarkerNodes[0] != pathEndNodes[0]){
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
            if(parentMarkerNodes[1] == pathEndNodes[1]){
                // Path closes a cycle with parent marker edge.
                if (childMarkerNodes[0] == parentMarkerNodes[0] || childMarkerNodes[0] == parentMarkerNodes[1]
                    || childMarkerNodes[1] == parentMarkerNodes[0] || childMarkerNodes[1] == parentMarkerNodes[1])
                {
                    newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
                    return;
                }
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }else{
                if(childMarkerNodes[0] == pathEndNodes[1] || childMarkerNodes[1] == pathEndNodes[1]){
                    newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
                    return;
                }else if(childMarkerNodes[0] == parentMarkerNodes[1] || childMarkerNodes[1] == parentMarkerNodes[1]){
                    newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                    return;
                }
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
        }
        else if(numOneEnd == 2){
            node_id otherParentNode;
            if(pathEndNodes[0] == parentMarkerNodes[0]){
                otherParentNode = parentMarkerNodes[1];
            }else if(pathEndNodes[0] == parentMarkerNodes[1]){
                otherParentNode = parentMarkerNodes[0];
            }else{
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
            if(pathEndNodes[1] == otherParentNode){
                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
                newCol->remainsGraphic = false;
                return;
            }
            bool childMatched[2] = {false,false};
            bool pathEndMatched = false;
            bool otherParentMatched = false;
            for (int i = 0; i < 4; ++i) {
                if(childMarkerNodes[i] == pathEndNodes[1]){
                    childMatched[i/2] = true;
                    pathEndMatched = true;
                }
                if(childMarkerNodes[i] == otherParentNode){
                    childMatched[i/2] = true;
                    otherParentMatched = true;
                }
            }
            if(childMatched[0] && childMatched[1] && pathEndMatched  && otherParentMatched){
                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                return;
            }
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;

        }
        else if(numTwoEnds == 0){
            if ((parentMarkerDegrees[0] % 2 == 0 && parentMarkerDegrees[1] == 1) ||
                    (parentMarkerDegrees[0] == 1 && parentMarkerDegrees[1] % 2 == 0))
            {
                newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
                return;
            }
            else if (parentMarkerDegrees[0] == 1 && parentMarkerDegrees[1] == 1)
            {
                newCol->reducedMembers[reducedMemberId].type = TYPE_CYCLE_CHILD;
                return;
            }
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;

        }
        else{
            assert(numTwoEnds == 1);
            if ((pathEndNodes[0] == parentMarkerNodes[0] && parentMarkerNodes[1] == childMarkerNodes[0]
                 && childMarkerNodes[1] == pathEndNodes[1])
                || (pathEndNodes[0] == parentMarkerNodes[0] && parentMarkerNodes[1] == childMarkerNodes[1]
                    && childMarkerNodes[0] == pathEndNodes[1])
                || (pathEndNodes[0] == parentMarkerNodes[1] && parentMarkerNodes[0] == childMarkerNodes[0]
                    && childMarkerNodes[1] == pathEndNodes[1])
                || (pathEndNodes[0] == parentMarkerNodes[1] && parentMarkerNodes[0] == childMarkerNodes[1]
                    && childMarkerNodes[0] == pathEndNodes[1]))
            {
                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                return;
            }
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
    }
    else {
        assert(numPathEndNodes == 4);
        if(pathEndNodes[0] != parentMarkerNodes[0] && pathEndNodes[0] != parentMarkerNodes[1]){
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
        if(pathEndNodes[2] != parentMarkerNodes[0] && pathEndNodes[2] != parentMarkerNodes[1]){
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
        if(numOneEnd == 1){
            if((pathEndNodes[1] == childMarkerNodes[0] || pathEndNodes[1] == childMarkerNodes[1]) ||
            (pathEndNodes[3] == childMarkerNodes[0] || pathEndNodes[3] == childMarkerNodes[1])){
                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                return;
            }
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
        else if(numOneEnd == 2){
            bool pathConnected[2] = {false,false};
            bool childConnected[2] = {false,false};
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if(pathEndNodes[1+2*i] == childMarkerNodes[j]){
                        pathConnected[i] = true;
                        childConnected[j/2] = true;
                    }
                }
            }
            if(pathConnected[0] && pathConnected[1] && childConnected[0] && childConnected[1]){
                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                return;
            }
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
        else if(numTwoEnds == 0){
            newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
            return;
        }
        else{
            if((pathEndNodes[1] == childMarkerNodes[0] && pathEndNodes[3] == childMarkerNodes[1]) ||
                    (pathEndNodes[1] == childMarkerNodes[1] && pathEndNodes[3] == childMarkerNodes[0])){
                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
                return;
            }
            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
            newCol->remainsGraphic = false;
            return;
        }
    }
}
void determineTypes(Decomposition *dec, SPQRNewColumn *newCol,SPQRColReducedComponent * component,
                    reduced_member_id reducedMember,
                    int depth ){
    assert(dec);
    assert(newCol);

    for (children_idx idx = newCol->reducedMembers[reducedMember].firstChild;
         idx < newCol->reducedMembers[reducedMember].firstChild
               + newCol->reducedMembers[reducedMember].numChildren;
         ++idx) {
        reduced_member_id reducedChild = newCol->childrenStorage[idx];
        assert(reducedMemberIsValid(reducedChild));
        determineTypes(dec,newCol,component,reducedChild,depth + 1);
        if(!newCol->remainsGraphic){
            return;
        }
    }

    countChildrenTypes(dec,newCol,reducedMember);
    if(2*newCol->reducedMembers[reducedMember].numTwoEnds + newCol->reducedMembers[reducedMember].numOneEnd > 2){
        newCol->remainsGraphic = false;
        return;
    }
    //Determine type of this
    bool isRoot = reducedMember == component->root;
    member_id member = findMember(dec,newCol->reducedMembers[reducedMember].member); //TODO: find necessary?
    MemberType type = getMemberType(dec,member);
    if(type == PARALLEL){
        determineTypeParallel(newCol,reducedMember,depth);
    }else if (type == SERIES){
        determineTypeSeries(dec,newCol,reducedMember,depth);
    }else{
        assert(type == RIGID);
        determineTypeRigid(dec,newCol,reducedMember,depth);
    }

    //Add a marked edge to the path edge of the parent of this
    if(newCol->remainsGraphic && !isRoot && newCol->reducedMembers[reducedMember].type == TYPE_CYCLE_CHILD){
        member_id parentMember = findMemberParent(dec,newCol->reducedMembers[reducedMember].member);
        reduced_member_id reducedParent = newCol->memberInformation[parentMember].reducedMember;
        edge_id marker = markerOfParent(dec,member);

        createPathEdge(dec,newCol,marker,reducedParent);
    }
}
void determineComponentTypes(Decomposition * dec, SPQRNewColumn * newCol, SPQRColReducedComponent * component){
    assert(dec);
    assert(newCol);
    assert(component);

    determineTypes(dec,newCol,component,component->root,0);
}



SPQR_ERROR
checkNewColumn(Decomposition *dec, SPQRNewColumn *newCol, col_idx column, const row_idx *rows, size_t numRows) {
    assert(dec);
    assert(newCol);
    assert(numRows == 0 || rows);

    newCol->remainsGraphic = true;
    cleanupPreviousIteration(dec, newCol);
    //assert that previous iteration was cleaned up

    //Store call data
    SPQR_CALL(newColUpdateColInformation(dec, newCol, column, rows, numRows));

    //compute reduced decomposition
    SPQR_CALL(constructReducedDecomposition(dec, newCol));
    //initialize path edges in reduced decomposition
    SPQR_CALL(createPathEdges(dec,newCol));
    //determine types
    for (int i = 0; i < newCol->numReducedComponents; ++i) {
        determineComponentTypes(dec,newCol,&newCol->reducedComponents[i]);
    }
    //clean up memberInformation
    cleanUpMemberInformation(newCol);

    return SPQR_OKAY;
}

///Contains the data which tells us where to store the new column after the graph has been modified
///In case member is a parallel or series node, the respective new column and rows are placed in parallel (or series) with it
///Otherwise, the rigid member has a free spot between firstNode and secondNode
typedef struct {
    member_id member;
    node_id terminalNode[2];
    int numTerminals;
} NewColInformation;

static NewColInformation emptyNewColInformation(void){
    NewColInformation information;
    information.member = INVALID_MEMBER;
    information.terminalNode[0] = INVALID_NODE;
    information.terminalNode[1] = INVALID_NODE;
    information.numTerminals = 0;
    return information;
}
static void addTerminal(NewColInformation * info, node_id node){
    assert(info->numTerminals < 2);
    info->terminalNode[info->numTerminals] = node;
    info->numTerminals += 1;
}
static void setTerminalMember(NewColInformation * info, member_id member){
    info->member = member;
}

static SPQR_ERROR mergeMemberIntoParent(Decomposition *dec, member_id member,
                                        bool headToHead // controls the orientation of the merge, e.g. merge heads of the two edges into node if headToHead is true
){
    assert(dec);
    assert(memberIsValid(member));
    assert(memberIsRepresentative(dec,member));
    member_id parentMember = findMemberParent(dec,member);

    assert(memberIsValid(parentMember));
    assert(memberIsRepresentative(dec,parentMember));

    edge_id parentToChild = markerOfParent(dec,member);
    removeEdgeFromMemberEdgeList(dec,parentToChild,parentMember);
    edge_id childToParent = markerToParent(dec,member);
    removeEdgeFromMemberEdgeList(dec,childToParent,member);

    node_id parentEdgeNodes[2] = {findEdgeTail(dec,parentToChild), findEdgeHead(dec,parentToChild)};
    node_id childEdgeNodes[2] = {findEdgeTail(dec,childToParent), findEdgeHead(dec,childToParent)};

    clearEdgeHeadAndTail(dec,parentToChild);
    clearEdgeHeadAndTail(dec,childToParent);

    node_id first = childEdgeNodes[headToHead ? 0 : 1];
    node_id second = childEdgeNodes[headToHead ? 1 : 0];
    {
        node_id newNode = mergeNodes(dec, parentEdgeNodes[0], first);
        node_id toRemoveFrom = newNode == first ? parentEdgeNodes[0] : first;
        mergeNodeEdgeList(dec,newNode,toRemoveFrom);
    }
    {
        node_id newNode = mergeNodes(dec, parentEdgeNodes[1], second);
        node_id toRemoveFrom = newNode == second ? parentEdgeNodes[1] : second;
        mergeNodeEdgeList(dec,newNode,toRemoveFrom);
    }


    member_id newMember = mergeMembers(dec,member,parentMember);
    member_id toRemoveFrom = newMember == member ? parentMember : member;
    mergeMemberEdgeList(dec,newMember,toRemoveFrom);
    if(toRemoveFrom == parentMember){
        updateMemberParentInformation(dec,newMember,toRemoveFrom);
    }
    updateMemberType(dec,newMember,RIGID);

    return SPQR_OKAY;
}



static SPQR_ERROR createParallelNodes(Decomposition * dec, member_id member){
    node_id head,tail;
    SPQR_CALL(createNode(dec,&head));
    SPQR_CALL(createNode(dec,&tail));

    edge_id firstEdge = getFirstMemberEdge(dec,member);
    edge_id edge = firstEdge;
    do{
        setEdgeHeadAndTail(dec,edge,head,tail);
        edge = getNextMemberEdge(dec,edge);
    }while(edge != firstEdge);
    return SPQR_OKAY;
}

static SPQR_ERROR splitParallel(Decomposition *dec, member_id parallel,
                                edge_id edge1, edge_id edge2,
                                member_id * childParallel){
    assert(dec);
    assert(edgeIsValid(edge1));
    assert(edgeIsValid(edge2));
    assert(memberIsValid(parallel));

    bool childContainsTree = edgeIsTree(dec,edge1) || edgeIsTree(dec,edge2);
    edge_id toParent = markerToParent(dec,parallel);
    bool parentMoved = toParent == edge1 || toParent == edge2;
    SPQR_CALL(createMember(dec,PARALLEL,childParallel));

    moveEdgeToNewMember(dec,edge1,parallel,*childParallel);
    moveEdgeToNewMember(dec,edge2,parallel,*childParallel);

    if(parentMoved){
        SPQR_CALL(createMarkerPair(dec,*childParallel,parallel,!childContainsTree));
    }else{
        SPQR_CALL(createMarkerPair(dec,parallel,*childParallel,childContainsTree));
    }
    return SPQR_OKAY;
}
static SPQR_ERROR transformParallel(Decomposition *dec, SPQRNewColumn *newCol, reduced_member_id reducedMemberId,
                                    NewColInformation * newColInfo,int depth){
    assert(dec);
    assert(newCol);
    SPQRColReducedMember * reducedMember = &newCol->reducedMembers[reducedMemberId];
    if(depth != 0){
        assert(reducedMember->numOneEnd == 1);
        //TODO: split parallel
        if(getNumMemberEdges(dec,reducedMember->member) > 3){
            member_id child;
            SPQR_CALL(splitParallel(dec,reducedMember->member,reducedMember->childMarkerEdges[0],
                                    markerToParent(dec,reducedMember->member),&child));
            assert(memberIsRepresentative(dec,child));
            reducedMember->member = child;
        }
        //TODO: can cleanup parallel merging to not immediately identify the nodes by assigning instead
        SPQR_CALL(createParallelNodes(dec,reducedMember->member));
        SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec,reducedMember->childMarkerEdges[0]),
                              pathEdgeIsInvalid(reducedMember->firstPathEdge)));
        return SPQR_OKAY;
    }
    if(reducedMember->numOneEnd == 0 && reducedMember->numTwoEnds == 0){
        assert(pathEdgeIsValid(reducedMember->firstPathEdge) && reducedMember->numPathEdges == 1);
        //The new edge can be placed in parallel; just add it to this member
        setTerminalMember(newColInfo,reducedMember->member);
        return SPQR_OKAY;
    }
    assert(reducedMember->numOneEnd == 2);

    //split off if the parallel contains more than three edges
    if(getNumMemberEdges(dec,reducedMember->member) > 3){
        member_id child;
        SPQR_CALL(splitParallel(dec,reducedMember->member,reducedMember->childMarkerEdges[0],
                                reducedMember->childMarkerEdges[1],&child));
        assert(memberIsRepresentative(dec,child));
        reducedMember->member = child;
    }
    assert(getNumMemberEdges(dec,reducedMember->member) == 3);

    //TODO: again, this can probably be done more efficiently
    SPQR_CALL(createParallelNodes(dec,reducedMember->member));
    SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec,reducedMember->childMarkerEdges[0]),true));

    //If this doesn't work, try;
    bool headToHead = reducedMember->numPathEdges == 0;
//    bool headToHead = reducedMember->numPathEdges == 0 && reducedMember->type != TYPE_DOUBLE_CHILD && reducedMember->type != TYPE_ROOT;
    SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec,reducedMember->childMarkerEdges[1]),headToHead));
    setTerminalMember(newColInfo, findMember(dec,reducedMember->member));
    return SPQR_OKAY;
}

static SPQR_ERROR splitSeries(Decomposition *dec, SPQRNewColumn * newCol,
                              SPQRColReducedMember * reducedMember,
                              member_id member, member_id * loopMember){
    assert(dec);
    assert(reducedMember);
    assert(memberIsValid(member));
    assert(memberIsRepresentative(dec,member));

    bool createPathSeries = reducedMember->numPathEdges > 1;
    bool convertOriginal = reducedMember->numPathEdges == getNumMemberEdges(dec,member) -1;
    if(!createPathSeries && convertOriginal){
        //only one path edge; we are in a loop; no need to change anything
        assert(getNumMemberEdges(dec,member) == 2);
        assert(reducedMember->numPathEdges == 1);
        *loopMember = member;
        changeLoopToParallel(dec,member);
        return SPQR_OKAY;
    }

    member_id pathMember;
    SPQR_CALL(createMember(dec,SERIES,&pathMember));

    path_edge_id pathEdgeId = reducedMember->firstPathEdge;
    bool parentMoved = false;
    while(pathEdgeIsValid(pathEdgeId)){
        edge_id pathEdge = newCol->pathEdges[pathEdgeId].edge;
        pathEdgeId = newCol->pathEdges[pathEdgeId].nextMember;
        if(pathEdge == markerToParent(dec,member)){
            parentMoved = true;
        }
        moveEdgeToNewMember(dec,pathEdge,member,pathMember);
    }
    if(convertOriginal == createPathSeries){
        if(parentMoved){
            SPQR_CALL(createMarkerPair(dec,pathMember,member,false));
        }else{
            SPQR_CALL(createMarkerPair(dec,member,pathMember,true));
        }
        *loopMember = convertOriginal ? member : pathMember;
        changeLoopToParallel(dec,*loopMember);
        return SPQR_OKAY;
    }
    SPQR_CALL(createMember(dec,PARALLEL,loopMember));
    if(parentMoved){
        SPQR_CALL(createMarkerPair(dec,pathMember,*loopMember,false));
        SPQR_CALL(createMarkerPair(dec,*loopMember,member,false));
    }else{
        SPQR_CALL(createMarkerPair(dec,member,*loopMember,true));
        SPQR_CALL(createMarkerPair(dec,*loopMember,pathMember,true));
    }

    return SPQR_OKAY;
}


static SPQR_ERROR splitSeriesMerging(Decomposition *dec, SPQRNewColumn * newCol,
                              SPQRColReducedMember * reducedMember,
                              member_id member,
                              edge_id * pathRepresentative,
                              edge_id * nonPathRepresentative,
                              edge_id exceptionEdge1,
                              edge_id exceptionEdge2){
    assert(dec);
    assert(reducedMember);
    assert(memberIsValid(member));
    assert(memberIsRepresentative(dec,member));

    int numExceptionEdges = (exceptionEdge1 == INVALID_EDGE ? 0 : 1) + (exceptionEdge2 == INVALID_EDGE ? 0 : 1);
    int numNonPathEdges = getNumMemberEdges(dec,member) - reducedMember->numPathEdges - numExceptionEdges;
    bool createPathSeries = reducedMember->numPathEdges > 1;
    //If this holds, there are 2 or more non-parent marker non-path edges
    bool createNonPathSeries = numNonPathEdges > 1;
    assert(exceptionEdge1 == INVALID_EDGE || !newCol->edgeInPath[exceptionEdge1]);
    assert(exceptionEdge2 == INVALID_EDGE || !newCol->edgeInPath[exceptionEdge2]);

    if(createPathSeries){
        member_id pathMember;
        SPQR_CALL(createMember(dec,SERIES,&pathMember));

        path_edge_id pathEdgeId = reducedMember->firstPathEdge;
        bool parentMoved = false;
        while(pathEdgeIsValid(pathEdgeId)){
            edge_id pathEdge = newCol->pathEdges[pathEdgeId].edge;
            pathEdgeId = newCol->pathEdges[pathEdgeId].nextMember;
            assert(pathEdge != exceptionEdge1 && pathEdge != exceptionEdge2);
            parentMoved = parentMoved || markerToParent(dec,member) == pathEdge;
            moveEdgeToNewMember(dec,pathEdge,member,pathMember);
        }
        assert(getNumMemberEdges(dec,pathMember) >= 2);

        edge_id ignored;
        if(parentMoved){
            SPQR_CALL(createMarkerPairWithReferences(dec,pathMember,member,false,&ignored,pathRepresentative));
        }else{
            SPQR_CALL(createMarkerPairWithReferences(dec,member,pathMember,true,pathRepresentative,&ignored));
        }
    }else{
        if(pathEdgeIsValid(reducedMember->firstPathEdge)){
            *pathRepresentative = newCol->pathEdges[reducedMember->firstPathEdge].edge;
        }else{
            *pathRepresentative = INVALID_EDGE;
        }
    }

    if(createNonPathSeries){
        member_id nonPathMember;
        SPQR_CALL(createMember(dec,SERIES,&nonPathMember));

        edge_id edge = getFirstMemberEdge(dec,member);
        bool parentMoved = false;
        bool canStop = false; //hack when the first edge is moved in the below loop to prevent that we immediately terminate
        do{
            edge_id nextEdge = getNextMemberEdge(dec,edge);
            if(edge != *pathRepresentative && edge != exceptionEdge1 && edge != exceptionEdge2){
                parentMoved = parentMoved || markerToParent(dec,member) == edge;
                moveEdgeToNewMember(dec,edge,member,nonPathMember);
            }else{
                canStop = true;
            }
            edge = nextEdge;
            if(canStop && edge == getFirstMemberEdge(dec,member)){
                break;
            }
        }while(true);
        assert(getNumMemberEdges(dec,nonPathMember) >= 2);
        bool representativeIsTree = !edgeIsTree(dec,exceptionEdge1);
        if(edgeIsValid(exceptionEdge2)){
            representativeIsTree = representativeIsTree || !edgeIsTree(dec,exceptionEdge2);
        }
        edge_id ignored;
        if(parentMoved){
            SPQR_CALL(createMarkerPairWithReferences(dec,nonPathMember,member,!representativeIsTree,&ignored,nonPathRepresentative));
        }else{
            SPQR_CALL(createMarkerPairWithReferences(dec,member,nonPathMember,representativeIsTree,nonPathRepresentative,&ignored));
        }
    }else{
        *nonPathRepresentative = INVALID_EDGE;
        if(numNonPathEdges != 0) {
            edge_id firstEdge = getFirstMemberEdge(dec, member);
            edge_id edge = firstEdge;
            do {
                if (edge != *pathRepresentative && edge != exceptionEdge1 && edge != exceptionEdge2) {
                    *nonPathRepresentative = edge;
                    break;
                }
                edge = getNextMemberEdge(dec, edge);
            } while (edge != firstEdge);
            assert(*nonPathRepresentative != INVALID_EDGE);
        }
    }

    return SPQR_OKAY;
}

static SPQR_ERROR transformSeries(Decomposition *dec, SPQRNewColumn *newCol,reduced_member_id reducedMemberId,
                                  NewColInformation * newColInfo, int depth){
    assert(dec);
    assert(newCol);
    assert(newColInfo);
    SPQRColReducedMember * reducedMember = &newCol->reducedMembers[reducedMemberId];
    if(depth == 0){
        // If we have a child containing both ends then we should have moved the reduced root there.
        //This means that in this case, we know this is the only reduced member
        assert(reducedMember->numTwoEnds == 0);
        if(reducedMember->numPathEdges == getNumMemberEdges(dec,reducedMember->member)-1){
            member_id adjacentMember = INVALID_MEMBER;
            edge_id adjacentMarker = INVALID_EDGE;
            {
                edge_id firstEdge = getFirstMemberEdge(dec,reducedMember->member);
                edge_id edge = firstEdge;
                do{
                    if(!newCol->edgeInPath[edge]){
                        if(edge == markerToParent(dec,reducedMember->member)){
                            adjacentMember = findMemberParent(dec,reducedMember->member);
                            adjacentMarker = markerOfParent(dec,reducedMember->member);
                        }else if(edgeIsMarker(dec,edge)){
                            adjacentMember = findEdgeChildMember(dec,edge);
                            adjacentMarker = markerToParent(dec,adjacentMember);
                        }

                        break; //There is only a singular such edge
                    }
                    edge = getNextMemberEdge(dec,edge);
                }while(edge != firstEdge);
            }

            if(memberIsValid(adjacentMember)){
                MemberType adjacentType = getMemberType(dec,adjacentMember);
                if(adjacentType == PARALLEL){
                    setTerminalMember(newColInfo,adjacentMember);
                    return SPQR_OKAY;
                }
            }
        }

        if(reducedMember->numOneEnd == 0){
            //Isolated single cycle
            member_id loopMember;
            SPQR_CALL(splitSeries(dec,newCol,reducedMember,reducedMember->member,&loopMember));
            setTerminalMember(newColInfo,loopMember);
            return SPQR_OKAY;
        }
        if(reducedMember->numOneEnd == 1){
            //Cycle is root of multiple components
            //Split off all path edges and all non-path edges, except the unique child marker
            edge_id childMarker = reducedMember->childMarkerEdges[0];
            assert(childMarker != INVALID_EDGE);

            edge_id pathRepresentative,nonPathRepresentative;
            SPQR_CALL(splitSeriesMerging(dec,newCol,reducedMember,reducedMember->member,&pathRepresentative, &nonPathRepresentative,
                                         childMarker,INVALID_EDGE));
            assert(getNumMemberEdges(dec,reducedMember->member) == 3);

            node_id a,b,c;
            SPQR_CALL(createNode(dec,&a));
            SPQR_CALL(createNode(dec,&b));
            SPQR_CALL(createNode(dec,&c));

            assert(pathRepresentative != childMarker &&
            nonPathRepresentative != childMarker &&
            pathRepresentative != nonPathRepresentative);
            assert(edgeIsTree(dec,pathRepresentative) );

            setEdgeHeadAndTail(dec,childMarker,a,b);
            setEdgeHeadAndTail(dec,pathRepresentative,b,c);
            setEdgeHeadAndTail(dec,nonPathRepresentative,c,a);

            addTerminal(newColInfo,c);

            //parent path always ends in the tail node, so we identify head to head
            SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec,childMarker),true));

            member_id member = findMember(dec,reducedMember->member);
            setTerminalMember(newColInfo,member);

            return SPQR_OKAY;
        }
        assert(reducedMember->numOneEnd == 2);
        // If there is more than 1 (non)-path edge, we split off by moving them to a new series member and creating a parallel
        edge_id pathEdge = INVALID_EDGE;
        edge_id nonPathEdge = INVALID_EDGE;

        SPQR_CALL(splitSeriesMerging(dec, newCol, reducedMember, reducedMember->member, &pathEdge, &nonPathEdge,
                                     reducedMember->childMarkerEdges[0],
                                     reducedMember->childMarkerEdges[1]));

        //Check that all edges are unique
        assert(reducedMember->childMarkerEdges[0] != reducedMember->childMarkerEdges[1]);
        assert(pathEdge != reducedMember->childMarkerEdges[0] && pathEdge != reducedMember->childMarkerEdges[1] &&
        nonPathEdge != reducedMember->childMarkerEdges[0] && nonPathEdge != reducedMember->childMarkerEdges[1]);
        assert(pathEdge != INVALID_EDGE || nonPathEdge != INVALID_EDGE);
        assert(getNumMemberEdges(dec, reducedMember->member) == 3 ||
               getNumMemberEdges(dec, reducedMember->member) == 4);
        //After splitting there is the following possibilities for nodes a-d:
        //(a)-child-(b)-path-(c)-child-(d)-nonpath-(a)
        //(a)-child-(b)-path-(c)-child-(d==a)
        //(a)-child-(b)=(c)-child-(d)-nonpath-(a)

        node_id a = INVALID_NODE;
        node_id b = INVALID_NODE;
        node_id c = INVALID_NODE;
        node_id d = INVALID_NODE;
        SPQR_CALL(createNode(dec, &a));
        SPQR_CALL(createNode(dec, &b));
        if (edgeIsValid(pathEdge)) {
            SPQR_CALL(createNode(dec, &c));
        } else {
            c = b;
        }
        if (edgeIsValid(nonPathEdge)) {
            SPQR_CALL(createNode(dec, &d));
        } else {
            d = a;
        }

        setEdgeHeadAndTail(dec, reducedMember->childMarkerEdges[0], a, b);
        setEdgeHeadAndTail(dec, reducedMember->childMarkerEdges[1], d, c);
        if (edgeIsValid(pathEdge)) {
            setEdgeHeadAndTail(dec, pathEdge, b, c);
        }
        if (edgeIsValid(nonPathEdge)) {
            setEdgeHeadAndTail(dec, nonPathEdge, d, a);
        }
        SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec, reducedMember->childMarkerEdges[0]), true));
        SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec, reducedMember->childMarkerEdges[1]), true));
        setTerminalMember(newColInfo, findMember(dec,reducedMember->member));

        return SPQR_OKAY;
    }
    assert(reducedMember->type == TYPE_SINGLE_CHILD);
    edge_id parentMarker = markerToParent(dec, reducedMember->member);
    assert(edgeIsValid(parentMarker));

    if (reducedMember->numOneEnd == 0) {
        assert(reducedMember->numTwoEnds == 0);
        assert(pathEdgeIsValid(reducedMember->firstPathEdge));
        //propagate path edge

        //Split off all path edges and all non-path edges EXCEPT the parent marker if there are more than 2
        edge_id pathRepresentative, nonPathRepresentative;
        SPQR_CALL(splitSeriesMerging(dec, newCol, reducedMember, reducedMember->member, &pathRepresentative,
                                     &nonPathRepresentative,
                                     parentMarker, INVALID_EDGE));

        assert(getNumMemberEdges(dec, reducedMember->member) == 3);

        //add nodes edges and a single terminal

        node_id a, b, c;
        SPQR_CALL(createNode(dec, &a));
        SPQR_CALL(createNode(dec, &b));
        SPQR_CALL(createNode(dec, &c));

        assert(pathRepresentative != parentMarker && nonPathRepresentative != parentMarker &&
               pathRepresentative != nonPathRepresentative);

        setEdgeHeadAndTail(dec, parentMarker, a, b);
        setEdgeHeadAndTail(dec, pathRepresentative, b, c);
        setEdgeHeadAndTail(dec, nonPathRepresentative, c, a);

        addTerminal(newColInfo, c);
        return SPQR_OKAY;
    }

    //Path passes through this marker and the parent marker
    assert(reducedMember->numOneEnd == 1);

    edge_id childMarker = reducedMember->childMarkerEdges[0];
    assert(edgeIsValid(childMarker));

    // If there is more than 1 (non)-path edge, we split off by moving them to a new series member and creating a parallel
    edge_id pathEdge = INVALID_EDGE;
    edge_id nonPathEdge = INVALID_EDGE;

    SPQR_CALL(splitSeriesMerging(dec, newCol, reducedMember, reducedMember->member, &pathEdge, &nonPathEdge,
                                 childMarker,parentMarker));

    //Check that all edges are unique
    assert(childMarker != parentMarker);
    assert(pathEdge != childMarker && pathEdge != parentMarker &&
        nonPathEdge != childMarker && nonPathEdge != parentMarker);
    assert(pathEdge != INVALID_EDGE || nonPathEdge != INVALID_EDGE);
    assert(getNumMemberEdges(dec, reducedMember->member) == 3 ||
           getNumMemberEdges(dec, reducedMember->member) == 4);
    //After splitting there is the following possibilities for nodes a-d:
    //(a)-parent-(b)-path-(c)-child-(d)-nonpath-(a)
    //(a)-parent-(b)-path-(c)-child-(d==a)
    //(a)-parent-(b)=(c)-child-(d)-nonpath-(a)

    node_id a = INVALID_NODE;
    node_id b = INVALID_NODE;
    node_id c = INVALID_NODE;
    node_id d = INVALID_NODE;
    SPQR_CALL(createNode(dec, &a));
    SPQR_CALL(createNode(dec, &b));
    if (edgeIsValid(pathEdge)) {
        SPQR_CALL(createNode(dec, &c));
    } else {
        c = b;
    }
    if (edgeIsValid(nonPathEdge)) {
        SPQR_CALL(createNode(dec, &d));
    } else {
        d = a;
    }

    setEdgeHeadAndTail(dec, parentMarker, a, b);
    setEdgeHeadAndTail(dec, childMarker, d, c);
    if (edgeIsValid(pathEdge)) {
        setEdgeHeadAndTail(dec, pathEdge, b, c);
    }
    if (edgeIsValid(nonPathEdge)) {
        setEdgeHeadAndTail(dec, nonPathEdge, d, a);
    }
    SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec, childMarker), true));

    return SPQR_OKAY;
}
static SPQR_ERROR transformRigid(Decomposition *dec, SPQRNewColumn * newCol, reduced_member_id id,
                                 NewColInformation * newColInfo, int depth){
    assert(dec);
    assert(newCol);
    assert(reducedMemberIsValid(id));
    assert(depth >= 0);
    member_id member = findMember(dec,newCol->reducedMembers[id].member);
    assert(getMemberType(dec,member) == RIGID);

    edge_id * childMarkerEdges = newCol->reducedMembers[id].childMarkerEdges;
    edge_id parentMarker = markerToParent(dec,member);
    node_id parentMarkerNodes[2] = {
            parentMarker == INVALID_EDGE ? INVALID_NODE : findEdgeHead(dec,parentMarker),
            parentMarker == INVALID_EDGE ? INVALID_NODE : findEdgeTail(dec,parentMarker)
    };
    node_id childMarkerNodes[4] = {
            childMarkerEdges[0] == INVALID_EDGE ? INVALID_NODE : findEdgeHead(dec,childMarkerEdges[0]),
            childMarkerEdges[0] == INVALID_EDGE ? INVALID_NODE : findEdgeTail(dec,childMarkerEdges[0]),
            childMarkerEdges[1] == INVALID_EDGE ? INVALID_NODE : findEdgeHead(dec,childMarkerEdges[1]),
            childMarkerEdges[1] == INVALID_EDGE ? INVALID_NODE : findEdgeTail(dec,childMarkerEdges[1]),
    };
    node_id * pathEndNodes = newCol->reducedMembers[id].rigidEndNodes;
    int numPathEndNodes = pathEndNodes[0] == INVALID_NODE ? 0 : (pathEndNodes[2] == INVALID_NODE ? 2 : 4);
    const int numOneEnd = newCol->reducedMembers[id].numOneEnd;
    const int numTwoEnds = newCol->reducedMembers[id].numTwoEnds;
    if(depth == 0){
        //modify the endnodes array if the parent marker is a path edge
        if(parentMarker != INVALID_EDGE && newCol->edgeInPath[parentMarker]){
            if(numPathEndNodes == 0){
                pathEndNodes[0] = parentMarkerNodes[0];
                pathEndNodes[1] = parentMarkerNodes[1];
                numPathEndNodes = 1;
            }else if(numPathEndNodes == 2){
                if(pathEndNodes[0] == parentMarkerNodes[0]){
                    pathEndNodes[0] = parentMarkerNodes[1];
                }else if(pathEndNodes[0] == parentMarkerNodes[1]){
                    pathEndNodes[0] = parentMarkerNodes[0];
                }
            }else{
                pathEndNodes[0] = pathEndNodes[3];
                pathEndNodes[2] = INVALID_NODE;
                pathEndNodes[3] = INVALID_NODE;
                numPathEndNodes = 2;
            }
        }
        assert(numPathEndNodes <= 2);
        if(numOneEnd == 0 && numTwoEnds == 0){
            //Check if an edge already exists. If there is one, we either make a parallel out of it, and the new edge
            //or if it is a parallel marker we add the edge there
            edge_id connectingEdge = INVALID_EDGE;
            {
                edge_id iterEdge = getFirstNodeEdge(dec,pathEndNodes[0]);
                edge_id first = iterEdge;
                do{
                    node_id edgeHead = findEdgeHead(dec,iterEdge);
                    node_id other = edgeHead != pathEndNodes[0] ? edgeHead : findEdgeTail(dec,iterEdge);
                    if(pathEndNodes[1] == other){
                        connectingEdge = iterEdge;
                        break;
                    }
                    iterEdge = getNextNodeEdge(dec,iterEdge,pathEndNodes[0]);
                }while(iterEdge != first);
            }
            if(edgeIsInvalid(connectingEdge)){
                addTerminal(newColInfo,pathEndNodes[0]);
                addTerminal(newColInfo,pathEndNodes[1]);
                setTerminalMember(newColInfo,member);
                return SPQR_OKAY;
            }

            member_id parallelMember;

            member_id connectingMember = INVALID_MEMBER;
            bool connectingIsParent = false;
            if(edgeIsMarker(dec,connectingEdge)){
                connectingMember = findEdgeChildMember(dec,connectingEdge);
            }else if(connectingEdge == markerToParent(dec,member)){
                connectingMember = findMemberParent(dec,member);
                assert(connectingMember != INVALID_MEMBER);
                connectingIsParent = true;
            }

            if(memberIsValid(connectingMember) && getMemberType(dec,connectingMember) == PARALLEL){
                parallelMember = connectingMember;
            }
            else{
                //newly create the parallel member and move the edge to it
                SPQR_CALL(createMember(dec, PARALLEL, &parallelMember));
                bool isTreeEdge = edgeIsTree(dec,connectingEdge);
                moveEdgeToNewMember(dec, connectingEdge, member, parallelMember);


                node_id oldHead = findEdgeHead(dec,connectingEdge);
                node_id oldTail = findEdgeTail(dec,connectingEdge);

                clearEdgeHeadAndTail(dec,connectingEdge);

                edge_id ignore = INVALID_EDGE;
                edge_id rigidMarker = INVALID_EDGE;
                if (connectingIsParent) {
                    SPQR_CALL(createMarkerPairWithReferences(dec, parallelMember,member, !isTreeEdge,&ignore,&rigidMarker));
                } else {
                    SPQR_CALL(createMarkerPairWithReferences(dec, member, parallelMember, isTreeEdge,&rigidMarker,&ignore));
                }
                setEdgeHeadAndTail(dec,rigidMarker,oldHead,oldTail);
            }
            setTerminalMember(newColInfo,parallelMember);
            return SPQR_OKAY;
        }
        if(numOneEnd == 1){
            node_id terminalNode = (pathEndNodes[0] == childMarkerNodes[0] || pathEndNodes[0] == childMarkerNodes[1])
                                   ? pathEndNodes[1] : pathEndNodes[0];
            addTerminal(newColInfo,terminalNode);

            member_id childMember = findEdgeChildMember(dec,childMarkerEdges[0]);
            bool headToHead = pathEndNodes[0] == childMarkerNodes[1] || pathEndNodes[1] == childMarkerNodes[1];
            assert(headToHead || pathEndNodes[0] == childMarkerNodes[0] || pathEndNodes[1] == childMarkerNodes[0]);

            SPQR_CALL(mergeMemberIntoParent(dec,childMember,headToHead));
            setTerminalMember(newColInfo, findMember(dec,member));

            return SPQR_OKAY;
        }else{
            assert(numOneEnd == 2);
            assert(newColInfo->numTerminals == 2);
            member_id childMember[2] = {
                    findEdgeChildMember(dec,childMarkerEdges[0]),
                    findEdgeChildMember(dec,childMarkerEdges[1])
            };
            //count to how many path end nodes the child marker is adjacent
            int numIncidentPathNodes[2] = {0,0};
            for (int c = 0; c < 2; ++c) {
                for (int i = 0; i < numPathEndNodes; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        if(pathEndNodes[i] == childMarkerNodes[2*c+j]){
                            numIncidentPathNodes[c] += 1;
                        }
                    }
                }
            }
            // If a child marker is incident to both path ends, then we ensure it is the second one.
            if(numIncidentPathNodes[0] == 2){
                member_id tempChildMember = childMember[0];
                childMember[0] = childMember[1];
                childMember[1] = tempChildMember;

                node_id tempChildMarkerNodes = childMarkerNodes[0];
                childMarkerNodes[0] = childMarkerNodes[2];
                childMarkerNodes[2] = tempChildMarkerNodes;

                tempChildMarkerNodes = childMarkerNodes[1];
                childMarkerNodes[1] = childMarkerNodes[3];
                childMarkerNodes[3] = tempChildMarkerNodes;

            }

            // Check if the two child markers are parallel. We then create a parallel with the two
            if ((childMarkerNodes[0] == childMarkerNodes[2] && childMarkerNodes[1] == childMarkerNodes[3])
                || (childMarkerNodes[0] == childMarkerNodes[3] && childMarkerNodes[1] == childMarkerNodes[2])){
                //TODO: create parallel here
                assert(false);
            }
            if(numPathEndNodes == 0){
                //We fake a path by setting the end nodes to be the common node of the child markers
                if(childMarkerNodes[0] == childMarkerNodes[2] || childMarkerNodes[0] == childMarkerNodes[3]){
                    pathEndNodes[0] = childMarkerNodes[0];
                    pathEndNodes[1] = childMarkerNodes[0];
                }else{
                    assert(childMarkerNodes[1] == childMarkerNodes[2] || childMarkerNodes[1] == childMarkerNodes[3]);
                    pathEndNodes[0] = childMarkerNodes[1];
                    pathEndNodes[1] = childMarkerNodes[1];
                }
            }
            if(pathEndNodes[0] != childMarkerNodes[0] && pathEndNodes[0] != childMarkerNodes[1]){
                node_id tempNode = pathEndNodes[0];
                pathEndNodes[0] = pathEndNodes[1];
                pathEndNodes[1] = tempNode;
            }
            assert(pathEndNodes[0] == childMarkerNodes[0] || pathEndNodes[0] == childMarkerNodes[1]);
            SPQR_CALL(mergeMemberIntoParent(dec,childMember[0],pathEndNodes[0] == childMarkerNodes[1]));
            SPQR_CALL(mergeMemberIntoParent(dec,childMember[1],pathEndNodes[1] == childMarkerNodes[3]));
            setTerminalMember(newColInfo, findMember(dec,member));

            return SPQR_OKAY;
        }
    }

    //non-root member

    if (numOneEnd == 0 && numTwoEnds == 0)
    {
        assert(pathEdgeIsValid(newCol->reducedMembers[id].firstPathEdge));
        assert(pathEndNodes[0] != INVALID_NODE);
        addTerminal(newColInfo,pathEndNodes[1]);
        if (parentMarkerNodes[0] == pathEndNodes[0]){
            flipEdge(dec,parentMarker);
        }
        return SPQR_OKAY;
    }
    assert(numOneEnd == 1);

    if (numPathEndNodes >= 2) {

        if (pathEndNodes[1] != childMarkerNodes[0] && pathEndNodes[1] != childMarkerNodes[1]){
            node_id temp = pathEndNodes[0];
            pathEndNodes[0] = pathEndNodes[1];
            pathEndNodes[1] = temp;
        }
        assert(pathEndNodes[1] == childMarkerNodes[0] || pathEndNodes[1] == childMarkerNodes[1]);

        /* Flip parent if necessary. */
        if (pathEndNodes[0] == parentMarkerNodes[0]) {
            flipEdge(dec, parentMarker);

            node_id temp = parentMarkerNodes[0];
            parentMarkerNodes[0] = parentMarkerNodes[1];
            parentMarkerNodes[1] = temp;
        }

        assert(pathEndNodes[0] == parentMarkerNodes[1]);

        SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec,childMarkerEdges[0]),
                                        pathEndNodes[1] == childMarkerNodes[1]));
        return SPQR_OKAY;
    }
    /* Tested in UpdateInnerRigidNoPath. */

    /* Parent marker and child marker must be next to each other. */
    if (parentMarkerNodes[0] == childMarkerNodes[0] || parentMarkerNodes[0] == childMarkerNodes[1]){
        flipEdge(dec, parentMarker);
    }

    bool headToHead = parentMarkerNodes[0] == childMarkerNodes[1] || parentMarkerNodes[1] == childMarkerNodes[1];
    SPQR_CALL(mergeMemberIntoParent(dec, findEdgeChildMember(dec,childMarkerEdges[0]),headToHead));

    return SPQR_OKAY;
}
static SPQR_ERROR transformReducedMember(Decomposition *dec, SPQRNewColumn * newCol, SPQRColReducedComponent * component,
                                         reduced_member_id reducedMemberId, NewColInformation * newColInfo, int depth){
    SPQRColReducedMember * reducedMember = &newCol->reducedMembers[reducedMemberId];
    if(reducedMember->type == TYPE_CYCLE_CHILD && depth > 0){
        return SPQR_OKAY; //path has been propagated away; no need to do anything
    }

    //handle children recursively
    for (children_idx idx = reducedMember->firstChild; idx < reducedMember->firstChild + reducedMember->numChildren; ++idx){
        reduced_member_id reducedChild = newCol->childrenStorage[idx];
        assert(reducedMemberIsValid(reducedChild));
        SPQR_CALL(transformReducedMember(dec,newCol,component,reducedChild,newColInfo,depth+1));
    }
    assert(memberIsRepresentative(dec,reducedMember->member));
    MemberType type = getMemberType(dec,reducedMember->member); //TODO: find, or not?
    if(type == SERIES){
        SPQR_CALL(transformSeries(dec,newCol,reducedMemberId,newColInfo,depth));
    }else if(type == PARALLEL){
        SPQR_CALL(transformParallel(dec,newCol,reducedMemberId,newColInfo,depth));
    }else if(type == RIGID){
        SPQR_CALL(transformRigid(dec,newCol,reducedMemberId,newColInfo,depth));
    }
    return SPQR_OKAY;
}

static SPQR_ERROR moveReducedRoot(Decomposition *dec, SPQRNewColumn *newCol, SPQRColReducedComponent *component){
    assert(dec);
    assert(newCol);
    assert(component);

    reduced_member_id reducedMemberId = component->root;
    SPQRColReducedMember * reducedMember = &newCol->reducedMembers[reducedMemberId];
    member_id member = findMember(dec,reducedMember->member); //TODO: necessary find?

    MemberType type = getMemberType(dec,member);
    bool cycleWithUniqueEndChild = false;
    if(type == PARALLEL){
        cycleWithUniqueEndChild = (reducedMember->numOneEnd == 1 || reducedMember->numTwoEnds == 1);
    }else if(type == RIGID){
        if(reducedMember->numTwoEnds == 1 || reducedMember->numOneEnd == 1){
            node_id childMarkerNodes[2] = {
                    findEdgeHead(dec,reducedMember->childMarkerEdges[0]),
                    findEdgeTail(dec,reducedMember->childMarkerEdges[0]),
            };
            cycleWithUniqueEndChild = (reducedMember->rigidEndNodes[2] == INVALID_NODE
                    && ((reducedMember->rigidEndNodes[0] == childMarkerNodes[0]
                         && reducedMember->rigidEndNodes[1] == childMarkerNodes[1])
                        || (reducedMember->rigidEndNodes[0] == childMarkerNodes[1]
                            && reducedMember->rigidEndNodes[1] == childMarkerNodes[0])));

        }else{
            cycleWithUniqueEndChild = false;
        }
    }
    if(!cycleWithUniqueEndChild){
        return SPQR_OKAY;
    }
    while(cycleWithUniqueEndChild){
        for (children_idx idx = reducedMember->firstChild; idx < reducedMember->firstChild + reducedMember->numChildren;
             ++idx) {
            reducedMemberId = newCol->childrenStorage[idx];
            ReducedMemberType childType = newCol->reducedMembers[reducedMemberId].type;
            if(childType == TYPE_SINGLE_CHILD || childType == TYPE_DOUBLE_CHILD){
                reducedMember = &newCol->reducedMembers[reducedMemberId];
                break;
            }
        }
        assert(findMemberNoCompression(dec,reducedMember->member) != member); //Assert that we found a child in the above loop

        member = reducedMember->member; //TODO: need to find, or not?
        assert(memberIsRepresentative(dec,member));

        createPathEdge(dec,newCol, markerToParent(dec,member),reducedMemberId); //Add edge which closes cycle

        assert(reducedMember->member == member);
        type = getMemberType(dec,member);
        if(type == PARALLEL){
            assert(pathEdgeIsValid(reducedMember->firstPathEdge) && reducedMember->numPathEdges == 1);
            cycleWithUniqueEndChild = (reducedMember->numOneEnd == 1 || reducedMember->numTwoEnds == 1);
        }else if(type == RIGID){
            if(reducedMember->numOneEnd == 1 || reducedMember->numTwoEnds == 1){
                // For non-root rigid members we have to check whether the path edges together with the parent marker edge form a cycle with a two-end child.
                edge_id edge = markerToParent(dec,member);
                node_id parentMarkerNodes[2] = {
                        findEdgeTail(dec, edge),
                        findEdgeHead(dec, edge)
                };
                node_id childMarkerNodes[2] = {
                        findEdgeTail(dec, reducedMember->childMarkerEdges[0]),
                        findEdgeHead(dec, reducedMember->childMarkerEdges[0])
                };

                int numEndNodes = reducedMember->rigidEndNodes[0] == INVALID_NODE ?
                        0 : (reducedMember->rigidEndNodes[2] == INVALID_NODE ? 2 : 4);
                if (numEndNodes == 0)
                {
                    /* Without path edges the child marker would have to be parallel to the parent marker, which is detected
                     * during typing. */
                    cycleWithUniqueEndChild = false;
                    break;
                }

                /* Determine the end nodes of the path including the parent marker edge. */
                node_id endNodes[2];
                if (numEndNodes == 4)
                {
                    endNodes[0] = reducedMember->rigidEndNodes[1];
                    endNodes[1] = reducedMember->rigidEndNodes[3];
                }
                else if (reducedMember->rigidEndNodes[0] == parentMarkerNodes[0])
                {
                    endNodes[0] = parentMarkerNodes[1];
                    endNodes[1] = reducedMember->rigidEndNodes[1];
                }
                else
                {
                    assert(reducedMember->rigidEndNodes[0] == parentMarkerNodes[1]);
                    endNodes[0] = parentMarkerNodes[0];
                    endNodes[1] = reducedMember->rigidEndNodes[1];
                }

                cycleWithUniqueEndChild = (endNodes[0] == childMarkerNodes[0] && endNodes[1] == childMarkerNodes[1])
                                          || (endNodes[0] == childMarkerNodes[1] && endNodes[1] == childMarkerNodes[0]);
            }else{
                cycleWithUniqueEndChild = false;
            }
        }else{
            assert(type == SERIES);
            if(reducedMember->numOneEnd == 1 || reducedMember->numTwoEnds == 1){
                cycleWithUniqueEndChild = reducedMember->numPathEdges == (getNumMemberEdges(dec,member) -1);
            }else{
                cycleWithUniqueEndChild = false;
            }
        }
    }
    component->root = reducedMemberId;
    return SPQR_OKAY;
}
static SPQR_ERROR transformComponent(Decomposition *dec, SPQRNewColumn *newCol, SPQRColReducedComponent * component, NewColInformation * newColInfo){
    assert(dec);
    assert(newCol);
    assert(component);
    assert(newColInfo);

    //First, ensure that the reduced root member is moved up if it is a cycle
    SPQR_CALL(moveReducedRoot(dec,newCol,component));
    //Then, recursively transform the components
    SPQR_CALL(transformReducedMember(dec,newCol,component,component->root,newColInfo,0));
    return SPQR_OKAY;
}

SPQR_ERROR addNewColumn(Decomposition *dec, SPQRNewColumn *newCol){
    assert(dec);
    assert(newCol);

    if(newCol->numReducedComponents == 0){
        member_id member;
        SPQR_CALL(createStandaloneSeries(dec,newCol->newRowEdges,newCol->numNewRowEdges,newCol->newColIndex,&member));
    }else if(newCol->numReducedComponents == 1){
        NewColInformation information = emptyNewColInformation();
        SPQR_CALL(transformComponent(dec,newCol,&newCol->reducedComponents[0],&information));
        assert(memberIsRepresentative(dec,information.member));
        if(newCol->numNewRowEdges == 0){
            edge_id colEdge = INVALID_EDGE;
            SPQR_CALL(createColumnEdge(dec,information.member,&colEdge,newCol->newColIndex));
            if(nodeIsValid(information.terminalNode[0])){
                setEdgeHeadAndTail(dec,colEdge,
                                   findNode(dec,information.terminalNode[0]),findNode(dec,information.terminalNode[1]));
            }
        }else{
            member_id newSeries;
            SPQR_CALL(createConnectedSeries(dec,newCol->newRowEdges,newCol->numNewRowEdges,newCol->newColIndex,&newSeries));
            edge_id markerEdge = INVALID_EDGE;
            edge_id ignore = INVALID_EDGE;
            SPQR_CALL(createMarkerPairWithReferences(dec,information.member,newSeries,false,&markerEdge,&ignore));
            if(nodeIsValid(information.terminalNode[0])){
                setEdgeHeadAndTail(dec,markerEdge,findNode(dec,information.terminalNode[0]),
                                   findNode(dec,information.terminalNode[1]));
            }
        }
    }else{
#ifndef NDEBUG
        int numDecComponentsBefore = numConnectedComponents(dec);
#endif
        member_id newSeries;
        SPQR_CALL(createConnectedSeries(dec,newCol->newRowEdges,newCol->numNewRowEdges,newCol->newColIndex,&newSeries));
        for (int i = 0; i < newCol->numReducedComponents; ++i) {
            NewColInformation information = emptyNewColInformation();
            SPQR_CALL(transformComponent(dec,newCol,&newCol->reducedComponents[i],&information));
            reorderComponent(dec,information.member); //reorder the subtree so that the newly series member is a parent
            edge_id markerEdge = INVALID_EDGE;
            edge_id ignore = INVALID_EDGE;
            SPQR_CALL(createMarkerPairWithReferences(dec,newSeries,information.member,true,&ignore,&markerEdge));
            if (nodeIsValid(information.terminalNode[0])) {
                setEdgeHeadAndTail(dec, markerEdge, findNode(dec, information.terminalNode[0]),
                                   findNode(dec, information.terminalNode[1]));
            }
        }
        decreaseNumConnectedComponents(dec,newCol->numReducedComponents-1);
        assert(numConnectedComponents(dec) == (numDecComponentsBefore - newCol->numReducedComponents + 1));
    }
    return SPQR_OKAY;
}

bool columnAdditionRemainsGraphic(SPQRNewColumn *newCol){
    return newCol->remainsGraphic;
}
