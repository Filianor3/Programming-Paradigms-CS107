#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{   
    v->elemSize = elemSize;
    assert(v->elemSize > 0);
    v->logLength = 0;
    if(initialAllocation != 0) v->allocLength = initialAllocation;
    else v->allocLength = 4; 
    v->initLen = v->allocLength;
    v->elems = malloc(v->allocLength * elemSize);
    v->cleanerFunc = freeFn;
    assert(v->elems != NULL);
}

void VectorDispose(vector *v)
{
    if(v->cleanerFunc != NULL){
        for(int i = 0; i < v->logLength; i++){
            v->cleanerFunc((char*)v->elems + i * v->elemSize);
        }
    }
    free(v->elems);
}

int VectorLength(const vector *v)
{ 
    return v->logLength;
}

void *VectorNth(const vector *v, int position)
{ 
    assert(position >= 0 && position < v->logLength);
    return (char*)v->elems + position * v->elemSize;
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position < v->logLength);
    void* destination = (char*)v->elems + position * v->elemSize;
    if(v->cleanerFunc != NULL){
        v->cleanerFunc(destination);
    }
    memcpy(destination, elemAddr, v->elemSize);
}

static void vectorGrow(vector *v){
    //v->allocLength += v->initLen;
    v->allocLength *= 2;
    v->elems = realloc(v->elems, v->allocLength * v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0);
    if(v->logLength == v->allocLength){
        vectorGrow(v);
    }
    if(position == v->logLength){
        VectorAppend(v, elemAddr);
        return;
    }
    void* destination = (char*)v->elems + ((position + 1) * v->elemSize);
    void* source = (char*)v->elems + (position) * v->elemSize;
    memmove(destination, source, v->elemSize * (v->logLength - position));
    memcpy(source, elemAddr, v->elemSize);
    v->logLength++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
    if(v->logLength == v->allocLength){
        vectorGrow(v);
    }
    void* target = (char*)v->elems + v->logLength * v->elemSize;
    memcpy(target, elemAddr, v->elemSize);
    v->logLength++;
}

void VectorDelete(vector *v, int position)
{
    assert(position >= 0 && position < v->logLength);
    if(position == v->logLength - 1){
        v->logLength--;
        if(v->cleanerFunc != NULL){
            v->cleanerFunc((char*)v->elems + position * v->elemSize);        
        }
        return;
    }else{
        if(v->cleanerFunc != NULL){
            v->cleanerFunc((char*)v->elems + position * v->elemSize);
        } 
        void* destination = (char*)v->elems + position * v->elemSize;
        memmove(destination, (char*)v->elems + (position + 1)* v->elemSize, v->elemSize * (v->logLength - position - 1));
        v->logLength--;
    }
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
    assert(compare != NULL);
    void* start = v->elems;
    qsort(start, v->logLength, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{ 
    assert(mapFn != NULL);
    for(int i = 0; i < v->logLength; i++){
        mapFn((char*)v->elems + i * v->elemSize, auxData);
    }

}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    assert(searchFn != NULL && key != NULL && startIndex >= 0 && startIndex <= v->logLength);
    void* ourElem = NULL;
    if(isSorted){
        ourElem = bsearch(key, v->elems, v->logLength, v->elemSize, searchFn);
    }else{
        for(int i = startIndex; i < v->logLength; i++){
            if(searchFn(key, v->elems + i * v->elemSize) == 0){
                return i;
            }
        }
    }
    if(ourElem == NULL) return kNotFound;
    return ((char*)ourElem - (char*)v->elems) / v->elemSize;
} 
