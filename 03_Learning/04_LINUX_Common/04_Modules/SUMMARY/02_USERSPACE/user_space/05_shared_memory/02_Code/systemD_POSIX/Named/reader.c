#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

#define SHM_NAME "/posix_shm"
#define SHM_SIZE 1024

int main() {
    // Open shared memory segment.
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Get Map addresses
    void *ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("Reader got: %s\n", (char*)ptr);

    // Deallocate any mapping for the region
    munmap(ptr, SHM_SIZE);
    close(fd);

    // Remove shared memory segment
    shm_unlink(SHM_NAME); // remove shared memory

    return 0;
}
