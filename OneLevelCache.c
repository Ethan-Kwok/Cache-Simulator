#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

unsigned long memRead = 0;
unsigned long memWrite = 0;
unsigned long cacheHit = 0;
unsigned long cacheMiss = 0;
int policy = 0; //1 is fifo, 2 is lru. other numbers are null



struct block{
    unsigned long tag;
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



void readCache(struct block** cache, unsigned int addressIndex, unsigned long addressTag, unsigned int assoc) {

    //cache hit
    for (int i = 0; i < assoc; i++) {
        if (cache[addressIndex][i].isValid == 1 && cache[addressIndex][i].tag == addressTag) {
            cacheHit++;
            incrementLru(cache, addressIndex, assoc);
            cache[addressIndex][i].lruVal = 0;
            return;
        }
    }
    
    //cache miss
    cacheMiss++;
    memRead++;
    incrementLru(cache, addressIndex, assoc);
    //empty slot
    for (int i = 0; i < assoc; i++) {
        if (cache[addressIndex][i].isValid == 0) {
            cache[addressIndex][i].isValid = 1;
            cache[addressIndex][i].tag = addressTag;
            cache[addressIndex][i].fifoVal = 0;
            cache[addressIndex][i].lruVal = 0;
            return;
        }
    }
    //find which block to replace (fifo vs lru)
    unsigned int greatestVal = 0;
    unsigned int replaceIndex = 0;
    if (policy == 1) {
        for (int i = 0; i < assoc; i++) {
            if (cache[addressIndex][i].fifoVal > greatestVal) {
                greatestVal = cache[addressIndex][i].fifoVal;
                replaceIndex = i;
            }
        }

    }
    else if (policy == 2) {
        for (int i = 0; i < assoc; i++) {
            if (cache[addressIndex][i].lruVal > greatestVal) {
                greatestVal = cache[addressIndex][i].lruVal;
                replaceIndex = i;
            }
        }
    }
    //replace block
    cache[addressIndex][replaceIndex].tag = addressTag;
    cache[addressIndex][replaceIndex].fifoVal = 0;
    cache[addressIndex][replaceIndex].lruVal = 0;
    return;

}



int main(int argc, char** argv) {
    
    //setting variables from inputs + error check
    //cache size: should be a power of 2
    unsigned int cacheSize = atoi(argv[1]);
    if (cacheSize == 0 || (cacheSize & (cacheSize - 1)) != 0) {
        printf("Cache size must be a power of 2\n");
        exit(0);
        return 0;
    }
    //associativity
    unsigned int assoc;
    sscanf(argv[2], "assoc:%d", &assoc);
    if (assoc == 0) {
        printf("Associativity cannot be 0\n");
        exit(0);
        return 0;
    }
    //cache policy
    if (strcmp(argv[3], "fifo") == 0) {
        policy = 1;
    }
    else if (strcmp(argv[3], "lru") == 0) {
        policy = 2;
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
    unsigned int numSets = cacheSize / (blockSize * assoc);
    unsigned int numSetBits = log2(numSets);
    //read file 
    FILE *fp;
    fp = fopen(argv[5], "r");
    if(fp == NULL) {
        printf("File could not be found\n");
        exit(0);
        return 0;
    }

    //create cache
    struct block** cache = malloc(sizeof(struct block) * numSets);
    for (int i = 0; i < numSets; i++) {
        cache[i] = malloc(sizeof(struct block) * assoc);
        for (int j = 0; j < assoc; j++) {
            cache[i][j].isValid = 0;
            cache[i][j].fifoVal = 0;
            cache[i][j].lruVal = 0;
        }
    }

    //read/write from input file
    char accessType;
    unsigned long address;
    while(fscanf(fp, "%c %lx", &accessType, &address) != EOF) {
        unsigned int addressIndex = (address >> numOffsetBits) & ((1 << numSetBits) - 1);
        unsigned long addressTag = address >> (numOffsetBits + numSetBits);
        //read
        if (accessType == 'R') {
            readCache(cache, addressIndex, addressTag, assoc);
        }
        //write
        if (accessType == 'W') {
            readCache(cache, addressIndex, addressTag, assoc);
            memWrite++;
        }
    }

    //print
    printf("memread:%ld\n", memRead);
    printf("memwrite:%ld\n", memWrite);
    printf("cachehit:%ld\n", cacheHit);
    printf("cachemiss:%ld\n", cacheMiss);

    //free cache
    for (int i = 0; i < numSets; i++) {
		free(cache[i]);
	}
	free(cache);

}