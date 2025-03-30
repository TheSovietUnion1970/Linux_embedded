#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sig_handler1(int signum) {
    printf("Im signal handler1\n");
    exit(EXIT_SUCCESS);
}

int main() {
    sigset_t new_set, old_set;

    // Register SIGINT handler
    if (signal(SIGINT, sig_handler1) == SIG_ERR) {
        fprintf(stderr, "Cannot handle SIGINT\n");
        exit(EXIT_FAILURE);
    }

    sigemptyset(&old_set);
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGINT);  // Add SIGINT to new_set

    // Block SIGINT
    if (sigprocmask(SIG_BLOCK, &new_set, &old_set) == 0) {
        printf("SIGINT is now blocked. Try pressing Ctrl+C...\n");
    }

    // Sleep while SIGINT is blocked
    sleep(5);

    // Unblock SIGINT (restore old mask)
    printf("Unblocking SIGINT now.\n");
    sigprocmask(SIG_SETMASK, &old_set, NULL);

    while (1) {
        printf("Running...\n");
        sleep(2);
    }

    return 0;
}
