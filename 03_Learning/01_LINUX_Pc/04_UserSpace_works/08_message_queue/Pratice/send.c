#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define QUEUE_NAME  "/myqueue"
#define MAX_SIZE    1024

int main() {
    mqd_t mq;
    char buffer[MAX_SIZE];
    struct mq_attr attr;

    // Set queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;     // Maximum number of messages
    attr.mq_msgsize = MAX_SIZE; // Max size of each message
    attr.mq_curmsgs = 0;     // Number of current messages (ignored for mq_open)

    //mq_unlink(QUEUE_NAME); // Unlink old queue first!

    // Open or create the message queue
    mq = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }

    printf("Enter a message: ");
    fgets(buffer, MAX_SIZE, stdin);

    // Send the message
    if (mq_send(mq, buffer, strlen(buffer) + 1, 20) == -1) {
        perror("mq_send");
        exit(1);
    }


    printf("Enter a message: ");
    fgets(buffer, MAX_SIZE, stdin);
    // Send the message
    if (mq_send(mq, buffer, strlen(buffer) + 1, 0) == -1) {
        perror("mq_send");
        exit(1);
    }


    printf("Message sent!\n");

    mq_close(mq);  // Close the queue
    return 0;
}
