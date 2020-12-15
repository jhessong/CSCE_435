// Game of Drones
//
// Find the drone in a grid 
//
// *** Lines marked "Do not remove" should be retained in your code ***
//
// Warning: Return values of calls are not checked for error to keep 
// the code simple.
//
// Requires drone.h to be in the same directory
// 
// Compilation command on ADA:
//
//   module load intel/2017A
//   icc -o drone.exe drone.c -lpthread -lrt
//
// Sample execution and output ($ sign is the shell prompt):
// 
// $ ./drone.exe 
//   Need four integers as input 
//   Use: <executable_name> <grid_size> <random_seed> <delay_nanosecs> <move_count>
// $ ./drone.exe 256 0 1000000 0
//   Drone = (111,195), success = 1, time (sec) =   3.4672
//
//
/*
** Author: Jacob Hessong
** Comments: Most efficient when grid size is a power of 2, as the number of threads
** is a power of 2 that also has a square root. This allows the grid to be easily 
** divided between threads for most efficient running time. Also, when compiled, a
** warning appears but after testing the warning does not effect the program in any
** way so it can be ignored.
*/
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include "drone.h"		// Do not remove
struct timespec start, stop; 	// Do not remove
//clock_t start, stop;

double total_time;		// Do not remove
int gridsize;			// Do not remove

unsigned int drone_x, drone_y; 	//Coordinates of drone (to be found)
unsigned int found_x_start, found_y_start; //starting points if path is found for smaller grid
bool path_found = false; //flag if the path has been found to narrow down grid
bool drone_found = false; //flag in case drone is found before path
bool second_run = false; //flag for second, smaller scale run so that it doesn't end early
static int num_threads = 64; //number of threads is power of 2 and perfect square for easy division of grid
int length; //length to check in both x and y axes

// -------------------------------------------------------------------------
// Data structures and multithreaded code to find drone in the grid 
// ...
// ...
// ...
// ...
// ...
// ...
int thread_id[1000]; //array of thread ids that assist in dividing up grid
pthread_t p_threads[1000]; //array of threads
pthread_mutex_t lock_found; //lock for flags and coordinate variables
pthread_attr_t attr;

void thread_func(void* thread_id) {
    int my_thread_id = *((int*)thread_id);
    unsigned int i, j;
    unsigned int x_start, y_start;
    if(!second_run) { //calculates starting x and y values for threads based on entire grid
        if(my_thread_id == 0) { //base case for first thread to avoid DBZ error
            x_start = 0;
            y_start = 0;
        }
        else {
            //calculate starting x value by taking mod of thread id and sqrt of the number
            //of threads and multiplying it by length
            x_start = (my_thread_id % (int)sqrt(num_threads)) * length;
            //for loop to create rows of subgrids
            for(int y_index = 1; y_index <= sqrt(num_threads); y_index++) {
                if(my_thread_id < y_index*sqrt(num_threads)) { //used to determine which row
                    y_start = (y_index-1)*length;
                    break;
                }
            }
        }
    }
    else {
        if(my_thread_id == 0) { //starts threads at square that path was found at
            x_start = found_x_start;
            y_start = found_y_start;
        }
        else {
            //calculates starts in same way as above, just adds start values
            x_start = ((my_thread_id % (int)sqrt(num_threads)) * length) + found_x_start;
            for(int y_index = 1; y_index <= sqrt(num_threads); y_index++) {
                if(my_thread_id < y_index*sqrt(num_threads)) {
                    y_start = ((y_index-1)*length) + found_y_start;
                    break;
                }
            }
        }
    }
    int chk;
    for(i = x_start; i < length + x_start; i++) {
        for(j = y_start; j < length + y_start; j++) {
            chk = check_grid(i, j);
            if(chk == 0) {
                //when drone is found, sets coordinates and flag for other threads
                pthread_mutex_lock(&lock_found);
                drone_x = i;
                drone_y = j;
                drone_found = true;
                pthread_mutex_unlock(&lock_found);
            }
            else if((chk > 0) && (chk != _MAX_PATH_LENGTH + 1)) {
                //if path is found, sets found coordinates and flag (if on first 
                //run) for other threads
                pthread_mutex_lock(&lock_found);
                found_x_start = i;
                found_y_start = j;
                if(second_run == false) path_found = true;
                pthread_mutex_unlock(&lock_found);
            }
            if((drone_found == true) || (path_found == true)) {
                break;
            }
        }
        if((drone_found == true) || (path_found == true)) {
            break;
        }
        //double breaks cause all threads to stop searching once path or drone has
        //been found
    }
}

