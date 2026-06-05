#ifndef CRI_LOGGER_H
#define CRI_LOGGER_H

#include "credo.h"

#ifdef __cplusplus
extern "C" {
#endif

CREDOAPI void cri_logger_set_shell_logger(CredoLog_t shell_log_cb);

#ifdef __cplusplus
}
#endif

#endif  // CRI_LOGGER_H
