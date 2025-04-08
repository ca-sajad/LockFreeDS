#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "lock_based_queue.h"

#define NUM_THREADS     4
#define ITERATION_COUNT 150   // Use 400 for higher workload
#define TOTAL_ITEMS     1000000

LockBasedQueue queue;

// Spin work simulation
void do_other_work()
{
    for (int i = 0; i < ITERATION_COUNT; ++i)
    {
        __asm__ volatile("nop");
    }
}

// Worker thread function
void* workerFunction(void* arg)
{
    long items_per_thread = TOTAL_ITEMS / NUM_THREADS;
    for (long i = 0; i < items_per_thread; ++i)
    {
        long* value = malloc(sizeof(long));
        *value = i;
        qEnqueue(&queue, value);
        do_other_work();
        void* dequeued = qDequeue(&queue);
        do_other_work();
        if (dequeued) free(dequeued);
    }
    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];

    qInit(&queue);

    printf("Starting benchmark with %d threads, %d iteration count, %d total items...\n",
           NUM_THREADS, ITERATION_COUNT, TOTAL_ITEMS);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        pthread_create(&threads[i], NULL, workerFunction, NULL);
    }

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed_sec = end.tv_sec - start.tv_sec +
                         (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Benchmark completed in %.4f seconds.\n", elapsed_sec);

    return 0;
}
