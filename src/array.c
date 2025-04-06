#include "array.h"
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>

#define FIRST_BUCKET_SIZE 1024

// Pool for recycling objects
_Atomic(Descriptor*) descriptorPool = NULL;
_Atomic(WriteDescriptor*) writeDescriptorPool = NULL;

// Helper functions
Descriptor* getDescriptor();
void recycleDescriptor(Descriptor* d);
WriteDescriptor* getWriteDescriptor();
void recycleWriteDescriptor(WriteDescriptor* wd);
void CompleteWrite(LockFreeArray* array, WriteDescriptor* writeOp);
_Atomic(void*)* Get(LockFreeArray* array, size_t index);
size_t HighestBit(size_t value);
void AllocBucket(LockFreeArray* array, size_t bucket);


void arrayInit(LockFreeArray* array) 
{
    atomic_init(&array->size, 0);
    array->memory = (_Atomic(void*)*)calloc(FIRST_BUCKET_SIZE, sizeof(void*));
    array->descriptor = (Descriptor*)malloc(sizeof(Descriptor));
    atomic_init(&((Descriptor*)array->descriptor)->size, 0);
    atomic_init(&((Descriptor*)array->descriptor)->pendingWrite, NULL);
    atomic_init(&((Descriptor*)array->descriptor)->refCount, 1);
}


size_t arraySize(LockFreeArray* array)
{
	Descriptor* descriptorCurrent = atomic_load(&array->descriptor);
    return atomic_load(&descriptorCurrent->size);
}

// function to get a Descriptor from the pool or allocate a new one
Descriptor* getDescriptor()
{
    Descriptor* d = atomic_exchange(&descriptorPool, NULL);
    if (d)
    {
        atomic_store(&d->refCount, 1);
        return d;  // reuse Descriptor
    }
    return (Descriptor*)malloc(sizeof(Descriptor)); // allocate if none available
}

// function to return a Descriptor to the pool
void recycleDescriptor(Descriptor* d)
{
    if (d)
    {
        if (atomic_fetch_sub(&d->refCount, 1) == 1)
        {
            free(d);  // only free if the last reference is gone
        } else
        {
            atomic_store(&d->refCount, 1);
            atomic_store(&d->pendingWrite, NULL);
            atomic_store(&descriptorPool, d);
        }
    }
}

// function to get a WriteDescriptor from the pool or allocate a new one
WriteDescriptor* getWriteDescriptor()
{
    WriteDescriptor* wd = atomic_exchange(&writeDescriptorPool, NULL);
    if (wd)
    {
        atomic_store(&wd->completed, false);
        return wd;  // reuse write descriptor
    }
    return (WriteDescriptor*)malloc(sizeof(WriteDescriptor)); // allocate if none available
}

// function to return a WriteDescriptor to the pool
void recycleWriteDescriptor(WriteDescriptor* wd)
{
    if (wd)
    {
        WriteDescriptor* expected = NULL;
        if (!atomic_compare_exchange_strong(&writeDescriptorPool, &expected, wd))
        {
            free(wd); // free if the pool is already occupied
        }
    }
}

// modified arrayPush with recycling
int arrayPush(LockFreeArray* array, void* element) 
{
    Descriptor* descriptorCurrent = NULL;
    Descriptor* descriptorNext = NULL;
    WriteDescriptor* writeOp = NULL;
    WriteDescriptor *oldWrite = NULL;
    size_t bucket;
        
    while (true)
    {
        descriptorCurrent = atomic_load(&array->descriptor);
        atomic_fetch_add(&descriptorCurrent->refCount, 1);
        CompleteWrite(array, atomic_load(&descriptorCurrent->pendingWrite));

        // free old pending write if it was completed
        oldWrite = atomic_exchange(&descriptorCurrent->pendingWrite, NULL);
        if (oldWrite && atomic_load(&oldWrite->completed))
        {  
            recycleWriteDescriptor(oldWrite); 
        }

        bucket = HighestBit(descriptorCurrent->size + FIRST_BUCKET_SIZE) - HighestBit(FIRST_BUCKET_SIZE);
        if (atomic_load(&array->memory[bucket]) == NULL)
        {
            AllocBucket(array, bucket);
        }

        writeOp = getWriteDescriptor();
        atomic_init(&writeOp->pos, atomic_load(&descriptorCurrent->size));
        writeOp->oldValue = atomic_load((_Atomic(void*)*) Get(array, atomic_load(&descriptorCurrent->size)));
        writeOp->newValue = malloc(sizeof(size_t));
        *(size_t*)writeOp->newValue = *(size_t*)element;
        atomic_init(&writeOp->completed, false);
        
        descriptorNext = getDescriptor();
        atomic_init(&descriptorNext->size, atomic_load(&descriptorCurrent->size) + 1);
        atomic_init(&descriptorNext->pendingWrite, writeOp);
        atomic_init(&descriptorNext->refCount, 1);
        
        if (atomic_compare_exchange_strong(&array->descriptor, (void**)&descriptorCurrent, descriptorNext)) 
        {
            // successfully updated the descriptor, decrement the reference count
            if (atomic_fetch_sub(&descriptorCurrent->refCount, 1) == 1) 
            {
                recycleDescriptor(descriptorCurrent);
            }
            break; // exit loop on success
        }

        recycleDescriptor(descriptorNext);
        free(writeOp->newValue);
        recycleWriteDescriptor(writeOp);

        atomic_fetch_sub(&descriptorCurrent->refCount, 1);
    }
    
    // decrement ref count and free descriptor if it's no longer in use
    if (atomic_fetch_sub(&descriptorCurrent->refCount, 1) == 1) 
    {
        oldWrite = atomic_load(&descriptorCurrent->pendingWrite);
        if (oldWrite)
        {
            recycleWriteDescriptor(oldWrite);
        }
        recycleDescriptor(descriptorCurrent);
    }
    
    CompleteWrite(array, atomic_load(&descriptorNext->pendingWrite));
            
    return 0;
}


