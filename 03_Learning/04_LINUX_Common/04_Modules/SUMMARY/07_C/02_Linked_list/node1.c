#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node* next;
};

struct Node* new_node(int data){
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));

    node->data = data;
    node->next = NULL;

    return node;
}

void add_node(struct Node* head, int data){
    while(head->next){
        head = head->next;
    }
    head->next = new_node(data);
}

void add_link(struct Node* head, struct Node* head1){
    while(head->next){
        head = head->next;
    }
    head->next = head1;
}

void print_list(struct Node* head){
    while(head){
        printf("[%d] -> ", head->data);
        head = head->next;
    }
    printf("NULL\n");
}

void safe_free(struct Node* head){

    struct Node* tmp;
    while(head){
        tmp = head;
        head = head->next;
        free(tmp);
        tmp = NULL;
    }
}

void safe_free_NfromStart(struct Node* head, int n){

    struct Node* tmp;
    int i = 0;
    while(i < n){
        tmp = head;
        head = head->next;
        free(tmp);
        tmp = NULL;
        i++;
    }
}

void safe_free_cycle(struct Node* head){
    struct Node* tmp;
    struct Node* start = head;
    int loop_done = 0;
    while(head != start || !loop_done){
        //printf("head = 0x%x\n", head);
        tmp = head;
        head = head->next;
        free(tmp);
        tmp = NULL;
        loop_done = 1;
    }
}

struct Node* reverse_list(struct Node* head){
    struct Node* next = NULL;
    struct Node* prev = NULL;
    struct Node* cur = head;

    while(cur){
        next = cur->next;
        cur->next = prev;
        prev = cur;

        cur = next;
    }

    return prev; // return new head
}

int hasCycle(struct Node* head){
    struct Node* tail = head;
    while(tail->next){
        tail = tail->next;
        //printf();
        if (tail == head){
            return 1;
        }
    }
    return 0;
}

int markCycle(struct Node* head){
    struct Node* tail = head;
    int isCycle = hasCycle(head);
    if (isCycle == 0){
        while(tail->next){
            tail = tail->next;
        }
        tail->next = head;
    }
    else {
        printf("Already cycle\n");
        return -1;
    }
    return 0;
}

void Clean(struct Node* head){
    if (!head) return;
    if (hasCycle(head)) safe_free_cycle(head);
    else safe_free(head);
}

struct Node* merge(struct Node* l1, struct Node* l2) {
    struct Node* new;
    struct Node* new_head;

    if (l1->data < l2->data){
        new = l1;
        l1 = l1->next;
    }
    else{
        new = l2;
        l2 = l2->next;       
    }
    new_head = new;
    //new = new->next;  

    while(l1 && l2){
        if (l1->data < l2->data){
            //printf("l1\n");
            new->next = l1;
            l1 = l1->next;
        }
        else{
            //printf("l2\n");
            new->next = l2;
            l2 = l2->next;       
        } 
        //printf("%d\n", new->data); 
        new = new->next;  
    }

    // update the rest
    new->next = (l1) ? l1 : l2;
    return new_head;
}

int GetSize(struct Node* head){
    struct Node* tmp;
    tmp = head;
    int i = 0;
    while(tmp){
        tmp = tmp->next;
        i++;
    }
    return i;
}

struct Node* removeNth(struct Node* head, int n) {
    int sz = GetSize(head);
    int pos = sz - n;
    int i = 0;
    struct Node* tmp = head;
    struct Node* prev = NULL;
    printf("pos = %d, sz = %d\n", pos, sz);
    if (pos < 0 || pos > sz){
        return NULL;
    }
    
    tmp = head;

    if (pos){
        while(i < pos){
            prev = tmp;
            tmp = tmp->next;
            i++;
        }

        prev->next = tmp->next;
    }
    else {
        printf("ELSE\n");
        head = tmp->next;
    }

    free(tmp);
    return head;

    
}

int searchValFromNull(struct Node* head, int val){
    int sz = GetSize(head);
    struct Node* tmp = head;
    int i = 0;

    while(i < sz){
        if (tmp->data == val){
            return sz - i;
            //break;
        }

        tmp = tmp->next;
        i++;
    }

    return -1;

}

struct Node* deleteNode(struct Node* head, int val){
    int idx = searchValFromNull(head, val);
    //printf("idx = %d\n", idx);
    if (idx > 0) head = removeNth(head, idx);
    //printf("head = %x\n", head);
    return head;
}

/* === Optimized === */
struct Node* removeNthFromEnd(struct Node* head, int n) {
    struct Node* dummy = (struct Node*)malloc(sizeof(struct Node));
    dummy->next = head;        // dummy -> real head
    
    struct Node* fast = dummy;
    struct Node* slow = dummy;
    
    // Move fast n steps ahead
    int i = 0;
    while (i < n){
        fast = fast->next;
        i++;
    }
    
    // Move both until fast reaches end
    while (fast->next != NULL) {
        fast = fast->next;
        slow = slow->next;
    }
    
    // Remove the node
    struct Node* to_delete = slow->next;
    slow->next = slow->next->next;
    free(to_delete);
    
    head = dummy->next;   // Update real head
    free(dummy);
    
    return head;
}

struct Node* getNthNode(struct Node* head, int n){
    struct Node* dummy = (struct Node*)malloc(sizeof(struct Node));
    dummy->next = head;

    struct Node* fast = dummy;
    struct Node* slow = dummy;

    int i = 0;
    while(i < n){
        fast = fast->next;
        i++;
    }

    while(fast->next){
        fast = fast->next;
        slow = slow->next;
    }

    free(dummy);

    return slow->next;
}

struct Node* findMiddle(struct Node* head) {
    struct Node *slow = head, *fast = head;

    while (fast && fast->next && fast->next->next && fast->next->next->next) {
        slow = slow->next;
        fast = fast->next->next->next->next;
    }
    //printf("fast = 0x%x\n", fast);
    return slow;
}

struct Node *getIntersectionNode(struct Node *headA, struct Node *headB) {
    if (headA == NULL || headB == NULL) return NULL;
    
    struct Node *a = headA;
    struct Node *b = headB;
    
    while (a != b) {
        a = a->next;
        b = b->next;
    }
    
    return a;  // Either intersection node or NULL
}

int main(){
    struct Node* headA = new_node(0);
    add_node(headA, 2);
    add_node(headA, 4);

    struct Node* headB = new_node(1);
    add_node(headB, 3);
    add_node(headB, 5);

    // add_node(headA, 10);
    // add_link(headB, getNthNode(headA, 1));
    // add_node(headA, 11);
    // add_node(headA, 12);

    print_list(headA);
    print_list(headB);

    struct Node* head1 = getIntersectionNode(headA, headB);
    print_list(head1);


    Clean(headA);
    Clean(headB);
    //safe_free_NfromStart(headB, 3);
}