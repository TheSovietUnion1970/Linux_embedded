#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>

int main() {
    struct pollfd fds[1];  
    fds[0].fd = STDIN_FILENO;  // Monitor standard input (keyboard)
    fds[0].events = POLLIN;    // Wait for data to be available for reading

    while (1) {
        printf("Waiting for input (press any key)...\n");

        int poll_count = poll(fds, 1, -1);  // Block indefinitely

        if (poll_count > 0) {
            if (fds[0].revents & POLLIN) {  
                printf("Input detected!\n");

                // Reset `revents` manually before the next `poll()` call
                //fds[0].revents = 0;

                // Read and discard input to prevent repeated triggers
                char buffer[256];
                read(STDIN_FILENO, buffer, sizeof(buffer));
            }
        } else {
            perror("poll()");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
