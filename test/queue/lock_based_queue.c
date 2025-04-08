#include "lock_based_queue.h"
#include <stdlib.h>
#include <pthread.h>

void qInit(LockBasedQueue* queue)
{
    Node* dummy = (Node*)malloc(sizeof(Node));
    dummy->value = NULL;
    dummy->next = NULL;

    queue->head = dummy;
    queue->tail = dummy;
    pthread_mutex_init(&queue->enqLock, NULL);
    pthread_mutex_init(&queue->deqLock, NULL);
}

void qEnqueue(LockBasedQueue* queue, void* value)
{
    Node* node = (Node*)malloc(sizeof(Node));
    node->value = value;
    node->next = NULL;

    pthread_mutex_lock(&queue->enqLock);
    queue->tail->next = node;
    queue->tail = node;
    pthread_mutex_unlock(&queue->enqLock);
}

void* qDequeue(LockBasedQueue* queue)
{
    void* result = NULL;

    pthread_mutex_lock(&queue->deqLock);
    Node* next = queue->head->next;
    if (next == NULL)
    {
        // Queue is empty
        result = NULL;
    } 
    else
    {
        result = next->value;
        Node* old_head = queue->head;
        queue->head = next;
        free(old_head); // Safe to free dummy
    }
    pthread_mutex_unlock(&queue->deqLock);

    return result;
}

void qDestroy(LockBasedQueue* queue)
{
    // Clean up remaining nodes
    while (queue->head != NULL)
    {
        Node* temp = queue->head;
        queue->head = queue->head->next;
        free(temp);
    }
    pthread_mutex_destroy(&queue->enqLock);
    pthread_mutex_destroy(&queue->deqLock);
}
