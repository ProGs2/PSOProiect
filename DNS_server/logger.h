#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>

// Logger structure
typedef struct {
    FILE* log_file;          // File pointer for the log file
    pthread_mutex_t lock;    // Mutex to ensure thread-safe logging
} Logger;

// Function prototypes
Logger* initLogger(const char* filename);
void logMessage(Logger* logger, const char* level, const char* format, ...);
void destroyLogger(Logger* logger);

#endif // LOGGER_H
