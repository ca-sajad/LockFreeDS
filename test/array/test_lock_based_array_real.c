#include "lock_based_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#define NUM_THREADS 32
#define TOTAL_OPERATIONS_PER_THREAD 100000

typedef struct 
{
    LockBasedArray* array;
    int threadID;
} arrayStruct;

pthread_mutex_t sizeMutex = PTHREAD_MUTEX_INITIALIZER;
size_t currentSize = 0;


void* threadMixedOperations(void* arg)
{
    arrayStruct* arr = (arrayStruct*)arg;
    LockBasedArray* array = arr->array;
    int threadID = arr->threadID;

    for (int i = 0; i < TOTAL_OPERATIONS_PER_THREAD; i++) {
        int op = rand() % 100;
        int* val = malloc(sizeof(int));
        *val = threadID * 100000 + i;

        if (op < 15)
        {
            // push operations, probability 15%
            printf("Thread %d: Push %d\n", threadID, *val);
            assert(arrayPush(array, val) == 0);
            pthread_mutex_lock(&sizeMutex);
            currentSize++;
            pthread_mutex_unlock(&sizeMutex);
        } 
        else if (op < 20)
        {
            // pop operations, probability 5%
            pthread_mutex_lock(&sizeMutex);
            if (currentSize > 0)
            {
                pthread_mutex_unlock(&sizeMutex);
                int* popped = arrayPop(array);
                if (popped)
                {
                    printf("Thread %d: Pop %d\n", threadID, *popped);
                    pthread_mutex_lock(&sizeMutex);
                    currentSize--;
                    pthread_mutex_unlock(&sizeMutex);
                    free(popped);
                }
            } else
            {
                pthread_mutex_unlock(&sizeMutex);
            }
            free(val);
        } 
        else if (op < 30)
        {
            // write operations, probability 10%
            pthread_mutex_lock(&sizeMutex);
    		size_t size = currentSize;
    		pthread_mutex_unlock(&sizeMutex);

    		if (size > 0)
    		{
        		size_t index = rand() % size;
        		int result = arrayWrite(array, index, val);
        		if (result == 0)
        		{
            		printf("Thread %d: Write %d to index %zu\n", threadID, *val, index);
        		} else 
        		{
            		free(val);
        		}
    		} else
    		{
        		free(val);
    		}
        } 
        else
        {
            // read operations, probability 70%
            pthread_mutex_lock(&sizeMutex);
            if (currentSize > 0)
            {
                int index = rand() % currentSize;
                pthread_mutex_unlock(&sizeMutex);
                int* readVal = arrayRead(array, index);
                if (readVal)
                    printf("Thread %d: Read index %d = %d\n", threadID, index, *readVal);
            } else
            {
                pthread_mutex_unlock(&sizeMutex);
            }
            free(val);
        }
    }

    return NULL;
}

int main()
{
    srand(time(NULL));
    LockBasedArray array;
    arrayInit(&array);

    pthread_t threads[NUM_THREADS];
    arrayStruct* arrs[NUM_THREADS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        arrs[i] = malloc(sizeof(arrayStruct));
        arrs[i]->threadID = i;
        arrs[i]->array = &array;
        pthread_create(&threads[i], NULL, threadMixedOperations, arrs[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        free(arrs[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    size_t finalSize = arraySize(&array);
    printf("\nFinal array size: %zu\n", finalSize);

    double elapsedSec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Execution time: %.6f seconds\n", elapsedSec);

    arrayDestroy(&array);
    printf("All mixed-operation tests passed!\n");

    return 0;
}
