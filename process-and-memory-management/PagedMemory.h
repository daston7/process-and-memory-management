#ifndef PAGED_MEMORY_H
#define PAGED_MEMORY_H

#include "Process.h"

// Total frames in memory based on 2048 KB total and 4 KB per frame
#define TOTAL_FRAMES 512 
#define PAGE_SIZE 4 

typedef struct {
    int frame_number; // Frame number
    Process *process; // Process in a frame
    int page_number; // Page number of a frame
} Frame;

void initializeFrames();
int calculateMemoryUsage();
int allocatePages(Process *process, int simulationTime);
void deallocatePages(Process *process, int simulationTime);
int findFreeFrames();
int* swapOutLeastRecentlyUsed(Process *currentProcess, int neededFrames, int simulationTime);
int allocateVirtualPages(Process *process, int simulationTime);
int *swapOutFrames(Process *currentProcess, int neededFrames, int simulationTime);
int frameCompare(const void *a, const void *b);
Process *findLeastRecentlyUsedProcess(Process *currentProcess);
int collectFrames(Process *process, Frame **sortedFrames);
void evictFrames(Frame **frames, int count, int simulationTime);
void printSortedFrames(Frame **frames, int count);

#endif