// -------------------------------------------------------------------------
// Main program to find drone in a grid
int main(int argc, char *argv[]) {

    if (argc != 5) {
	printf("Need four integers as input \n"); 
	printf("Use: <executable_name> <grid_size> <random_seed> <delay_nanosecs> <move_count>\n"); 
	exit(0);
    }
    // Initialize grid
    gridsize = abs((int) atoi(argv[argc-4])); 		// Do not remove
    int seed = (int) atoi(argv[argc-3]); 		// Do not remove
    int delay_nsecs = abs((int) atoi(argv[argc-2]));// Do not remove
    int move_count = abs((int) atoi(argv[argc-1]));	// Do not remove
    initialize_grid(gridsize, seed, delay_nsecs, move_count); // Do not remove
    gridsize = get_gridsize();	 			// Do not remove

//    print_drone_path(); 

    clock_gettime(CLOCK_REALTIME, &start); 	// Do not remove
//start = clock();

    // Multithreaded code to find drone in the grid 
    // ...
    // ...
    // ... sample serial code shown below ...
    // ... works when drone is not allowed to move in check_grid() ...
    /*unsigned int i, j;
    int chk;
    for (i = 0; i < gridsize; i++) {
	for (j = 0; j < gridsize; j++) {
	    chk = check_grid(i,j);
	    if (chk == 0) {
		drone_x = i; 
		drone_y = j; 
	    }
	}
    }*/
    pthread_mutex_init(&lock_found, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    length = gridsize/(int)sqrt(num_threads); //calculates how wide/tall subgrids should be
    for(int i = 0; i < num_threads; i++) {
        thread_id[i] = i;
        pthread_create(&p_threads[i], &attr, thread_func, (void*)&thread_id[i]);
    }
    for(int i = 0; i < num_threads; i++) {
        pthread_join(p_threads[i], NULL);
    }
    if(!drone_found) { //moves to second, smaller run if drone is not found before path
        //saw that drone moves 16 spaces by default, so calculates new length and adds 16
        length = move_count/(int)sqrt(num_threads) + 16;
        path_found = false; //sets to false so that threads can go through entire subgrid
        second_run = true; //flags that this is second run so that thread function works differently
        for(int i = 0; i < num_threads; i++) {
            thread_id[i] = i;
            pthread_create(&p_threads[i], &attr, thread_func, (void*)&thread_id[i]);
        }
        for(int i = 0; i < num_threads; i++) {
            pthread_join(p_threads[i], NULL);
        }
    }
    // ...
    // ...
    // ...

    // Compute time taken
    clock_gettime(CLOCK_REALTIME, &stop);			// Do not remove
    total_time = (stop.tv_sec-start.tv_sec)			// Do not remove
	+0.000000001*(stop.tv_nsec-start.tv_nsec);		// Do not remove
//stop = clock();
//total_time = (1.0*stop-start)/CLOCKS_PER_SEC;

    // Check if drone found, print time taken		
    printf("Drone = (%u,%u), success = %d, time (sec) = %8.4f\n", // Do not remove
    drone_x, drone_y, check_drone_location(drone_x,drone_y), total_time);// Do not remove

    // Other code to wrap up things
    // ...
    // ...
    // ...

}

