//
// Created by rolf on 23-11-23.
//

#include "mipworkshop2024/presolve/NetworkColumnAddition.h"

static int max(int a, int b){
    return (a > b) ? a : b;
}

typedef int path_arc_id;
#define INVALID_PATH_ARC (-1)

static bool pathArcIsInvalid(const path_arc_id arc) {
    return arc < 0;
}

static bool pathArcIsValid(const path_arc_id arc) {
    return !pathArcIsInvalid(arc);
}

typedef struct {
    arc_id arc;
    bool reversed; //true if the matrix sign of this entry was -1, false otherwise
    node_id arcHead; //These can be used in various places to prevent additional find()'s
    node_id arcTail;
    path_arc_id nextMember;
    path_arc_id nextOverall;
} PathArcListNode;

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

    path_arc_id firstPathArc;
    int numPathArcs;

    int numOneEnd;
    int numTwoEnds;
    arc_id childMarkerArcs[2];
    node_id rigidEndNodes[4];
} ReducedMember;


typedef struct {
    int rootDepth;
    reduced_member_id root;
} ReducedComponent;

typedef struct {
    reduced_member_id reducedMember;
    reduced_member_id rootDepthMinimizer;
} MemberInfo;

typedef struct {
    member_id member;
} CreateReducedMembersCallstack;

struct NetworkColumnAdditionImpl {
    bool remainsNetwork;

    ReducedMember *reducedMembers;
    int memReducedMembers;
    int numReducedMembers;

    ReducedComponent *reducedComponents;
    int memReducedComponents;
    int numReducedComponents;

    MemberInfo *memberInformation;
    int memMemberInformation;
    int numMemberInformation;

    reduced_member_id *childrenStorage;
    int memChildrenStorage;
    int numChildrenStorage;

    PathArcListNode *pathArcs;
    int memPathArcs;
    int numPathArcs;
    path_arc_id firstOverallPathArc;

    int * nodeInDegree;
    int * nodeOutDegree;
    int memNodeDegree;

    bool *arcInPath;
    int memArcsInPath;

    CreateReducedMembersCallstack * createReducedMembersCallStack;
    int memCreateReducedMembersCallStack;

    col_idx newColIndex;

    row_idx *newRowArcs;
    bool * newRowArcReversed;
    int memNewRowArcs;
    int numNewRowArcs;

    arc_id *decompositionRowArcs;
    bool * decompositionRowReversed;
    int memDecompositionRowArcs;
    int numDecompositionRowArcs;
};

SPQR_ERROR createNetworkColumnAddition(SPQR* env, NetworkColumnAddition ** pNetworkColumnAddition ){
    assert(env);

    SPQR_CALL(SPQRallocBlock(env, pNetworkColumnAddition));
    NetworkColumnAddition *nca = *pNetworkColumnAddition;

    nca->remainsNetwork = false;
    nca->reducedMembers = NULL;
    nca->memReducedMembers = 0;
    nca->numReducedMembers = 0;

    nca->reducedComponents = NULL;
    nca->memReducedComponents = 0;
    nca->numReducedComponents = 0;

    nca->memberInformation = NULL;
    nca->memMemberInformation = 0;
    nca->numMemberInformation = 0;

    nca->childrenStorage = NULL;
    nca->memChildrenStorage = 0;
    nca->numChildrenStorage = 0;

    nca->pathArcs = NULL;
    nca->memPathArcs = 0;
    nca->numPathArcs = 0;
    nca->firstOverallPathArc = INVALID_PATH_ARC;

    nca->nodeInDegree = NULL;
    nca->nodeOutDegree = NULL;
    nca->memNodeDegree = 0;

    nca->arcInPath = NULL;
    nca->memArcsInPath = 0;

    nca->createReducedMembersCallStack = NULL;
    nca->memCreateReducedMembersCallStack = 0;

    nca->newColIndex = INVALID_COL;

    nca->newRowArcs = NULL;
    nca->newRowArcReversed = NULL;
    nca->memNewRowArcs = 0;
    nca->numNewRowArcs = 0;

    nca->decompositionRowArcs = NULL;
    nca->decompositionRowReversed = NULL;
    nca->memDecompositionRowArcs = 0;
    nca->numDecompositionRowArcs = 0;

    return SPQR_OKAY;
}

void freeNetworkColumnAddition(SPQR* env, NetworkColumnAddition ** pNetworkColumnAddition){
    assert(env);
    NetworkColumnAddition *nca = *pNetworkColumnAddition;
    SPQRfreeBlockArray(env, &nca->decompositionRowArcs);
    SPQRfreeBlockArray(env, &nca->decompositionRowReversed);
    SPQRfreeBlockArray(env, &nca->newRowArcs);
    SPQRfreeBlockArray(env, &nca->newRowArcReversed);
    SPQRfreeBlockArray(env, &nca->createReducedMembersCallStack);
    SPQRfreeBlockArray(env, &nca->arcInPath);
    SPQRfreeBlockArray(env, &nca->nodeOutDegree);
    SPQRfreeBlockArray(env, &nca->nodeInDegree);
    SPQRfreeBlockArray(env, &nca->pathArcs);
    SPQRfreeBlockArray(env, &nca->childrenStorage);
    SPQRfreeBlockArray(env, &nca->memberInformation);
    SPQRfreeBlockArray(env, &nca->reducedComponents);
    SPQRfreeBlockArray(env, &nca->reducedMembers);

    SPQRfreeBlock(env, pNetworkColumnAddition);
}



