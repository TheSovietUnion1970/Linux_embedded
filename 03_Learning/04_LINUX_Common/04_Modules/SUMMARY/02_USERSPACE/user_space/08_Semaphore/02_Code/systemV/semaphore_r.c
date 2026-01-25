// sem_consumer.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>

#define SEM_KEY_FILE  "aa"
#define SEM_ID        'S'

int main() {
    key_t key;
    int semid;
    struct sembuf sop;

    key = ftok(SEM_KEY_FILE, SEM_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    // Get existing semaphore (do NOT use IPC_CREAT here!)
    semid = semget(key, 1, 0666);
    if (semid == -1) {
        perror("semget");
        printf("Make sure producer is running first!\n");
        return 1;
    }

    printf("Consumer: Connected to semaphore ID = %d\n", semid);

    while (1) {
        printf("Consumer: Waiting (sem_wait)...\n");

        // Decrease semaphore by 1 (blocks if == 0)
        sop.sem_num = 0;
        sop.sem_op  = -1;          // -1
        sop.sem_flg = 0;

        if (semop(semid, &sop, 1) == -1) {
            perror("semop wait");
            break;
        }

        printf("Consumer: Got resource! Doing work...\n");
        //sleep(1);  // simulate work
    }

    return 0;
}