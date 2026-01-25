// unnamed_semaphore_example.c
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/*
sem val = 0
Worker: Waiting for signal... (sem_wait -> lock)
... 2s ...
Main: Sending signal to worker. (sem_post -> unlock -> worker continues ...)
Worker: Got signal! Proceeding... in 2s
... 2s ...
Worker: done
*/

sem_t sem;

void* worker(void* arg) {
    printf("Worker: Waiting for signal...\n");
    sem_wait(&sem); // Wait (decrement)
    printf("Worker: Got signal! Proceeding... in 2s\n");
    sleep(2); // Simulate some work
    return NULL;
}

int main() {
    pthread_t t;
    
    // Initialize unnamed semaphore
    sem_init(&sem, 0, 0); // 0 means shared between threads, initial value 0

    pthread_create(&t, NULL, worker, NULL);

    sleep(2); // Simulate some work
    printf("Main: Sending signal to worker.\n");
    sem_post(&sem); // Post (increment)

    pthread_join(t, NULL);
    printf("Worker: done\n");

    sem_destroy(&sem); // Cleanup
    return 0;
}