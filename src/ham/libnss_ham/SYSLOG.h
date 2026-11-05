#ifndef __SYSLOG_H__
#define __SYSLOG_H__

#include <stdio.h>      // fprintf()
#include <syslog.h>     // syslog()

//#define SYSLOG_TO_STDOUT

#ifdef SYSLOG_TO_STDOUT
#   define SYSLOG(LEVEL, args...) fprintf(stdout, args)
#else
#   define SYSLOG(LEVEL, args...) \
    do \
    { \
        if (log_p != nullptr) fprintf(log_p, args); \
        else                  syslog(LEVEL, args);  \
    } while (0)
#endif

#define SYSLOG_CONDITIONAL(condition, LEVEL, args...) \
do \
{ \
    if (condition) \
    { \
        SYSLOG(LEVEL, args); \
    } \
} while (0)

#endif // __SYSLOG_H__
