#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ==================== Kernel-style List Head ====================
struct Node {
    int data;
    struct Node *next;
    struct Node *prev;
};

struct Node* CreateNode(int data){
    struct Node* head = (struct Node *)malloc(sizeof(*head));

    head->data = data;

    head->next = head;
    head->prev = head;

    return head;
}

void safe_free(struct Node* head){
    struct Node* tmp = head->next;
    struct Node* tmp1;
    while(head != tmp){
        printf("FREE 0x%x\n", tmp);
        tmp1 = tmp;
        tmp = tmp->next;
        free(tmp1);
    }

    printf("0x%x - 0x%x\n", tmp, head);
    free(tmp);
}

void add_list_head(struct Node* head, int data){
    struct Node* node = (struct Node *)malloc(sizeof(*node));
    node->data = data;

    node->next = head->next;
    node->prev = head;

    head->next->prev = node; // must be 1st
    head->next = node; // 2nd
}

void add_list_tail(struct Node* head, int data){
    struct Node* node = (struct Node *)malloc(sizeof(*node));
    node->data = data;

    node->next = head;
    node->prev = head->prev;

    head->prev->next = node; // must be 1st
    head->prev = node; // 2nd
}

bool list_del(struct Node* head, int data){
    struct Node* tmp = head->prev;
    struct Node* to_delete;
    bool flag = false;
    do {
        if (tmp->next->data == data){
            to_delete = tmp->next;
            flag = true;
            break;
        }
        tmp = tmp->next;
    } while(tmp->next != head);

    if (flag == true){
        tmp->next->next->prev = tmp;
        tmp->next = tmp->next->next;
        free(to_delete);
    }

    return flag;
}

void print_list(struct Node* head){
    struct Node* tmp = head;

    do{
        printf("[%d] -> ", tmp->data);
        tmp = tmp->next;
    } while(tmp != head);
    printf("[%d]\n", tmp->data);

}

int main(){
    struct Node* head = CreateNode(0);
    add_list_head(head, 1);
    add_list_head(head, 2);
    add_list_tail(head, 3);

    print_list(head);

    list_del(head, 1);
    print_list(head);

    safe_free(head);
}