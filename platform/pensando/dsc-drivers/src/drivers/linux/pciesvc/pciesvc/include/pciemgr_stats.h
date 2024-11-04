/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __PCIEMGR_STATS_H__
#define __PCIEMGR_STATS_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef union pciemgr_stats {
    struct {

#define PCIEMGR_STATS_DEF(S) \
        uint64_t S;
#include "pciemgr_stats_defs.h"

    };
    /* pad to 64 entries, room to grow */
    uint64_t _pad[64];

} pciemgr_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* __PCIEMGR_STATS_H__ */
