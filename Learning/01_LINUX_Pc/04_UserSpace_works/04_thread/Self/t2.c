#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

static void *handle_th1(void *args) 
{   
    //sleep(1);

    //pthread_mutex_lock(&lock1);
    // critical section 
    printf("thread1 handler\n");
    sleep(1);

    //pthread_mutex_unlock(&lock1);

    pthread_exit(NULL); // exit

}

int main(){
    int ret;
    pthread_t thread_id1;

    if (ret = pthread_create(&thread_id1, NULL, &handle_th1, NULL)) {
        printf("pthread_create() error number=%d\n", ret);
        return -1;
    }

    // pthread_join(thread_id1, NULL);
    pthread_detach(thread_id1);
    printf("Done thread 1\n");

    return 0;
}