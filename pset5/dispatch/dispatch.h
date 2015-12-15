#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "queue.h"
#include "request.h"

// The world struct: the shared view of the Uber world
struct world {
    queue_t* request_queue;
    // You need to add more states to the world here!
    pthread_mutex_t *lock;
    pthread_cond_t *queue_not_empty;
    pthread_cond_t *queue_not_full;
    int is_finished;
};

typedef struct world world_t;

// init_world: initialize the Uber world
int init_world(world_t* state);

// dispatcher_thread: implements the dispatcher
// the dispatcher takes requests for rides from STDIN and ensure them are
// properly enqueued in the request queue
void* dispatcher_thread(void* arg);

// dispatch: puts a request into the request queue, only to be called by
// dispatcher_thread
// should go to sleep if the request queue is full and to be waken up again
// as soon as there are empty slots in the queue
void dispatch(world_t* state, void* req);

// driver_thread: this implements the driver
// drivers respond to requests by taking jobs from the queue, go to sleep if
// queue is empty, and to be waken up by the dispatcher when new jobs are
// available
void* driver_thread(void* arg);
