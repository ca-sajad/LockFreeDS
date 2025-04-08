#include "queue.h"
#include <stdlib.h>
#include <stdatomic.h>

void qInit(LockFreeQueue* queue)
{
    Node* dummy = (Node*)malloc(sizeof(Node));
    dummy->value = NULL;
    atomic_store(&dummy->next, NULL);
    atomic_store(&queue->head, dummy);
    atomic_store(&queue->tail, dummy);
}

void qEnqueue(LockFreeQueue* queue, void* value)
{
    Node* node = (Node*)malloc(sizeof(Node));
    node->value = value;
    atomic_store(&node->next, NULL);

    while (1)
    {
        Node* last = atomic_load(&queue->tail);
        Node* next = atomic_load(&last->next);
        if (last == atomic_load(&queue->tail))
        {
            if (next == NULL)
            {
                if (atomic_compare_exchange_weak(&last->next, &next, node))
                {
                    atomic_compare_exchange_weak(&queue->tail, &last, node);
                    return;
                }
            } 
            else
            {
                atomic_compare_exchange_weak(&queue->tail, &last, next);
            }
        }
    }
}

void* qDequeue(LockFreeQueue* queue)
{
    while (1)
    {
        Node* first = atomic_load(&queue->head);
        Node* last = atomic_load(&queue->tail);
        Node* next = atomic_load(&first->next);

        if (first == atomic_load(&queue->head))
        {
            if (first == last)
            {
                if (next == NULL)
                {
                    return NULL; // Queue is empty
                }
                atomic_compare_exchange_weak(&queue->tail, &last, next);
            } 
            else
            {
                void* value = next->value;
                if (atomic_compare_exchange_weak(&queue->head, &first, next))
                {
                    free(first); // safe to reclaim the old dummy
                    return value;
                }
            }
        }
    }
}
