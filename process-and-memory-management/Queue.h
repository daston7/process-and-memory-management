#ifndef QUEUE_H
#define QUEUE_H


#include "Process.h"


typedef struct Node {
    Process* data;  // Change to Process* to hold pointers to Process structures
    struct Node* next; // Pointing to the next node
} Node;



typedef struct {
    Node* front; // Pointing to the front of the queue
    Node* rear; // Pointing to the rear of the queue
    int count;  // Add count to track the number of items in the queue
} Queue;

// Updated to take Process* as parameter
Node* createNode(Process* data);  
void freeNode(Node* node);
Queue* createQueue();
void enqueue(Queue* queue, Process* process);
Process* dequeue(Queue* queue);
void freeQueue(Queue* queue);
int isQueueEmpty(Queue* queue);
Process* peek(Queue* queue);
void printQueueContents(Queue* queue);

#endif
