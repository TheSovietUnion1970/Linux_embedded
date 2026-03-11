#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

// Define node structure
struct Node {
    int data;
    struct Node* prev;
};

struct Node* top = NULL;

void stack_push(int value){
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));

    if (!newNode){
        printf("Stack overflow\n");
        return;
    }

    newNode->data = value;
    newNode->prev = top;
    top = newNode;
}

int stack_pop(){
    if (!top){
        printf("Stack underflow\n");
        return -1;
    }

    struct Node* tmp = top;
    int tmp_data;

    top = top->prev;
    tmp_data = tmp->data;
    free(tmp);

    return tmp_data;
}

void stack_display(){
    printf("\n=== Display:\n");
    struct Node* tmp;
    tmp = top;
    while(tmp){
        printf("# %d\n", tmp->data);
        tmp = tmp->prev;
    }
    printf("=== End\n\n");
}

void stack_safe_free(){
    struct Node* tmp;

    while(top != NULL){
        tmp = top;
        top = top->prev;
        free(tmp);
    }
}
