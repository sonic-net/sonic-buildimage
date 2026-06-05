#ifndef CREDO_LOGGER_H
#define CREDO_LOGGER_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
#ifndef CR_CPPAPI
/**
 * @brief Log levels of the sdk.
 *
 */
typedef enum {
    CR_LOG_ERROR = 0,  // error logging
    CR_LOG_WARN = 1,
    CR_LOG_INFO = 2,
    CR_LOG_DEBUG = 3,
    CR_LOG_TRACE = 4,
} CredoLogLevel_t;
#else
typedef enum {
    CREDO_LOG_ERROR = 0,  // error logging
    CREDO_LOG_WARN = 1,
    CREDO_LOG_INFO = 2,
    CREDO_LOG_DEBUG = 3,
    CREDO_LOG_TRACE = 4
} CredoLogLevel_t;
#endif

// Logging API
/**
 * @brief Logger function signature
 * @param[in] slice_context optional slice_context if it is a log pertaining to a slice
 * @param[in] user_data optionally provided user_data set for a slice
 * @param[in] scope the scope of the message (slice id or global)
 * @param[in] level the log level of the message
 * @param[in] message the log message
 */
typedef void (*CredoLog_t)(void* slice_context, void* user_data, CredoLogLevel_t level, const char* scope,
                           const char* message);

/**
 * @brief Set the logger
 * @param logger
 */
CREDOAPI void cr_logger_set(CredoLog_t logger);

/**
 * @brief Set the log level
 * @param loglevel
 */
CREDOAPI void cr_logger_set_level(CredoLogLevel_t loglevel);

/**
 * @brief Get the log level
 * @return loglevel
 **/
CREDOAPI CredoLogLevel_t cr_logger_get_level(void);

typedef enum {
    CR_LOG_FEAT_API = 1,  //!< Use 0 for off, anything else will turn on
    CR_LOG_FEAT_REG = 2,
    CR_LOG_FEAT_TCM = 3
} CredoLogFeature_t;

typedef enum {
    CR_LOG_IO_OFF = 0,
    CR_LOG_IO_READ = 1,
    CR_LOG_IO_WRITE = 2,
    CR_LOG_IO_ON = 3,
} CredoLogIO_t;

/**
 * @brief Set logging feature
 *
 * @param[in] feature what feature to configure
 * @param[in] value  what to configure the feature
 */
CREDOAPI void cr_logger_set_feature(CredoLogFeature_t feature, CredoLogIO_t value);

/**
 * @brief Get feature configuration
 *
 * @param[in] feature what feature to get setting
 * @return CREDOAPI
 */
CREDOAPI CredoLogIO_t cr_logger_get_feature(CredoLogFeature_t feature);

#ifdef __cplusplus
}
#endif

#endif
