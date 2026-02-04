#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int main() {
    printf("Parent process (PID %d) is about to fork...\n", getpid());

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // === This is the child process ===

        printf("Child process (PID %d) starting...\n", getpid());
        printf("Child will now call exec() to become /bin/objdump\n");
        printf("→ Anything printed after this line will NOT come from this program!\n\n");

        // Prepare arguments for objdump (equivalent to running: objdump -t)
        char *args[] = {"objdump_name", "-t", "ex", NULL};

        // execvp() replaces the current process image with /bin/objdump
        execvp("objdump", args);

        // --- This line is ONLY reached if execvp() FAILS ---
        printf("ERROR here\n");
        perror("execvp failed");
        exit(1);  // Exit child only — parent continues
    }

    // === This is the parent process ===
    printf("Parent (PID %d) created child with PID %d\n", getpid(), pid);

    // Wait for child to finish
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("\nChild exited with status %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("\nChild was killed by signal %d\n", WTERMSIG(status));
    }

    printf("Parent: all done.\n");

    return 0;
}