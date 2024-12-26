#include "PagedMemory.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// Assume Frame is defined in PagedMemory.h
Frame frames[TOTAL_FRAMES];  

void initializeFrames() {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        frames[i].frame_number = i;
        frames[i].process = NULL;
        frames[i].page_number = -1;
    }
}

int calculateMemoryUsage() {
    int occupiedFrames = 0;

    // Iterate through all frames and count those that are occupied
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        // Assuming that a non-null process pointer means the frame is occupied
        if (frames[i].process != NULL) {  
            occupiedFrames++;
        }
    }

    // Calculate percentage of used frames
    int usagePercentage = (occupiedFrames * 100 + TOTAL_FRAMES - 1) / TOTAL_FRAMES;
    return usagePercentage;
}

// Try to allocate pages and return 0 if successful, -1 otherwise
// Updated to track allocated frames directly within the process structure
int allocatePages(Process *process, int simulationTime) {
    int pages_needed = (process->memoryRequirement + PAGE_SIZE - 1) / PAGE_SIZE;

    if (process->frameAllocations == NULL) {
        // Ensure memory for frame allocation tracking
        process->frameAllocations = malloc(pages_needed * sizeof(int));  
    }

    int *evictedFrames = (int *)(malloc(sizeof(int) * TOTAL_FRAMES));
    for (int i = 0; i < TOTAL_FRAMES; i++) evictedFrames[i] = 0;

    int free_frames = findFreeFrames();
    while (free_frames < pages_needed) {
        int *evictedFramesProcess = swapOutLeastRecentlyUsed(process, pages_needed - free_frames, simulationTime);
        for (int i = 0; i < TOTAL_FRAMES; i++) evictedFrames[i] |= evictedFramesProcess[i];
        // Update count after attempting to free frames
        free_frames = findFreeFrames();  
        free(evictedFramesProcess);
    }

    int count = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) count += evictedFrames[i];

    if (count > 0) {
        int output_count = 0;
        printf("%d,EVICTED,evicted-frames=[", simulationTime);
        for (int i = 0; i < TOTAL_FRAMES; i++) {
            if (evictedFrames[i]) {
                printf("%d", i);
                if (output_count < count - 1) {
                    printf(",");
                }
                output_count++;
            }
        }
        printf("]\n");
    }

    free(evictedFrames);

    int allocated_pages = 0;
    for (int i = 0; i < TOTAL_FRAMES && allocated_pages < pages_needed; i++) {
        if (frames[i].process == NULL) {
            frames[i].process = process;
            frames[i].page_number = allocated_pages;
            // Store the frame index
            process->frameAllocations[allocated_pages] = i;  
            allocated_pages++;
        }
    }

    // Store number of frames actually allocated
    process->numFramesAllocated = allocated_pages;  
    return allocated_pages == pages_needed ? 0 : -1;
}

// Evict pages of the least recently used process to make room for new pages
// Updated to print evicted frame indices
int *swapOutLeastRecentlyUsed(Process *currentProcess, int neededFrames, int simulationTime) {
    Process *least_recently_used = NULL;
    // Temporary storage for evicted frames
    int *evictedFrames = malloc(TOTAL_FRAMES * sizeof(int));  
    for (int i = 0; i < TOTAL_FRAMES; i++) evictedFrames[i] = 0;

    // Identify the least recently used process
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (frames[i].process && frames[i].process != currentProcess &&
            (!least_recently_used || frames[i].process->lastUsed < least_recently_used->lastUsed)) {
            least_recently_used = frames[i].process;
        }
    }

    // Evict all pages of the identified process
    if (least_recently_used) {
        for (int i = 0; i < TOTAL_FRAMES; i++) {
            if (frames[i].process == least_recently_used) {
                frames[i].process = NULL;
                frames[i].page_number = -1;
                evictedFrames[i] = 1;
            }
        }
        free(least_recently_used->frameAllocations);
        least_recently_used->frameAllocations = NULL;
    }

    least_recently_used->isAllocated = false;

    return evictedFrames;
}

