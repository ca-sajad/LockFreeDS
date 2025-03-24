#include "array.h"
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>

#define FIRST_BUCKET_SIZE 8

// Helper functions
void CompleteWrite(LockFreeArray* array, WriteDescriptor* writeOp);
_Atomic(void*)* Get(LockFreeArray* array, size_t index);
size_t HighestBit(size_t value);
void AllocBucket(LockFreeArray* array, size_t bucket);


void arrayInit(LockFreeArray* array) 
{
    atomic_init(&array->size, 0);
    atomic_init(&array->capacity, FIRST_BUCKET_SIZE);
    array->memory = (_Atomic(void*)*)calloc(FIRST_BUCKET_SIZE, sizeof(void*));
    array->descriptor = (Descriptor*)malloc(sizeof(Descriptor));
    atomic_init(&((Descriptor*)array->descriptor)->size, 0);
    atomic_init(&((Descriptor*)array->descriptor)->capacity, FIRST_BUCKET_SIZE);
    atomic_init(&((Descriptor*)array->descriptor)->pendingWrite, NULL);
    atomic_init(&((Descriptor*)array->descriptor)->refCount, 1);
}


size_t arraySize(LockFreeArray* array)
{
	Descriptor* descriptorCurrent = atomic_load(&array->descriptor);
    return atomic_load(&descriptorCurrent->size);
}


int arrayPush(LockFreeArray* array, void* element) 
{
    Descriptor* descriptorCurrent = NULL;
    Descriptor* descriptorNext = NULL;
    WriteDescriptor* writeOp = NULL;
    size_t bucket;
        
    while (!atomic_compare_exchange_strong(&array->descriptor, (void**)&descriptorCurrent, descriptorNext))
    {
    	descriptorCurrent = atomic_load(&array->descriptor);
    	atomic_fetch_add(&descriptorCurrent->refCount, 1);
    	//printf("----CompleteWrite Inside started, %d\n", *(size_t*)element);
    	CompleteWrite(array, descriptorCurrent->pendingWrite);
    	//printf("----CompleteWrite Inside ended, %d\n", *(size_t*)element);
    	// free pendingWrite but do not free its newValue
    	if (descriptorCurrent->pendingWrite) {
        	//free(descriptorCurrent->pendingWrite);
        	//descriptorCurrent->pendingWrite = NULL;
    	}
    	bucket = HighestBit(descriptorCurrent->size + FIRST_BUCKET_SIZE) - HighestBit(FIRST_BUCKET_SIZE);
    	
    	if (atomic_load(&array->memory[bucket]) == NULL)
    	{
    		AllocBucket(array, bucket);
    	}
    	
    	writeOp = (WriteDescriptor*)malloc(sizeof(WriteDescriptor));
    	atomic_init(&writeOp->pos, atomic_load(&descriptorCurrent->size));
    	// writeOp->oldValue = *(void **)Get(array, descriptorCurrent->size);
    	writeOp->oldValue = atomic_load((_Atomic(void*)*) Get(array, atomic_load(&descriptorCurrent->size)));
    	writeOp->newValue = malloc(sizeof(size_t));
    	*(size_t*)writeOp->newValue = *(size_t*)element;
    	atomic_init(&writeOp->completed, false);
    	
    	descriptorNext = (Descriptor*)malloc(sizeof(Descriptor));
	   	atomic_init(&descriptorNext->size, atomic_load(&descriptorCurrent->size) + 1);
        atomic_init(&descriptorNext->capacity, atomic_load(&descriptorCurrent->capacity));
        atomic_init(&descriptorNext->pendingWrite, writeOp);
        atomic_init(&descriptorNext->refCount, 1);
    	//printf("**** %d added to descriptor\n", *(size_t*)element);
    }
    
    // Decrement ref count and free descriptor if it's no longer in use
    if (atomic_fetch_sub(&descriptorCurrent->refCount, 1) == 1) {
        WriteDescriptor *oldWrite = atomic_load(&descriptorCurrent->pendingWrite);
        if (oldWrite) {
            free(oldWrite);
        }
        free(descriptorCurrent);
    }
    
    //printf("---------CompleteWrite Outside started, %d\n", *(size_t*)element);
    CompleteWrite(array, atomic_load(&descriptorNext->pendingWrite));
    //printf("---------CompleteWrite Outside ended, %d\n", *(size_t*)element);
    
    return 0;
}


