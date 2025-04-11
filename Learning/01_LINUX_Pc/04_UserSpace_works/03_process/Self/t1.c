#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int a = 0;
    
    if (fd < 0) {
        perror("open");
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0) {  // Child process
        write(fd, "Child writing...\n", 17);
        printf("a = %d\n", ++a);

        close(fd);
    } else {  // Parent process
        wait(NULL);  // Wait for child to finish
        
        write(fd, "Parent writing...\n", 18);
        printf("a = %d\n", ++a);

        close(fd);
    }

    return 0;
}
