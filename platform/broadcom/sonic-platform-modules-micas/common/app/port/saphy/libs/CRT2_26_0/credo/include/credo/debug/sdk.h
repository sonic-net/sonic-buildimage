#ifndef CR_DBG_SDK
#define CR_DBG_SDK

#include "credo/base.h"
#include "credo/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

// Logger

/**
 * @brief Set the max log level
 * @deprecated Use cr_logger_set_level v2.2.0
 * @param[in] sdk sdk handle
 * @param[in] max_level logging threshold to set
 */
CREDOAPI void cr_sdk_set_loglevel(CredoSdk_t* sdk, CredoLogLevel_t max_level);

/**
 * @brief Get the max log level object
 * @deprecated Use cr_logger_set_level v2.2.0
 * @param[in] sdk sdk handle
 * @return current logging threshold
 */
CREDOAPI CredoLogLevel_t cr_sdk_get_loglevel(CredoSdk_t* sdk);

#ifdef __cplusplus
}
#endif

#endif