static SPQR_ERROR
updateColInformation(Decomposition *dec, NetworkColumnAddition *nca, col_idx column, const row_idx *rows,
                           const double * values, size_t numRows){
    nca->newColIndex = column;

    nca->numDecompositionRowArcs = 0;
    nca->numNewRowArcs = 0;

    for (size_t i = 0; i < numRows; ++i) {
        bool reversed = values[i] < 0.0;
        arc_id rowArc = getDecompositionRowArc(dec, rows[i]);
        if (arcIsValid(rowArc)) { //If the arc is the current decomposition: save it in the array
            if (nca->numDecompositionRowArcs == nca->memDecompositionRowArcs) {
                int newNumArcs = nca->memDecompositionRowArcs == 0 ? 8 :
                        2 * nca->memDecompositionRowArcs; //TODO: make reallocation numbers more consistent with rest?
                nca->memDecompositionRowArcs = newNumArcs;
                SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->decompositionRowArcs,
                                                (size_t) nca->memDecompositionRowArcs));
                SPQR_CALL(SPQRreallocBlockArray(dec->env,&nca->decompositionRowReversed,
                                                (size_t) nca->memDecompositionRowArcs));
            }
            nca->decompositionRowArcs[nca->numDecompositionRowArcs] = rowArc;
            nca->decompositionRowReversed[nca->numDecompositionRowArcs] = reversed;
            ++nca->numDecompositionRowArcs;
        } else {
            //Not in the decomposition: add it to the set of arcs which are newly added with this row.
            if (nca->numNewRowArcs == nca->memNewRowArcs) {
                int newNumArcs = nca->memNewRowArcs == 0 ? 8 : 2 *
                                                                    nca->memNewRowArcs; //TODO: make reallocation numbers more consistent with rest?
                nca->memNewRowArcs = newNumArcs;
                SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->newRowArcs,
                                                (size_t) nca->memNewRowArcs));
                SPQR_CALL(SPQRreallocBlockArray(dec->env,&nca->newRowArcReversed,
                                                (size_t) nca->memNewRowArcs));
            }
            nca->newRowArcs[nca->numNewRowArcs] = rows[i];
            nca->newRowArcReversed[nca->numNewRowArcs] = reversed;
            nca->numNewRowArcs++;
        }
    }

    return SPQR_OKAY;
}

static reduced_member_id createReducedMembersToRoot(Decomposition *dec, NetworkColumnAddition * nca, const member_id firstMember ){
    assert(memberIsValid(firstMember));

    CreateReducedMembersCallstack * callstack = nca->createReducedMembersCallStack;
    callstack[0].member = firstMember;
    int callDepth = 0;

    while(callDepth >= 0){
        member_id member = callstack[callDepth].member;
        reduced_member_id reducedMember = nca->memberInformation[member].reducedMember;

        bool reducedValid = reducedMemberIsValid(reducedMember);
        if(!reducedValid) {
            //reduced member was not yet created; we create it
            reducedMember = nca->numReducedMembers;

            ReducedMember *reducedMemberData = &nca->reducedMembers[reducedMember];
            ++nca->numReducedMembers;

            reducedMemberData->member = member;
            reducedMemberData->numChildren = 0;

            reducedMemberData->type = TYPE_INVALID;
            reducedMemberData->firstPathArc = INVALID_PATH_ARC;
            reducedMemberData->numPathArcs = 0;
            for (int i = 0; i < 4; ++i) {
                reducedMemberData->rigidEndNodes[i] = INVALID_NODE;
            }
            //The children are set later

            nca->memberInformation[member].reducedMember = reducedMember;
            assert(memberIsRepresentative(dec, member));
            member_id parentMember = findMemberParent(dec, member);

            if (memberIsValid(parentMember)) {
                //recursive call to parent member
                ++callDepth;
                assert(callDepth < nca->memCreateReducedMembersCallStack);
                callstack[callDepth].member = parentMember;
                continue;

            } else {
                //we found a new reduced decomposition component

                reducedMemberData->parent = INVALID_REDUCED_MEMBER;
                reducedMemberData->depth = 0;
                reducedMemberData->rootMember = member;

                assert(nca->numReducedComponents < nca->memReducedComponents);
                nca->reducedComponents[nca->numReducedComponents].root = reducedMember;
                ++nca->numReducedComponents;
            }
        }
        if(reducedValid){
            assert(reducedMember < nca->numReducedMembers);
            //Reduced member was already created in earlier call
            //update the depth of the root if appropriate
            reduced_member_id * depthMinimizer = &nca->memberInformation[nca->reducedMembers[reducedMember].rootMember].rootDepthMinimizer;
            if(reducedMemberIsInvalid(*depthMinimizer) ||
               nca->reducedMembers[reducedMember].depth < nca->reducedMembers[*depthMinimizer].depth){
                *depthMinimizer = reducedMember;
            }
        }
        while(true){
            --callDepth;
            if(callDepth < 0 ) break;
            member_id parentMember = callstack[callDepth+1].member;
            reduced_member_id parentReducedMember = nca->memberInformation[parentMember].reducedMember;
            member_id currentMember = callstack[callDepth].member;
            reduced_member_id currentReducedMember = nca->memberInformation[currentMember].reducedMember;

            ReducedMember *parentReducedMemberData = &nca->reducedMembers[parentReducedMember];
            ReducedMember *reducedMemberData = &nca->reducedMembers[currentReducedMember];

            reducedMemberData->parent = parentReducedMember;
            reducedMemberData->depth = parentReducedMemberData->depth + 1;
            reducedMemberData->rootMember = parentReducedMemberData->rootMember;

            nca->reducedMembers[parentReducedMember].numChildren++;
        }

    }

    reduced_member_id returnedMember = nca->memberInformation[callstack[0].member].reducedMember;
    return returnedMember;
}

