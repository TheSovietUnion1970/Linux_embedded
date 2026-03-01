#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include "com.h"

#define NUM_THREADS  20
#define INCREMENTS   10000

#if (ATOMIC)
atomic_int counter = 0;
#else
int counter = 0; 
#endif

void* increment(void* arg) {
    for (int i = 0; i < INCREMENTS; i++) {
#if (ATOMIC)
        // Option 1 – simple macro (most compatible)
        atomic_fetch_add(&counter, 1);
#else
        counter++;
#endif
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    printf("Starting %d threads, each incrementing %d times...\n",
           NUM_THREADS, INCREMENTS);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

#if (ATOMIC)
    printf("Final counter value: %d\n", atomic_load(&counter));
    printf("Expected value: %d\n", NUM_THREADS * INCREMENTS);
#else
    if (counter == NUM_THREADS * INCREMENTS) {
        printf("→ No race condition occurred (lucky!)\n");
    } else {
        printf("→ Race condition! Lost updates detected.\n");
        printf("Difference: %d\n", NUM_THREADS * INCREMENTS - counter);
    }
#endif
}