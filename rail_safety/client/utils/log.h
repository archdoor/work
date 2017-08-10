#ifndef __LOG_H__
#define __LOG_H__

#include <iostream>
#include <stdio.h>

using std::string;

enum e_log_level
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
};

enum e_log_mode{
    LOG_CONSOLE,
    LOG_LOGFILE,
    LOG_DUAL
};

// const int LOG_MAX_FILE_SIZE = 20*1024*1024;
const int LOG_MAX_FILE_SIZE = 60*1024;

#define LOG_ADD_FORM  "[%s:%d] "

#define LogPrint(format, args...) \
    printf(format, ##args)

#define LogMemPrint(mem, len) \
    for( int i = 0; i < len; ++i )  \
    {                               \
        printf("%.2x ", mem[i]);    \
        if ( (i+1) % 20 == 0 ) {    \
            printf("\n");           \
            continue;               \
        }                           \
        if ( (i+1) % 5 == 0 )       \
        {                           \
            printf(" ");            \
        }                           \
    }                               \
    printf("\n");

#define LogDebug(format, args...) \
    log_print(LOG_DEBUG, LOG_ADD_FORM format, __FILE__, __LINE__, ##args)

#define LogInfo(format, args...) \
    log_print(LOG_INFO, LOG_ADD_FORM format, __FILE__, __LINE__, ##args)

#define LogWarning(format, args...) \
    log_print(LOG_WARNING, LOG_ADD_FORM format, __FILE__, __LINE__, ##args)

#define LogError(format, args...) \
    log_print(LOG_ERROR, LOG_ADD_FORM format, __FILE__, __LINE__, ##args)

#define LogCritical(format, args...) \
    log_print(LOG_CRITICAL, LOG_ADD_FORM format, __FILE__, __LINE__, ##args)


int log_config(string level, string mode, const char *file_path);
void log_print(int level, const char *fmt, ...);

#endif
