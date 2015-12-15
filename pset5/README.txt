README for CS 61 Problem Set 5
------------------------------
OTHER COLLABORATORS AND CITATIONS (if any):



NOTES FOR THE GRADER (if any):


WP1 - Lock without condition variable. On this scenario, the ball is the shared state, the soccer players are threads and which ever player first heads the ball will be the ball's owner for that play. A lock is ideal because it provides mutual exclusion (thus removing race condition for the ball), and has the idea of ownership: the player who successfully locks the ball is the play's owner. I would use pthread_trylock, and each player thread would: (1) trylock the ball (2.1) run towards adversary goal (if acquire the lock) or (2.2) run after player with ball. Neither player would be blocked thus yield in high performance.

WP2 - Lock and 1 counting semaphore. This is a good example for counting semaphores, where we have a producer thread (Prof. Margot) and 3 consumers threads (hungry teenagers). The critical section is an array of pancakes of size = 3. The 4 threads will have access to pancake_array and pancake_sem, a semaphore that starts with count = 0 and increases by 3 (batch size). The producer thread would call pancakes_sem.V() as each pancake batch is made and insert pancakes in pancake_array. The teenager threads would call pancakes_sem.P() first (size = 0) and become blocked, waiting for producer thread to call pancakes_sem.V(), increment count += 3 and broadcast. In that instant, all blocked teenager threads would get notified and one would decrement pancake_sem and remove 1 pancake from pancake_array. To protect the pancake_array from concurrent access, the teenager threads of Margo thread need to acquire the lock before inserting / removing from the array. The teenagers who didn't get pancakes will wait (blocked) on the semaphore queue until the producer thread increases it and broadcasts again.

WP3 - Lock and array of condition variables. Each athlete (thread) wants to start running as soon as possible, but it is only allowed after his/her teammate complete the previous lap (condition variable). The lock will represent the baton as each athlete must "hold the baton" (acquire the lock) before running and the same athlete must release the baton when lap is complete. Also, he/she must signal the next athlete to get the baton (CV signal) before unlocking. The problem is of a scheduler and each athlete thread needs to execute in sequence. The way I would construct it: The lock (baton) grants access to the array of condition variables of size NUMBER_OF_ATHLETES. Each athlete thread will execute run_thread(int athlete, lock, cv_array), which does in sequence: Athlete i tries to get the lock (and blocks), receives notification and runs (knowing he/she holds the lock), signals athlete (i+1), releases the lock and finishes. The next athlete (i+1) would execute the same steps until the last athlete runs and finishes. The main thread would join on all athlete threads, printing the total time. 
Because the athlete can only run if he/she has the baton (lock), this implementation makes sure mutual exclusion of threads, namely, only 1 athlete thread can run at any period (pun intented). And since we know the sequence the athletes should run, each one would signal the next (using the shared cv array) before releasing the lock. 

WP4 - 2 counting semaphores and one lock . The neighborhood thread and the driver threads have access to one array of parking tokens (shared memory), 2 semaphores namely "checkin_sem" and "release_sem" and 1 lock to protect access to token_array. Checking_sem is initialized with count = number of available spots for that neighborhood, and release_sem is initialized with zero.

The neighborhood thread inserts parking tokens in the array as they are fred, and driver threads fetch tokens to use. To avoid race confition, the "driver_thread" will:

1 - Call checkin_sem.P() and blocks until there are spots available. 
2 - Once available, the driver will acquire the lock, 
3 - Retrieve one token from token_array
4 - Release the lock
5 - Make the best use of the parking spot (eg. having a wonderful time at the Quincy Market).
6 - Once it's ready to release the spot, call release_sem.P() causing the neighborhood thread to unblock. 

The neighborhood thread, on the other hand, will:

1 - Call release_sem.P() and be blocked until some driver releases a parking spot. 
2 - Once a spot has been released, it will acquire the lock
3 - Print "Thank you and come back soon!"
4 - Create and insert a new token into token_array.
5 - Call checkint_sem.V() to unblock sleeping drivers
6 - release the lock

By doing so, we avoid race condition for fetching and inserting parking tokens (using the lock), avoid busy waiting (drivers and neighborhood treads are blocked by the semaphore), the semaphores will ensure X parking spots are available at any given time, and no driver would block the "checkin" area (iow, hold the lock) while their car is parked and they are enjoying Boston. On the bad side, the system is not entirely fair because any driver can wake-up and acquire the lock because semaphores does not guarantee ordering.

WP5 - Binary semaphore. To ensure the signal from e-mail application is never lost, the student thread must retrieve the messages by trying to decrement "free_food_sem". Initially, this semaphore will be configured to zero, and the student thread is blocked, waiting on the semaphore's internal condition variable to trigger. When the email thread increases the semaphore, it will internally signal the waiting thread (student). By using semaphores, we guarantee no messages will be post between attempting to decrement the semaphore (student) and increasing the semaphore (email application). The student thread will not starve (pun intented), because the student thread is guaranteed to run after email increments the semaphore, and vice-versa. 

WP6 - It's slow because every passenger thread will compete for the same driver (shared resource) in sequence, and only 1 passenger thread will succeed, while the other passanger threads are blocked until the first passanger unlocks it.

A5 Grade Me