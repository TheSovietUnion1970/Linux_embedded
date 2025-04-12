// named_semaphore_writer.c
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    sem_t *sem = sem_open("/my_named_semaphore", O_CREAT, 0644, 0);
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
