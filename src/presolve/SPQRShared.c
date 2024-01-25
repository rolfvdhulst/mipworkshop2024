#include "mipworkshop2024/presolve/SPQRShared.h"

#ifndef NDEBUG
//Only necessary for overflow check assertions
#include <limits.h>
#endif

SPQR_ERROR SPQRcreateEnvironment(SPQR** pSpqr){
    *pSpqr = (SPQR*) malloc(sizeof(SPQR));
    SPQR * env = *pSpqr;
    if(!env){
        return SPQR_ERROR_MEMORY;
    }
    env->output = stdout;
    return SPQR_OKAY;
}
SPQR_ERROR SPQRfreeEnvironment(SPQR** pSpqr){
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


bool SPQRnodeIsInvalid(spqr_node node) {
    return node < 0;
}

bool SPQRnodeIsValid(spqr_node node) {
    return !SPQRnodeIsInvalid(node);
}
bool SPQRmemberIsInvalid(spqr_member member) {
    return member < 0;
}
bool SPQRmemberIsValid(spqr_member member) {
    return !SPQRmemberIsInvalid(member);
}
bool SPQRedgeIsInvalid(spqr_edge edge) {
    return edge == SPQR_INVALID_EDGE;
}
bool SPQRedgeIsValid(spqr_edge edge){
    return !SPQRedgeIsInvalid(edge);
}
bool SPQRarcIsInvalid(spqr_arc arc) {
    return arc < 0;
}
bool SPQRarcIsValid(spqr_arc arc){
    return !SPQRarcIsInvalid(arc);
}

bool SPQRrowIsInvalid(spqr_row row){
    return row == SPQR_INVALID_ROW;
}
bool SPQRrowIsValid(spqr_row row){
    return !SPQRrowIsInvalid(row);
}
bool SPQRcolIsInvalid(spqr_col col){
    return col == SPQR_INVALID_COL;
}
bool SPQRcolIsValid(spqr_col col){
    return !SPQRcolIsInvalid(col);
}

bool SPQRelementIsRow(spqr_element element){
    return element < 0;
}
bool SPQRelementIsColumn(spqr_element element){
    return !SPQRelementIsRow(element);
}
spqr_row SPQRelementToRow(spqr_element element){
    assert(SPQRelementIsRow(element));
    return (spqr_row) (-element - 1);
}
spqr_element SPQRrowToElement(spqr_row row){
    assert(SPQRrowIsValid(row));
    return (spqr_element) -row-1;
}
spqr_col SPQRelementToColumn(spqr_element element){
    assert(SPQRelementIsColumn(element));
    return (spqr_col) element;
}
spqr_element SPQRcolumnToElement(spqr_col column){
    assert(SPQRcolIsValid(column));
    return (spqr_element) column;
}