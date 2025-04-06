#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "lock_based_array.h"


void arrayInit(LockBasedArray* array)
{
    array->size = 0;
    array->capacity = 4;
    array->data = malloc(array->capacity * sizeof(void*));
    pthread_mutex_init(&array->lock, NULL);
}


void arrayDestroy(LockBasedArray* array)
{
    pthread_mutex_destroy(&array->lock);
    free(array->data);
}


int arrayPush(LockBasedArray* array, void* element)
{
    pthread_mutex_lock(&array->lock);
    if (array->size == array->capacity)
    {
        size_t new_capacity = array->capacity * 2;
        void** new_data = realloc(array->data, new_capacity * sizeof(void*));
        if (!new_data)
        {
            pthread_mutex_unlock(&array->lock);
            return -1;
        }
        array->data = new_data;
        array->capacity = new_capacity;
    }
    array->data[array->size++] = element;
    pthread_mutex_unlock(&array->lock);
    return 0;
}


void* arrayPop(LockBasedArray* array)
{
    pthread_mutex_lock(&array->lock);
    if (array->size == 0)
    {
        pthread_mutex_unlock(&array->lock);
        return NULL;
    }
    void* element = array->data[--array->size];
    pthread_mutex_unlock(&array->lock);
    return element;
}


void* arrayRead(LockBasedArray* array, size_t index)
{
    pthread_mutex_lock(&array->lock);
    if (index >= array->size)
    {
        pthread_mutex_unlock(&array->lock);
        return NULL;
    }
    void* element = array->data[index];
    pthread_mutex_unlock(&array->lock);
    return element;
}


int arrayWrite(LockBasedArray* array, size_t index, void* element) 
{
    pthread_mutex_lock(&array->lock);
    if (index >= array->size)
    {
        pthread_mutex_unlock(&array->lock);
        return -1;
    }
    array->data[index] = element;
    pthread_mutex_unlock(&array->lock);
    return 0;
}


int arrayReserve(LockBasedArray* array, size_t size) 
{
    pthread_mutex_lock(&array->lock);
    if (size > array->capacity)
    {
        void** new_data = realloc(array->data, size * sizeof(void*));
        if (!new_data)
        {
            pthread_mutex_unlock(&array->lock);
            return -1;
        }
        array->data = new_data;
        array->capacity = size;
    }
    pthread_mutex_unlock(&array->lock);
    return 0;
}


size_t arraySize(LockBasedArray* array) 
{
    pthread_mutex_lock(&array->lock);
    size_t size = array->size;
    pthread_mutex_unlock(&array->lock);
    return size;
}