void deallocatePages(Process *process, int simulationTime) {
    // Array to store evicted frame indices
    int *evictedFrames = malloc(TOTAL_FRAMES * sizeof(int));  
    if (!evictedFrames) {
        perror("Failed to allocate memory for evictedFrames");
        return;
    }
    // Counter for the number of evicted frames
    int count = 0;  

    // Iterate over all frames and deallocate those used by the process
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (frames[i].process == process) {
            frames[i].process = NULL;
            frames[i].page_number = -1;
            // Store the frame index that is being evicted
            evictedFrames[count] = i;  
            count++;
        }
    }

    // Print the evicted frames
    if (count > 0) {
        printf("%d,EVICTED,evicted-frames=[", simulationTime);
        for (int i = 0; i < count; i++) {
            printf("%d", evictedFrames[i]);
            if (i < count - 1) {
                printf(",");
            }
        }
        printf("]\n");
    } else {
        printf("No frames were evicted for Process %s\n", process->name);
    }

    free(process->frameAllocations);
    // Free the memory allocated for tracking evicted frames
    free(evictedFrames);  
}



int allocateVirtualPages(Process *process, int simulationTime) {
    int total_pages_needed = (process->memoryRequirement + PAGE_SIZE - 1) / PAGE_SIZE;
    int min_required_pages = total_pages_needed < 4 ? total_pages_needed : 4;

    if (process->frameAllocations == NULL) {
        process->frameAllocations = (int *)malloc(total_pages_needed * sizeof(int));
        // Memory allocation failed
        if (!process->frameAllocations) return -1;  
        for (int i = 0; i < total_pages_needed; i++) {
            process->frameAllocations[i] = -1;
        }
    }

    int free_frames = findFreeFrames();

    int *evicted_frames = (int *)(malloc(sizeof(int) * TOTAL_FRAMES));
    for (int i = 0; i < TOTAL_FRAMES; i++) evicted_frames[i] = 0;
    int pages_to_allocate = total_pages_needed - process->numFramesAllocated;
    while (free_frames < pages_to_allocate && free_frames < 4) {

        int frames_to_evict = min_required_pages - (process->numFramesAllocated + free_frames);
        int *swapOutFrameProcess = swapOutFrames(process, frames_to_evict, simulationTime);
        for (int i = 0; i < TOTAL_FRAMES; i++) evicted_frames[i] |= swapOutFrameProcess[i];
        free(swapOutFrameProcess);


        free_frames = findFreeFrames();  

        
    }
    int count = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) count += evicted_frames[i];
    if (count > 0) {
        printf("%d,EVICTED,evicted-frames=[", simulationTime);
        int count_output = 0;
        for (int i = 0; i < TOTAL_FRAMES; i++) {
            if (evicted_frames[i]) {
                printf("%d", i);

                if (count_output < count - 1) printf(",");
                count_output++;
            }
        }
        printf("]\n");
    }

    // Allocate as many pages as possible, but at least min_required_pages
    for (int i = 0, frame_index = 0; frame_index < TOTAL_FRAMES && i < pages_to_allocate; frame_index++) {
        if (frames[frame_index].process == NULL) {
            frames[frame_index].process = process;
            frames[frame_index].page_number = process->numFramesAllocated;
            process->frameAllocations[process->numFramesAllocated] = frame_index;
            process->numFramesAllocated++;
            i++;
        }
    }

    free(evicted_frames);
    return process->numFramesAllocated >= min_required_pages ? 0 : -1;  
}

int findFreeFrames() {
    int count = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (frames[i].process == NULL) {
            count++;
        }
    }
    return count;
}

void printSortedFrames(Frame **frames, int count) {
    printf("Sorted Frames Debug Information:\n");
    for (int i = 0; i < count; i++) {
        if (frames[i]->process != NULL) {
            printf("Frame Number: %d, Process ID: %s, Page Number: %d\n",
                   frames[i]->frame_number,
                   frames[i]->process->name,
                   frames[i]->page_number);
        } else {
            printf("Frame Number: %d is free\n", frames[i]->frame_number);
        }
    }
}

