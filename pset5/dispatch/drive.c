#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "request.h"

static pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;

static float distance(coordinate_t* org, coordinate_t* dst) {
    float lat_diff = fabs(dst->latitude - org->latitude);
    float long_diff = fabs(dst->longitude - org->longitude);
    return (sqrt(lat_diff*lat_diff + long_diff*long_diff));
}

void drive(request_t* req) {
    pthread_t driver_id = pthread_self();
    pthread_mutex_lock(&io_mutex);
    printf("Driver %lu: Responding to customer %lu at time %lu\n",
                    (unsigned long)driver_id, req->customer_id, req->timestamp);
    pthread_mutex_unlock(&io_mutex);
    float dist = distance(&req->origin, &req->destination);
    unsigned long int_dist = (unsigned long)(dist * 100);
    pthread_mutex_lock(&io_mutex);
    printf("Driver %lu: Drove distance %lu units\n",
                    (unsigned long)driver_id, int_dist);
    pthread_mutex_unlock(&io_mutex);
    return;
}
