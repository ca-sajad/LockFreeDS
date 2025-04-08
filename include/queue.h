#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>

typedef struct Node
{
    void* value;
    _Atomic(struct Node*) next;
} Node;

typedef struct
{
    _Atomic(Node*) head;
    _Atomic(Node*) tail;
} LockFreeQueue;

void qInit(LockFreeQueue* queue);
void qEnqueue(LockFreeQueue* queue, void* value);
void* qDequeue(LockFreeQueue* queue); // Returns NULL if empty

#endif // LOCK_FREE_QUEUE_H
