//
// Created by rolf on 01-07-22.
//

#ifndef NETWORKDETECTION_SPQRDECOMPOSITION_H
#define NETWORKDETECTION_SPQRDECOMPOSITION_H

#include "SPQRShared.h"
#include <limits.h>

#ifdef __cplusplus
extern "C"{
#endif
//TODO: assert that member/nodes/edges are representative in many functions

void swap_ints(int *a, int *b);


typedef int node_id;
#define INVALID_NODE (-1)

bool nodeIsInvalid(node_id node);

bool nodeIsValid(node_id node);

typedef int member_id;
#define INVALID_MEMBER (-1)

bool memberIsInvalid(member_id member);
bool memberIsValid(member_id member);

typedef int edge_id;
#define INVALID_EDGE (INT_MAX)

bool edgeIsInvalid(edge_id edge);
bool edgeIsValid(edge_id edge);


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
    edge_id previous;
    edge_id next;
}EdgeListNode;

typedef struct {
    node_id representativeNode;
    edge_id firstEdge;//first edge of the neighbouring edges
    int numEdges;
} DecompositionNodeData;

typedef struct {

    node_id head;
    node_id tail;
    member_id member;
    member_id childMember;
    EdgeListNode headEdgeListNode;
    EdgeListNode tailEdgeListNode;
    EdgeListNode edgeListNode; //Linked-list node of the array of edges of the member which this edge is in

    spqr_element element;
} DecompositionEdgeData;

typedef enum {
    RIGID = 0, //Also known as triconnected components
    PARALLEL = 1,//Also known as a 'bond'
    SERIES = 2, //Also known as 'polygon' or 'cycle'
	LOOP = 3,
    UNASSIGNED = 4 // To indicate that the member has been merged/is not representative; this is just there to catch errors.
} MemberType;

typedef struct {
    member_id representativeMember;
    MemberType type;

    member_id parentMember;
    edge_id markerToParent;
    edge_id markerOfParent;

    edge_id firstEdge; //First of the members' linked-list edge array
    int num_edges;
} DecompositionMemberData;

typedef struct {
    int numEdges;
    int memEdges;
    DecompositionEdgeData *edges;
    edge_id firstFreeEdge;

    int memMembers;
    int numMembers;
    DecompositionMemberData *members;

    int memNodes;
    int numNodes;
    DecompositionNodeData *nodes;

    int memRows;
    int numRows;
    edge_id * rowEdges;

    int memColumns;
    int numColumns;
    edge_id * columnEdges;

    SPQR * env;

    int numConnectedComponents;
} Decomposition;


void changeLoopToSeries(Decomposition * dec, member_id member);
void changeLoopToParallel(Decomposition * dec, member_id member);
////Section: Mapping functions for rows/columns
bool decompositionHasRow(Decomposition *dec, spqr_row row);
bool decompositionHasCol(Decomposition *dec, spqr_col col);
void setDecompositionColumnEdge(Decomposition *dec, spqr_col col, edge_id edge);
void setDecompositionRowEdge(Decomposition *dec, spqr_row row, edge_id edge);
edge_id getDecompositionColumnEdge(const Decomposition *dec, spqr_col col);
edge_id getDecompositionRowEdge(const Decomposition *dec, spqr_row row);
//// Section: Disjoint union operations for nodes

/**
 * Returns true if and only if the node is a representative node
 * @return
 */
bool nodeIsRepresentative(const Decomposition *dec, node_id node);

/**
 * Finds the representative of the node
 * @return the representative of node
 */
node_id findNode(Decomposition *dec, node_id node);

/**
 * Similar to findNode, except it does not perform any compression
 * @return The representative of node
 */
node_id findNodeNoCompression(const Decomposition *dec, node_id node);

/**
 * Merges the disjoint set datastructure for 2 representative nodes
 * @return The node which is the representative of the two nodes
 */
node_id mergeNodes(Decomposition *dec, node_id first, node_id second);


//// Section: Disjoint union operations for members.
//// Essentially duplicate to the node functions.

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
 * Merges the disjoint set datastructure for 2 representative members
 * @return The member which is the representative of the two members
 */
member_id mergeMembers(Decomposition *dec, member_id first, member_id second);


////Helpers
/**
 * Returns the representative node of the edge tail
 */
node_id findEdgeTail(Decomposition *dec, edge_id edge);



void mergeLoop(Decomposition * dec,member_id loopMember);


/**
 * Returns the representative node of the edge head
 */
node_id findEdgeHead(Decomposition *dec, edge_id edge);

///Similar to the above, but do not do path compression for the nodes
node_id findEdgeTailNoCompression(const Decomposition *dec, edge_id edge);
node_id findEdgeHeadNoCompression(const Decomposition *dec, edge_id edge);

/**
 * @return The representative member to which the edge belongs
 */
member_id findEdgeMember(Decomposition *dec, edge_id edge);
member_id findEdgeMemberNoCompression(const Decomposition *dec, edge_id edge);

/**
 * @The member which is a child of this edge
 */
member_id findEdgeChildMember(Decomposition *dec, edge_id edge);
member_id findEdgeChildMemberNoCompression(const Decomposition *dec, edge_id edge);
/**
 * @return The representative member of the parent of this member
 */
member_id findMemberParent(Decomposition *dec, member_id member);
member_id findMemberParentNoCompression(const Decomposition *dec, member_id member);

MemberType getMemberType(const Decomposition *dec, member_id member);
void updateMemberType(const Decomposition *dec, member_id member, MemberType type);

edge_id markerToParent(const Decomposition *dec, member_id member);
edge_id markerOfParent(const Decomposition *dec, member_id member);
char typeToChar(MemberType type);

