#include <stdatomic.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

//node structure
typedef struct Node {
    void* value;
    _Atomic(struct Node*) next;
} Node;

//queue structure
typedef struct LockFreeQueue {
    _Atomic(Node*) head;
    _Atomic(Node*) tail;
} LockFreeQueue;

//queue initialization
void queue_init(LockFreeQueue* q) {
    Node* dummy = (Node*)malloc(sizeof(Node));
    dummy->value = NULL;
    atomic_init(&dummy->next, NULL);
    atomic_init(&q->head, dummy);
    atomic_init(&q->tail, dummy);
}

void enqueue(LockFreeQueue* q, void* value) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->value = value;
    atomic_init(&new_node->next, NULL);

    while (1) {
        Node* tail = atomic_load(&q->tail);
        Node* next = atomic_load(&tail->next);

        if (next == NULL) {
            if (atomic_compare_exchange_weak(&tail->next, &next, new_node)) {
                atomic_compare_exchange_weak(&q->tail, &tail, new_node);
                return;
            }
        } else {
            atomic_compare_exchange_weak(&q->tail, &tail, next);
        }
    }
}

void* dequeue(LockFreeQueue* q) {
    while (1) {
        Node* head = atomic_load(&q->head);
        Node* next = atomic_load(&head->next);

        if (next == NULL) {
            return NULL;  //queue is empty
        }

        if (atomic_compare_exchange_weak(&q->head, &head, next)) {
            void* value = next->value;
            free(head);  //free the old dummy node
            return value;
        }
    }
}

// Testing:

LockFreeQueue q;

void* producer(void* arg) {
    for (int i = 0; i < 100; i++) {
        int* val = malloc(sizeof(int));
        *val = i;
        enqueue(&q, val);
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < 100; i++) {
        int* val;
        while ((val = dequeue(&q)) == NULL) usleep(10);
        printf("Dequeued: %d\n", *val);
        free(val);
    }
    return NULL;
}

int main() {
    queue_init(&q);

    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    return 0;
}
