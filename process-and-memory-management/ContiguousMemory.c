#include "ContiguousMemory.h"
#include <stdlib.h>


MemoryManager* createContiguousMemory(int totalMemory) {
    MemoryManager *manager = (MemoryManager *)malloc(sizeof(MemoryManager));
    if (!manager) return NULL;
    
    manager->totalMemory = totalMemory;
    manager->head = (MemoryHole *)malloc(sizeof(MemoryHole));
    if (!manager->head) {
        free(manager);
        return NULL;
    }

    manager->head->start = 0;
    manager->head->size = totalMemory;
    manager->head->prev = NULL;
    manager->head->next = NULL;
    
    return manager;
}

int allocateMemory(MemoryManager *manager, int size) {
    MemoryHole *current = manager->head;
    while (current != NULL) {
        if (current->size >= size) {
            int allocatedAddress = current->start;
            current->start += size;
            current->size -= size;
            
            if (current->size == 0) {
                // Remove the hole if it's completely used
                if (current->prev) {
                    current->prev->next = current->next;
                }
                if (current->next) {
                    current->next->prev = current->prev;
                }
                if (current == manager->head) {
                    manager->head = current->next;
                }
                free(current);
                current = NULL;
            }

            return allocatedAddress;
        }
        current = current->next;
    }
    // Return -1 if no sufficient hole is found
    return -1;
}

void deallocateMemory(MemoryManager *manager, int start, int size) {
    MemoryHole *newHole = (MemoryHole *)malloc(sizeof(MemoryHole));
     // Handle allocation failure gracefully.
    if (!newHole) return; 

    newHole->start = start;
    newHole->size = size;
    newHole->prev = NULL;
    newHole->next = NULL;

    if (!manager->head) {
        manager->head = newHole;
    } else {
        MemoryHole *current = manager->head;
        while (current && current->start < start) {
            current = current->next;
        }

        // Insert new hole before current
        if (current) {
            newHole->next = current;
            newHole->prev = current->prev;
            if (current->prev) {
                current->prev->next = newHole;
            } else {
                manager->head = newHole;
            }
            current->prev = newHole;
        } else {
            // Insert at the end
            current = manager->head;
            while (current->next) {
                current = current->next;
            }
            current->next = newHole;
            newHole->prev = current;
        }
    }
    
    // Merge only around the new hole.
    mergeHoles(manager, newHole);  
}


void mergeHoles(MemoryManager *manager, MemoryHole *startHole) {
    // Start merging from the newly added hole or the previous hole to account for both directions.
    MemoryHole *current = startHole->prev ? startHole->prev : startHole;
    while (current && current->next) {
        if (current->start + current->size == current->next->start) {
            current->size += current->next->size;
            MemoryHole *toDelete = current->next;
            current->next = toDelete->next;
            if (toDelete->next) {
                toDelete->next->prev = current;
            }
            free(toDelete);
        } else {
            current = current->next;
        }
    }
}

