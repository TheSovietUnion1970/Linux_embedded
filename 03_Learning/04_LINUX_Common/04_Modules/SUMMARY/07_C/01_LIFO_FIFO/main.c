#include "stack.h"
#include "queue.h"
#include <stdio.h>

int main(){
    // stack_push(10);
    // stack_push(20);
    // stack_push(30);
    // stack_push(40);
    // stack_push(50);
    // stack_display();

    // for (int i = 0; i < 3; i++){
    //     printf("Pop: %d\n", stack_pop());
    // }

    // stack_display();

    // stack_safe_free();


    enqueue(10);
    enqueue(20);
    enqueue(30);

    queue_display();

    printf("Removed: %d\n", dequeue());
    queue_display();

    queue_safe_free();
    return 0;
}