#ifndef SDK_BASE_H
#define SDK_BASE_H

#include "credo.h"

#define MAX_HAL_COUNT 128

#ifdef __cplusplus
extern "C" {
#endif

// branch prediction helpers for performance
#if defined(__GNUC__)
#define cr_likely(x)   (__builtin_expect(((x) != 0), 1))
#define cr_unlikely(x) (__builtin_expect(((x) != 0), 0))
#else
#define cr_likely(x)   (x)
#define cr_unlikely(x) (x)
#endif

typedef CredoError_t (*CredoPostReadRegister_t)(void* slice_context, unsigned reg_addr, unsigned* val);
typedef CredoError_t (*CredoPostWriteRegister_t)(void* slice_context, unsigned reg_addr, unsigned val);

struct CredoSdk {
    CredoReadRegister_t read_register;
    CredoWriteRegister_t write_register;
    CredoReadRegisterBurst_t burst_read_register;
    CredoWriteRegisterBurst_t burst_write_register;

    // new broadcast functions
    CredoWriteRegisterBroadcast_t broadcast_write_register;
    CredoWriteRegisterBroadcastBurst_t burst_broadcast_write_register;

    CredoLog_t logger;
    CredoLogLevel_t max_log_level;

    CredoPostReadRegister_t post_read;
    CredoPostWriteRegister_t post_write;
};

#define FUNC_ALIAS(func) __attribute__((weak, alias("func")));

CREDOAPI CredoLog_t cr_logger_get(void);

#ifndef CREDO_LOGGING_DISABLED
CREDOAPI void cr_log_slice(CredoSlice_t* slice, CredoLogLevel_t level, const char* format, ...)
    CR_PRINTF_ATTRIBUTE_FORMAT(3, 4);
CREDOAPI void cr_log(const char* scope, CredoLogLevel_t level, const char* format, ...)
    CR_PRINTF_ATTRIBUTE_FORMAT(3, 4);

// skips cr_log formatting step
CREDOAPI void cr_llog(const char* scope, CredoLogLevel_t level, const char* message);
// skips cr_log_slice formatting step
CREDOAPI void cr_llog_slice(CredoSlice_t* slice, CredoLogLevel_t level, const char* message);
#else
static inline void cr_log_slice(CredoSlice_t* slice, CredoLogLevel_t level, const char* format, ...) {
    (void)(slice);
    (void)(level);
    (void)(format);
}
static inline void cr_log(const char* scope, CredoLogLevel_t level, const char* format, ...) {
    (void)(scope);
    (void)(level);
    (void)(format);
}
static inline void cr_llog_slice(CredoSlice_t* slice, CredoLogLevel_t level, const char* message) {
    (void)(slice);
    (void)(level);
    (void)(message);
}
static inline void cr_llog(const char* scope, CredoLogLevel_t level, const char* message) {
    (void)(scope);
    (void)(level);
    (void)(message);
}
#endif

#define LOG_SDK(...) cr_log("SDK", cr_logger_get_level(), __VA_ARGS__)

#define LOG_INFO(slice, ...)  cr_log_slice(slice, CR_LOG_INFO, __VA_ARGS__)
#define LOG_DEBUG(slice, ...) cr_log_slice(slice, CR_LOG_DEBUG, __VA_ARGS__)
#define LOG_WARN(slice, ...)  cr_log_slice(slice, CR_LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(slice, ...) cr_log_slice(slice, CR_LOG_ERROR, __VA_ARGS__)
#define LOG_TRACE(slice, ...) cr_log_slice(slice, CR_LOG_TRACE, __VA_ARGS__)
#define LOG_FORCE(slice, ...) cr_log_slice(slice, cr_logger_get_level(), __VA_ARGS__)

#define LOGS_INFO(...)  cr_log_slice(slice, CR_LOG_INFO, __VA_ARGS__)
#define LOGS_DEBUG(...) cr_log_slice(slice, CR_LOG_DEBUG, __VA_ARGS__)
#define LOGS_WARN(...)  cr_log_slice(slice, CR_LOG_WARN, __VA_ARGS__)
#define LOGS_ERROR(...) cr_log_slice(slice, CR_LOG_ERROR, __VA_ARGS__)
#define LOGS_TRACE(...) cr_log_slice(slice, CR_LOG_TRACE, __VA_ARGS__)

#define __LOG_API_BASIC(log_func, format, ...)               \
    do {                                                     \
        if (cr_logger_get_feature(CR_LOG_FEAT_API)) {        \
            log_func("[%s] " format, __func__, __VA_ARGS__); \
        }                                                    \
    } while (0)
#define __LOG_API_0(log_func, format, ...) __LOG_API_BASIC(log_func, format, __VA_ARGS__)
#define __LOG_API_1(log_func, string)      __LOG_API_0(log_func, "%s", string "")
#define __LOG_API_n(N, log_func, ...)      __LOG_API_##N(log_func, __VA_ARGS__)
#define __EXPAND_LOG_API(log_func, ...)    EXPAND__(__LOG_API_n, IS_SINGLE_ARGS(__VA_ARGS__), log_func, __VA_ARGS__)
#define LOGS_API(...)                      __EXPAND_LOG_API(LOGS_DEBUG, __VA_ARGS__)
#define LOGSDK_API(...)                    __EXPAND_LOG_API(LOG_SDK, __VA_ARGS__)

// propogate error up the stack
#define ERR_PROP(func)                \
    do {                              \
        CredoError_t retval = (func); \
        if (retval != CR_OK) {        \
            return retval;            \
        }                             \
    } while (0)

#define ERR_PROP_SLICE(slice, func)                                                                    \
    do {                                                                                               \
        CredoError_t retval = (func);                                                                  \
        if (retval != CR_OK) {                                                                         \
            cr_log_slice(slice, CR_LOG_TRACE, "Error occured func: %s, line: %d", __func__, __LINE__); \
            return retval;                                                                             \
        }                                                                                              \
    } while (0)

// shorthand for ERR_PROP_SLICE that assumes "slice" variable exists
#define ERR_PROPS(func) ERR_PROP_SLICE(slice, func)

#define ERR_CATCH_SLICE(slice, func, catch)                                                            \
    do {                                                                                               \
        CredoError_t retval = (func);                                                                  \
        if (retval != CR_OK) {                                                                         \
            cr_log_slice(slice, CR_LOG_TRACE, "Error occured func: %s, line: %d", __func__, __LINE__); \
            catch;                                                                                     \
        }                                                                                              \
    } while (0)

// shorthand for ERR_CATCH_SLICE where "slice" variable exists
#define ERR_CATCH(func, catch) ERR_CATCH_SLICE(slice, func, catch)

// propogate error up the stack and perform logging function
#define ERR_PROP_LOG(func, log_func)  \
    do {                              \
        CredoError_t retval = (func); \
        if (retval != CR_OK) {        \
            (log_func);               \
            return retval;            \
        }                             \
    } while (0)

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#ifdef __cplusplus
}
#endif

#endif
