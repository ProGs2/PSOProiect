#include "logger.h"
#include <stdlib.h>
#include <time.h>

// Initialize the logger
Logger* initLogger(const char* filename) {
    Logger* logger = malloc(sizeof(Logger));
    if (!logger) {
        perror("Failed to allocate memory for logger");
        return NULL;
    }

    logger->log_file = fopen(filename, "a");
    if (!logger->log_file) {
        perror("Failed to open log file");
        free(logger);
        return NULL;
    }

    pthread_mutex_init(&(logger->lock), NULL);
    return logger;
}

// Write a message to the log
void logMessage(Logger* logger, const char* level, const char* format, ...) {
    pthread_mutex_lock(&(logger->lock));

    // Get current timestamp
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    // Write the timestamp and log level
    fprintf(logger->log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, level);

    // Write the formatted message
    va_list args;
    va_start(args, format);
    vfprintf(logger->log_file, format, args);
    va_end(args);

    // Add a newline and flush the log
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);

    pthread_mutex_unlock(&(logger->lock));
}

// Clean up the logger
void destroyLogger(Logger* logger) {
    if (logger) {
        fclose(logger->log_file);
        pthread_mutex_destroy(&(logger->lock));
        free(logger);
    }
}
