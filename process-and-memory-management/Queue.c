#include <stdlib.h>
#include <stdio.h>
#include "Queue.h"
#include "Process.h"

Node* createNode(Process* data) {
    Node* newNode = (Node*) malloc(sizeof(Node));
    if (newNode == NULL) {
        // Allocation failed
        return NULL; 
    }
    // Store the pointer to Process
    newNode->data = data;  
    newNode->next = NULL;
    return newNode;
}

// Free a Node
void freeNode(Node* node) {

    free(node);
}

// Create a new empty Queue
Queue* createQueue() {
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    if (queue == NULL) {
        // Allocation failed
        return NULL; 
    }
    queue->front = NULL;
    queue->rear = NULL;
    queue->count = 0;
    return queue;
}

// Enqueue a new process
void enqueue(Queue* queue, Process* process) {
    Node* newNode = createNode(process);
    if (!newNode) {
        return;
    }
    if (!queue->rear) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    queue->count++;  
}

// Dequeue a process
Process* dequeue(Queue* queue) {
    if (isQueueEmpty(queue)) {
        return NULL;
    }
    Node* temp = queue->front;
    Process* process = temp->data;
    queue->front = temp->next;
    if (!queue->front) {
        queue->rear = NULL;
    }
    freeNode(temp);
    queue->count--;  
    return process;
}

// Free the queue
void freeQueue(Queue* queue) {
    Node* current = queue->front;
    Node* next;
    while (current != NULL) {
        next = current->next;
        free(current->data->frameAllocations);
        free(current->data);
        freeNode(current);
        current = next;
    }
    free(queue);
}

// Check if the queue is empty
int isQueueEmpty(Queue* queue) {
    return queue->front == NULL;  
}

Process* peek(Queue* queue) {
    if (isQueueEmpty(queue)) {
        return NULL;
    }
    return queue->front->data;
}



void printQueueContents(Queue* queue) {
    if (isQueueEmpty(queue)) {
        printf("The queue is empty.\n");
        return;
    }

    Node* current = queue->front;
    printf("Queue Contents: \n");
    while (current != NULL) {
        printProcessDetails(current->data);
        current = current->next;
    }
}
