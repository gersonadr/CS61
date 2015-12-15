#include <assert.h>
#include "queue.h"

// queue.c
//
// A very simple but not thread-safe implementation
// of a 'circular' FIFO queue
//
// Read the code and see how it works!

void queue_init(queue_t* q) {
    q->head = q->tail = 0;
}

void push_back(queue_t* q, void *payload) {
    assert(size(q) < MAX_QUEUE_SIZE);
    q->array[q->tail] = payload;
    q->tail = (q->tail + 1) % (QUEUE_BUF_SIZE + 1);
}

void* pop_front(queue_t* q) {
    assert(size(q) != 0);
    assert(size(q) <= MAX_QUEUE_SIZE); 
    void* popped = q->array[q->head];
    q->head = (q->head + 1) % (QUEUE_BUF_SIZE + 1);
    return popped;
}

unsigned int size(queue_t* q) {
    unsigned int sz = (q->tail - q->head) % (QUEUE_BUF_SIZE + 1);
    assert(sz <= MAX_QUEUE_SIZE);
    return sz;
}

int empty(queue_t* q) {
    return (q->head == q->tail)? 1 : 0;
}
