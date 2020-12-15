//
// Computes the minimum of a list using multiple threads
//
// Warning: Return values of calls are not checked for error to keep 
// the code simple.
//
// Compilation command on ADA ($ sign is the shell prompt):
//  $ module load intel/2017A
//  $ icc -o list_minimum.exe list_minimum.c -lpthread -lc -lrt
//
// Sample execution and output ($ sign is the shell prompt):
//  $ ./list_minimum.exe 1000000 9
// Threads = 9, minimum = 148, time (sec) =   0.0013
//
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_THREADS     65536
#define MAX_LIST_SIZE   268435456


int num_threads;		// Number of threads to create - user input 

int thread_id[MAX_THREADS];	// User defined id for thread
pthread_t p_threads[MAX_THREADS];// Threads
pthread_attr_t attr;		// Thread attributes 

pthread_mutex_t lock_minimum;	// Protects minimum, count
pthread_cond_t cond_barrier; //Added to help with calculations
int minimum;			// Minimum value in the list
int count;			// Count of threads that have updated minimum
double mean;
double standard_deviation;

int list[MAX_LIST_SIZE];	// List of values
int list_size;			// List size

// Thread routine to compute minimum of sublist assigned to thread; 
// update global value of minimum if necessary
void *find_minimum (void *s) {
    int j;
    int my_thread_id = *((int *)s);

    int block_size = list_size/num_threads;
    int my_start = my_thread_id*block_size;
    int my_end = (my_thread_id+1)*block_size-1;
    if (my_thread_id == num_threads-1) my_end = list_size-1;

    // Thread computes local sum for computing mean when all threads are done
    long int sum_local = 0;
    for(j = my_start; j <= my_end; j++) {
        sum_local += list[j];
    }
    //after local sums are computed sum them in mean for calculation
    pthread_mutex_lock(&lock_minimum);
    if(count == 0) {
        mean = sum_local;
    }
    else {
        mean += sum_local;
    }
    count++;
    //calculate mean when all threads have added their local sums to total
    if(count == num_threads) {
        mean /= (double)list_size;
        //reset count for later use
        count = 0;
        pthread_cond_broadcast(&cond_barrier);
        pthread_mutex_unlock(&lock_minimum);
    }
    else {
        pthread_cond_wait(&cond_barrier, &lock_minimum);
        pthread_mutex_unlock(&lock_minimum);
    }
    //mean is computed and all threads have moved to standard deviation point of calculations
    //start with local standard deviations
    double local_standard_deviation = 0.0;
    for(j = my_start; j <= my_end; j++) {
        local_standard_deviation += ((list[j] - mean) * (list[j] - mean));
    }
    //now move to global standard deviation
    pthread_mutex_lock(&lock_minimum);
    if(count == 0) {
        standard_deviation = local_standard_deviation;
    }
    else {
        standard_deviation += local_standard_deviation;
    }
    count++;
    //calculate final standard deviation if final thread
    if(count == num_threads) {
        standard_deviation = sqrt(standard_deviation/((double)list_size));
        pthread_cond_broadcast(&cond_barrier);
        pthread_mutex_unlock(&lock_minimum);
    }
    else {
        pthread_cond_wait(&cond_barrier, &lock_minimum);
        pthread_mutex_unlock(&lock_minimum);
    }
    

    // Thread exits
    pthread_exit(NULL);
}

// Main program - set up list of randon integers and use threads to find
// the minimum value; assign minimum value to global variable called minimum
int main(int argc, char *argv[]) {

    struct timespec start, stop;
    double total_time, time_res;
    int i, j; 
    double true_mean = 0.0;
    double true_standard_deviation = 0.0;
    

    if (argc != 3) {
	printf("Need two integers as input \n"); 
	printf("Use: <executable_name> <list_size> <num_threads>\n"); 
	exit(0);
    }
    if ((list_size = atoi(argv[argc-2])) > MAX_LIST_SIZE) {
	printf("Maximum list size allowed: %d.\n", MAX_LIST_SIZE);
	exit(0);
    }; 
    if ((num_threads = atoi(argv[argc-1])) > MAX_THREADS) {
	printf("Maximum number of threads allowed: %d.\n", MAX_THREADS);
	exit(0);
    }; 
    if (num_threads > list_size) {
	printf("Number of threads (%d) < list_size (%d) not allowed.\n", num_threads, list_size);
	exit(0);
    }; 

    // Initialize mutex and attribute structures
    pthread_cond_init(&cond_barrier, NULL);
    pthread_mutex_init(&lock_minimum, NULL); 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Initialize list, compute statistics to verify result
    srand48(0); 	// seed the random number generator
    list[0] = lrand48(); 
    true_mean = list[0];
    for (j = 1; j < list_size; j++) {
	list[j] = lrand48(); 
	true_mean += list[j];
    }
    //calculate true mean
    true_mean /= (double)list_size;
    //calculate true standard deviation
    for(j = 0; j < list_size; j++) {
        true_standard_deviation += (double)((list[j] - true_mean)*(list[j] - true_mean));
    }
    true_standard_deviation = sqrt(true_standard_deviation/(double)list_size);

    // Initialize count
    count = 0;

    // Create threads; each thread executes find_minimum
    clock_gettime(CLOCK_REALTIME, &start);
    for (i = 0; i < num_threads; i++) {
	thread_id[i] = i; 
	pthread_create(&p_threads[i], &attr, find_minimum, (void *) &thread_id[i]); 
    }
    // Join threads
    for (i = 0; i < num_threads; i++) {
	pthread_join(p_threads[i], NULL);
    }

    // Compute time taken
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec-start.tv_sec)
	+0.000000001*(stop.tv_nsec-start.tv_nsec);

    // Check answer
    //occasional small floating point errors occur, so the values will be floored to check for validity withing a margin of error
    if (floor(true_mean) != floor(mean)) {
	printf("Incorrect mean\n"); 
    }
    if (floor(true_standard_deviation) != floor(standard_deviation)) {
	printf("Incorrect standard deviation\n"); 
    }
    // Print time taken
    printf("Threads = %d, mean = %f, standard_deviation = %f, time (sec) = %8.4f\n", 
	    num_threads, mean, standard_deviation, total_time);

    // Destroy mutex and attribute structures
    pthread_cond_destroy(&cond_barrier); //added to destroy barrier condition
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&lock_minimum);
}

