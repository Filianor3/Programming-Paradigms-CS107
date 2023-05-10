#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	h->numBuckets = numBuckets;
	h->size = 0;
	h->elemSize = elemSize;
	h->hashfn = hashfn;
	h->comparefn = comparefn;
	h->cleanerFunc = freefn;
	h->buckets = malloc(numBuckets * sizeof(vector*));
	for(int i = 0; i < numBuckets; i++){
		h->buckets[i] = malloc(sizeof(vector*));
		VectorNew(h->buckets[i], elemSize, freefn, 4);
	}
}

void HashSetDispose(hashset *h)
{
	for(int i = 0; i < h->numBuckets; i++){
		VectorDispose(h->buckets[i]);
		free(h->buckets[i]);
	}
	free(h->buckets);
	
}

int HashSetCount(const hashset *h)
{ 
	return h->numBuckets;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert(mapfn != NULL);
	for(int i = 0; i < h->numBuckets; i++){
		VectorMap(h->buckets[i], mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr)
{	
	assert(elemAddr != NULL);
	int bucket = h->hashfn(elemAddr, h->numBuckets);
	assert(bucket >= 0 && bucket < h->numBuckets);
	int index = VectorSearch(h->buckets[bucket], elemAddr, h->comparefn, 0, 0);
	if(index != -1){
		VectorReplace(h->buckets[bucket], elemAddr, index);
	}else{
		VectorAppend(h->buckets[bucket], elemAddr);
		h->size++;
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{ 
	assert(elemAddr != NULL);
	int bucket = h->hashfn(elemAddr, h->numBuckets);
	assert(bucket >= 0 && bucket < h->numBuckets);
	int index = VectorSearch(h->buckets[bucket], elemAddr, h->comparefn, 0, 0);
	if(index != -1){
		return VectorNth(h->buckets[bucket], index);
	}
	return NULL;
}