static SPQR_ERROR constructReducedDecomposition(Decomposition *dec, NetworkColumnAddition *nca) {
    assert(dec);
    assert(nca);
#ifndef NDEBUG
    for (int i = 0; i < nca->memMemberInformation; ++i) {
        assert(reducedMemberIsInvalid(nca->memberInformation[i].reducedMember));
    }
#endif
    nca->numReducedComponents = 0;
    nca->numReducedMembers = 0;
    if (nca->numDecompositionRowArcs == 0) { //Early return in case the reduced decomposition will be empty
        return SPQR_OKAY;
    }
    assert(nca->numReducedMembers == 0);
    assert(nca->numReducedComponents == 0);

    int newSize = largestMemberID(dec); //Is this sufficient?
    if (newSize > nca->memReducedMembers) {
        nca->memReducedMembers = max(2 * nca->memReducedMembers, newSize);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->reducedMembers, (size_t) nca->memReducedMembers));
    }
    if (newSize > nca->memMemberInformation) {
        int updatedSize = max(2 * nca->memMemberInformation, newSize);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->memberInformation, (size_t) updatedSize));
        for (int i = nca->memMemberInformation; i < updatedSize; ++i) {
            nca->memberInformation[i].reducedMember = INVALID_REDUCED_MEMBER;
            nca->memberInformation[i].rootDepthMinimizer = INVALID_REDUCED_MEMBER;
        }
        nca->memMemberInformation = updatedSize;

    }

    int numComponents = numConnectedComponents(dec);
    if (numComponents > nca->memReducedComponents) {
        nca->memReducedComponents = max(2 * nca->memReducedComponents, numComponents);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->reducedComponents, (size_t) nca->memReducedComponents));
    }

    int numMembers = getNumMembers(dec);
    if (nca->memCreateReducedMembersCallStack < numMembers) {
        nca->memCreateReducedMembersCallStack = max(2 * nca->memCreateReducedMembersCallStack, numMembers);
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->createReducedMembersCallStack,
                                        (size_t) nca->memCreateReducedMembersCallStack));
    }

    //Create the reduced members (recursively)
    for (int i = 0; i < nca->numDecompositionRowArcs; ++i) {
        assert(i < nca->memDecompositionRowArcs);
        arc_id arc = nca->decompositionRowArcs[i];
        member_id arcMember = findArcMember(dec, arc);
        reduced_member_id reducedMember = createReducedMembersToRoot(dec, nca, arcMember);
        reduced_member_id *depthMinimizer = &nca->memberInformation[nca->reducedMembers[reducedMember].rootMember].rootDepthMinimizer;
        if (reducedMemberIsInvalid(*depthMinimizer)) {
            *depthMinimizer = reducedMember;
        }
    }

    //Set the reduced roots according to the root depth minimizers
    for (int i = 0; i < nca->numReducedComponents; ++i) {
        ReducedComponent *component = &nca->reducedComponents[i];
        member_id rootMember = nca->reducedMembers[component->root].member;
        reduced_member_id reducedMinimizer = nca->memberInformation[rootMember].rootDepthMinimizer;
        component->rootDepth = nca->reducedMembers[reducedMinimizer].depth;
        component->root = reducedMinimizer;

        //This simplifies code further down which does not need to be component-aware; just pretend that the reduced member is the new root.
        nca->reducedMembers[component->root].parent = INVALID_REDUCED_MEMBER;
        assert(memberIsRepresentative(dec, rootMember));
    }

    //update the children array
    int numTotalChildren = 0;
    for (int i = 0; i < nca->numReducedMembers; ++i) {
        ReducedMember *reducedMember = &nca->reducedMembers[i];
        reduced_member_id minimizer = nca->memberInformation[reducedMember->rootMember].rootDepthMinimizer;
        if (reducedMember->depth >= nca->reducedMembers[minimizer].depth) {
            reducedMember->firstChild = numTotalChildren;
            numTotalChildren += reducedMember->numChildren;
            reducedMember->numChildren = 0;
        }
    }

    if (nca->memChildrenStorage < numTotalChildren) {
        int newMemSize = max(nca->memChildrenStorage * 2, numTotalChildren);
        nca->memChildrenStorage = newMemSize;
        SPQR_CALL(SPQRreallocBlockArray(dec->env, &nca->childrenStorage, (size_t) nca->memChildrenStorage));
    }
    nca->numChildrenStorage = numTotalChildren;

    //Fill up the children array`
    for (reduced_member_id reducedMember = 0; reducedMember < nca->numReducedMembers; ++reducedMember) {
        ReducedMember *reducedMemberData = &nca->reducedMembers[reducedMember];
        if (reducedMemberData->depth <=
            nca->reducedMembers[nca->memberInformation[reducedMemberData->rootMember].rootDepthMinimizer].depth) {
            continue;
        }
        member_id parentMember = findMemberParent(dec, reducedMemberData->member);
        reduced_member_id parentReducedMember = memberIsValid(parentMember)
                                                ? nca->memberInformation[parentMember].reducedMember
                                                : INVALID_REDUCED_MEMBER;
        if (reducedMemberIsValid(parentReducedMember)) {
            //TODO: probably one of these two checks/branches is unnecessary, as there is a single failure case? (Not sure)
            ReducedMember *parentReducedMemberData = &nca->reducedMembers[parentReducedMember];
            nca->childrenStorage[parentReducedMemberData->firstChild +
                                 parentReducedMemberData->numChildren] = reducedMember;
            ++parentReducedMemberData->numChildren;
        }
    }

    //Clean up the root depth minimizers.
    for (int i = 0; i < nca->numReducedMembers; ++i) {
        ReducedMember *reducedMember = &nca->reducedMembers[i];
        assert(reducedMember);
        member_id rootMember = reducedMember->rootMember;
        assert(rootMember >= 0);
        assert(rootMember < dec->memMembers);
        nca->memberInformation[rootMember].rootDepthMinimizer = INVALID_REDUCED_MEMBER;
    }

    return SPQR_OKAY;
}

