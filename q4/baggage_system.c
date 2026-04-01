#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// --- System Configuration Constants ---
#define BELT_CAPACITY 5
#define MAX_LUGGAGE 20
#define PRODUCER_TIME 2
#define CONSUMER_TIME 4
#define MONITOR_TIME 5

// --- Shared Memory (Circular Buffer & Counters) ---
int conveyor_belt[BELT_CAPACITY];
int belt_count = 0;      // Current number of items on the belt
int belt_head = 0;       // Index to insert next luggage
int belt_tail = 0;       // Index to remove next luggage

int total_loaded = 0;
int total_dispatched = 0;

// --- Synchronization Primitives ---
// Mutex to protect access to the shared memory above
pthread_mutex_t belt_mutex;
// Condition variable to signal the producer when space becomes available
pthread_cond_t cond_not_full;
// Condition variable to signal the consumer when luggage is available
pthread_cond_t cond_not_empty;

// ==========================================
// THREAD 1: Conveyor Belt Loader (Producer)
// ==========================================
void* producer_loader(void* arg) {
    for (int i = 1; i <= MAX_LUGGAGE; i++) {
        sleep(PRODUCER_TIME); // Simulate time to load luggage

        // 1. Lock the mutex before accessing shared memory
        pthread_mutex_lock(&belt_mutex);

        // 2. Wait if the belt is full (Capacity Constraint)
        // We use a 'while' loop instead of 'if' to handle spurious wakeups.
        while (belt_count == BELT_CAPACITY) {
            printf("[Producer] Belt is FULL. Waiting for space...\n");
            // pthread_cond_wait automatically unlocks the mutex while waiting, 
            // and re-locks it before returning.
            pthread_cond_wait(&cond_not_full, &belt_mutex);
        }

        // 3. Load luggage onto the circular buffer
        conveyor_belt[belt_head] = i;
        belt_head = (belt_head + 1) % BELT_CAPACITY;
        belt_count++;
        total_loaded++;

        printf("[Producer] Loaded Luggage ID: %d | Belt Size: %d/5\n", i, belt_count);

        // 4. Signal the consumer that the belt is no longer empty
        pthread_cond_signal(&cond_not_empty);

        // 5. Unlock the mutex
        pthread_mutex_unlock(&belt_mutex);
    }
    return NULL;
}

// ==========================================
// THREAD 2: Aircraft Loader (Consumer)
// ==========================================
void* consumer_loader(void* arg) {
    for (int i = 1; i <= MAX_LUGGAGE; i++) {
        sleep(CONSUMER_TIME); // Simulate time to load onto aircraft

        // 1. Lock the mutex
        pthread_mutex_lock(&belt_mutex);

        // 2. Wait if the belt is empty
        while (belt_count == 0) {
            printf("[Consumer] Belt is EMPTY. Waiting for luggage...\n");
            pthread_cond_wait(&cond_not_empty, &belt_mutex);
        }

        // 3. Remove luggage from the circular buffer
        int luggage_id = conveyor_belt[belt_tail];
        belt_tail = (belt_tail + 1) % BELT_CAPACITY;
        belt_count--;
        total_dispatched++;

        printf("[Consumer] Dispatched Luggage ID: %d to Aircraft | Belt Size: %d/5\n", luggage_id, belt_count);

        // 4. Signal the producer that the belt is no longer full
        pthread_cond_signal(&cond_not_full);

        // 5. Unlock the mutex
        pthread_mutex_unlock(&belt_mutex);
    }
    return NULL;
}

// ==========================================
// THREAD 3: Monitoring System
// ==========================================
void* monitor_system(void* arg) {
    // Run until all luggage is dispatched to gracefully terminate
    while (1) {
        sleep(MONITOR_TIME);

        // Lock mutex to safely read shared counters without race conditions
        pthread_mutex_lock(&belt_mutex);
        
        printf("\n=== [MONITOR REPORT] ===\n");
        printf("Total Loaded onto Belt: %d\n", total_loaded);
        printf("Total Dispatched to Aircraft: %d\n", total_dispatched);
        printf("Current Belt Size: %d/5\n", belt_count);
        printf("========================\n\n");

        int dispatched_check = total_dispatched; // copy for loop condition

        pthread_mutex_unlock(&belt_mutex);

        // Graceful exit condition
        if (dispatched_check >= MAX_LUGGAGE) {
            break;
        }
    }
    return NULL;
}

// ==========================================
// MAIN FUNCTION
// ==========================================
int main() {
    printf("Starting Airport Baggage Handling Simulation (Target: %d items)\n\n", MAX_LUGGAGE);

    // Initialize synchronization primitives
    pthread_mutex_init(&belt_mutex, NULL);
    pthread_cond_init(&cond_not_full, NULL);
    pthread_cond_init(&cond_not_empty, NULL);

    // Declare threads
    pthread_t producer_thread, consumer_thread, monitor_thread;

    // Create threads
    pthread_create(&producer_thread, NULL, producer_loader, NULL);
    pthread_create(&consumer_thread, NULL, consumer_loader, NULL);
    pthread_create(&monitor_thread, NULL, monitor_system, NULL);

    // Wait for all threads to finish (graceful termination)
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    pthread_join(monitor_thread, NULL);

    // Clean up synchronization primitives
    pthread_mutex_destroy(&belt_mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    printf("\nSimulation Complete. All %d luggage items successfully dispatched.\n", MAX_LUGGAGE);
    return 0;
}
