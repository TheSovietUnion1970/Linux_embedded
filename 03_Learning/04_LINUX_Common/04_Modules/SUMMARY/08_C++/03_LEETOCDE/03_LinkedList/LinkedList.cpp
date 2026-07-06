#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <queue>

// Definition for singly-linked list.
struct ListNode {
    int val;
    struct ListNode *next;
};

// Helper function to create a linked list from array
struct ListNode* createList(std::vector<int> arr) {
    if (arr.empty()) return nullptr;
    
    ListNode* head = new ListNode{arr[0]};
    ListNode* current = head;
    
    for (size_t i = 1; i < arr.size(); ++i) {
        current->next = new ListNode{arr[i]};
        current = current->next;
    }
    return head;
}

// Helper to print linked list
void printList(struct ListNode* head) {
    while (head) {
        printf("%d ", head->val);
        head = head->next;
    }
    printf("\n");
}

// Helper to free list
void freeList(struct ListNode* head) {
    while (head) {
        struct ListNode* temp = head;
        head = head->next;
        free(temp);
    }
}

// ========================== Main func ================================
/*
    1.  IN  : 2 -> 4 -> 3
              5 -> 6 -> 4
        OUT : 7 -> 0 -> 8
      -> 342 + 465 = 807
    2. carry = sum / 10 ("Nhớ 1, nhớ 2, ...")
       sum = (carry (next turn) + l1 + l2) % 10
*/
struct ListNode* addTwoNumbers(struct ListNode* l1, struct ListNode* l2){
    struct ListNode dummy;
    struct ListNode* dummy_ptr = &dummy;
    
    int sum = 0, carry = 0;
    while (l1 or l2 or carry){
        sum = carry;

        if (l1){
            sum += l1->val;
            l1 = l1->next;
        }

        if (l2){
            sum += l2->val;
            l2 = l2->next;
        }

        carry = sum / 10; // take the second num if > 0

        struct ListNode* cur = new ListNode{sum % 10};
        cur->next = NULL;

        dummy_ptr->next = cur;
        dummy_ptr = cur;
    }

    return dummy.next;
}

/*
    1.  IN  : 1 -> 2 -> 3
              1 -> 4 -> 7 -> 9
              2 -> 5
        OUT : 1 1 2 2 3 4 5 7 9
    2. -> priority_queue (IN1[0], IN2[0], IN3[0])
       -> priority_queue (IN1[1], IN2[0], IN3[0]), OUT: IN1[0]
       -> priority_queue (IN1[1], IN2[1], IN3[0]), OUT: IN1[0], IN2[0]
       ...
*/
struct Compare {
        bool operator()(ListNode* a, ListNode* b) {
            return a->val > b->val;   // Min-heap
        }
    };

struct ListNode* mergeKLists(std::vector<ListNode*>& lists) {
    if (lists.empty()) return nullptr;

    // Min-heap
    std::priority_queue<ListNode*, std::vector<ListNode*>, Compare> pq;

    // Add first node of each list
    for (ListNode* list : lists) {
        if (list) {
            pq.push(list);
        }
    }

    ListNode* dummy = new ListNode{0,nullptr};
    ListNode* tail = dummy;

    while (!pq.empty()) {
        ListNode* smallest = pq.top();
        pq.pop();

        tail->next = smallest;
        tail = tail->next;

        if (smallest->next) {
            pq.push(smallest->next);
        }
    }

    ListNode* result = dummy->next;
    delete dummy;
    return result;
}

/*
    1.  IN  : 1,2,3,4,5     
    
        k = 2
        OUT : 2,1,4,3,5
    2. GlobalPrev -> dummy
       First -> head of k = 1

       GlobalPrev -> First (head of k = 1)
       First -> head of k = 2
       ...
*/
struct ListNode* reverseKNodes(struct ListNode* head, int k){
    struct ListNode* prev = NULL;
    struct ListNode* next = NULL;
    struct ListNode* curr = head;

    struct ListNode* first = head;
    int cnt = 0;

    struct ListNode dummy;
    prev = &dummy;
    struct ListNode* GlobPrev = &dummy;