/**
 * Returns the largest number of members
 */
member_id largestMemberID(const Decomposition *dec);
int numConnectedComponents(const Decomposition *dec);
edge_id largestEdgeID(const Decomposition *dec);
node_id largestNodeID(const Decomposition *dec);

void updateMemberParentInformation(Decomposition *dec, member_id newMember, member_id toRemove);

/**
 * Edge iteration for members
 */
edge_id getFirstMemberEdge(const Decomposition * dec, member_id member);
edge_id getNextMemberEdge(const Decomposition * dec, edge_id edge);
edge_id getPreviousMemberEdge(const Decomposition *dec, edge_id edge);
int getNumMemberEdges(const Decomposition * dec, member_id member);

int getNumNodes(const Decomposition *dec);
int getNumMembers(const Decomposition *dec);
/**
 * Edge iteration for nodes
 */
edge_id getFirstNodeEdge(const Decomposition * dec, node_id node);
edge_id getNextNodeEdge(Decomposition * dec, edge_id edge, node_id node);
edge_id getPreviousNodeEdge(Decomposition * dec, edge_id edge, node_id node);
//Crucial edge functions
bool edgeIsMarker(const Decomposition * dec,edge_id edge);
bool edgeIsTree(const Decomposition *dec,edge_id edge);
spqr_element edgeGetElement(const Decomposition * dec, edge_id edge);

//Decomposition manipulation
SPQR_ERROR createDecomposition(SPQR * env,
                               Decomposition ** dec,
                               int numRows,
                               int numColumns
                               );

void freeDecomposition(Decomposition ** dec);

//Creation
SPQR_ERROR createEdge(Decomposition * dec, member_id member, edge_id * pEdge);
SPQR_ERROR createRowEdge(Decomposition *dec, member_id member, edge_id *pEdge, spqr_row row);
SPQR_ERROR createColumnEdge(Decomposition *dec, member_id member, edge_id *pEdge, spqr_col col);
void addEdgeToMemberEdgeList(Decomposition *dec, edge_id edge, member_id member);
void removeEdgeFromMemberEdgeList(Decomposition *dec, edge_id edge, member_id member);
void moveEdgeToNewMember(Decomposition *dec, edge_id edge, member_id oldMember, member_id newMember);
void mergeMemberEdgeList(Decomposition *dec, member_id toMergeInto, member_id toRemove);

SPQR_ERROR createMember(Decomposition *dec, MemberType type, member_id * pMember);
SPQR_ERROR createNode(Decomposition *dec, node_id * pNode);
void addEdgeToNodeEdgeList(Decomposition *dec, edge_id edge, node_id node, bool nodeIsHead);
void setEdgeHeadAndTail(Decomposition *dec, edge_id edge, node_id head, node_id tail);
void clearEdgeHeadAndTail(Decomposition *dec, edge_id edge);
void changeEdgeHead(Decomposition *dec, edge_id edge,node_id oldHead, node_id newHead);
void changeEdgeTail(Decomposition *dec, edge_id edge,node_id oldTail, node_id newTail);
void flipEdge(Decomposition *dec, edge_id edge);
void mergeNodeEdgeList(Decomposition *dec, node_id toMergeInto, node_id toRemove);
void removeEmptyMember(Decomposition *dec,member_id member);

int nodeDegree(Decomposition *dec, node_id node);

//TODO: fix connected components interface somehow...
SPQR_ERROR createStandaloneParallel(Decomposition *dec, spqr_col * columns, int num_columns, spqr_row row,member_id * pMember);
SPQR_ERROR createConnectedParallel(Decomposition *dec, spqr_col * columns, int num_columns, spqr_row row, member_id * pMember);
SPQR_ERROR createStandaloneSeries(Decomposition *dec, spqr_row * rows, int numRows, spqr_col col, member_id * pMember);
SPQR_ERROR createConnectedSeries(Decomposition *dec, spqr_row * rows, int numRows, spqr_col col, member_id * pMember);

void decreaseNumConnectedComponents(Decomposition *dec, int by);

SPQR_ERROR createChildMarker(Decomposition *dec, member_id member, member_id child, bool isTree, edge_id * pEdge);
SPQR_ERROR createParentMarker(Decomposition *dec, member_id member, bool isTree, member_id parent, member_id parentMarker, edge_id * pEdge);
SPQR_ERROR createMarkerPair(Decomposition *dec, member_id parentMember, member_id childMember, bool parentIsTree);
SPQR_ERROR createMarkerPairWithReferences(Decomposition *dec, member_id parentMember, member_id childMember, bool parentIsTree,
                                          edge_id * parentToChild, edge_id *childToParent);

void reorderComponent(Decomposition *dec, member_id newRoot);


void removeComponents(Decomposition *dec,const spqr_row * componentRows, size_t numRows,const spqr_col  * componentCols, size_t numCols);
//Debugging
void decompositionToDot(FILE * stream, const Decomposition *dec, bool useElementNames);

/**
 * Note that the callers are supposed to allocate enough memory (The # of rows of the matrix is always sufficient)
 * The resulting found rows are stored in the passed pointer.
 * @return the number of found rows stored in the given array
 */
int decompositionGetFundamentalCycleRows(Decomposition *dec, spqr_col column, spqr_row * output);

bool checkCorrectnessColumn(Decomposition * dec, spqr_col column, spqr_row * column_rows, int num_rows, spqr_row * computed_column_storage);

bool checkDecompositionMinimal(Decomposition * dec);


#ifdef __cplusplus
}
#endif

#endif //NETWORKDETECTION_SPQRDECOMPOSITION_H
