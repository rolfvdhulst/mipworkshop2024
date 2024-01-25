//
// Created by rolf on 23-11-23.
//

#ifndef MIPWORKSHOP2024_NETWORKDECOMPOSITION_H
#define MIPWORKSHOP2024_NETWORKDECOMPOSITION_H


#include "SPQRShared.h"
#include <limits.h>

#ifdef __cplusplus
extern "C"{
#endif
//TODO: assert that member/nodes/edges are representative in many functions

typedef int node_id;
#define INVALID_NODE (-1)

bool nodeIsInvalid(node_id node);

bool nodeIsValid(node_id node);

typedef int member_id;
#define INVALID_MEMBER (-1)

bool memberIsInvalid(member_id member);
bool memberIsValid(member_id member);

typedef int arc_id;
#define INVALID_ARC (-1)

bool arcIsInvalid(arc_id arc);
bool arcIsValid(arc_id arc);

typedef int spqr_element;


//TODO: test efficiency of swapping these around
//Columns 0..x correspond to elements 0..x
//Rows 0..y correspond to elements -1.. -y-1

#define MARKER_ROW_ELEMENT (INT_MIN)
#define MARKER_COLUMN_ELEMENT (INT_MAX)

bool elementIsRow(spqr_element element);
bool elementIsColumn(spqr_element element);
spqr_row elementToRow(spqr_element element);
spqr_col elementToColumn(spqr_element element);

spqr_element rowToElement(spqr_row row);
spqr_element columnToElement(spqr_col column);

typedef struct{
    arc_id previous;
    arc_id next;
}ArcListNode;

typedef struct {
    node_id representativeNode;
    arc_id firstArc; //first arc of the linked list of neighbouring arcs.
                     // Both out and in arcs are stored in the same list as we typically need both at the same time
    int numArcs;
} DecompositionNodeData;

typedef struct {
    node_id head;
    node_id tail;
    member_id member;
    member_id childMember; //Optionally valid; if this is a virtual edge to a child member, indicates the member id of the child
    ArcListNode headArcListNode; //Linked list node of the array of arcs of the head node
    ArcListNode tailArcListNode; //Linked list node of the array of arcs of the tail node
    ArcListNode arcListNode; //Linked-list node of the array of arcs of the member which this edge is in

    spqr_element element; //The element which this arc represents
    bool reversed; //Indicates whether the arc points forwards or backwards in non-rigid nodes (where the head and tail have not been allocated yet)
                   //Has no effect in rigid nodes
} DecompositionArcData;

typedef enum {
    RIGID = 0, //Also known as triconnected components
    PARALLEL = 1,//Also known as a 'bond'
    SERIES = 2, //Also known as 'polygon' or 'cycle'
    LOOP = 3,
    UNASSIGNED = 4 // To indicate that the member is not representative; this is just there to catch errors.
} MemberType;

typedef struct {
    member_id representativeMember;
    MemberType type;

    member_id parentMember;
    arc_id markerToParent;
    arc_id markerOfParent;

    arc_id firstArc; //First of the members' linked-list edge array
    int numArcs;
} DecompositionMemberData;

typedef struct {
    //Array (with replacement) of the members' edges
    int numArcs; //Not similar to vector size, but rather is equal to the number of allocated arcs.
    int memArcs;
    DecompositionArcData *arcs;
    arc_id firstFreeArc;

    //Dynamic array with the data of the members of the decomposition
    int memMembers;
    int numMembers;
    DecompositionMemberData *members;

    //Dynamic array with the data of the members' nodes of the decomposition
    int memNodes;
    int numNodes;
    DecompositionNodeData *nodes;

    //Dynamic array for storing the mapping from matrix rows to decomposition arcs
    int memRows;
    int numRows;
    arc_id * rowArcs;

    //Dynamic array for storing the mapping from matrix columns to decomposition arcs
    int memColumns;
    int numColumns;
    arc_id * columnArcs;

    SPQR * env;

    int numConnectedComponents;
    //TODO: fix this ugly number of connected components interface.
    //Perhaps add another union-find datastructure for distinguishing separate SPQR trees?
} Decomposition;

///Mapping functions for rows/columns
bool decompositionHasRow(Decomposition *dec, row_idx row);
bool decompositionHasCol(Decomposition *dec, col_idx col);
void setDecompositionColumnArc(Decomposition *dec, col_idx col, arc_id arc);
void setDecompositionRowArc(Decomposition *dec, row_idx row, arc_id arc);
arc_id getDecompositionColumnArc(const Decomposition *dec, col_idx col);
arc_id getDecompositionRowArc(const Decomposition *dec, row_idx row);

/// Arc properties
bool arcIsMarkerToChild(const Decomposition *dec, arc_id arc);
bool arcIsTree(const Decomposition *dec, arc_id arc);
spqr_element arcGetElement(const Decomposition * dec, arc_id arc);

//TODO: test if storing rank separately speeds up computations
/// Union-find operations for the nodes
/**
 * @return Returns true if the node is a representative node
 */
bool nodeIsRepresentative(const Decomposition *dec, node_id node);

/**
 * Finds the representative of the node
 * @return the representative of node
 */
node_id findNode(Decomposition *dec, node_id node);

/**
 * Similar to findNode, except it does not perform any compression in the union-find tree.
 * @return The representative of node
 */
node_id findNodeNoCompression(const Decomposition *dec, node_id node);

/**
 * Performs a Union operation to merge two representative nodes
 * @return The node which is the representative of the two merged nodes
 */
node_id mergeNodes(Decomposition *dec, node_id first, node_id second);


/// Union find operations for members.

/**
 * Returns true if and only if the member is a representative member
 */
bool memberIsRepresentative(const Decomposition *dec, member_id member);

/**
 * Finds the representative of the member
 * @return the representative of the member
 */
member_id  findMember(Decomposition *dec, member_id member);

/**
 * Similar to findMember, except it does not perform any compression
 * @return The representative of the member
 */
member_id findMemberNoCompression(const Decomposition *dec, member_id member);

/**
 * Performs a union operation, merging two representative members into one
 * @return The member which is the representative of the two members
 */
member_id mergeMembers(Decomposition *dec, member_id first, member_id second);

/**
 * Returns the representative node of the arc tail
 */
node_id findArcTail(Decomposition *dec, arc_id arc);
/**
 * Returns the representative node of the edge head
 */
node_id findArcHead(Decomposition *dec, arc_id arc);

///Similar to the above, but do not perform path compression for the nodes whilst finding
node_id findArcTailNoCompression(const Decomposition *dec, arc_id arc);
node_id findArcHeadNoCompression(const Decomposition *dec, arc_id arc);

/**
 * @return The representative member to which the edge belongs
 */
member_id findArcMember(Decomposition *dec, arc_id arc);
member_id findArcMemberNoCompression(const Decomposition *dec, arc_id arc);

/**
 * Arc iteration for members
 */
arc_id getFirstMemberArc(const Decomposition * dec, member_id member);
arc_id getNextMemberArc(const Decomposition * dec, arc_id arc);
arc_id getPreviousMemberArc(const Decomposition *dec, arc_id arc);
int getNumMemberArcs(const Decomposition * dec, member_id member);

/**
 * Arc iteration for nodes
 */
arc_id getFirstNodeArc(const Decomposition * dec, node_id node);
arc_id getNextNodeArc(Decomposition * dec, arc_id arc, node_id node);
arc_id getPreviousNodeArc(Decomposition * dec, arc_id arc, node_id node);

int getNumNodes(const Decomposition *dec);
int getNumMembers(const Decomposition *dec);

/**
 * @return The member which is a child of this arc
 */
member_id findArcChildMember(Decomposition *dec, arc_id arc);
member_id findArcChildMemberNoCompression(const Decomposition *dec, arc_id arc);
/**
 * @return The representative member of the parent of this member
 */
member_id findMemberParent(Decomposition *dec, member_id member);
member_id findMemberParentNoCompression(const Decomposition *dec, member_id member);

MemberType getMemberType(const Decomposition *dec, member_id member);
void updateMemberType(const Decomposition *dec, member_id member, MemberType type);

arc_id markerToParent(const Decomposition *dec, member_id member);
arc_id markerOfParent(const Decomposition *dec, member_id member);
char memberTypeToChar(MemberType type);

/**
 * Returns the largest number of members
 */
member_id largestMemberID(const Decomposition *dec);
int numConnectedComponents(const Decomposition *dec);
arc_id largestArcID(const Decomposition *dec);
node_id largestNodeID(const Decomposition *dec);

/**
 * This function moves the parent information from two members which were merged together to the new member
 */
void updateMemberParentInformation(Decomposition *dec, member_id newMember, member_id toRemove);

/**
 * Create a network decomposition which holds enough space for the given number of matrix rows and columns.
 * Note that numRows and numColumns indicate the the maximum ID of the columns which are supplied is smaller than
 * numRows and numColumns, respectively.
 */
SPQR_ERROR createDecomposition(SPQR * env, Decomposition ** dec, int numRows,int numColumns);
/**
 * Free the network decomposition
 */
void freeDecomposition(Decomposition ** dec);

/**
 * Creates an arc in the given member
 * Arcs are not immediately assigned start and end-nodes, because we do not need to track these for non-rigid nodes
 */
SPQR_ERROR createArc(Decomposition * dec, member_id member, arc_id * pArc, bool reverse);
SPQR_ERROR createRowArc(Decomposition *dec, member_id member, arc_id *pArc, row_idx row, bool reverse);
SPQR_ERROR createColumnArc(Decomposition *dec, member_id member, arc_id *pArc, col_idx col, bool reverse);
void addArcToMemberArcList(Decomposition *dec, arc_id arc, member_id member);
void removeArcFromMemberArcList(Decomposition *dec, arc_id arc, member_id member);
void mergeMemberArcList(Decomposition *dec, member_id toMergeInto, member_id toRemove);
void mergeNodeArcList(Decomposition *dec, node_id toMergeInto, node_id toRemove);



//void moveArcToNewMember(Decomposition *dec, arc_id arc, member_id oldMember, member_id newMember);
//void addArcToNodeArcList(Decomposition *dec, arc_id arc, node_id node, bool nodeIsHead);
//void setArcHeadAndTail(Decomposition *dec, arc_id arc, node_id head, node_id tail);
//void clearArcHeadAndTail(Decomposition *dec, arc_id arc);
//void changeArcHead(Decomposition *dec, arc_id arc,node_id oldHead, node_id newHead);
//void changeArcTail(Decomposition *dec, arc_id arc,node_id oldTail, node_id newTail);
//

SPQR_ERROR createMember(Decomposition *dec, MemberType type, member_id * pMember);
SPQR_ERROR createNode(Decomposition *dec, node_id * pNode);
//int nodeDegree(Decomposition *dec, node_id node);
//
//SPQR_ERROR createStandaloneParallel(Decomposition *dec, col_idx * columns, int num_columns, row_idx row,member_id * pMember);
//SPQR_ERROR createConnectedParallel(Decomposition *dec, col_idx * columns, int num_columns, row_idx row, member_id * pMember);
SPQR_ERROR createStandaloneSeries(Decomposition *dec, const row_idx * rows, const bool * reflected, int numRows,
                                  col_idx col, member_id * pMember);
//SPQR_ERROR createConnectedSeries(Decomposition *dec, row_idx * rows, int numRows, col_idx col, member_id * pMember);
//
//void decreaseNumConnectedComponents(Decomposition *dec, int by);
//
//SPQR_ERROR createChildMarker(Decomposition *dec, member_id member, member_id child, bool isTree, edge_id * pEdge);
//SPQR_ERROR createParentMarker(Decomposition *dec, member_id member, bool isTree, member_id parent, member_id parentMarker, edge_id * pEdge);
//SPQR_ERROR createMarkerPair(Decomposition *dec, member_id parentMember, member_id childMember, bool parentIsTree);
//SPQR_ERROR createMarkerPairWithReferences(Decomposition *dec, member_id parentMember, member_id childMember, bool parentIsTree,
//                                          edge_id * parentToChild, edge_id *childToParent);
//
//void reorderComponent(Decomposition *dec, member_id newRoot);
//
//
//void removeComponents(Decomposition *dec,const row_idx * componentRows, size_t numRows,const col_idx  * componentCols, size_t numCols);
////Debugging
//void decompositionToDot(FILE * stream, const Decomposition *dec, bool useElementNames);
//
///**
// * Note that the callers are supposed to allocate enough memory (The # of rows of the matrix is always sufficient)
// * The resulting found rows are stored in the passed pointer.
// * @return the number of found rows stored in the given array
// */
//int decompositionGetFundamentalCycleRows(Decomposition *dec, col_idx column, row_idx * output);
//
//bool checkCorrectnessColumn(Decomposition * dec, col_idx column, row_idx * column_rows, int num_rows, row_idx * computed_column_storage);
//
bool checkDecompositionMinimal(Decomposition * dec);


#ifdef __cplusplus
}
#endif

#endif //MIPWORKSHOP2024_NETWORKDECOMPOSITION_H