    while(head){
        cnt = 0;
        while (curr && cnt < k){
            curr = curr->next;
            cnt++;
        } // output: curr -> the new head of k + 1

        if (cnt < k) break; // end of liked list

        struct ListNode* first = head;
        for (int i = 0; i < k; i++){
            next = head->next;
            head->next = prev;
            prev = head;
            head = next;
        } // output: prev -> end of k, head = curr = next (new head of k + 1)
        //printf("next = %x\n", next);

        // connect
        // connect to end of k
        GlobPrev->next = prev;
        // connect to new head of k + 1 -> needed here as prev of the new head 
                            // is changed from next to the prev 
        first->next = curr; // or head
        // update new GlobalPrev
        GlobPrev = first;
    }

    return dummy.next;
 
}

/*
    1.  IN  : 1,2,3,4,5     
    
        l = 0. r = 2
        OUT : 3,2,1,4,5    
    2. GlobalPrev -> dummy 
       First -> head of l = 0

       or

       GlobalPrev -> before head
       First -> head of l (!= 0)
      
*/
struct ListNode* reverseRLNodes(struct ListNode* head, int l, int r){
    struct ListNode dummy;
    struct ListNode* prev = &dummy;
    struct ListNode* curr = head;
    struct ListNode* next = NULL;
    prev->next = head;

    struct ListNode* GlobalPrev;
    struct ListNode* First;


    for (int i = 0; i < l; i++){
        prev = prev->next;
    } // prev points to the starting node or dummy if l = 0

    curr = prev->next;
    GlobalPrev = prev;
    First = curr;

    for (int i = 0; i < r - l + 1; i++){
        next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }

    GlobalPrev->next = prev;
    First->next = curr;

    return dummy.next;

}

/*
    1.  IN  : 1,2,3,4,5     
    
        Kth = 1
        OUT : 1,4,3,2,5     
    2. Find left, right
*/
struct ListNode* SwapNodes(struct ListNode* head, int Kth){
    struct ListNode* left = head, *right = head;
    struct ListNode* tmp;
    int cnt = 0;

    struct ListNode dummy;
    tmp = &dummy;
    tmp->next = head;
    tmp = tmp->next;
    struct ListNode* prev_left = &dummy, *prev_right;

    while(tmp){
        tmp = tmp->next;
        cnt++;
    }

    for (int i = 0; i < Kth; i++){
        prev_left = left;
        left = left->next;
    }
    for (int i = 0; i < cnt - Kth - 1; i++){
        prev_right = right;
        right = right->next;
    }

    prev_left->next = right;
    prev_right->next = left;

    tmp = right->next;
    right->next = left->next;
    left->next = tmp;

    return dummy.next;
}

int main() {
    // Example 1: 342 + 465 = 807
    std::cout << "########## Example 1 ########" << std::endl;
    std::vector<int> arr1 = {2,4,3};
    std::vector<int> arr2 = {5,6,4};
    struct ListNode* l1 = createList(arr1);
    struct ListNode* l2 = createList(arr2);
    struct ListNode* l3;

    l3 = addTwoNumbers(l1, l2);
    std::cout << "L3 Result: ";
    printList(l3);
    
    // Clean up
    freeList(l1);
    freeList(l2);
    freeList(l3);

    // Example 2:
    std::cout << "########## Example 2 ########" << std::endl;
    std::vector<ListNode*> lists;
    lists.push_back(createList({1,2,3}));
    lists.push_back(createList({1,4,7,9}));
    lists.push_back(createList({2,5}));

    std::cout << "List 1: "; printList(lists[0]);
    std::cout << "List 2: "; printList(lists[1]);
    std::cout << "List 3: "; printList(lists[2]);

    ListNode* result = mergeKLists(lists);
    std::cout << "Merged Result: ";
    printList(result);

    // Clean up
    freeList(result);

    std::cout << "########## Example 3 ########" << std::endl;
    l1 = createList({1,2,3,4,5});
    l1 = reverseKNodes(l1, 2);
    std::cout << "reverseKNodes: ";
    printList(l1);
    freeList(l1);

    std::cout << "########## Example 4 ########" << std::endl;
    l1 = createList({1,2,3,4,5});
    l1 = reverseRLNodes(l1, 0, 2);
    std::cout << "reverseRLNodes: ";
    printList(l1);
    freeList(l1);

    std::cout << "########## Example 5 ########" << std::endl;
    l1 = createList({1,2,3,4,5});
    l1 = SwapNodes(l1, 1);
    std::cout << "SwapNodes: ";
    printList(l1);
    freeList(l1);
    
    return 0;
}
