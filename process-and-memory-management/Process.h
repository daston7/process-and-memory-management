// Process.h
#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>

// Enum for process state
typedef enum {
    NEW,        // Process has arrived but not yet entered the queue
    READY,      // Process is in the queue ready to run
    RUNNING,    // Process is currently running
    FINISHED    // Process has completed execution
} ProcessState;

// Struct for a process
typedef struct {
    char name[9];          // Process name (up to 8 characters + null terminator)
    int arrivalTime;       // Time when process arrives
    int serviceTime;       // Total required CPU time
    int remainingTime;     // Remaining CPU time needed
    int memoryRequirement; // Memory required in KB
    int memoryAddress; // Memory address of a process
    int completionTime; // Completion time of a process
    double turnaroundTime; // Finish time - arrival time of a process
    double timeOverhead; // Time overhead of a process
    ProcessState state;    // Current state of the process
    int lastUsed;             // Last used time for LRU calculations
    bool isAllocated;         // Check if a process is allocated to memory or not
    int *frameAllocations;      // Array of frame numbers allocated to this process
    int numFramesAllocated;     // Number of frames allocated  
} Process;

void printProcessDetails(Process *process);
void printMemoryFrames(const Process *process);

#endif
