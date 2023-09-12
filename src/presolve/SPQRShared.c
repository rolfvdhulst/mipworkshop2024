//
// Created by rolf on 18-07-22.
//

#include "mipworkshop2024/presolve/SPQRShared.h"

#ifndef NDEBUG
//Only necessary for overflow check assertions
#include <limits.h>
#endif

SPQR_ERROR spqrCreateEnvironment(SPQR** pSpqr){
    *pSpqr = (SPQR*) malloc(sizeof(SPQR));
    SPQR * env = *pSpqr;
    if(!env){
        return SPQR_ERROR_MEMORY;
    }
    env->output = stdout;
    return SPQR_OKAY;
}
SPQR_ERROR spqrFreeEnvironment(SPQR** pSpqr){
    if(!pSpqr){
        return SPQR_ERROR_MEMORY;
    }
    SPQR * env = *pSpqr;
    if(!env){
        return SPQR_ERROR_MEMORY;
    }

    free(*pSpqr);
    *pSpqr = NULL;
    return SPQR_OKAY;
}


//TODO: implement other malloc-type functions such as reallocarray and calloc throughout the codebase?

SPQR_ERROR implSPQRallocBlockArray(__attribute__((unused)) SPQR * env, void** ptr, size_t size, size_t length){
    assert(env);
    assert(ptr);
    //assert(*ptr == NULL); //TODO: why is this check here, is it necessary?
    assert(!(size > 0 && length > UINT_MAX / size)); //overflow check

    *ptr = malloc(size * length); //TODO check for overflows, possibly use realloc here?

    return *ptr ? SPQR_OKAY : SPQR_ERROR_MEMORY;
}
SPQR_ERROR implSPQRreallocBlockArray(__attribute__((unused)) SPQR* env, void** ptr, size_t size, size_t length)
{
    assert(env);
    assert(ptr);
    assert(!(size > 0 && length > UINT_MAX / size)); //overflow check
    *ptr = realloc(*ptr, size * length);
    //*ptr = reallocarray(*ptr,length,size); //reallocarray can also work and is a bit safer, but is non-standard...
    return *ptr ? SPQR_OKAY : SPQR_ERROR_MEMORY;
}
void implSPQRfreeBlockArray(__attribute__((unused)) SPQR* env, void ** ptr){
    assert(env);
    assert(ptr);
    free(*ptr);
    *ptr = NULL;
}

SPQR_ERROR implSPQRallocBlock(__attribute__((unused)) SPQR * env, void **ptr, size_t size){
    assert(env);
    assert(ptr);
    *ptr = malloc(size);

    return *ptr ? SPQR_OKAY : SPQR_ERROR_MEMORY;
}

void implSPQRfreeBlock(__attribute__((unused)) SPQR * env, void **ptr){
    assert(env);
    assert(ptr);
    assert(*ptr);
    free(*ptr);
    *ptr = NULL;
}
bool rowIsInvalid(row_idx row) {
    return row == INVALID_ROW;
}

bool rowIsValid(row_idx row){
    return !rowIsInvalid(row);
}

bool colIsInvalid(col_idx col) {
    return col == INVALID_COL;
}
bool colIsValid(col_idx col){
    return !colIsInvalid(col);
}
