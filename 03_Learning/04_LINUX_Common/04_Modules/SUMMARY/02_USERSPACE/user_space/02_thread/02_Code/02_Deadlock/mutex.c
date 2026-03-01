// deadlock_example.c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutexA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexB = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    printf("Thread 1: Trying to lock mutexA...\n");
    pthread_mutex_lock(&mutexA);
    printf("Thread 1: mutexA locked\n");

    sleep(1);  // Simulate some work

    printf("Thread 1: Trying to lock mutexB...\n");
    pthread_mutex_lock(&mutexB);           // ← will block forever
    printf("Thread 1: mutexB locked\n");

    pthread_mutex_unlock(&mutexB);
    pthread_mutex_unlock(&mutexA);
    return NULL;
}

void* thread2_func(void* arg) {
    printf("Thread 2: Trying to lock mutexB...\n");
    pthread_mutex_lock(&mutexB);
    printf("Thread 2: mutexB locked\n");

    sleep(1);  // Simulate some work

    printf("Thread 2: Trying to lock mutexA...\n");
    pthread_mutex_lock(&mutexA);           // ← will block forever
    printf("Thread 2: mutexA locked\n");

    pthread_mutex_unlock(&mutexA);
    pthread_mutex_unlock(&mutexB);
    return NULL;
}

int main() {
    pthread_t t1, t2;

    printf("=== Starting deadlock example ===\n");

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("This line will never be reached.\n");
    return 0;
}
