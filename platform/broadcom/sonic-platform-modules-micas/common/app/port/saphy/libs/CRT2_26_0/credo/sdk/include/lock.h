#ifndef LOCK_H
#define LOCK_H

#include "sdk_utility.h"

#include <errno.h>
#include <pthread.h>

// utility to perform slice lock
#define SLICE_LOCK_GUARD(slice)                                                                          \
    do {                                                                                                 \
        unsigned lock_timeout_usec = cr_lock_get_timeout();                                              \
        if (cr_likely(lock_timeout_usec == 0)) {                                                         \
            int lock_status = pthread_mutex_lock(&(slice)->lock);                                        \
            if (lock_status != 0) {                                                                      \
                LOG_ERROR(slice, "[%s]unexpected mutex lock error code: %d", __func__, lock_status);     \
                return CR_FAIL;                                                                          \
            }                                                                                            \
        } else {                                                                                         \
            CredoTime_t start_time;                                                                      \
            bool locked = false;                                                                         \
            get_time(&start_time);                                                                       \
            do {                                                                                         \
                int lock_status = pthread_mutex_trylock(&(slice)->lock);                                 \
                if (lock_status == 0) {                                                                  \
                    locked = true;                                                                       \
                    break;                                                                               \
                } else if (lock_status != EBUSY) {                                                       \
                    LOG_ERROR(slice, "[%s]unexpected mutex lock error code: %d", __func__, lock_status); \
                    return CR_FAIL;                                                                      \
                }                                                                                        \
                sleep_ms(5);                                                                             \
            } while (!is_timeout(&start_time, lock_timeout_usec));                                       \
            if (!locked) {                                                                               \
                return CR_MUTEX_TIMEOUT;                                                                 \
            }                                                                                            \
        }                                                                                                \
    } while (0)

#define SLICE_UNLOCK(slice)                                                                         \
    do {                                                                                            \
        int lock_status = pthread_mutex_unlock(&(slice)->lock);                                     \
        if (lock_status != 0) {                                                                     \
            LOG_ERROR(slice, "[%s] unexpected mutex unlock error code: %d", __func__, lock_status); \
        }                                                                                           \
    } while (0)

#endif
