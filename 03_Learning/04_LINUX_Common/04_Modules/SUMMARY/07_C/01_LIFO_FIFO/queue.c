#include <stdio.h>
#include <stdlib.h>

// Node structure
struct Node {
    int data;
    struct Node* next;
};

struct Node* front = NULL;
struct Node* rear = NULL;

// Enqueue (insert element)
void enqueue(int value) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));

    if (!newNode) {
        printf("Queue Overflow\n");
        return;
    }

    // always for block setup
    newNode->data = value;
    newNode->next = NULL;

    // for the first block created
    if (rear == NULL) {
        front = rear = newNode;
        return;
    }

    // only for the second block now on
    rear->next = newNode;
    rear = newNode;
}

// Dequeue (remove element)
int dequeue() {
    if (front == NULL) {
        printf("Queue Underflow\n");
        return -1;
    }

    struct Node* temp = front;
    int value = temp->data;

    front = front->next;

    // check if the next block is NULL or not
    if (front == NULL)
        rear = NULL;

    free(temp);
    return value;
}

// Display queue
void queue_display() {
    struct Node* temp = front;

    if (temp == NULL) {
        printf("Queue is empty\n");
        return;
    }

    printf("Queue elements: ");
    while (temp != NULL) {
        printf("%d ", temp->data);
        temp = temp->next;
    }
    printf("\n");
}

// Free all nodes
void queue_safe_free() {
    struct Node* temp;

    while (front != NULL) {
        temp = front;
        front = front->next;
        free(temp);
    }
    rear = NULL;
}