void* arrayRead(LockFreeArray* array, size_t index)
{
    Descriptor* descriptorCurrent = atomic_load_explicit(&array->descriptor, memory_order_acquire);
    if (index >= descriptorCurrent->size)
    {
        return NULL;
    }
    _Atomic(void*)* address = Get(array, index);
    return atomic_load_explicit(address, memory_order_acquire);
}


void* arrayPop(LockFreeArray* array)
{
    Descriptor *descriptorCurrent, *descriptorNext;
    _Atomic(void*)* address;

    do
    {
        descriptorCurrent = atomic_load(&array->descriptor);
        CompleteWrite(array, descriptorCurrent->pendingWrite);

        if (descriptorCurrent->size == 0)
        {
            return NULL;
        }

        address = Get(array, descriptorCurrent->size - 1);
        descriptorNext = getDescriptor();
        atomic_init(&descriptorNext->size, atomic_load(&descriptorCurrent->size) - 1);
        atomic_init(&descriptorNext->pendingWrite, NULL);
        atomic_init(&descriptorNext->refCount, 1);
    } while (!atomic_compare_exchange_strong(&array->descriptor, (void**)&descriptorCurrent, descriptorNext));

    if (atomic_fetch_sub(&descriptorCurrent->refCount, 1) == 1)
    {
        recycleDescriptor(descriptorCurrent);
    }

    return atomic_load_explicit(address, memory_order_acquire);
}


int arrayReserve(LockFreeArray* array, size_t size)
{
    Descriptor* descriptor = atomic_load(&array->descriptor);
    size_t i = HighestBit(atomic_load(&descriptor->size) + FIRST_BUCKET_SIZE - 1) - HighestBit(FIRST_BUCKET_SIZE);
    if ((ssize_t)i < 0)
    {
        i = 0;
    }
    size_t max_i = HighestBit(size + FIRST_BUCKET_SIZE - 1) - HighestBit(FIRST_BUCKET_SIZE);
    while (i < max_i)
    {
        i++;
        AllocBucket(array, i);
    }
    return 0;
}


int arrayWrite(LockFreeArray* array, size_t index, void* element)
{
    _Atomic(void*)* address = Get(array, index);
    if (address)
    {
    	void* old = atomic_exchange_explicit(address, element, memory_order_acq_rel);
        if (old)
        	free(old);
    	return 0;
    }
    return -1;
}


void CompleteWrite(LockFreeArray* array, WriteDescriptor* writeOp)
{
    _Atomic(void*)* address;
    void* expected;
    if (writeOp && !atomic_load_explicit(&writeOp->completed, memory_order_acquire))
    {
        address = Get(array, writeOp->pos);
        expected = atomic_load((_Atomic(void*)*)&writeOp->oldValue);
        if (atomic_compare_exchange_strong(address, &expected, writeOp->newValue))
        {
            atomic_store_explicit(&writeOp->completed, true, memory_order_release);
        }
    }
}


_Atomic(void*)* Get(LockFreeArray* array, size_t index)
{
    size_t pos = index + FIRST_BUCKET_SIZE;
    size_t hibit = HighestBit(pos);
    size_t idx = pos ^ (1 << hibit);
    size_t bucket = hibit - HighestBit(FIRST_BUCKET_SIZE);
    if (atomic_load(&array->memory[bucket]) == NULL)
    {
        printf("Error: Attempting to access unallocated memory at bucket %zu\n", bucket);
        return NULL;
    }
    return &array->memory[bucket][idx * sizeof(void*)];
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
    size_t bucketSize = FIRST_BUCKET_SIZE << bucket;
    void** newMemory = (void**)calloc(bucketSize, sizeof(void*));
    if (newMemory) {
        void *expected = NULL;
        if (!atomic_compare_exchange_strong(&array->memory[bucket], &expected, newMemory))
        {
            free(newMemory);
        }
    }
}


void arrayDestroy(LockFreeArray* array)
{
    size_t size = arraySize(array);
    for (size_t i = 0; i < size; i++)
    {
        void* element = arrayRead(array, i);
        if (element)
        {
            free(element);
        }
    }

    Descriptor* descriptor = atomic_load(&array->descriptor);
    if (descriptor)
    {
        WriteDescriptor* pendingWrite = atomic_exchange(&descriptor->pendingWrite, NULL);
        if (pendingWrite && atomic_load(&pendingWrite->completed))
        {
            recycleWriteDescriptor(pendingWrite);
        }
        recycleDescriptor(descriptor);
    }

    size_t maxBucket = HighestBit(size + FIRST_BUCKET_SIZE - 1) - HighestBit(FIRST_BUCKET_SIZE);
    for (size_t i = 0; i <= maxBucket; i++)
    {
        void** bucket = atomic_load(&array->memory[i]);
        if (bucket)
        {
            free(bucket);
        }
    }

    free(array->memory);
}


