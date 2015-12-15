#include "dispatch.h"

#define NUM_DRIVERS 5

// The world all Uber drivers and the dispatcher live in
static world_t world;

void checkerr(int err, const char* func_name) {
    if (err != 0) {
        perror(func_name);
        exit(err);
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    int err, i;
    void* retval;
    pthread_t dispatcher;
    pthread_t drivers[NUM_DRIVERS];

    init_world(&world);
    
    err = pthread_create(&dispatcher, NULL, dispatcher_thread, (void*)&world);
    checkerr(err, "pthread_create");

    for (i = 0; i < NUM_DRIVERS; ++i) {
        err = pthread_create(&drivers[i], NULL, driver_thread, (void*)&world);
        checkerr(err, "pthread_create");
    }

   err = pthread_join(dispatcher, &retval);
   checkerr(err, "pthread_join");
   for (i = 0; i < NUM_DRIVERS; ++i) {
       err = pthread_join(drivers[i], &retval);
       checkerr(err, "pthread_join");
   }

   printf("All threads finished, exit\n");
   return 0;
}
