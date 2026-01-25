#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SHARED_MEM_SIZE  4096
#define FILE_PATH        "shared_data.dat"  // Regular file!

int main() {
    // 1. Create/truncate file
    int fd = open(FILE_PATH, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 2. Set file size (important!)
    if (ftruncate(fd, SHARED_MEM_SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    // 3. Memory map it (MAP_SHARED = changes visible to ALL processes)
    char *data = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap writer");
        return 1;
    }

    // 4. Write data
    strcpy(data, "Hello from File-mapping shared memory PID=");
    char pid_str[20];
    snprintf(pid_str, sizeof(pid_str), "%d", getpid());
    strcat(data, pid_str);

    printf("PID %d: '%s'\n", getpid(), data);
    //printf("File exists at: %s (size: %ld bytes)\n", FILE_PATH, SHARED_MEM_SIZE);

    // 5. Cleanup (file + data PERSIST!)
    munmap(data, SHARED_MEM_SIZE);
    close(fd);
    // NO unlink - file stays on disk!
    return 0;
}