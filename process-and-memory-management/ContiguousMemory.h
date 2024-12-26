#ifndef CONTIGUOUS_MEMORY_H
#define CONTIGUOUS_MEMORY_H

typedef struct MemoryHole {
    int start; // Start of MemoryHole
    int size; // Size of MemoryHole
    struct MemoryHole *prev; // Pointing to the previous memoryhole
    struct MemoryHole *next; // Pointing to the next memoryhole
} MemoryHole;

typedef struct {
    MemoryHole *head; // Pointing to the head of the memory manager
    int totalMemory; // Total memory of the memory manager
} MemoryManager;

MemoryManager* createContiguousMemory(int totalMemory);
int allocateMemory(MemoryManager *manager, int size);
void deallocateMemory(MemoryManager *manager, int start, int size);
void mergeHoles(MemoryManager *manager, MemoryHole *starthole);

#endif 
