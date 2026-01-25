// named_semaphore_reader.c
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>

#define SEMAPHORE_NAME "semaphore"

int sem_v = 0;

int main() {
    sem_t *sem = sem_open(SEMAPHORE_NAME, O_CREAT, 0644, 0);

    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    printf("Reader: Waiting on semaphore...\n");
    sem_wait(sem);
    sem_getvalue(sem, &sem_v);
    printf("Sem value = %d\n", sem_v);
    printf("Reader: Received signal!\n");

    sem_close(sem);
    sem_unlink("/my_named_semaphore"); // Remove it from system
    return 0;
}
