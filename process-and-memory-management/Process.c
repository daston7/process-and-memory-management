#include <stdio.h>
#include <stdlib.h>

#include "Process.h"

const char* processStateNames[] = {
    "NEW",    // Corresponds to NEW
    "READY",  // Corresponds to READY
    "RUNNING",// Corresponds to RUNNING
    "FINISHED"// Corresponds to FINISHED
};


// Print the details in a process
void printProcessDetails(Process *process) {
    printf("Process Name: %s, Arrival Time: %d, Service Time: %d, Remaining Time: %d, Memory Requirement: %d, State: %s\n",
           process->name, process->arrivalTime, process->serviceTime, process->remainingTime, process->memoryRequirement, processStateNames[process->state]);
}

// Print all of the memory frames
void printMemoryFrames(const Process *process) {

    if (process->numFramesAllocated > 0 && process->frameAllocations != NULL) {
        printf("mem-frames=[");
        int printing = process->numFramesAllocated;
        for (int i = 0; i < printing; i++) {
            if (process->frameAllocations[i] == -1) {
                printing++;
                continue;
            }
            printf("%d", process->frameAllocations[i]);
            if (i < printing - 1) {
            printf(",");
            }
   
            
        }
        printf("]\n");
    } else {
        printf("Mem-frames %s: None\n", process->name);
    }
}