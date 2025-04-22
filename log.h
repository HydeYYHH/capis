#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} level_t;

#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO

void dolog(level_t level, const char *fmt, ...);

#define LOG(level, fmt, ...) dolog(level, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  dolog(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  dolog(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) dolog(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#endif