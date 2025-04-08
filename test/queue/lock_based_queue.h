#ifndef LOCK_BASED_QUEUE_H
#define LOCK_BASED_QUEUE_H

#include <pthread.h>
#include <stdbool.h>


typedef struct Node
{
    void* value;
    struct Node* next;
} Node;

typedef struct
{
    Node* head;
    Node* tail;
    pthread_mutex_t enqLock;
    pthread_mutex_t deqLock;
} LockBasedQueue;

void qInit(LockBasedQueue* queue);
void qEnqueue(LockBasedQueue* queue, void* value);
void* qDequeue(LockBasedQueue* queue); // Returns NULL if empty
void qDestroy(LockBasedQueue* queue);

#endif // LOCK_BASED_QUEUE_H
