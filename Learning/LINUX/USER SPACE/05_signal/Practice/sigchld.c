#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Signal handler for SIGCHLD
void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // // Reap all terminated child processes
    // while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    //     printf("Child process %d terminated.\n", pid);
    // }

    printf("sigchld_handler here -> child process is done\n");
}

int main() {
    // Register SIGCHLD handler
    signal(SIGCHLD, sigchld_handler);

    pid_t pid = fork();
    
    if (pid == 0) { 
        // Child process
        printf("Child process (PID: %d) is running...\n", getpid());
        sleep(2); // Simulate some work
        printf("Child process (PID: %d) exiting.\n", getpid());
        exit(0);
    } else if (pid > 0) { 
        // Parent process
        printf("Parent process (PID: %d) waiting for child to terminate...\n", getpid());
        
        // Keep the parent process running to receive SIGCHLD
        // while (1) {
        //     sleep(1);
        // }
    } else {
        perror("fork failed");
        exit(1);
    }

    return 0;
}
