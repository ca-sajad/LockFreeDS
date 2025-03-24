#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>


#define NUM_THREADS 5
#define OPERATIONS_PER_THREAD 5
#define ARRAY_SIZE (NUM_THREADS * OPERATIONS_PER_THREAD)

typedef struct 
{
    LockFreeArray* array;
    int threadID;
} arrayStruct;

int readArray[NUM_THREADS * OPERATIONS_PER_THREAD];


void* threadArrayPush(void* arg) 
{
	arrayStruct* arr = (arrayStruct*)arg;
    LockFreeArray* array = arr->array;
    int threadID = arr->threadID;
    
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) 
    {
        int* element = malloc(sizeof(int));
        *element = i + threadID*100;
        printf("Thread %d: Pushing %d\n", threadID, *element);
        assert(arrayPush(array, element) == 0);
	}
	
    return NULL;
}


int readFromArray(LockFreeArray array)
{
	int count = 0;
	
	for (int i = 0; i < ARRAY_SIZE; i++) 
	{
        printf("Reading index %d, ", i);
        int* readElement = (int*)arrayRead(&array, i);
        assert(readElement != NULL);
        printf("Read: %d\n", *readElement);
        // check if readElement has been read before
        for (int j = 0; j < i; j++) {
        	assert(readArray[j] != *readElement);
    	}
    	// add to readArray
    	readArray[count++] = *readElement;
    }
    
    return 0;
}

int popFromArray(LockFreeArray* array)
{
    int poppedArray[ARRAY_SIZE];
    int count = 0;

    for (int i = 0; i < ARRAY_SIZE; i++) {
        printf("Popping index %d, ", ARRAY_SIZE - i - 1);
        int* poppedElement = (int*)arrayPop(array);
        
        // ensure the popped element is not NULL
        assert(poppedElement != NULL);
        printf("Popped: %d\n", *poppedElement);

        // check if the popped element has been popped before
        for (int j = 0; j < i; j++) {
            assert(poppedArray[j] != *poppedElement);
        }
        
        // check if the correct element has been popped
        assert(*poppedElement == readArray[ARRAY_SIZE - i - 1]);

        // add the popped element to the poppedArray
        poppedArray[count++] = *poppedElement;
    }

    return 0;
}

void* threadArrayWrite(void* arg)
{
    arrayStruct* arr = (arrayStruct*)arg;
    LockFreeArray* array = arr->array;
    int threadID = arr->threadID;
    int index;
    
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) 
    {
        int* element = malloc(sizeof(int));
        *element = i + threadID * 100;
        index = threadID * OPERATIONS_PER_THREAD + i;
        printf("Thread %d: Writing %d to %d\n", threadID, *element, index);
        assert(arrayWrite(array, index, element) == 0);
	}
	
    return NULL;
}


int main() {
    // initialize the array
    LockFreeArray array;
    arrayInit(&array);
    
    // create threads
    pthread_t threads[NUM_THREADS];
    arrayStruct* arrs[NUM_THREADS];
    
    printf("\n----- Pushing Test Started -----\n");
    for (int i = 0; i < NUM_THREADS; i++) 
    {
    	arrs[i] = malloc(sizeof(arrayStruct)); 
    	arrs[i]->threadID = i;
    	arrs[i]->array = &array;
        pthread_create(&threads[i], NULL, threadArrayPush, arrs[i]);
    }
    // wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
        free(arrs[i]);
    }
    printf("----- Pushing Test Ended Successfully -----\n\n");
    
    // verify array size
    size_t size = arraySize(&array);
    printf("\narray size: %zu\n", size);
    
    // reading test
    printf("\n----- Reading Test Started -----\n");
    readFromArray(array);
    printf("----- Reading Test Ended Successfully -----\n\n");
    
    // popping test
    printf("\n----- Popping Test Started -----\n");
    popFromArray(&array);
    printf("----- Popping Test Ended Successfully -----\n\n");

    // verify array size
    size = arraySize(&array);
    printf("\narray size: %zu\n", size);
    assert(size == 0); // all elements should have been popped
    
    // reserve space in the array
    if (arrayReserve(&array, ARRAY_SIZE) != 0) 
    {
        fprintf(stderr, "Failed to reserve array space\n");
        return EXIT_FAILURE;
    }
    
    printf("\n----- Writing Test Started -----\n");
    for (int i = 0; i < NUM_THREADS; i++) 
    {
    	arrs[i] = malloc(sizeof(arrayStruct)); 
    	arrs[i]->threadID = i;
    	arrs[i]->array = &array;
        pthread_create(&threads[i], NULL, threadArrayWrite, arrs[i]);
    }
    // wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
        free(arrs[i]);
    }
    printf("----- Writing Test Ended Successfully -----\n\n");
    
    printf("All tests passed!\n");
    return 0;
}