// Allocate virtual pages
int *swapOutFrames(Process *currentProcess, int neededFrames, int simulationTime) {
    Process *least_recently_used = findLeastRecentlyUsedProcess(currentProcess);

    int *evictedFrames = (int *)(malloc(sizeof(int) * TOTAL_FRAMES));
    for (int i = 0; i < TOTAL_FRAMES; i++) evictedFrames[i] = 0;

    // No process to evict frames from
    if (!least_recently_used) return 0;  

    Frame *sortedFrames[TOTAL_FRAMES];
    int count = collectFrames(least_recently_used, sortedFrames);  

    if (count == 0) return evictedFrames;  

    if (count <= neededFrames) {
        // Evict all pages in the least_recently_used
        for (int i = 0; i < count; i++) {
            sortedFrames[i]->process = NULL;
            sortedFrames[i]->page_number = -1;
            evictedFrames[sortedFrames[i]->frame_number] = 1;  

            int n = least_recently_used->numFramesAllocated;
            for (int j = 0; j < n; j++) {
                if (least_recently_used->frameAllocations[j] == -1) {
                    n++;
                    continue;
                }

                if (least_recently_used->frameAllocations[j] == sortedFrames[i]->frame_number) {
                    least_recently_used->frameAllocations[j] = -1;
                    least_recently_used->numFramesAllocated--;
                }
            }
        }

        least_recently_used->isAllocated = false;


    } else if (count > neededFrames) {
        // Evict partial pages in the least_recently_used: `neededFrames`
        for (int i = 0; i < neededFrames; i++) {
            Frame *frame = sortedFrames[i];

            frame->process = NULL;
            frame->page_number = -1;

            evictedFrames[frame->frame_number] = 1;  // Save

      

            // evict frame from least_recently_used->frameAllocations;
            // Find index of frame
            int n = least_recently_used->numFramesAllocated;
            for (int j = 0; j < n; j++) {
                if (least_recently_used->frameAllocations[j] == -1) {
                    n++;
                    continue;
                }

                if (least_recently_used->frameAllocations[j] == sortedFrames[i]->frame_number) {
                    least_recently_used->frameAllocations[j] = -1;
                    least_recently_used->numFramesAllocated--;
                }
            }
        }
    }


    return evictedFrames;
}

// Finds the least recently used process among those allocated in the memory frames,
// excluding the current process.
Process *findLeastRecentlyUsedProcess(Process *currentProcess) {
    Process *least_recently_used = NULL;
    int oldest_time = INT_MAX;

    // Traverse all frames to find the least recently used process
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (frames[i].process != NULL && frames[i].process != currentProcess) {
            if (frames[i].process->lastUsed < oldest_time) {
                oldest_time = frames[i].process->lastUsed;
                least_recently_used = frames[i].process;
            }
        }
    }

    return least_recently_used;
}

int collectFrames(Process *process, Frame **sortedFrames) {
    int index = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (frames[i].process == process) {
            sortedFrames[index++] = &frames[i];
        }
    }
    return index;
}


void evictFrames(Frame **frames, int count, int simulationTime) {
    printf("%d,EVICTED,evicted-frames=[", simulationTime);
    for (int i = 0; i < count; i++) {
        if (frames[i]->process != NULL) {
            printf("%d", frames[i]->frame_number);
            if (i < count - 1) {
                printf(",");
            }

            // Reference the process that is losing a frame
            Process *proc = frames[i]->process;

            // Find the index in the frameAllocations array and set it to -1
            for (int j = 0; j < proc->numFramesAllocated; j++) {
                if (proc->frameAllocations[j] == frames[i]->frame_number) {
                    proc->frameAllocations[j] = -1;  // Mark as unallocated
                    break;
                }
            }

            // Decrement the allocated frame count
            proc->numFramesAllocated--;

            // Clear the frame's allocation
            frames[i]->process = NULL;
            frames[i]->page_number = -1;
        }
    }
    printf("]\n");

    // Compact the frameAllocations array to remove any `-1` entries
    for (int i = 0; i < count; i++) {
        Process *proc = frames[i]->process;
        if (proc) {
            int shift = 0;
            for (int j = 0; j < proc->numFramesAllocated + shift; j++) {
                if (proc->frameAllocations[j] == -1) {
                    shift++;
                }
                proc->frameAllocations[j] = proc->frameAllocations[j + shift];
            }
        }
    }
}

// Helper function for sorting frames by frame number
int frameCompare(const void *a, const void *b) {
    Frame *frameA = *(Frame **)a;
    Frame *frameB = *(Frame **)b;
    return frameA->frame_number - frameB->frame_number;
}

