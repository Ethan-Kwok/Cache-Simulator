#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

unsigned long memRead = 0;
unsigned long memWrite = 0;
unsigned long cache1Hit = 0;
unsigned long cache1Miss = 0;
unsigned long cache2Hit = 0;
unsigned long cache2Miss = 0;
int policy1 = 0; //1 is fifo, 2 is lru. other numbers are null
int policy2 = 0;



struct block{
    unsigned long tag;
    unsigned long address;
    int isValid; //0 is invalid, 1 is valid
    unsigned long fifoVal; //every time a new line is read, all other lines in the set are incremented. the highest fifoVal is kicked first for fifo.
    unsigned long lruVal; //every time a line is read, it is set to 0 and all others are incremented. the highest lruVal is kicked for lru
};



void incrementLru(struct block** cache, unsigned int addressIndex, unsigned int assoc) {
    for (int i = 0; i < assoc; i++) {
        if (cache[addressIndex][i].isValid == 1) {
            cache[addressIndex][i].lruVal++;
            cache[addressIndex][i].fifoVal++;
        }
    }
}


//returns the addressTag of the evicted block
unsigned long addAndEvict(struct block** cache, unsigned int addressIndex, unsigned long addressTag, unsigned int assoc, unsigned long address) {

    incrementLru(cache, addressIndex, assoc);
    //empty slot
    for (int i = 0; i < assoc; i++) {
        if (cache[addressIndex][i].isValid == 0) {
            cache[addressIndex][i].isValid = 1;
            cache[addressIndex][i].tag = addressTag;
            cache[addressIndex][i].address = address;
            cache[addressIndex][i].fifoVal = 0;
            cache[addressIndex][i].lruVal = 0;
            return -1;
        }
    }
    //find which block to evict (fifo vs lru)
    unsigned int greatestVal = 0;
    unsigned int replaceIndex = 0;
    if (policy1 == 1) {
        for (int i = 0; i < assoc; i++) {
            if (cache[addressIndex][i].fifoVal > greatestVal) {
                greatestVal = cache[addressIndex][i].fifoVal;
                replaceIndex = i;
            }
        }

    }
    else if (policy1 == 2) {
        for (int i = 0; i < assoc; i++) {
            if (cache[addressIndex][i].lruVal > greatestVal) {
                greatestVal = cache[addressIndex][i].lruVal;
                replaceIndex = i;
            }
        }
    }
    //temp variable evictTag
    unsigned long evictAddress = cache[addressIndex][replaceIndex].address;
    //add new block
    cache[addressIndex][replaceIndex].tag = addressTag;
    cache[addressIndex][replaceIndex].address = address;
    cache[addressIndex][replaceIndex].fifoVal = 0;
    cache[addressIndex][replaceIndex].lruVal = 0;
    
    return evictAddress;

}



void readCache2(struct block** cache, unsigned int addressIndex, unsigned long addressTag, unsigned int assoc, unsigned long address) {
    incrementLru(cache, addressIndex, assoc);
    //cache hit
    for (int i = 0; i < assoc; i++) {
        if (cache[addressIndex][i].isValid == 1 && cache[addressIndex][i].tag == addressTag) {
            cache[addressIndex][i].lruVal = 0;
            return;
        }
    }
    //cache miss
    //empty slot
    for (int i = 0; i < assoc; i++) {
        if (cache[addressIndex][i].isValid == 0) {
            cache[addressIndex][i].isValid = 1;
            cache[addressIndex][i].tag = addressTag;
            cache[addressIndex][i].address = address;
            cache[addressIndex][i].fifoVal = 0;
            cache[addressIndex][i].lruVal = 0;
            return;
        }
    }
    //find which block to replace (fifo vs lru)
    unsigned int greatestVal = 0;
    unsigned int replaceIndex = 0;
    if (policy2 == 1) {
        for (int i = 0; i < assoc; i++) {
            if (cache[addressIndex][i].fifoVal > greatestVal) {
                greatestVal = cache[addressIndex][i].fifoVal;
                replaceIndex = i;
            }
        }

    }
    else if (policy2 == 2) {
        for (int i = 0; i < assoc; i++) {
            if (cache[addressIndex][i].lruVal > greatestVal) {
                greatestVal = cache[addressIndex][i].lruVal;
                replaceIndex = i;
            }
        }
    }
    //replace block
    cache[addressIndex][replaceIndex].tag = addressTag;
    cache[addressIndex][replaceIndex].address = address;
    cache[addressIndex][replaceIndex].fifoVal = 0;
    cache[addressIndex][replaceIndex].lruVal = 0;
    return;

}



