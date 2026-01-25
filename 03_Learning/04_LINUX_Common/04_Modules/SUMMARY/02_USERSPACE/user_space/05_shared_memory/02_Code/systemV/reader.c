#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 1024

int main() {
    // Shared memory key
    key_t key = ftok("shmfile", 65);

    // Get shared memory segment id
    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    // Attaching a shared memory segment to the address space of the calling process
    char *data = (char*) shmat(shmid, (void*)0, 0);
    if (data == (char*)-1) {
        perror("shmat");
        return 1;
    }

    printf("Client read: %s\n", data);

    // Detaching the shared memory segment from the address space of the calling process
    shmdt(data);

    // Cleanup: remove shared memory
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
