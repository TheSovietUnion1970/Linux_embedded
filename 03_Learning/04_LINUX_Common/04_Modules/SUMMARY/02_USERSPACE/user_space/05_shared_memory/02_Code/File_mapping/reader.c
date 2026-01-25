#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SHARED_MEM_SIZE  4096
#define FILE_PATH        "shared_data.dat"

int main() {
    // 1. Open EXISTING file (no O_CREAT needed)
    int fd = open(FILE_PATH, O_RDWR);  // ← O_RDWR even for pure reader
    if (fd == -1) {
        perror("open (file not found?)");
        printf("Run writer first!\n");
        return 1;
    }

    // 2. NO ftruncate() - don't resize existing file!

    // 3. Map it
    char *data = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap reader");
        return 1;
    }

    // 4. Read (data persists from writer!)
    printf("Reader (PID %d): Read  '%s'\n", getpid(), data);

    // Optional: modify (visible to future readers!)
    strcat(data, " <-- reader modified!");

    //printf("Reader modified it to: '%s'\n", data);

    // 5. Cleanup
    munmap(data, SHARED_MEM_SIZE);
    close(fd);

    // Optional: rm file when done
    unlink(FILE_PATH);  // Comment out to keep file
    return 0;
}