void readCache(struct block** cache1, struct block** cache2, unsigned int addressIndex1, unsigned long addressTag1, unsigned int assoc1, 
               unsigned int addressIndex2, unsigned long addressTag2, unsigned int assoc2, unsigned long address, unsigned int numOffsetBits,
               unsigned int numSetBits2) {
    //cache1 hit
    for (int i = 0; i < assoc1; i++) {
        if (cache1[addressIndex1][i].isValid == 1 && cache1[addressIndex1][i].tag == addressTag1) {
            cache1Hit++;
            incrementLru(cache1, addressIndex1, assoc1);
            cache1[addressIndex1][i].lruVal = 0;
            return;
        }
    }
    cache1Miss++;
    //cache2 hit
    for (int i = 0; i < assoc2; i++) {
        if (cache2[addressIndex2][i].isValid == 1 && cache2[addressIndex2][i].tag == addressTag2) {
            cache2Hit++;
            cache2[addressIndex2][i].isValid = 0;
            //recalculate new tag/index
            unsigned long newAddress = addAndEvict(cache1, addressIndex1, addressTag1, assoc1, address);
            unsigned long newTag = newAddress >> (numOffsetBits + numSetBits2);
            unsigned int newIndex = (newAddress >> numOffsetBits) & ((1 << numSetBits2) - 1);
            readCache2(cache2, newIndex, newTag, assoc2, newAddress);
            return;
        }
    }
    
    //cache1 and cache2 miss
    cache2Miss++;
    memRead++;
    incrementLru(cache1, addressIndex1, assoc1);
    //empty slot
    for (int i = 0; i < assoc1; i++) {
        if (cache1[addressIndex1][i].isValid == 0) {
            cache1[addressIndex1][i].isValid = 1;
            cache1[addressIndex1][i].tag = addressTag1;
            cache1[addressIndex1][i].address = address;
            cache1[addressIndex1][i].fifoVal = 0;
            cache1[addressIndex1][i].lruVal = 0;
            return;
        }
    }
    //find which block to replace (fifo vs lru)
    unsigned int greatestVal = 0;
    unsigned int replaceIndex = 0;
    if (policy1 == 1) {
        for (int i = 0; i < assoc1; i++) {
            if (cache1[addressIndex1][i].fifoVal > greatestVal) {
                greatestVal = cache1[addressIndex1][i].fifoVal;
                replaceIndex = i;
            }
        }

    }
    else if (policy1 == 2) {
        for (int i = 0; i < assoc1; i++) {
            if (cache1[addressIndex1][i].lruVal > greatestVal) {
                greatestVal = cache1[addressIndex1][i].lruVal;
                replaceIndex = i;
            }
        }
    }
    //recalculate and move block to cache2
    unsigned long newAddress = cache1[addressIndex1][replaceIndex].address;
    unsigned long newTag = newAddress >> (numOffsetBits + numSetBits2);
    unsigned int newIndex = (newAddress >> numOffsetBits) & ((1 << numSetBits2) - 1);
    readCache2(cache2, newIndex, newTag, assoc2, newAddress);

    //replace block from cache1
    cache1[addressIndex1][replaceIndex].tag = addressTag1;
    cache1[addressIndex1][replaceIndex].address = address;
    cache1[addressIndex1][replaceIndex].fifoVal = 0;
    cache1[addressIndex1][replaceIndex].lruVal = 0;
    return;

}



