#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define SHM_NAME "/posix_shm"
#define SHM_SIZE 1024

int main() {
    // Open shared memory segment.
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Configure the size of the shared memory object.
    ftruncate(fd, SHM_SIZE);

    // Get Map addresses
    void *ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    strcpy(ptr, "Hello from POSIX named shared memory!");
    printf("Writer wrote message.\n");

    // Deallocate any mapping for the region
    munmap(ptr, SHM_SIZE);
    close(fd);

    return 0;
}
