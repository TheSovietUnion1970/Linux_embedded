// sem_producer.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>

#define SEM_KEY_FILE  "aa"   // must exist or we create it
#define SEM_ID        'S'                   // project id for ftok

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

int main() {
    key_t key;
    int semid;
    struct sembuf sop;

    // Create a file if it doesn't exist (just for ftok)
    if (access(SEM_KEY_FILE, F_OK) != 0) {
        FILE *f = fopen(SEM_KEY_FILE, "w");
        if (f) fclose(f);
    }

    // Get System V key
    key = ftok(SEM_KEY_FILE, SEM_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    // Create or get semaphore set (1 semaphore)
    semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    printf("Producer: Semaphore created/got. ID = %d\n", semid);

    // Optional: initialize to 0 if newly created (only once!)
    // In real programs, you usually do this only in one place
    union semun arg;
    arg.val = 0;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        // continue anyway - maybe already initialized
    }

    while (1) {
        printf("Producer: Posting (increasing) semaphore...\n");

        // Increase semaphore by 1
        sop.sem_num = 0;
        sop.sem_op  = 1;           // +1
        sop.sem_flg = 0;

        if (semop(semid, &sop, 1) == -1) {
            perror("semop post");
            break;
        }

        sleep(5);  // simulate some work
    }

    return 0;
}