int main(int argc, char** argv) {
    
    //setting variables from inputs + error check
    //cache size: should be a power of 2
    unsigned int cacheSize1 = atoi(argv[1]);
    unsigned int cacheSize2 = atoi(argv[5]);
    if (cacheSize1 == 0 || (cacheSize1 & (cacheSize1 - 1)) != 0 || cacheSize2 == 0 || (cacheSize2 & (cacheSize2 - 1)) != 0) {
        printf("Cache size must be a power of 2\n");
        exit(0);
        return 0;
    }
    //associativity
    unsigned int assoc1;
    sscanf(argv[2], "assoc:%d", &assoc1);
    unsigned int assoc2;
    sscanf(argv[6], "assoc:%d", &assoc2);
    if (assoc1 == 0 || assoc2 == 0) {
        printf("Associativity cannot be 0\n");
        exit(0);
        return 0;
    }
    //cache policy
    if (strcmp(argv[3], "fifo") == 0) {
        policy1 = 1;
    }
    else if (strcmp(argv[3], "lru") == 0) {
        policy1 = 2;
    }
    else {
        printf("Invalid cache policy\n");
        exit(0);
        return 0;
    }
    if (strcmp(argv[7], "fifo") == 0) {
        policy2 = 1;
    }
    else if (strcmp(argv[7], "lru") == 0) {
        policy2 = 2;
    }
    else {
        printf("Invalid cache policy\n");
        exit(0);
        return 0;
    }
    //block size
    unsigned int blockSize = atoi(argv[4]);
        if (blockSize == 0 || (blockSize & (blockSize - 1)) != 0) {
        printf("Block size must be a power of 2\n");
        exit(0);
        return 0;
    }
    //# offset bits, # sets, # set bits, # tag bits
    unsigned int numOffsetBits = log2(blockSize);
    unsigned int numSets1 = cacheSize1 / (blockSize * assoc1);
    unsigned int numSetBits1 = log2(numSets1);
    unsigned int numSets2 = cacheSize2 / (blockSize * assoc2);
    unsigned int numSetBits2 = log2(numSets2);
    //read file 
    FILE *fp;
    fp = fopen(argv[8], "r");
    if(fp == NULL) {
        printf("File could not be found\n");
        exit(0);
        return 0;
    }

    //create caches
    struct block** cache1 = malloc(sizeof(struct block) * numSets1);
    struct block** cache2 = malloc(sizeof(struct block) * numSets2);
    for (int i = 0; i < numSets1; i++) {
        cache1[i] = malloc(sizeof(struct block) * assoc1);
        for (int j = 0; j < assoc1; j++) {
            cache1[i][j].isValid = 0;
            cache1[i][j].fifoVal = 0;
            cache1[i][j].lruVal = 0;
        }
    }
    for (int i = 0; i < numSets2; i++) {
        cache2[i] = malloc(sizeof(struct block) * assoc2);
        for (int j = 0; j < assoc2; j++) {
            cache2[i][j].isValid = 0;
            cache2[i][j].fifoVal = 0;
            cache2[i][j].lruVal = 0;
        }
    }

    //read/write from input file
    char accessType;
    unsigned long address;
    while(fscanf(fp, "%c %lx", &accessType, &address) != EOF) {
        unsigned int addressIndex1 = (address >> numOffsetBits) & ((1 << numSetBits1) - 1);
        unsigned long addressTag1 = address >> (numOffsetBits + numSetBits1);
        unsigned int addressIndex2 = (address >> numOffsetBits) & ((1 << numSetBits2) - 1);
        unsigned long addressTag2 = address >> (numOffsetBits + numSetBits2);
        //read
        if (accessType == 'R') {
            readCache(cache1, cache2, addressIndex1, addressTag1, assoc1, addressIndex2, addressTag2, assoc2, 
                      address, numOffsetBits, numSetBits2);
        }
        //write
        if (accessType == 'W') {
            readCache(cache1, cache2, addressIndex1, addressTag1, assoc1, addressIndex2, addressTag2, assoc2, 
                      address, numOffsetBits, numSetBits2);
            memWrite++;
        }
    }

    //print
    printf("memread:%ld\n", memRead);
    printf("memwrite:%ld\n", memWrite);
    printf("l1cachehit:%ld\n", cache1Hit);
    printf("l1cachemiss:%ld\n", cache1Miss);
    printf("l2cachehit:%ld\n", cache2Hit);
    printf("l2cachemiss:%ld\n", cache2Miss);


    //free cache
    for (int i = 0; i < numSets1; i++) {
		free(cache1[i]);
	}
    for (int i = 0; i < numSets2; i++) {
		free(cache2[i]);
	}

	free(cache1);
    free(cache2);

}