//
// Created by rolf on 01-07-22.
//

#ifndef NETWORKDETECTION_SPQRSHARED_H
#define NETWORKDETECTION_SPQRSHARED_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h> //defines bool when c++ is not defined
#include <limits.h>

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

SPQR_ERROR spqrCreateEnvironment(SPQR** pSpqr);
SPQR_ERROR spqrFreeEnvironment(SPQR** pSpqr);

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

typedef size_t matrix_size;
typedef matrix_size row_idx;
typedef matrix_size col_idx;


#define INVALID_IDX ULLONG_MAX
#define INVALID_ROW INVALID_IDX
#define INVALID_COL INVALID_IDX

bool rowIsInvalid(row_idx row);

bool rowIsValid(row_idx row);

bool colIsInvalid(col_idx col);
bool colIsValid(col_idx col);

#ifdef __cplusplus
}
#endif

#endif //NETWORKDETECTION_SPQRSHARED_H

