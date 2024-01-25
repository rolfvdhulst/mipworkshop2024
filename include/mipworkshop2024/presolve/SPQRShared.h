//
// Created by rolf on 01-07-22.
//

#ifndef NETWORKDETECTION_SPQRSHARED_H
#define NETWORKDETECTION_SPQRSHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h> //defines bool when c++ is not defined
#include <limits.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
    SPQR_OKAY = 0, ///No error
    SPQR_ERROR_MEMORY = 1, ///Error in (re)allocation
    SPQR_ERROR_INPUT = 2, ///Error in input matrix/
} SPQR_ERROR;


#define SPQR_CALL(call)                             \
do                                                  \
{                                                   \
    SPQR_ERROR _spqr_error = call;                  \
    if(_spqr_error){                                \
        switch(_spqr_error){                        \
            case SPQR_ERROR_MEMORY:                 \
            {   printf("Memory allocation failed"); \
                break;}                             \
            case SPQR_ERROR_INPUT:                  \
            {   printf("User input error");         \
                break;}                             \
            default:                                \
                printf("Unrecognized error code "); \
        }                                           \
        printf(" in %s:%d.\n", __FILE__, __LINE__); \
        return _spqr_error;                         \
    }                                               \
} while(false)                                      \




struct SPQR_ENVIRONMENT{
    FILE * output;
};

typedef struct SPQR_ENVIRONMENT SPQR;

SPQR_ERROR SPQRcreateEnvironment(SPQR** pSpqr);
SPQR_ERROR SPQRfreeEnvironment(SPQR** pSpqr);
//TODO: rename these impl() to SPQRimpl

#define SPQRallocBlockArray(spqr, ptr, length) \
    implSPQRallocBlockArray(spqr,(void **) (ptr), sizeof(**(ptr)),length)

#define SPQRreallocBlockArray(spqr, ptr, length) \
    implSPQRreallocBlockArray(spqr,(void **) (ptr), sizeof(**(ptr)),length)

#define SPQRfreeBlockArray(spqr, ptr) \
    implSPQRfreeBlockArray(spqr,(void **) (ptr))

SPQR_ERROR implSPQRallocBlockArray(SPQR * env, void** ptr, size_t size, size_t length);
SPQR_ERROR implSPQRreallocBlockArray(SPQR* env, void** ptr, size_t size, size_t length);
void implSPQRfreeBlockArray(SPQR* env, void ** ptr);


#define SPQRallocBlock(spqr, ptr) \
    implSPQRallocBlock(spqr,(void **) (ptr), sizeof(**(ptr)))

#define SPQRfreeBlock(spqr, ptr) \
    implSPQRfreeBlock(spqr,(void **) (ptr))

SPQR_ERROR implSPQRallocBlock(SPQR * env, void **ptr, size_t size);

void implSPQRfreeBlock(SPQR * env, void **ptr);

///Types which define matrix sizes
///Aliased so that switching is much easier if ever desired

typedef size_t spqr_matrix_size;
typedef spqr_matrix_size spqr_row;
typedef spqr_matrix_size spqr_col;

#define SPQR_INVALID ULLONG_MAX //TODO: this should maybe be ULLONG_MAX or SSIZE_MAX? Check...
#define SPQR_INVALID_ROW SPQR_INVALID
#define SPQR_INVALID_COL SPQR_INVALID

bool SPQRrowIsInvalid(spqr_row row);
bool SPQRrowIsValid(spqr_row row);
bool SPQRcolIsInvalid(spqr_col col);
bool SPQRcolIsValid(spqr_col col);

//Columns 0..x correspond to elements 0..x
//Rows 0..y correspond to elements -1.. -y-1
#define MARKER_ROW_ELEMENT (INT_MIN)
#define MARKER_COLUMN_ELEMENT (INT_MAX)
typedef int spqr_element;

bool SPQRelementIsRow(spqr_element element);
bool SPQRelementIsColumn(spqr_element element);
spqr_row SPQRelementToRow(spqr_element element);
spqr_col SPQRelementToColumn(spqr_element element);

spqr_element SPQRrowToElement(spqr_row row);
spqr_element SPQRcolumnToElement(spqr_col column);

typedef int spqr_node;
#define SPQR_INVALID_NODE (-1)

bool SPQRnodeIsInvalid(spqr_node node);
bool SPQRnodeIsValid(spqr_node node);

typedef int spqr_member;
#define SPQR_INVALID_MEMBER (-1)

bool SPQRmemberIsInvalid(spqr_member member);
bool SPQRmemberIsValid(spqr_member member);

typedef int spqr_edge;
#define SPQR_INVALID_EDGE (INT_MAX)

bool SPQRedgeIsInvalid(spqr_edge edge);
bool SPQRedgeIsValid(spqr_edge edge);

typedef int spqr_arc;
#define SPQR_INVALID_ARC (-1)

bool SPQRarcIsInvalid(spqr_arc arc);
bool SPQRarcIsValid(spqr_arc arc);

typedef enum {
    SPQR_MEMBERTYPE_RIGID = 0, //Also known as triconnected components
    SPQR_MEMBERTYPE_PARALLEL = 1,//Also known as a 'bond'
    SPQR_MEMBERTYPE_SERIES = 2, //Also known as 'polygon' or 'cycle'
    SPQR_MEMBERTYPE_LOOP = 3,
    SPQR_MEMBERTYPE_UNASSIGNED = 4 // To indicate that the member has been merged/is not representative; this is just there to catch errors.
} SPQRMemberType;


#ifdef __cplusplus
}
#endif

#endif //NETWORKDETECTION_SPQRSHARED_H

