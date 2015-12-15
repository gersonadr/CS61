#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// timestamp()
//    Return the current time as a double.

static inline double timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

#define NUM_THREADS 8
#define NUM_PASSENGERS NUM_THREADS
#define NUM_UBERS 4
#define CALCS_PER_UBER 1500000
#define RIDES_PER_PASSENGER 3
#define PI 3.14159265358979323846264338327

pthread_mutex_t uber_locks[NUM_UBERS];
double uber_times[NUM_UBERS];
volatile double inside[NUM_UBERS];
volatile double outside[NUM_UBERS];

static inline double rand_unif() {
	return (double)rand() / (double)RAND_MAX;
}

void drive(int thread_id, int uber_id) {
	(void) thread_id;

    double start_time = timestamp();
	double sample_x;
	double sample_y;
	double res;
	for (int k = 0; k < CALCS_PER_UBER; ++k) {
		sample_x = rand_unif();
		sample_y = rand_unif();

		res = pow(sample_x, 2) + pow(sample_y, 2);
		if (res < 1.0) {
			inside[uber_id]++;
		}
		else {
			outside[uber_id]++;
		}
	}

    uber_times[uber_id] += (timestamp() - start_time);

}

// Question to be answered in your README: Why is this function so slow?
void* passenger(void* params) {
	int me = (int)params;
	for (int k = 0; k < RIDES_PER_PASSENGER; ++k) {
		for (int i = 0; i < NUM_UBERS; ++i) {
			pthread_mutex_lock(&uber_locks[i]);
			drive(me, i);
			pthread_mutex_unlock(&uber_locks[i]);
			break;
		}
	}
	return NULL;
}


/*
Explanation: to avoid the problem stated on WP6 in README file,
the passanger thread will acquire the lock from a 
random uber driver (shared resources), minimizing the
risk of 2 passengers competing for the same driver's lock.
Note that it does not eliminate the problem, only mitigates it.

The synchronization strategy is the same as in passanger() method:
if a passenger thread whats to use the driver (shared resource), 
it must acquire the driver's lock, otherwise we would have a 
race condition. And because the array of drivers (each one protected
by a lock) is the only shared resource between passenger threads, 
it is the only resource that needs a lock (correctness).

Also, it will not run into deadlocks because the passenger thread
will acquire 1 driver (shared resource) and release before attempting
to get another driver. The direction of dependency does not change
(passengers trying to get hold of drivers, and not the other way around),
so we can't have a circular dependency. And the passangers should never
starve, unless a passenger utilizes a driver for a long period of time
before releasing it (such as getting a uber ride from Boston to California), 
but that is one of the exercise implicit assumptions.
*/
void* passenger_better_init(void* params) {
	int me;
	
	(void)me;	// Avoid unused variable warnings
	me = (int)params;

	for (int k = 0; k < RIDES_PER_PASSENGER; ++k) {
		for (int i = 0; i < NUM_UBERS; ++i) {

			int uber_driver = (rand() % NUM_UBERS);
			pthread_mutex_lock(&uber_locks[uber_driver]);
			drive(me, uber_driver);
			pthread_mutex_unlock(&uber_locks[uber_driver]);
			break;
		}
	}	

	return NULL;
}

/*
Trylock is a mechanism which a thread can acquire a lock while
not getting blocked if the operation was unsuccessful. This helps
to minimize passenger()'s blocking problem further, essentially
because each passenger thread would not be blocked by a shared
resource (driver) already in use.

I also think it's a good practice to randomize the driver the 
passenger will try to lock, reusing the strategy from passanger_better_init().

There won't be deadlocks on this code because the locking mechanism
is non-blocking. Also, the correctness and starvation points made above
still apply: no starvation assuming each passenger wont hold the
lock for longer periods (long period is completelly arbitrary), and 
the driver (shared resource) is still protected by the lock, avoiding 
race conditions and enforcing correctness (every shared resource must have a lock).
*/
void* passenger_trylock(void* params) {
	int me;
	
	(void)me;
	me = (int)params;

	for (int k = 0; k < RIDES_PER_PASSENGER; ++k) {
		while (1){
			int uber_driver = (rand() % NUM_UBERS);
			
			if (pthread_mutex_trylock(&uber_locks[uber_driver]) == 0){
				drive(me, uber_driver);
				pthread_mutex_unlock(&uber_locks[uber_driver]);
				break;
			}
		}
	}

	return NULL;
}

static void print_usage() {
	printf("Usage: ./uber-pi [PASSENGER_TYPE]\n");
	exit(1);
}

int main (int argc, char** argv) {
	srand((unsigned)time(NULL));
	pthread_t threads[NUM_THREADS];

	if (argc < 2) {
		print_usage();
	}

	for (int j = 0; j < NUM_UBERS; ++j) {
		pthread_mutex_init(&uber_locks[j], NULL);
	}

    double timevar = timestamp();

	for (long long i = 0; i < NUM_PASSENGERS; ++i) {
		if (strcmp(argv[1], "2") == 0) {
			if (pthread_create(&threads[i], NULL, passenger_trylock, (void *)i)) {
				printf("pthread_create failed\n");
				exit(1);
			}
		}
		else if (strcmp(argv[1], "1") == 0) {
			if (pthread_create(&threads[i], NULL, passenger_better_init, (void *)i)) {
				printf("pthread_create failed\n");
				exit(1);
			}
		}
		else if (strcmp(argv[1], "0") == 0) {
			if (pthread_create(&threads[i], NULL, passenger, (void *)i)) {
				printf("pthread_create failed\n");
				exit(1);
			}
		}
		else {
			print_usage();
		}
	}

	for (int i = 0; i < NUM_PASSENGERS; ++i) {
		pthread_join(threads[i], NULL);
	}

    timevar = (timestamp() - timevar);

	double inside_sum = 0;
	double outside_sum = 0;
    double total_uber_time = 0.0;
	for (int u = 0; u < NUM_UBERS; ++u) {
		inside_sum += inside[u];
		outside_sum += outside[u];
        total_uber_time += uber_times[u];
	}

	double mc_pi = 4.0 * inside_sum/(inside_sum + outside_sum);

    printf("Average fraction of time Uber drivers were driving: %5.3f\n",
            (total_uber_time / NUM_UBERS) / timevar);
	printf("Value of pi computed was: %f\n", mc_pi);
	if (!(fabs(mc_pi - PI) < 0.02)) {
		printf("Your computation of pi was not very accurate, something is probably wrong!\n");
		exit(1);
	}
	return 0;
}
