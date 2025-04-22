#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // For O_* constants
#include <sys/stat.h>   // For mode constants
#include <mqueue.h>

#define QUEUE_NAME  "/myqueue"
#define MAX_SIZE    1024

typedef struct queue {
    int priority;
    char data[100];
} msg_queue;

int main() {
    mqd_t mq;
    char buffer[MAX_SIZE];
    struct mq_attr attr;

    // Set queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;     // Maximum number of messages
    attr.mq_msgsize = MAX_SIZE; // Max size of each message
    attr.mq_curmsgs = 0;     // Number of current messages (ignored for mq_open)
    unsigned int priority;

    //mq_unlink(QUEUE_NAME); // Unlink old queue first!

    // Open or create the message queue
    mq = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }

    msg_queue msg1, msg2, msg3;

    msg1.priority = 5;
    strncpy(msg1.data, "Normal report", sizeof("Normal report"));

    msg2.priority = 10;
    strncpy(msg2.data, "Emergency report", sizeof("Emergency report"));

    msg3.priority = 4;
    strncpy(msg3.data, "Low report", sizeof("Low report"));

    // Send the message with the chosen priority
    if (mq_send(mq, msg1.data, strlen(msg1.data) + 1, msg1.priority) == -1) {
        perror("mq_send");
        exit(1);
    }
    printf("Message1 sent with priority %u!\n", msg1.priority);

    if (mq_send(mq, msg2.data, strlen(msg2.data) + 1, msg2.priority) == -1) {
        perror("mq_send");
        exit(1);
    }
    printf("Message3 sent with priority %u!\n", msg2.priority);

    if (mq_send(mq, msg3.data, strlen(msg3.data) + 1, msg3.priority) == -1) {
        perror("mq_send");
        exit(1);
    }
    printf("Message3 sent with priority %u!\n", msg3.priority);

    mq_close(mq);  // Close the queue
    return 0;
}
