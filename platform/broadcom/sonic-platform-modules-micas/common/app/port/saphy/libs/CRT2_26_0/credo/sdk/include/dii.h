#ifndef CREDO_DII_H
#define CREDO_DII_H

#include "lock.h"

#include "sdk.h"

#define CALL_HAL(slice, func)                \
    do {                                     \
        CredoError_t err = CR_OK;            \
        if ((slice) == NULL) return CR_FAIL; \
        SLICE_LOCK_GUARD(slice);             \
        err = func;                          \
        SLICE_UNLOCK(slice);                 \
        return err;                          \
    } while (0);

#define CHECK_LANE_VALID(slice, lane)                                                                     \
    do {                                                                                                  \
        if ((slice) == NULL) return CR_INVALID_ARGS;                                                      \
        ERR_PROP_LOG(((((lane) < 0) || ((slice)->desc->lane_count <= (lane))) ? CR_INVALID_ARGS : CR_OK), \
                     LOGS_ERROR("Lane %d is invalid", lane));                                             \
    } while (0)

#define CHECK_PORT_VALID(slice, port_id)                                                                         \
    do {                                                                                                         \
        if ((slice) == NULL) return CR_INVALID_ARGS;                                                             \
        ERR_PROP_LOG(                                                                                            \
            (((port_id) >= (uint32_t)(slice)->desc->port_count) ? CR_INVALID_ARGS : CR_OK),                      \
            LOGS_ERROR("Incorrect port_id %u (valid port_id is 0-%u)", port_id, (slice)->desc->port_count - 1)); \
    } while (0);

#endif
