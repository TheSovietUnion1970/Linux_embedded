#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "thread.h"

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;
pthread_t thread_id1, thread_id2;

/* Shared data between threads */
int counter = 2; // shared variable/shared resources/global variable

typedef struct {
    char name[30];
    char msg[30];
} thread_args_t;

static void *handle_th1(void *args) 
{   

    thread_args_t *thr = (thread_args_t *)args;

#if (MUTEX_USED)
    pthread_mutex_lock(&lock1);
#endif
#if (THREAD_EQUAL)
    pthread_t tid = pthread_self();
    if (pthread_equal(tid, thread_id1)) {
        printf("[1] I'm thread_id1\n");
    }
#endif
#if (COND_VAR)
    // pthread_cond_signal(&cond);
    pthread_cond_wait(&cond, &lock1);
#endif
    // critical section 
    printf("[1] The rest ...\n");
    printf("[1] hello %s !\n", thr->name);
    printf("[1] thread1 handler, counter: %d\n", ++counter);
    sleep(5);
#if (MUTEX_USED)
    pthread_mutex_unlock(&lock1);
#endif
    pthread_exit(NULL); // exit

}

static void *handle_th2(void *args) 
{
#if (MUTEX_USED)
    pthread_mutex_lock(&lock1);
#endif
#if (THREAD_EQUAL)
    pthread_t tid = pthread_self();
    if (pthread_equal(tid, thread_id2)) {
        printf("[2] I'm thread_id2\n");
    }
#endif
#if (COND_VAR)
    // pthread_cond_wait(&cond, &lock1);
    pthread_cond_signal(&cond);
#endif
    printf("[2] thread2 handler, counter: %d\n", ++counter);
#if (MUTEX_USED)
    pthread_mutex_unlock(&lock1);
#endif

    pthread_exit(NULL); // exit
}

int main(int argc, char const *argv[])
{
    /* code */
    int ret;
    thread_args_t thr;

    memset(&thr, 0x0, sizeof(thread_args_t));
    strncpy(thr.name, "Vinh", sizeof(thr.name));

    if (ret = pthread_create(&thread_id1, NULL, &handle_th1, &thr)) {
        printf("pthread_create() error number=%d\n", ret);
        return -1;
    }

    if (ret = pthread_create(&thread_id2, NULL, &handle_th2, NULL)) {
        printf("pthread_create() error number=%d\n", ret);
        return -1;
    }
#if (THREAD_JOIN)
    // used to block for the end of a thread and release
    pthread_join(thread_id1,NULL);  
    pthread_join(thread_id2,NULL);
#endif
#if (THREAD_DETACH)
    sleep(2);
    pthread_detach(thread_id1);  
    pthread_detach(thread_id2);
#endif
    return 0;
}