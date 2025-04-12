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
    int priority;

    // Set queue attributes (should match sender)
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Open or create the message queue
    mq = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }

    printf("Wait the msg\n");
    // Receive the message
    if (mq_receive(mq, buffer, MAX_SIZE, &priority) == -1) {
        perror("mq_receive");
        exit(1);
    }

    printf("Received message: %s\n", buffer);

    mq_close(mq);           // Close the queue
    mq_unlink(QUEUE_NAME);   // Remove the queue

    return 0;
}
