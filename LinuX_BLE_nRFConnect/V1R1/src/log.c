#include "../include/common.h"
#include "../include/log.h"

#include <stdarg.h>

void log_info(const char *fmt, ...)
{
    va_list args;

    printf("[INFO] ");

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

void log_error(const char *fmt, ...)
{
    va_list args;

    printf("[ERROR] ");

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}