static void createPathArc(
        Decomposition * dec, NetworkColumnAddition * newCol,
        const arc_id arc,const bool reversed, const reduced_member_id reducedMember){
    assert(dec);
    assert(newCol);

    path_arc_id path_arc = newCol->numPathArcs;
    PathArcListNode * listNode = &newCol->pathArcs[path_arc];
    listNode->arc = arc;

    listNode->nextMember = newCol->reducedMembers[reducedMember].firstPathArc;
    newCol->reducedMembers[reducedMember].firstPathArc = path_arc;
    newCol->reducedMembers[reducedMember].numPathArcs += 1;

    listNode->nextOverall = newCol->firstOverallPathArc;
    newCol->firstOverallPathArc = path_arc;

    ++newCol->numPathArcs;
    assert(newCol->numPathArcs <= newCol->memPathArcs);

    assert(arc < newCol->memArcsInPath);
    newCol->arcInPath[arc] = true;
    listNode->reversed = reversed;
    if(getMemberType(dec,newCol->reducedMembers[reducedMember].member) == RIGID){
        listNode->arcHead = findArcHead(dec,arc);
        listNode->arcTail = findArcTail(dec,arc);
        assert(nodeIsValid(listNode->arcHead) && nodeIsValid(listNode->arcTail));
        assert(listNode->arcHead < newCol->memNodeDegree && listNode->arcTail < newCol->memNodeDegree);
        if(reversed){
            ++newCol->nodeInDegree[listNode->arcHead]; //TODO: if it is not invalid, we can already terminate here. How to clean up properly?
            ++newCol->nodeOutDegree[listNode->arcTail];
        }else{
            ++newCol->nodeInDegree[listNode->arcTail];
            ++newCol->nodeOutDegree[listNode->arcHead];
        }

    }else{
        listNode->arcHead = INVALID_NODE;
        listNode->arcTail = INVALID_NODE;
    }

}

static SPQR_ERROR createPathArcs(Decomposition *dec, NetworkColumnAddition *newCol){
    int maxNumPathArcs = newCol->numDecompositionRowArcs + getNumMembers(dec);
    if(newCol->memPathArcs < maxNumPathArcs){
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->pathArcs,(size_t) maxNumPathArcs)); //TODO: fix reallocation strategy
        newCol->memPathArcs = maxNumPathArcs;
    }
    int maxPathArcIndex = largestArcID(dec);
    if(newCol->memArcsInPath < maxPathArcIndex){
        int newSize = maxPathArcIndex;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->arcInPath,(size_t) newSize));//TODO: fix reallocation strategy
        for (int i = newCol->memArcsInPath; i < newSize; ++i) {
            newCol->arcInPath[i] = false;
        }
        newCol->memArcsInPath = newSize;
    }
    int maxNumNodes = largestNodeID(dec);
    if(newCol->memNodeDegree < maxNumNodes){
        int newSize = maxNumNodes;
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->nodeInDegree,(size_t) newSize));
        SPQR_CALL(SPQRreallocBlockArray(dec->env,&newCol->nodeOutDegree,(size_t) newSize));
        for (int i = newCol->memNodeDegree; i < newSize; ++i) {
            newCol->nodeInDegree[i] = 0;
            newCol->nodeOutDegree[i] = 0;
        }
        newCol->memNodeDegree = newSize;
    }
    for (int i = 0; i < newCol->numDecompositionRowArcs; ++i) {
        arc_id arc = newCol->decompositionRowArcs[i];
        bool reversed = newCol->decompositionRowReversed[i];
        member_id member = findArcMember(dec,arc);
        reduced_member_id reducedMember = newCol->memberInformation[member].reducedMember;
        createPathArc(dec,newCol,arc,reversed,reducedMember);
    }

    return SPQR_OKAY;
}

