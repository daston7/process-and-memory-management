#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Process.h"
#include "Queue.h"
#include "ContiguousMemory.h"
#include "PagedMemory.h"

// Define an enum for memory strategies
typedef enum {
    INFINITE,
    FIRST_FIT,
    PAGED,
    VIRTUAL
} MemoryStrategy;


// Function declarations
int parseArguments(int argc, char *argv[], char **filename, int *quantum, MemoryStrategy *strategy);
Queue* readProcessesFromFile(char *filename);
void runRoundRobinScheduling(Queue *allProcesses, Queue *queue, int quantum, MemoryStrategy strategy);
void printProcessStats(Process *process, int simulationTime, Queue *queue);
int min(int x, int y);

int main(int argc, char *argv[]) {
    char *filename = NULL;
    int quantum = 0;
    MemoryStrategy strategy;

    if (parseArguments(argc, argv, &filename, &quantum, &strategy) != 0) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    // Store input from file to allProcesses queue
    Queue *allProcesses = readProcessesFromFile(filename);
    if (!allProcesses) {
        fprintf(stderr, "Failed to read processes from file\n");
        return 1;
    }

    Queue *readyQueue = createQueue();
    runRoundRobinScheduling(allProcesses, readyQueue, quantum, strategy);
    freeQueue(readyQueue);
    freeQueue(allProcesses);

    return 0;
}

int parseArguments(int argc, char *argv[], char **filename, int *quantum, MemoryStrategy *strategy) {
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-f") == 0) {
            *filename = argv[i + 1];
        } else if (strcmp(argv[i], "-q") == 0) {
            *quantum = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0) {
            if (strcmp(argv[i + 1], "infinite") == 0) {
                *strategy = INFINITE;
            } else if (strcmp(argv[i + 1], "first-fit") == 0) {
                *strategy = FIRST_FIT;
            } else if (strcmp(argv[i + 1], "paged") == 0) {
                *strategy = PAGED;
            } else if (strcmp(argv[i + 1], "virtual") == 0) {
                *strategy = VIRTUAL;
            } else {
                fprintf(stderr, "Invalid memory strategy\n");
                return -1;
            }
        }
    }
    return (*filename && *quantum > 0) ? 0 : -1;
}

Queue* readProcessesFromFile(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    Queue *queue = createQueue();
    Process *temp;

    while (!feof(file)) {
        temp = (Process *)malloc(sizeof(Process));
        if (fscanf(file, "%d %s %d %d", &temp->arrivalTime, temp->name, &temp->serviceTime, &temp->memoryRequirement) == 4) {
            temp->remainingTime = temp->serviceTime;
            temp->state = NEW;
            temp->memoryAddress = -1;
            temp->isAllocated =false;
            temp->lastUsed = 0;
            temp->numFramesAllocated = 0;
            temp->frameAllocations = NULL; 
            enqueue(queue, temp);
        } else {
            free(temp);
        }
    }
    fclose(file);
    return queue;
}



