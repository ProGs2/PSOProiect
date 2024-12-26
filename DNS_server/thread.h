#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

#define MAX_THREADS 10
#define MAX_QUEUE 100

// Task structure
typedef struct {
    void (*function)(void*); // Function pointer for the task
    void* argument;          // Argument for the function
} ThreadPoolTask;

// Thread pool structure
typedef struct {
    pthread_t threads[MAX_THREADS];      // array of worker threads
    ThreadPoolTask task_queue[MAX_QUEUE]; // task queue
    int queue_size;                      // Current size of the task queue
    int queue_front;                     // Front index of the task queue
    int queue_rear;                      // Rear index of the task queue
    pthread_mutex_t lock;                // Mutex for thread synchronization
    pthread_cond_t notify;               // Condition variable to signal workers
    int stop;                            // Flag to stop the thread pool
} ThreadPool;

ThreadPool* initThreadPool(int thread_count);
int addTaskToThreadPool(ThreadPool* pool, void (*function)(void*), void* argument);
void destroyThreadPool(ThreadPool* pool);

#endif