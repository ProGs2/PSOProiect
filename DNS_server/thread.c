#include "thread.h"
#include <stdlib.h>
#include <stdio.h>

// Worker thread function
void* threadWorker(void* threadpool) {
    ThreadPool* pool = (ThreadPool*)threadpool;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        // Wait for tasks
        while (pool->queue_size == 0 && !pool->stop) {
            printf("Worker waiting for tasks.\n");
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->stop) {
            pthread_mutex_unlock(&(pool->lock));
            printf("Worker stopping.\n");
            pthread_exit(NULL);
        }

        // Fetch task
        ThreadPoolTask task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % MAX_QUEUE;
        pool->queue_size--;

        printf("Worker fetched a task. Queue size: %d\n", pool->queue_size);

        pthread_mutex_unlock(&(pool->lock));

        // Execute the task
        printf("Worker executing task.\n");
        (*(task.function))(task.argument);
    }
}

// Initialize the thread pool
ThreadPool* initThreadPool(int thread_count) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));

    pool->queue_size = 0;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->stop = 0;

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadWorker, (void*)pool) != 0) {
            perror("Failed to create worker thread");
            exit(EXIT_FAILURE);
        }
        printf("Worker thread %d created.\n", i);
    }

    return pool;
}

// Add a task to the thread pool
int addTaskToThreadPool(ThreadPool* pool, void (*function)(void*), void* argument) {
    pthread_mutex_lock(&(pool->lock));

    if (pool->queue_size == MAX_QUEUE) {
        pthread_mutex_unlock(&(pool->lock));
        return -1; // Queue is full
    }

    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].argument = argument;
    pool->queue_rear = (pool->queue_rear + 1) % MAX_QUEUE;
    pool->queue_size++;

    printf("Task added. Queue size: %d\n", pool->queue_size);

    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

// Destroy the thread pool
void destroyThreadPool(ThreadPool* pool) {
    pthread_mutex_lock(&(pool->lock));
    pool->stop = 1;
    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
}