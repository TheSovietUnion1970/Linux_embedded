// named_semaphore_writer.c
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#define SEMAPHORE_NAME "semaphore" // located in /dev/shm/sem.<SEMAPHORE_NAME>

int main() {
    sem_t *sem = sem_open(SEMAPHORE_NAME, O_CREAT, 0644, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    printf("Writer: Sleeping 3 seconds before posting...\n");
    sleep(3);

    printf("Writer: Posting to semaphore.\n");
    sem_post(sem);

    sem_close(sem);
    return 0;
}
