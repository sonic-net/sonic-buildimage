/*
 * SDK context
 */
#include "sdk_rev.h"

#include "crintl/logger.h"
#include "sdk.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* For internal use */
#define LOG_BUFFER_SIZE 2048

static CredoLog_t logger = NULL;
static CredoLog_t shell_logger = NULL;
static CredoLogLevel_t loglevel = CR_LOG_INFO;
static CredoLogIO_t tcm_access = CR_LOG_IO_OFF;
static CredoLogIO_t reg_access = CR_LOG_IO_OFF;
static CredoLogIO_t api_access = CR_LOG_IO_OFF;

#ifndef CREDO_LOGGING_DISABLED
static const char* loglevel_to_str(CredoLogLevel_t level) {
    switch (level) {
        case CR_LOG_TRACE:
            return "TRACE";
        case CR_LOG_DEBUG:
            return "DEBUG";
        case CR_LOG_INFO:
            return "INFO";
        case CR_LOG_WARN:
            return "WARNING";
        case CR_LOG_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
static void default_logger(void* slice_context, void* user_data, CredoLogLevel_t level, const char* scope,
                           const char* message) {
    // store log output
    const char separator = strchr(message, '\n') != NULL ? '\n' : ' ';
    const char* level_str = loglevel_to_str(level);
    printf("[%-7s%8s]%c%s\n", level_str, scope, separator, message);
}

void cr_log_slice(CredoSlice_t* slice, CredoLogLevel_t level, const char* format, ...) {
    if (level > loglevel) {
        return;
    }
    char buf[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    int size = vsnprintf(buf, LOG_BUFFER_SIZE, format, args);
    va_end(args);
    if (size < 0) {
        return;
    }
    // if not enough space, malloc and rewrite the data
    if (size >= LOG_BUFFER_SIZE) {
        char* heapbuf = malloc(size + 1);
        if (heapbuf == NULL) {
            cr_llog_slice(slice, level, buf);  // at least log truncated if not enough space
            return;
        };
        va_start(args, format);
        vsnprintf(heapbuf, size + 1, format, args);
        va_end(args);
        cr_llog_slice(slice, level, heapbuf);
        free(heapbuf);
    } else {
        cr_llog_slice(slice, level, buf);
    }
}

void cr_llog_slice(CredoSlice_t* slice, CredoLogLevel_t level, const char* message) {
    char scope[64];

    if (level > loglevel) {
        return;
    }

    sprintf(scope, "Slice %u", slice->slice_id);

    // when a slice is used by shell and shell_logger set
    if (shell_logger != NULL && slice->in_shell) {
        (shell_logger)(slice->slice_context, NULL, level, scope, message);
    } else if (logger) {
        logger(slice->slice_context, NULL, level, scope, message);
    } else {
        default_logger(slice->slice_context, NULL, level, scope, message);
    }
}

void cr_llog(const char* scope, CredoLogLevel_t level, const char* message) {
    if (level > loglevel) {
        return;
    }
    if (logger) {
        logger(NULL, NULL, level, scope, message);
    } else {
        default_logger(NULL, NULL, level, scope, message);
    }
}

void cr_log(const char* scope, CredoLogLevel_t level, const char* format, ...) {
    if (level > loglevel) {
        return;
    }

    va_list args;
    va_start(args, format);
    char buf[LOG_BUFFER_SIZE];
    int size = vsnprintf(buf, LOG_BUFFER_SIZE, format, args);
    va_end(args);
    if (size < 0) {
        return;
    }
    // if not enough space, malloc and rewrite the data
    if (size >= LOG_BUFFER_SIZE) {
        char* heapbuf = malloc(size + 1);
        if (heapbuf == NULL) {
            cr_llog(scope, level, buf);  // at least log truncated if not enough space
            return;
        };
        va_start(args, format);
        vsnprintf(heapbuf, size + 1, format, args);
        va_end(args);
        cr_llog(scope, level, heapbuf);
        free(heapbuf);
    } else {
        cr_llog(scope, level, buf);
    }
}
#endif
void cr_logger_set(CredoLog_t new_logger) {
    logger = new_logger;
}

CredoLog_t cr_logger_get(void) {
    return logger;
}

void cr_logger_set_level(CredoLogLevel_t new_loglevel) {
    loglevel = new_loglevel;
}

CredoLogLevel_t cr_logger_get_level(void) {
    return loglevel;
}

void cri_logger_set_shell_logger(CredoLog_t shell_log_cb) {
    shell_logger = shell_log_cb;
}

CREDOAPI void cr_logger_set_feature(CredoLogFeature_t feature, CredoLogIO_t value) {
    switch (feature) {
        case CR_LOG_FEAT_API:
            api_access = (value != CR_LOG_IO_OFF) ? CR_LOG_IO_ON : CR_LOG_IO_OFF;
            break;
        case CR_LOG_FEAT_REG:
            reg_access = value;
            break;
        case CR_LOG_FEAT_TCM:
            tcm_access = value;
            break;
        default:
            break;
    }
}

CREDOAPI CredoLogIO_t cr_logger_get_feature(CredoLogFeature_t feature) {
    switch (feature) {
        case CR_LOG_FEAT_API:
            return api_access;
        case CR_LOG_FEAT_REG:
            return reg_access;
        case CR_LOG_FEAT_TCM:
            return tcm_access;
        default:
            return CR_LOG_IO_OFF;
    }
}
