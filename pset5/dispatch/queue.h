// queue.h: super simple queue used as a dispatcher queue
#pragma once

// DO NOT modify this number
#define QUEUE_BUF_SIZE 15

// MAX_QUEUE_SIZE <= QUEUE_BUF_SIZE shall always hold
#define MAX_QUEUE_SIZE 5

struct queue61 {
    unsigned int head;
    unsigned int tail;
    void *array[QUEUE_BUF_SIZE + 1];
};

typedef struct queue61 queue_t;

// Queue interface, not thread-safe!
void queue_init(queue_t* q);
// Pushing to a full queue is undefined behavior
void push_back(queue_t* q, void* payload);
// Poping off an empty queue is undefined behavior
void* pop_front(queue_t* q);
// Returns the current size of the queue
unsigned int size(queue_t* q);
// Returns 1 if queue is empty, otherwise returns 0
int empty(queue_t* q);
