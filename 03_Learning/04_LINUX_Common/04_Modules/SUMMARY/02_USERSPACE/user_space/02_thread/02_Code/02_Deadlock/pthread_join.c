// thread_join_deadlock.c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_t threadA;
pthread_t threadB;

void* threadA_func(void* arg) {
    printf("Thread A: started\n");
    sleep(1);  // simulate some work

    printf("Thread A: now waiting for Thread B to finish (join)...\n");
    pthread_join(threadB, NULL);   // ← Thread A waits for Thread B

    printf("Thread A: Thread B finished\n");
    return NULL;
}

void* threadB_func(void* arg) {
    printf("Thread B: started\n");
    sleep(1);  // simulate some work

    printf("Thread B: now waiting for Thread A to finish (join)...\n");
    pthread_join(threadA, NULL);   // ← Thread B waits for Thread A

    printf("Thread B: Thread A finished\n");
    return NULL;
}

int main() {
    printf("=== Starting join deadlock example ===\n");

    // Create both threads
    pthread_create(&threadA, NULL, threadA_func, NULL);
    pthread_create(&threadB, NULL, threadB_func, NULL);

    printf("Main thread: waiting for both threads...\n");

    // Main thread waits for both (this line will never be reached)
    pthread_join(threadA, NULL);
    pthread_join(threadB, NULL);

    printf("This line will NEVER be printed.\n");
    return 0;
}
