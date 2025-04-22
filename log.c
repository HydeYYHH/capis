#include "log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>


static const char *cc[] = {
    "\033[0m",   // LOG_LEVEL_INFO
    "\033[33m",  // LOG_LEVEL_WARN
    "\033[31m"   // LOG_LEVEL_ERROR
};
static const char *rc = "\033[0m";


static const char *lss[] = {
    "INFO",
    "WARN",
    "ERROR"
};


#define DEFAULT_TIME_FORMAT "%Y-%m-%d %H:%M:%S"

void dolog(level_t level, const char *fmt, ...) {
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char timebuf[20];
    strftime(timebuf, sizeof(timebuf), DEFAULT_TIME_FORMAT, lt);

    fprintf(stderr, "%s[%s %s]  ", cc[level], timebuf, lss[level]);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "%s\n", rc);
}