//static void countChildrenTypes(Decomposition* dec, NetworkColumnAddition * newCol, reduced_member_id reducedMember){
//    newCol->reducedMembers[reducedMember].numOneEnd = 0;
//    newCol->reducedMembers[reducedMember].numTwoEnds = 0;
//    newCol->reducedMembers[reducedMember].childMarkerArcs[0] = INVALID_ARC;
//    newCol->reducedMembers[reducedMember].childMarkerArcs[1] = INVALID_ARC;
//
//    int nextChildMarker = 0;
//    for (children_idx idx = newCol->reducedMembers[reducedMember].firstChild;
//         idx < newCol->reducedMembers[reducedMember].firstChild
//               + newCol->reducedMembers[reducedMember].numChildren;
//         ++idx) {
//        reduced_member_id reducedChild = newCol->childrenStorage[idx];
//        assert(reducedMemberIsValid(reducedChild));
//        if(newCol->reducedMembers[reducedChild].type == TYPE_SINGLE_CHILD){
//            if(nextChildMarker < 2){
//                newCol->reducedMembers[reducedMember].childMarkerArcs[nextChildMarker] = markerOfParent(dec, findMember(dec,newCol->reducedMembers[reducedChild].member)); //TODO: check if find is necessary
//                ++nextChildMarker;
//            }
//            newCol->reducedMembers[reducedMember].numOneEnd++;
//        }else if(newCol->reducedMembers[reducedChild].type == TYPE_DOUBLE_CHILD){
//            if(nextChildMarker < 2){
//                newCol->reducedMembers[reducedMember].childMarkerArcs[nextChildMarker] = markerOfParent(dec, findMember(dec,newCol->reducedMembers[reducedChild].member)); //TODO: check if find is necessary
//                ++nextChildMarker;
//            }
//            newCol->reducedMembers[reducedMember].numTwoEnds++;
//        }
//    }
//}
//void determineTypeLoop(NetworkColumnAddition *newCol, reduced_member_id reducedMemberId){
//    assert(pathArcIsValid(newCol->reducedMembers[reducedMemberId].firstPathArc));
//    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
//}
//
//void determineTypeParallel(
//        NetworkColumnAddition *newCol, reduced_member_id reducedMemberId, int depth){
//    const int numOneEnd = newCol->reducedMembers[reducedMemberId].numOneEnd;
//    const int numTwoEnds = newCol->reducedMembers[reducedMemberId].numTwoEnds;
//    assert(numOneEnd >= 0);
//    assert(numTwoEnds >= 0);
//    assert(numOneEnd + 2*numTwoEnds <= 2);
//    ReducedMember * reducedMember = &newCol->reducedMembers[reducedMemberId];
//    if( depth == 0){
//        reducedMember->type = TYPE_ROOT;
//        return;
//    }
//    int numEnds = numOneEnd + 2*numTwoEnds;
//
//    if(numEnds == 0 && pathArcIsValid(reducedMember->firstPathArc)){
//        reducedMember->type = TYPE_CYCLE_CHILD;
//    }else if(numEnds == 1){
//        reducedMember->type = TYPE_SINGLE_CHILD;
//    }else if(numEnds == 2){
//        if(pathArcIsValid(reducedMember->firstPathArc)){
//            newCol->remainsNetwork = false;
//            reducedMember->type = TYPE_INVALID;
//        }else{
//            reducedMember->type = TYPE_DOUBLE_CHILD;
//        }
//    }else{
//        //no child contains path arcs, so we are a leaf of the reduced decomposition
//        assert(pathArcIsValid(reducedMember->firstPathArc));
//        reducedMember->type = TYPE_CYCLE_CHILD; //TODO: is this not duplicate with first case? Should be able to turn into a switch case
//    }
//}
//void determineTypeSeries(Decomposition* dec, NetworkColumnAddition* newCol, reduced_member_id reducedMemberId,
//                         int depth){
//    const int numOneEnd = newCol->reducedMembers[reducedMemberId].numOneEnd;
//    const int numTwoEnds = newCol->reducedMembers[reducedMemberId].numTwoEnds;
//    assert(dec);
//    assert(newCol);
//    assert(numOneEnd >= 0);
//    assert(numTwoEnds >= 0);
//    assert(numOneEnd + 2*numTwoEnds <= 2);
//    assert(getMemberType(dec, findMemberNoCompression(dec,newCol->reducedMembers[reducedMemberId].member)) == SERIES);
//
//    ReducedMember *reducedMember =&newCol->reducedMembers[reducedMemberId];
//    member_id member = findMember(dec,reducedMember->member); //We could also pass this as function argument
//    int countedPathArcs = 0;
//    for(path_arc_id pathArc = reducedMember->firstPathArc; pathArcIsValid(pathArc);
//        pathArc = newCol->pathArcs[pathArc].nextMember){
//        ++countedPathArcs;
//    } //TODO: replace loop by count
//    int numMemberArcs = getNumMemberArcs(dec,member);
//    if(depth == 0){
//        if(numTwoEnds != 0){
//            reducedMember->type = TYPE_INVALID;
//            newCol->remainsNetwork = false;
//        }else{
//            reducedMember->type = countedPathArcs == numMemberArcs-1 ? TYPE_CYCLE_CHILD : TYPE_ROOT;
//        }
//        return;
//    }
//
//    if(countedPathArcs == numMemberArcs-1){
//        reducedMember->type = TYPE_CYCLE_CHILD;
//    }else if(countedPathArcs + numTwoEnds == numMemberArcs-1){ //TODO: shouldn't this be numMemberArcs?
//        assert(numTwoEnds == 1);
//        reducedMember->type = TYPE_DOUBLE_CHILD;
//    }else if(numTwoEnds == 1){
//        newCol->remainsNetwork = false;
//        reducedMember->type = TYPE_INVALID;
//    }else{
//        assert(numTwoEnds == 0);
//        reducedMember->type = numOneEnd == 2 ? TYPE_DOUBLE_CHILD : TYPE_SINGLE_CHILD;
//    }
//}
//
//void determineTypeRigid(Decomposition* dec, NetworkColumnAddition * newCol, reduced_member_id reducedMemberId, int depth){
//    //Rough idea; first, we find the
//    const int numOneEnd = newCol->reducedMembers[reducedMemberId].numOneEnd;
//    const int numTwoEnds = newCol->reducedMembers[reducedMemberId].numTwoEnds;
//    assert(dec);
//    assert(newCol);
//    assert(numOneEnd >= 0);
//    assert(numTwoEnds >= 0);
//    assert(numOneEnd + 2*numTwoEnds <= 2);
//    assert(getMemberType(dec, findMemberNoCompression(dec,newCol->reducedMembers[reducedMemberId].member)) == RIGID);
//    member_id member = findMember(dec,newCol->reducedMembers[reducedMemberId].member);
//
//    node_id parentMarkerNodes[2] = {
//            depth == 0 ? INVALID_NODE : findArcHead(dec, markerToParent(dec,member)),
//            depth == 0 ? INVALID_NODE : findArcTail(dec, markerToParent(dec,member)),
//    };
//
//    arc_id * childMarkerArcs = newCol->reducedMembers[reducedMemberId].childMarkerArcs;
//    node_id childMarkerNodes[4] = {
//            childMarkerArcs[0] == INVALID_EDGE ? INVALID_NODE : findArcHead(dec,childMarkerArcs[0]),
//            childMarkerArcs[0] == INVALID_EDGE ? INVALID_NODE : findArcTail(dec,childMarkerArcs[0]),
//            childMarkerArcs[1] == INVALID_EDGE ? INVALID_NODE : findArcHead(dec,childMarkerArcs[1]),
//            childMarkerArcs[1] == INVALID_EDGE ? INVALID_NODE : findArcTail(dec,childMarkerArcs[1]),
//    };
//
//    //First, find the end nodes of the path.
//    //If there are too many (>4) or there is some node with degree > 2, we terminate
//    node_id * pathEndNodes = newCol->reducedMembers[reducedMemberId].rigidEndNodes;
//    int numPathEndNodes = 0;
//    for (path_arc_id pathArc = newCol->reducedMembers[reducedMemberId].firstPathArc; pathArcIsValid(pathArc);
//         pathArc = newCol->pathArcs[pathArc].nextMember) {
//        arc_id arc = newCol->pathArcs[pathArc].arc;
//        node_id nodes[2] = {findArcHead(dec,arc), findArcTail(dec,arc)};
//        for (int i = 0; i < 2; ++i) {
//            node_id node = nodes[i];
//            assert(newCol->nodePathDegree[node] > 0);
//            if(newCol->nodePathDegree[node] > 2){
//                //Node of degree 3 or higher implies that the given arcs cannot form a path.
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsNetwork = false;
//                return;
//            }
//            if(newCol->nodePathDegree[node] == 1){
//                //If we have 5 or more end nodes, stop
//                if(numPathEndNodes >= 4){
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                    newCol->remainsNetwork = false;
//                    return;
//                }
//                pathEndNodes[numPathEndNodes] = node;
//                ++numPathEndNodes;
//            }
//
//
//        }
//    }
//
//    //Exchange end nodes in the end nodes so that if there are 4 arcs, 0 is connected with 1 and 2 is connected with 3
//    //Parent marker should come first for each path
//    if(numPathEndNodes == 4){
//        //We try to follow the path
//        arc_id * nodeArcs;
//        SPQRallocBlockArray(dec->env,&nodeArcs, (size_t) (2* largestNodeID(dec))); //TODO: move to struct
//
//        //initialize for all relevant nodes
//        for (path_arc_id pathArc = newCol->reducedMembers[reducedMemberId].firstPathArc; pathArcIsValid(pathArc);
//             pathArc = newCol->pathArcs[pathArc].nextMember){
//            arc_id arc = newCol->pathArcs[pathArc].arc;
//            node_id nodes[2] = {findArcHead(dec,arc),findArcTail(dec,arc)};
//            for (int i = 0; i < 2; ++i) {
//                node_id node = nodes[i];
//                nodeArcs[2*node] = INVALID_EDGE;
//                nodeArcs[2*node + 1] = INVALID_EDGE;
//            }
//        }
//        //store incident arcs for every node
//        for (path_arc_id pathArc = newCol->reducedMembers[reducedMemberId].firstPathArc; pathArcIsValid(pathArc);
//             pathArc = newCol->pathArcs[pathArc].nextMember){
//            arc_id arc = newCol->pathArcs[pathArc].arc;
//            node_id nodes[2] = {findArcHead(dec,arc),findArcTail(dec,arc)};
//            for (int i = 0; i < 2; ++i) {
//                node_id node = nodes[i];
//                node_id index = 2*node;
//                if(nodeArcs[index] != INVALID_EDGE){
//                    ++index;
//                }
//                nodeArcs[index] = arc;
//            }
//        }
//        //Now follow the path starting from end node 0 to see where we end
//        arc_id previousArc = INVALID_EDGE;
//        node_id currentNode = pathEndNodes[0];
//        while(true){
//            arc_id arc = nodeArcs[2*currentNode];
//            if(arc == previousArc){
//                arc = nodeArcs[2*currentNode+1];
//            }
//            if(arc == INVALID_EDGE){
//                break;
//            }
//            previousArc = arc;
//            node_id node = findArcHead(dec,arc);
//            currentNode = (node != currentNode) ? node : findArcTail(dec,arc);
//        }
//        SPQRfreeBlockArray(dec->env,&nodeArcs);
//
//        if(currentNode == pathEndNodes[2]){
//            pathEndNodes[2] = pathEndNodes[1];
//            pathEndNodes[1] = currentNode;
//        }else if(currentNode == pathEndNodes[3]){
//            pathEndNodes[3] = pathEndNodes[1];
//            pathEndNodes[1] = currentNode;
//        }
//        //make sure node 2 is the parent marker. We can assume that 2 and 3 now also form a nice path
//        if(pathEndNodes[2] != parentMarkerNodes[0] && pathEndNodes[2] != parentMarkerNodes[1]){
//            node_id temp = pathEndNodes[2];
//            pathEndNodes[2] = pathEndNodes[3];
//            pathEndNodes[3] = temp;
//        }
//    }
//    //make sure node 0 is the parent marker node
//    if(numPathEndNodes >= 2 && pathEndNodes[0] != parentMarkerNodes[0] && pathEndNodes[0] != parentMarkerNodes[1]){
//        node_id temp = pathEndNodes[0];
//        pathEndNodes[0] = pathEndNodes[1];
//        pathEndNodes[1] = temp;
//    }
//
//    //Finally, we determine the type of the rigid node
//    if(depth == 0){
//        if(numPathEndNodes == 0){
//            if(numOneEnd == 2 && (
//                    childMarkerNodes[0] == childMarkerNodes[2] ||
//                    childMarkerNodes[0] == childMarkerNodes[3] ||
//                    childMarkerNodes[1] == childMarkerNodes[2] ||
//                    childMarkerNodes[1] == childMarkerNodes[3])){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
//                return;
//            }else{
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//        }else if(numPathEndNodes == 2){
//            if(numOneEnd == 1){
//                bool pathAdjacentToChildMarker = false;
//                for (int i = 0; i < 2; ++i) {
//                    for (int j = 0; j < 2; ++j) {
//                        if(pathEndNodes[i] == childMarkerNodes[j]){
//                            pathAdjacentToChildMarker = true;
//                        }
//                    }
//                }
//
//                if(pathAdjacentToChildMarker){
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
//                    return;
//                }else{
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                    newCol->remainsGraphic = false;
//                    return;
//                }
//            }else if(numOneEnd == 2){
//                bool childMarkerNodesMatched[2] = {false,false};
//                bool endNodesMatched[2] ={false, false};
//                for (int i = 0; i < 2; ++i) {
//                    for (int j = 0; j < 4; ++j) {
//                        if(pathEndNodes[i] == childMarkerNodes[j]){
//                            endNodesMatched[i] = true;
//                            childMarkerNodesMatched[j/2] = true;
//                        }
//                    }
//                }
//
//                if(childMarkerNodesMatched[0] && childMarkerNodesMatched[1] && endNodesMatched[0] && endNodesMatched[1]){
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
//                    return;
//                }else{
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                    newCol->remainsGraphic = false;
//                    return;
//                }
//            }else if(numTwoEnds == 0){
//                assert(numOneEnd == 0);
//                newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
//                return;
//            }else{
//                assert(numOneEnd == 0);
//                assert(numTwoEnds == 1);
//                if((childMarkerNodes[0] == pathEndNodes[0] && childMarkerNodes[1] == pathEndNodes[1]) ||
//                   (childMarkerNodes[0] == pathEndNodes[1] && childMarkerNodes[1] == pathEndNodes[0])
//                        ){
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_ROOT;
//                    return;
//                }else{
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                    newCol->remainsGraphic = false;
//                    return;
//                }
//            }
//        }else{
//            assert(numPathEndNodes == 4);
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//    }
//    int parentMarkerDegrees[2] = {
//            newCol->nodePathDegree[parentMarkerNodes[0]],
//            newCol->nodePathDegree[parentMarkerNodes[1]]
//    };
//    //Non-root rigid member
//    if(numPathEndNodes == 0){
//        // We have no path arcs, so there must be at least one child containing one/two path ends.
//        assert(numOneEnd + numTwoEnds > 0);
//        // We should not have a child marker arc parallel to the parent marker arc!
//        assert(!(parentMarkerNodes[0] == childMarkerNodes[0] && parentMarkerNodes[1] == childMarkerNodes[1])
//               && !(parentMarkerNodes[0] == childMarkerNodes[1] && parentMarkerNodes[1] == childMarkerNodes[0]));
//        if(numOneEnd == 0){
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }else if (numOneEnd == 1){
//            if (childMarkerNodes[0] == parentMarkerNodes[0] || childMarkerNodes[0] == parentMarkerNodes[1]
//                || childMarkerNodes[1] == parentMarkerNodes[0] || childMarkerNodes[1] == parentMarkerNodes[1]){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
//                return;
//            }else{
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//        }else{
//            assert(numOneEnd == 2);
//
//            int childMarkerParentNode[2] = {-1,-1};
//            bool isParallel = false;
//            for (int i = 0; i < 4; ++i) {
//                for (int j = 0; j < 2; ++j) {
//                    if(childMarkerNodes[i] == parentMarkerNodes[j]){
//                        if(childMarkerParentNode[i/2] >= 0){
//                            isParallel = true;
//                        }
//                        childMarkerParentNode[i/2] = j;
//                    }
//                }
//            }
//            if(!isParallel && childMarkerParentNode[0] != -1 && childMarkerParentNode[1] != -1 &&
//               childMarkerParentNode[0] != childMarkerParentNode[1]){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                return;
//            }else{
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//        }
//    }
//    else if(numPathEndNodes == 2){
//        if(numOneEnd == 1){
//            //TODO: is the below check necessary?
//            if(parentMarkerNodes[0] != pathEndNodes[0]){
//                node_id tempMarker = parentMarkerNodes[0];
//                parentMarkerNodes[0] = parentMarkerNodes[1];
//                parentMarkerNodes[1] = tempMarker;
//
//                int tempDegree = parentMarkerDegrees[0];
//                parentMarkerDegrees[0] = parentMarkerDegrees[1];
//                parentMarkerDegrees[1] = tempDegree;
//            }
//            if(parentMarkerNodes[0] != pathEndNodes[0]){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//            if(parentMarkerNodes[1] == pathEndNodes[1]){
//                // Path closes a cycle with parent marker arc.
//                if (childMarkerNodes[0] == parentMarkerNodes[0] || childMarkerNodes[0] == parentMarkerNodes[1]
//                    || childMarkerNodes[1] == parentMarkerNodes[0] || childMarkerNodes[1] == parentMarkerNodes[1])
//                {
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
//                    return;
//                }
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }else{
//                if(childMarkerNodes[0] == pathEndNodes[1] || childMarkerNodes[1] == pathEndNodes[1]){
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
//                    return;
//                }else if(childMarkerNodes[0] == parentMarkerNodes[1] || childMarkerNodes[1] == parentMarkerNodes[1]){
//                    newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                    return;
//                }
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//        }
//        else if(numOneEnd == 2){
//            node_id otherParentNode;
//            if(pathEndNodes[0] == parentMarkerNodes[0]){
//                otherParentNode = parentMarkerNodes[1];
//            }else if(pathEndNodes[0] == parentMarkerNodes[1]){
//                otherParentNode = parentMarkerNodes[0];
//            }else{
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//            if(pathEndNodes[1] == otherParentNode){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//                newCol->remainsGraphic = false;
//                return;
//            }
//            bool childMatched[2] = {false,false};
//            bool pathEndMatched = false;
//            bool otherParentMatched = false;
//            for (int i = 0; i < 4; ++i) {
//                if(childMarkerNodes[i] == pathEndNodes[1]){
//                    childMatched[i/2] = true;
//                    pathEndMatched = true;
//                }
//                if(childMarkerNodes[i] == otherParentNode){
//                    childMatched[i/2] = true;
//                    otherParentMatched = true;
//                }
//            }
//            if(childMatched[0] && childMatched[1] && pathEndMatched  && otherParentMatched){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                return;
//            }
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//
//        }
//        else if(numTwoEnds == 0){
//            if ((parentMarkerDegrees[0] % 2 == 0 && parentMarkerDegrees[1] == 1) ||
//                (parentMarkerDegrees[0] == 1 && parentMarkerDegrees[1] % 2 == 0))
//            {
//                newCol->reducedMembers[reducedMemberId].type = TYPE_SINGLE_CHILD;
//                return;
//            }
//            else if (parentMarkerDegrees[0] == 1 && parentMarkerDegrees[1] == 1)
//            {
//                newCol->reducedMembers[reducedMemberId].type = TYPE_CYCLE_CHILD;
//                return;
//            }
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//
//        }
//        else{
//            assert(numTwoEnds == 1);
//            if ((pathEndNodes[0] == parentMarkerNodes[0] && parentMarkerNodes[1] == childMarkerNodes[0]
//                 && childMarkerNodes[1] == pathEndNodes[1])
//                || (pathEndNodes[0] == parentMarkerNodes[0] && parentMarkerNodes[1] == childMarkerNodes[1]
//                    && childMarkerNodes[0] == pathEndNodes[1])
//                || (pathEndNodes[0] == parentMarkerNodes[1] && parentMarkerNodes[0] == childMarkerNodes[0]
//                    && childMarkerNodes[1] == pathEndNodes[1])
//                || (pathEndNodes[0] == parentMarkerNodes[1] && parentMarkerNodes[0] == childMarkerNodes[1]
//                    && childMarkerNodes[0] == pathEndNodes[1]))
//            {
//                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                return;
//            }
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//    }
//    else {
//        assert(numPathEndNodes == 4);
//        if(pathEndNodes[0] != parentMarkerNodes[0] && pathEndNodes[0] != parentMarkerNodes[1]){
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//        if(pathEndNodes[2] != parentMarkerNodes[0] && pathEndNodes[2] != parentMarkerNodes[1]){
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//        if(numOneEnd == 1){
//            if((pathEndNodes[1] == childMarkerNodes[0] || pathEndNodes[1] == childMarkerNodes[1]) ||
//               (pathEndNodes[3] == childMarkerNodes[0] || pathEndNodes[3] == childMarkerNodes[1])){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                return;
//            }
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//        else if(numOneEnd == 2){
//            bool pathConnected[2] = {false,false};
//            bool childConnected[2] = {false,false};
//            for (int i = 0; i < 2; ++i) {
//                for (int j = 0; j < 4; ++j) {
//                    if(pathEndNodes[1+2*i] == childMarkerNodes[j]){
//                        pathConnected[i] = true;
//                        childConnected[j/2] = true;
//                    }
//                }
//            }
//            if(pathConnected[0] && pathConnected[1] && childConnected[0] && childConnected[1]){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                return;
//            }
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//        else if(numTwoEnds == 0){
//            newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//            return;
//        }
//        else{
//            if((pathEndNodes[1] == childMarkerNodes[0] && pathEndNodes[3] == childMarkerNodes[1]) ||
//               (pathEndNodes[1] == childMarkerNodes[1] && pathEndNodes[3] == childMarkerNodes[0])){
//                newCol->reducedMembers[reducedMemberId].type = TYPE_DOUBLE_CHILD;
//                return;
//            }
//            newCol->reducedMembers[reducedMemberId].type = TYPE_INVALID;
//            newCol->remainsGraphic = false;
//            return;
//        }
//    }
//}
//void determineTypes(Decomposition *dec, NetworkColumnAddition *newCol, ReducedComponent * component,
//                    reduced_member_id reducedMember,
//                    int depth ){
//    assert(dec);
//    assert(newCol);
//
//    for (children_idx idx = newCol->reducedMembers[reducedMember].firstChild;
//         idx < newCol->reducedMembers[reducedMember].firstChild
//               + newCol->reducedMembers[reducedMember].numChildren;
//         ++idx) {
//        reduced_member_id reducedChild = newCol->childrenStorage[idx];
//        assert(reducedMemberIsValid(reducedChild));
//        determineTypes(dec,newCol,component,reducedChild,depth + 1);
//        if(!newCol->remainsNetwork){
//            return;
//        }
//    }
//
//    countChildrenTypes(dec,newCol,reducedMember);
//    if(2*newCol->reducedMembers[reducedMember].numTwoEnds + newCol->reducedMembers[reducedMember].numOneEnd > 2){
//        newCol->remainsNetwork = false;
//        return;
//    }
//    //Determine type of this
//    bool isRoot = reducedMember == component->root;
//    member_id member = findMember(dec,newCol->reducedMembers[reducedMember].member); //TODO: find necessary?
//    MemberType type = getMemberType(dec,member);
//    if(type == LOOP){
//        assert(depth == 0);
//        determineTypeLoop(newCol,reducedMember);
//    }else if(type == PARALLEL){
//        determineTypeParallel(newCol,reducedMember,depth);
//    }else if (type == SERIES){
//        determineTypeSeries(dec,newCol,reducedMember,depth);
//    }else{
//        assert(type == RIGID);
//        determineTypeRigid(dec,newCol,reducedMember,depth);
//    }
//
//    //Add a marked arc to the path arc of the parent of this
//    if(newCol->remainsNetwork && !isRoot && newCol->reducedMembers[reducedMember].type == TYPE_CYCLE_CHILD){
//        member_id parentMember = findMemberParent(dec,newCol->reducedMembers[reducedMember].member);
//        reduced_member_id reducedParent = newCol->memberInformation[parentMember].reducedMember;
//        arc_id marker = markerOfParent(dec,member);
//
//        createPathArc(dec,newCol,marker,reducedParent);
//    }
//}
//void determineComponentTypes(Decomposition * dec, NetworkColumnAddition * newCol, ReducedComponent * component){
//    assert(dec);
//    assert(newCol);
//    assert(component);
//
//    determineTypes(dec,newCol,component,component->root,0);
//}
SPQR_ERROR networkColumnAdditionCheck(Decomposition * dec,
                                      NetworkColumnAddition * networkColumnAddition,
                                      col_idx column,
                                      const row_idx * nonzeroRows,
                                      const double * nonzeroValues, //TODO: think about whether using this or int or bool is a more convenient interface..
                                      size_t numNonzeros){
    assert(dec);
    assert(networkColumnAddition);
    assert(numNonzeros == 0 || (nonzeroRows && nonzeroValues));
    assert(colIsValid(column));

    networkColumnAddition->remainsNetwork = true;
    //cleanupPreviousIteration(dec,networkColumnAddition);

    //Store nonzero slice/call data
    SPQR_CALL(updateColInformation(dec,networkColumnAddition,column,nonzeroRows,nonzeroValues,numNonzeros));

    SPQR_CALL(constructReducedDecomposition(dec,networkColumnAddition));
    SPQR_CALL(createPathArcs(dec,networkColumnAddition));
//    for (int i = 0; i < networkColumnAddition->numReducedComponents; ++i) {
//        determineComponentTypes(dec,networkColumnAddition,&networkColumnAddition->reducedComponents[i]);
//    }
//
//    cleanupMemberInformation(networkColumnAddition);

    
    return SPQR_OKAY;
}

SPQR_ERROR networkColumnAdditionAdd(Decomposition *dec, NetworkColumnAddition *nca){
    assert(dec);
    assert(nca);

    if(nca->numReducedComponents == 0){
        member_id member;
        SPQR_CALL(createStandaloneSeries(dec,nca->newRowArcs,nca->newRowArcReversed,nca->numNewRowArcs,nca->newColIndex,&member));
    }else if (nca->numReducedComponents == 1){

    }else{

    }

    return SPQR_OKAY;
}

bool networkColumnAdditionRemainsNetwork(NetworkColumnAddition *nca){
    assert(nca);
    return nca->remainsNetwork;
}