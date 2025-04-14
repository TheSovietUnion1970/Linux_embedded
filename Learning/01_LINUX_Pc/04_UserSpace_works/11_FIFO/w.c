#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    const char *fifo = "/tmp/myfifo";
    mkfifo(fifo, 0666);

    int fd = open(fifo, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    char msg[] = "Hello through FIFO!";
    ssize_t n = write(fd, msg, strlen(msg) + 1);
    if (n == -1) {
        perror("write");
    } else {
        printf("Wrote %ld bytes\n", n);
    }

    close(fd);
    return 0;
}
