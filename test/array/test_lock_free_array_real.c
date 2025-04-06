#include "array.h"
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
    LockFreeArray* array;
    int threadID;
} arrayStruct;


void* threadMixedOperations(void* arg)
{
    arrayStruct* arr = (arrayStruct*)arg;
    LockFreeArray* array = arr->array;
    int threadID = arr->threadID;

    for (int i = 0; i < TOTAL_OPERATIONS_PER_THREAD; i++) 
    {
        int op = rand() % 100;

        if (op < 15) 
        {
            // push operations, probability 15%
            int* val = malloc(sizeof(int));
        	*val = threadID * 100000 + i;
            printf("Thread %d: Push %d\n", threadID, *val);
            assert(arrayPush(array, val) == 0);
        } 
        else if (op < 20)
        {
            // pop operations, probability 5%
            int* popped = arrayPop(array);
            if (popped)
            {
                printf("Thread %d: Pop %d\n", threadID, *popped);
                free(popped);
            }
        } 
        else if (op < 30)
        {
            // write operations, probability 10%
            int* val = malloc(sizeof(int));
        	*val = threadID * 100000 + i;
            size_t size = arraySize(array);
            if (size == 0)
            {
    			free(val);
    			continue; 
			}
			int index = rand() % size;
            printf("Thread %d: Write %d to index %d\n", threadID, *val, index);
            assert(arrayWrite(array, index, val) == 0);
        } 
        else {
            // read operations, probability 70%
            size_t size = arraySize(array);
			if (size == 0) {
				continue;
			}
            int index = rand() % size;
            int* readVal = arrayRead(array, index);
            if (readVal)
            {
            	printf("Thread %d: Read index %d = %d\n", threadID, index, *readVal);
            }
        }
    }

    return NULL;
}

int main()
{
    srand(time(NULL));
    LockFreeArray array;
    arrayInit(&array);
//     arrayReserve(&array, 1024);

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

    for (int i = 0; i < NUM_THREADS; i++)
    {
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
