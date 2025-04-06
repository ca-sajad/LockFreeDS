#ifndef LOCK_BASED_ARRAY_H
#define LOCK_BASED_ARRAY_H

#include <stddef.h>
#include <pthread.h>

typedef struct
{
    void** data;
    size_t size;
    size_t capacity;
    pthread_mutex_t lock;
} LockBasedArray;

void arrayInit(LockBasedArray* array);
void arrayDestroy(LockBasedArray* array);
int arrayPush(LockBasedArray* array, void* element);
void* arrayPop(LockBasedArray* array);
void* arrayRead(LockBasedArray* array, size_t index);
int arrayWrite(LockBasedArray* array, size_t index, void* element);
int arrayReserve(LockBasedArray* array, size_t size);
size_t arraySize(LockBasedArray* array);

#endif