void* arrayRead(LockFreeArray* array, size_t index) 
{
    Descriptor* descriptorCurrent = atomic_load_explicit(&array->descriptor, memory_order_acquire);
    if (index >= descriptorCurrent->size) { // Out of bounds
        return NULL;
    }
    _Atomic(void*)* address = Get(array, index);
    
    return atomic_load_explicit(address, memory_order_acquire);
}


void* arrayPop(LockFreeArray* array)
{
	Descriptor *descriptorCurrent, *descriptorNext;
	_Atomic(void*)* address;

    do {
        descriptorCurrent = atomic_load(&array->descriptor);
        CompleteWrite(array, descriptorCurrent->pendingWrite);

        if (descriptorCurrent->size == 0) {
            return NULL; // array is empty
        }

        address = Get(array, descriptorCurrent->size - 1);
        
        descriptorNext = malloc(sizeof(Descriptor));
    	atomic_init(&((Descriptor*)descriptorNext)->size, atomic_load(&descriptorCurrent->size) - 1);
    	atomic_init(&((Descriptor*)descriptorNext)->capacity, atomic_load(&descriptorCurrent->capacity));
    	atomic_init(&((Descriptor*)descriptorNext)->pendingWrite, NULL);
    	atomic_init(&((Descriptor*)descriptorNext)->refCount, 1);
    
    } while (!atomic_compare_exchange_strong(&array->descriptor, (void**)&descriptorCurrent, descriptorNext));

    // decrement ref count and free descriptor if it's no longer in use
    if (atomic_fetch_sub(&descriptorCurrent->refCount, 1) == 1) {
        WriteDescriptor *oldWrite = atomic_load(&descriptorCurrent->pendingWrite);
        if (oldWrite) {
            free(oldWrite);
        }
        free(descriptorCurrent);
    }
    
    return atomic_load_explicit(address, memory_order_acquire);;
}


int arrayReserve(LockFreeArray* array, size_t size)
{
    Descriptor* descriptor = atomic_load(&array->descriptor);
    size_t i = HighestBit(atomic_load(&descriptor->size) + FIRST_BUCKET_SIZE - 1) - HighestBit(FIRST_BUCKET_SIZE);

    if ((ssize_t)i < 0) {  // Ensure i is non-negative
        i = 0;
    }

    size_t max_i = HighestBit(size + FIRST_BUCKET_SIZE - 1) - HighestBit(FIRST_BUCKET_SIZE);

    while (i < max_i) {
        i++;
        AllocBucket(array, i);
    }
    
    return 0;
}


int arrayWrite(LockFreeArray* array, size_t index, void* element)
{
	_Atomic(void*)* address = Get(array, index);
	atomic_store_explicit(address, element, memory_order_release);
	return 0;
}

void CompleteWrite(LockFreeArray* array, WriteDescriptor* writeOp)
{
	_Atomic(void*)* address;
	void* expected;
	
	if (writeOp && !atomic_load_explicit(&writeOp->completed, memory_order_acquire))
	{
		address = Get(array, writeOp->pos);
		expected = atomic_load((_Atomic(void*)*)&writeOp->oldValue);
		atomic_compare_exchange_strong(address, &expected, writeOp->newValue);
        // printf("Added: %zu\n", *(size_t*)atomic_load_explicit(address, memory_order_acquire));
        atomic_compare_exchange_strong(address, &expected, writeOp->newValue);
        atomic_store_explicit(&writeOp->completed, true, memory_order_release);
	}
}


_Atomic(void*)* Get(LockFreeArray* array, size_t index)
{
	size_t pos = index + FIRST_BUCKET_SIZE;
    size_t hibit = HighestBit(pos);
    size_t idx = pos ^ (1 << hibit);
    size_t bucket = hibit - HighestBit(FIRST_BUCKET_SIZE);
    if (atomic_load(&array->memory[bucket]) == NULL) {
    	printf("Error: Attempting to access unallocated memory at bucket %zu\n", bucket);
    	return NULL;
	}
    _Atomic(void*)* address = &array->memory[bucket][idx * sizeof(void*)];
    
    return address;
}


size_t HighestBit(size_t value)
{
	size_t i = 0;
	while (value >>= 1)
	{
		i++;
	}
	return i;
}


void AllocBucket(LockFreeArray* array, size_t bucket)
{
	size_t bucketSize;
	void** newMemory;
	
	bucketSize = FIRST_BUCKET_SIZE << bucket;
	newMemory = (void**)calloc(bucketSize, sizeof(void*));
	if (newMemory) 
	{
        void *expected = NULL;
		if (!atomic_compare_exchange_strong(&array->memory[bucket], &expected, newMemory))
		{
			free(newMemory);
		}
	}
}



