#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // For O_* constants
#include <sys/stat.h>   // For mode constants
#include <mqueue.h>
#include <errno.h>      // For errno

#define QUEUE_NAME  "/myqueue"
#define MAX_SIZE    1024

int main() {
    mqd_t mq;
    char buffer[MAX_SIZE];
    struct mq_attr attr;
    ssize_t bytes_read = -1;

    // Set queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;     // Maximum number of messages
    attr.mq_msgsize = MAX_SIZE; // Max size of each message
    attr.mq_curmsgs = 0;     // Number of current messages (ignored for mq_open)
    unsigned int priority = 5;

    //mq_unlink(QUEUE_NAME); // Unlink old queue first!

    // Open or create the message queue
    mq = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT | O_NONBLOCK, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }

    while (1){
        // Receive the message
        bytes_read = mq_receive(mq, buffer, MAX_SIZE, &priority);

        if (bytes_read >= 0){
            printf("Received message: %s\n", buffer);
            printf("Message priority: %u\n", priority);
        }
        else {
            if (errno == EAGAIN){
                printf("There is no data here\n");
            }
            else {
                perror("mq_receive");
            }
            exit(1);
        }

        //bytes_read = 0;
    }

    mq_close(mq);           // Close the queue
    mq_unlink(QUEUE_NAME);  // Remove the queue

    return 0;
}
