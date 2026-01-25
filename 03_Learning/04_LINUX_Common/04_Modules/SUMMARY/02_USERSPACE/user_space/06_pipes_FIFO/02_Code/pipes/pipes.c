#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
fd[0] -> read
fd[1] -> write
*/

int main() {
    int fd[2];
    pid_t pid;
    char buffer[100];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } 
    else if (pid == 0) {  // Child process for read
        close(fd[1]); // Close write end
        read(fd[0], buffer, sizeof(buffer));
        printf("Child received: %s\n", buffer);
        close(fd[0]);
    } 
    else {  // Parent process for write
        close(fd[0]); // Close read end
        char msg[] = "Hello from parent!";
        write(fd[1], msg, strlen(msg)+1);
        close(fd[1]);
    }

    return 0;
}