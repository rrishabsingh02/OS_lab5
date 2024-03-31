#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h> // included for seeding the random number generator with current time

// define constants for the number of customers and resources
#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

// global arrays to track the available, maximum, allocation, and need of resources
int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {0}; // initialized to zero
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
pthread_mutex_t lock; // mutex for synchronizing access to shared resources

// prototype functions for requesting and releasing resources, the customer routine, and the safety check
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
void *customer_routine(void *customer_id);
bool check_safety();

// main function begins here
int main(int argc, char *argv[]) {
    srand(time(NULL)); // seed the random number generator
    // check for correct number of command-line arguments
    if (argc != NUMBER_OF_RESOURCES + 1) {
        printf("incorrect number of arguments. provide the initial number of resources.\n");
        return -1;
    }

    // declare thread identifiers for customers
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    pthread_mutex_init(&lock, NULL); // initialize the mutex

    // initialize available resources from command-line arguments
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1); // simple randomization of maximum demand
            need[i][j] = maximum[i][j];
        }
    }

    // create customer threads
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        int *id = malloc(sizeof(int)); // dynamically allocate memory for thread id
        *id = i; // set the id
        pthread_create(&customers[i], NULL, customer_routine, (void *)id); // create the thread
    }

    // join customer threads
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL); 
    }

    pthread_mutex_destroy(&lock); // destroy the mutex
    return 0; 
}

// function to request resources
int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&lock); 
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        // check if request can be granted
        if (request[i] > need[customer_num][i] || request[i] > available[i]) {
            pthread_mutex_unlock(&lock); 
            return -1; // request cannot be granted
        }
    }

    // pretend to allocate resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    if (check_safety()) { // check if the system is still in a safe state
        pthread_mutex_unlock(&lock); // release the mutex lock
        return 0; // request can be safely granted
    } else {
        // revert state because it's not safe
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&lock); // release the mutex lock
        return -1; // request cannot be granted
    }
}

// function to release resources
int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&lock); // acquire the mutex lock to protect shared resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        // update allocation, need, and available based on the resources released
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
        available[i] += release[i];
    }
    pthread_mutex_unlock(&lock); // release the mutex lock
    return 0; // successful release of resources
}

// the routine that each customer thread executes
void *customer_routine(void *customer_id) {
    int id = *(int *)customer_id;

   
    
    int request[NUMBER_OF_RESOURCES];
    
    // Generate a random request
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (need[id][i] > 0) {
            request[i] = rand() % (need[id][i] + 1);
        } else {
            request[i] = 0; // If no need, do not request
        }
    }

    // Try to request resources
    if (request_resources(id, request) == 0) {
        printf("Customer %d's request granted.\n", id);
        
        // Simulate using the resources
        sleep(rand() % 3); // Random sleep to simulate work done
        
        // Release the resources
        release_resources(id, request);
        printf("Customer %d released resources.\n", id);
    } else {
        printf("Customer %d's request denied. Trying again...\n", id);
    }

    sleep(1); // Prevent spamming requests too quickly
    

    free(customer_id);
    return NULL;
}

// function to check if the system is in a safe state
bool check_safety() {
    int work[NUMBER_OF_RESOURCES]; // local copy of available resources
    bool finish[NUMBER_OF_CUSTOMERS] = {false}; // track completion status of customers

    // initialize work = available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i];
    }

    bool found;
    do {
        found = false;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            // try to find a customer that can still complete
            if (!finish[i]) {
                bool can_allocate = true;
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        can_allocate = false; // customer cannot complete
                        break;
                    }
                }

                if (can_allocate) { // if the customer can complete
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                        work[j] += allocation[i][j]; // pretend to allocate
                    }
                    finish[i] = true; // mark customer as complete
                    found = true;
                }
            }
        }
    } while (found); // loop until no more customers can complete

    // if all customers can finish, the state is safe
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i]) {
            return false; // if any customer cannot finish, the state is not safe
        }
    }

    return true; // if all customers can finish, the state is safe
}