void runRoundRobinScheduling(Queue *allProcesses, Queue *readyQueue, int quantum, MemoryStrategy strategy) {

    // Handling task 3 and 4
    if (strategy == VIRTUAL || strategy == PAGED) {
        int simulationTime = 0;
        Process *currentProcess = NULL;
        int totalServiceTime = 0;
        int numberOfProcesses = 0;
        double maxTimeOverhead = 0;
        double totTimeOverhead = 0;
        bool continuousRunning = false;
        initializeFrames();


        while (!isQueueEmpty(allProcesses) || !isQueueEmpty(readyQueue) || currentProcess != NULL) {
            // Check for new arrivals and move them to the ready queue or set as current process
            while (!isQueueEmpty(allProcesses) && peek(allProcesses)->arrivalTime <= simulationTime) {
                Process *newProcess = dequeue(allProcesses);

                if (currentProcess) {
                    enqueue(readyQueue, newProcess);
                    continue;
                }

                // If new process is not allocated before, allocate them
                if (!newProcess->isAllocated) {
                    if (strategy == PAGED) {
                        allocatePages(newProcess, simulationTime);
                    } else if (strategy == VIRTUAL) {
                        allocateVirtualPages(newProcess, simulationTime);
                    }
                    
                    newProcess->isAllocated = true;
                    currentProcess = newProcess;
                }
                
            }

            // Fetch the next process to run if there isn't a current process
            if (!currentProcess && !isQueueEmpty(readyQueue)) {
                currentProcess = dequeue(readyQueue);
                
            }

            if(!currentProcess->isAllocated) {

                if (strategy == PAGED) {
                    allocatePages(currentProcess, simulationTime);
                } else if (strategy == VIRTUAL) {
                    allocateVirtualPages(currentProcess, simulationTime);
                }
                currentProcess->isAllocated = true;
            }

            // Execute the current process
            if (currentProcess) {
                int runTime = min(quantum, currentProcess->remainingTime);
                if (!continuousRunning) {
                    printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,", 
                    simulationTime, 
                    currentProcess->name, 
                    currentProcess->remainingTime, 
                    (int)calculateMemoryUsage());  
                    // Call the function and cast the result to int
                    printMemoryFrames(currentProcess);
                    currentProcess->lastUsed = simulationTime;
                } else {
                    currentProcess->lastUsed = simulationTime;
                }
                
                currentProcess->remainingTime -= runTime;
                simulationTime += quantum;  

                // Check for new arrivals and move them to the ready queue or set as current process
                while (!isQueueEmpty(allProcesses) && peek(allProcesses)->arrivalTime <= simulationTime) {
                    Process *newProcess = dequeue(allProcesses);

                    if (currentProcess) {
                
                        enqueue(readyQueue, newProcess);
                        continue;
                    }

                    if (!newProcess->isAllocated) {
                        if (strategy == PAGED) {
                            allocatePages(newProcess, simulationTime);
                        } else if (strategy == VIRTUAL) {
                            allocateVirtualPages(newProcess, simulationTime);
                        }
                        
                        newProcess->isAllocated = true;
                        currentProcess = newProcess;
                    }
                    
                }

                // Check if the process has finished
                if (currentProcess->remainingTime <= 0) {
                    deallocatePages(currentProcess, simulationTime);
                    currentProcess->isAllocated = false;
                    printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", simulationTime, currentProcess->name, readyQueue->count);
                    numberOfProcesses++;
                    currentProcess->completionTime = simulationTime;
                    currentProcess->turnaroundTime = currentProcess->completionTime - currentProcess->arrivalTime;
                    totalServiceTime += currentProcess->turnaroundTime;
                    currentProcess->timeOverhead = currentProcess->turnaroundTime / (double)currentProcess->serviceTime;
                    totTimeOverhead += currentProcess->timeOverhead;

                    if (currentProcess->timeOverhead > maxTimeOverhead) {
                        maxTimeOverhead = currentProcess->timeOverhead;
                    }

                    continuousRunning = false;
                    // Deallocate memory when process finishes
                    free(currentProcess);
                    // Clear current process
                    currentProcess = NULL;  
                  // If process that was running still has remaining time
                } else {
                    if (isQueueEmpty(readyQueue)) {
                        continuousRunning = true;
                    } else {
                        continuousRunning = false;
                        // Re-queue if not finished
                        enqueue(readyQueue, currentProcess);  
                        currentProcess = NULL;
                    }   
                    
                }
               
            } else {
                simulationTime += quantum;  
            }
        }

        // Task 5 implemenation to calculate the statistics
        double averageTurnaroundTime = (double)totalServiceTime / numberOfProcesses;
        double timeOverheadCalculation = (totTimeOverhead / numberOfProcesses) * 100.0;

        int roundedAverageTurnaroundTime = (int)(averageTurnaroundTime + 0.999999);

        double roundedTimeOverhead = (int)(timeOverheadCalculation + 0.5) / 100.0;

        printf("Turnaround time %d\n", roundedAverageTurnaroundTime);
        printf("Time overhead %.2f %.2f\n", maxTimeOverhead, roundedTimeOverhead);
        printf("Makespan %d\n", simulationTime);

      // Logic and implementation for task 1 and 2   
    } else if (strategy == INFINITE || strategy == FIRST_FIT) {

         int simulationTime = 0;
        Process *currentProcess = NULL;
        MemoryManager *memoryManager = NULL;
        const int totalMemory = 2048;
        int memoryUsed = 0;
        

        int totalServiceTime = 0;
        int numberOfProcesses = 0;
        double maxTimeOverhead = 0;
        double totTimeOverhead = 0;

        if (strategy == FIRST_FIT) {
            memoryManager = createContiguousMemory(totalMemory);
        }
        

        while (!isQueueEmpty(allProcesses) || !isQueueEmpty(readyQueue) || currentProcess != NULL) {

            // Check for new arrivals and move them to the ready queue or set as current process
            while (!isQueueEmpty(allProcesses) && peek(allProcesses)->arrivalTime <= simulationTime) {
                
                if (strategy == INFINITE) {
                    Process *newProcess = dequeue(allProcesses);
                    if (isQueueEmpty(readyQueue) && !currentProcess) {
                        currentProcess = newProcess;
                        printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);
                    } else if (isQueueEmpty(readyQueue) && currentProcess && newProcess->arrivalTime != currentProcess->arrivalTime) {
                        enqueue(readyQueue, currentProcess);
                        currentProcess = newProcess;
                        printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);
                    } else {
                        enqueue(readyQueue, newProcess);
                    }
                } else if (strategy == FIRST_FIT) {
                    Process *temp = dequeue(allProcesses);

                    int address = allocateMemory(memoryManager, temp->memoryRequirement);

                    // If the allocation from allProcesses is not successful, just enqueue to the ready queue and continue
                    if (address == -1) {
                        enqueue(readyQueue, temp);
                        // Skip scheduling this process, keep it for later attempt
                        continue; 
                    } 


                    temp->memoryAddress = address;
                    memoryUsed += temp->memoryRequirement;


                    // Run process again if ready queue is empty and there is no process that is running
                    if (isQueueEmpty(readyQueue) && !currentProcess) {
                        currentProcess = temp;
                        printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                        simulationTime, 
                        currentProcess->name, 
                        currentProcess->remainingTime,
                        (memoryUsed * 100 + totalMemory - 1) / totalMemory,
                        currentProcess->memoryAddress);
                    
                    // Ensure that there is only one process that is running
                    } else if (isQueueEmpty(readyQueue) && currentProcess && temp->arrivalTime != currentProcess->arrivalTime) {
                        enqueue(readyQueue, currentProcess);
                        currentProcess = temp;
                        printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                        simulationTime, 
                        currentProcess->name, 
                        currentProcess->remainingTime,
                        (memoryUsed * 100 + totalMemory - 1) / totalMemory,
                        currentProcess->memoryAddress);
                    } else {
                        enqueue(readyQueue, temp);
                    }

                }
            }

        
            // Update the information of the process that is running
            if (currentProcess) {
                int runTime = min(quantum, currentProcess->remainingTime);
                currentProcess->remainingTime -= runTime;
                simulationTime += quantum;

                if (currentProcess->remainingTime > 0) {
                    if (!isQueueEmpty(readyQueue)) {

                        
                        // Check for new arrivals and move them to the ready queue or set as current process
                        while (!isQueueEmpty(allProcesses) && peek(allProcesses)->arrivalTime <= simulationTime) {
                            
                            if (strategy == INFINITE) {
                                Process *newProcess = dequeue(allProcesses);
                                if (isQueueEmpty(readyQueue) && !currentProcess) {
                                    currentProcess = newProcess;
                                    printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);
                                } else if (isQueueEmpty(readyQueue) && currentProcess && newProcess->arrivalTime != currentProcess->arrivalTime) {
                                    enqueue(readyQueue, currentProcess);
                                    currentProcess = newProcess;
                                    printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);
                                } else {
                                    enqueue(readyQueue, newProcess);
                                }
                            } else if (strategy == FIRST_FIT) {

                                Process *temp = dequeue(allProcesses);

                                if (isQueueEmpty(readyQueue) && !currentProcess) {
      
                                    int address = allocateMemory(memoryManager, temp->memoryRequirement);
                                    if (address == -1) {
                                        printf("%d, WAITING, process-name=%s, reason=Memory Allocation Failed\n", simulationTime, temp->name);
                                        // Skip scheduling this process, keep it for later attempt
                                        continue; 
                                    } 

                                    temp->memoryAddress = address;
                                    memoryUsed += temp->memoryRequirement;

                                    currentProcess = temp;
                                    printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                                    simulationTime, 
                                    currentProcess->name, 
                                    currentProcess->remainingTime,
                                    (memoryUsed * 100 + totalMemory - 1) / totalMemory,
                                    currentProcess->memoryAddress);
                                  // Ensure only one process is running
                                } else if (isQueueEmpty(readyQueue) && currentProcess && temp->arrivalTime != currentProcess->arrivalTime) {
                                    
                                    int address = allocateMemory(memoryManager, temp->memoryRequirement);
                                    if (address == -1) {
                                        enqueue(readyQueue, temp);
                                        continue; 
                                    } 

                                    temp->memoryAddress = address;
                                    memoryUsed += temp->memoryRequirement;

                                    enqueue(readyQueue, currentProcess);
                                    currentProcess = temp;
                                    printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                                    simulationTime, 
                                    currentProcess->name, 
                                    currentProcess->remainingTime,
                                    (memoryUsed * 100) / totalMemory,
                                    currentProcess->memoryAddress);
                                } else {
                                    
                                    enqueue(readyQueue, temp);
                        
                                
                                }

                            }
                        }

                        enqueue(readyQueue, currentProcess);
                        // Clear currentProcess to pick the next available process
                        currentProcess = NULL; 
                    }
                } else {

                    // Update for task 5 statistics 
                    numberOfProcesses++;
                    currentProcess->completionTime = simulationTime;
                    currentProcess->turnaroundTime = currentProcess->completionTime - currentProcess->arrivalTime;
                    totalServiceTime += currentProcess->turnaroundTime;
                    currentProcess->timeOverhead = currentProcess->turnaroundTime / currentProcess->serviceTime;
                    totTimeOverhead += currentProcess->timeOverhead;

                    if (currentProcess->timeOverhead > maxTimeOverhead) {
                        maxTimeOverhead = currentProcess->timeOverhead;
                    }

                    if (memoryManager) {
                        deallocateMemory(memoryManager, currentProcess->memoryAddress, currentProcess->memoryRequirement);
                        memoryUsed -= currentProcess->memoryRequirement;
                    }
                     // Check for new arrivals and move them to the ready queue or set as current process
                    while (!isQueueEmpty(allProcesses) && peek(allProcesses)->arrivalTime <= simulationTime) {
                        
                        if (strategy == INFINITE) {
                            Process *newProcess = dequeue(allProcesses);
                            if (isQueueEmpty(readyQueue) && !currentProcess) {
                                currentProcess = newProcess;
                                printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);
                            } else if (isQueueEmpty(readyQueue) && currentProcess && newProcess->arrivalTime != currentProcess->arrivalTime) {
                                enqueue(readyQueue, currentProcess);
                                currentProcess = newProcess;
                                printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);
                            } else {
                                enqueue(readyQueue, newProcess);
                            }
                        } else if (strategy == FIRST_FIT) {

                            Process *temp = dequeue(allProcesses);


                            if (isQueueEmpty(readyQueue) && !currentProcess) {

                                
                                int address = allocateMemory(memoryManager, temp->memoryRequirement);
                                if (address == -1) {
                                    printf("%d, WAITING, process-name=%s, reason=Memory Allocation Failed\n", simulationTime, temp->name);
                                    // Skip scheduling this process, keep it for later attempt
                                    continue; 
                                } 

                            

                                temp->memoryAddress = address;
                                memoryUsed += temp->memoryRequirement;


                                currentProcess = temp;
                                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                                simulationTime, 
                                currentProcess->name, 
                                currentProcess->remainingTime,
                                (memoryUsed * 100 + totalMemory - 1) / totalMemory,
                                currentProcess->memoryAddress);
                            
                            } else if (isQueueEmpty(readyQueue) && currentProcess && temp->arrivalTime != currentProcess->arrivalTime) {
                                
                                int address = allocateMemory(memoryManager, temp->memoryRequirement);
                                // If fail to allocate memory, enqueue to ready queue
                                if (address == -1) {
                                    enqueue(readyQueue, temp);
                                    continue; 
                                } 

                                temp->memoryAddress = address;
                                memoryUsed += temp->memoryRequirement;

                                enqueue(readyQueue, currentProcess);
                                currentProcess = temp;
                                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                                simulationTime, 
                                currentProcess->name, 
                                currentProcess->remainingTime,
                                (memoryUsed * 100) / totalMemory,
                                currentProcess->memoryAddress);
                            } else {   
                                enqueue(readyQueue, temp);  
                            }

                        }
                    }
                    
                    printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", simulationTime, currentProcess->name, readyQueue->count);
                    
                    // Assuming memory management is required
                    free(currentProcess); 
                    currentProcess = NULL;
                }
                
            } 
            
            else {
                simulationTime += quantum;
            }

            // Dequeue to ready queue when a process finishes running
            if (!currentProcess && !isQueueEmpty(readyQueue) && strategy == INFINITE) {
                currentProcess = dequeue(readyQueue);
                printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, currentProcess->name, currentProcess->remainingTime);


            } else if(!currentProcess && !isQueueEmpty(readyQueue) && strategy == FIRST_FIT) {

                // Iterate until a process with allocated is found    
                while(!isQueueEmpty(readyQueue)) {
                    Process *temp = dequeue(readyQueue);
                    if(temp->memoryAddress == -1) {
                        int address = allocateMemory(memoryManager, temp->memoryRequirement);
                        if (address == -1) {
                            enqueue(readyQueue, temp);
                            // Skip scheduling this process, keep it for later attempt
                            continue; 
                        } else {
                            temp->memoryAddress = address;
                            memoryUsed += temp->memoryRequirement;
                            currentProcess = temp;
                            break;
                        }

                        
                    } else {
                        currentProcess = temp;
                        break;
                    }
                }

                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n",
                    simulationTime,
                    currentProcess->name,
                    currentProcess->remainingTime,
                    (memoryUsed * 100 + totalMemory - 1) / totalMemory,
                    currentProcess->memoryAddress);
            }
        }
        // Statistics for task 5
        double averageTurnaroundTime = (double)totalServiceTime / numberOfProcesses;
        double timeOverheadCalculation = (totTimeOverhead / numberOfProcesses) * 100.0;

        int roundedAverageTurnaroundTime = (int)(averageTurnaroundTime + 0.999999);

        double roundedTimeOverhead = (int)(timeOverheadCalculation + 0.5) / 100.0;

        printf("Turnaround time %d\n", roundedAverageTurnaroundTime);
        printf("Time overhead %.2f %.2f\n", maxTimeOverhead, roundedTimeOverhead);
        printf("Makespan %d\n", simulationTime);
    }


    }

// Finding the minimum
int min(int x, int y) {
    return x < y ? x : y;
}

// Print running of a process
void printProcessStats(Process *process, int simulationTime, Queue *queue) {
    printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", simulationTime, process->name, process->remainingTime);
}


