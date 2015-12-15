// Simulation of a Uber dispatch mechanism
// This is the only file where you need to write your own code!
// Your solution should not rely on modification to any other files!
// See dispatch.h for descriptions of the following functions
#include <pthread.h>
#include <errno.h>
#include "dispatch.h"
#include "queue.h"

/**
 ** Describe your synchronization strategy here:
 **
 ** Racing conditions: request_queue is shared between N driver_thread and 1 dispatch_thread,
 ** so it must be protected using a mutex. Otherwise, the push_back() and pop_front() operations
 ** could behavior unexpectedly as more than 1 thread inserts / removes requests from queue.
 ** Same applies to "is_finished" indicator: it must be accessed (read/write) from inside
 ** the context of a lock.
 ** 
 ** To push into queue, dispatcher_thread acquires the mutex (gaining exclusive access), 
 ** checks the condition variable "queue_not_full" (which implicitly unlocks the mutex until 
 ** the CV is signaled by another thread), and once signaled by a driver_thread, it will hold
 ** the mutex but it should re-check the queue size to determine if the condition still holds.
 ** The dispatcher_thread checks the request_queue->size (and because it has the mutex at that
 ** point, it's a perfectly safe operation because no one else is inserting / removing from queue)
 ** and if there is some room, inserts the request on queue. Otherwise, it will to go sleep again
 ** waiting for another signal on CV "queue_not_full". Once it has the mutex and queue is not empty,
 ** it will insert to it, signal CV "queue_not_full" for any sleeping driver threads waiting for
 ** requests, and will release the mutex.

 ** The driver_thread works similarly: it needs to acquire the mutex in order to pop requests from
 ** the queue (shared resource), and it must re-check if the condition is still valid once mutex is
 ** acquired. In addition, the driver_thread must check "state->is_finished" as an indicator that
 ** the dispatcher_thread finished processing all request from customers, and that driver_threads
 ** must finish execution. The "is_finished" flag is set at the dispatcher thread (once it has
 ** acquired the mutex to change it without RC).
 ** 
 ** Correctness: because jobs are removed from queue after getting the mutex, the queue is guaranteed
 ** to remove 1 request at each pop_front() operation. The same can be said for inserting objects, and is
 ** also guaranteed to keep its internal state consistent (size, head, etc). This assumption can only
 ** hold if driver_thread and dispatcher_thread access the queue using a mutex.
 **
 ** Also, because the dispatcher_thread updates the "is_finished" flag (line 97) and signals "queue_not_empty"
 ** in between the same lock operation, there is no race condition from drivers checking the is_finished
 ** status just before it gets updated.
 **
 ** It's safe to say deadlocks can't occur because driver_thread only locks 1 shared resource (queue), and
 ** the direction of dependency does not change (no risk of circular dependency). Starvation is also not a problem
 ** because the driver_thread is guaranteed to check-and-retrieve requests with exclusive access to the queue and
 ** will either retrieve a request or go to sleep. The CV queue_not_empty guarantees the drivers will sleep until
 ** there is work to be done (no risk of busy waiting).
 ** 
 **/
int init_world(world_t* state) {
    // Your code here!
    (void)state;    

    state->request_queue = malloc(sizeof (queue_t));
    state->lock = malloc(sizeof (pthread_mutex_t));
    state->queue_not_empty = malloc(sizeof (pthread_cond_t));
    state->queue_not_full = malloc(sizeof (pthread_cond_t));
    
    queue_init(state->request_queue);
    assert(state->is_finished == 0);
    assert(size(state->request_queue) == 0);

    pthread_mutex_init(state->lock, NULL);
    pthread_cond_init(state->queue_not_empty, NULL);
    pthread_cond_init(state->queue_not_full, NULL);
    
    return 0;
}

void* dispatcher_thread(void* arg) {
    world_t* state = (world_t*)arg;
    char *line = NULL;
    size_t nbytes = 0;

    request_t* req;
    int scanned;
    // DO NOT change the following loop
    while(getline(&line, &nbytes, stdin) != -1) {
        req = (request_t*)malloc(sizeof(request_t));
        // Parse stdin inputs
        scanned = sscanf(line, "%lu %lu %f %f %f %f",
            &req->customer_id, &req->timestamp,
            &req->origin.longitude, &req->origin.latitude,
            &req->destination.longitude, &req->destination.latitude);
        assert(scanned == 6);
        dispatch(state, (void*)req);
        free(line);
        line = NULL;
        nbytes = 0;
    }

    // Your code here!
    pthread_mutex_lock(state->lock);
    state->is_finished = 1;
    pthread_cond_broadcast(state->queue_not_empty);
    pthread_mutex_unlock(state->lock);

    //Higiene
    pthread_cond_destroy(state->queue_not_empty);
    pthread_cond_destroy(state->queue_not_full);
    pthread_mutex_destroy(state->lock);

    return NULL;
}

// Implement the actual dispatch() and driver_thread() methods
void dispatch(world_t* state, void* req) {
    // Your code here!
    (void)state; (void)req;
    // get the lock
    pthread_mutex_lock(state->lock);
    
    // wait for condition variable
    while (size(state->request_queue) == MAX_QUEUE_SIZE){
        pthread_cond_wait(state->queue_not_full, state->lock);
    }
    //add to queue
    push_back(state->request_queue, req);

    pthread_cond_signal(state->queue_not_empty);
    pthread_mutex_unlock(state->lock);
}

void* driver_thread(void* arg) {
    world_t* state = (world_t*)arg;
    assert(state);

    assert(state->lock);
    int is_finished = 0;

    // Your code here!
    (void)state;
    while (!is_finished){
        // get the lock
        assert(state->lock);
        pthread_mutex_lock(state->lock);
    
        // wait for condition variable
        while (size(state->request_queue) == 0 && !state->is_finished){
            pthread_cond_wait(state->queue_not_empty, state->lock);
        }

        // remove from queue
        if (!state->is_finished){
            request_t* req = (request_t*)pop_front(state->request_queue);
            drive(req);
        } else {
            is_finished = 1;
        }
            
        pthread_cond_signal(state->queue_not_full);
        pthread_mutex_unlock(state->lock);
    }
    return NULL;
}
