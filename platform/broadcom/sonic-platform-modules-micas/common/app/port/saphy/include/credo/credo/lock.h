#ifndef CREDO_LOCK_H
#define CREDO_LOCK_H

#include "credo/base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set slice mutex locking timeout time
 * By default it is 0, which means there is no timeout.
 * @param[in] timeout_usec
 */
CREDOAPI void cr_lock_set_timeout(unsigned timeout_usec);

/**
 * @brief Get slice mutex locking timeout time
 * @return timeout duration
 */
CREDOAPI unsigned cr_lock_get_timeout(void);

#ifdef __cplusplus
}
#endif

#endif
