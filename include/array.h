#ifndef LOCK_FREE_ARRAY_H
#define LOCK_FREE_ARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct 
{
    _Atomic size_t pos;
    void* oldValue;
    void* newValue;
    _Atomic bool completed;
} WriteDescriptor;

typedef struct 
{
    _Atomic size_t size;
    _Atomic size_t capacity;
    _Atomic(WriteDescriptor*) pendingWrite;
    _Atomic int refCount;  // Reference counter for safe memory reclamation
} Descriptor;

typedef struct 
{
    _Atomic size_t size;
    _Atomic size_t capacity;
    _Atomic(void*)* memory;
    _Atomic(void*) descriptor;
} LockFreeArray;


// Initialize the array
void arrayInit(LockFreeArray* array);
// Destroy the array
void arrayDestroy(LockFreeArray* array);
// Add an element to the end of the array
int arrayPush(LockFreeArray* array, void* element);
// Remove an element from the end of the array
void* arrayPop(LockFreeArray* array);
// Read an element at a specific index
void* arrayRead(LockFreeArray* array, size_t index);
// Write an element at a specific index
int arrayWrite(LockFreeArray* array, size_t index, void* element);
// Reserve memory for the array
int arrayReserve(LockFreeArray* array, size_t size);
// Get the size of the array
size_t arraySize(LockFreeArray* array);



#endif // LOCK_FREE_ARRAY_H