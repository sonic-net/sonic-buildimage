/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#ifndef __INDIRECT_ENTRY_H__
#define __INDIRECT_ENTRY_H__

#include "tlpauxinfo.h"

typedef enum {
#define PCIEIND_REASON_DEF(NAME, VAL)               \
    PCIEIND_REASON_##NAME = VAL,
#include "indirect_reason.h"
    PCIEIND_REASON_MAX
} pcieind_reason_t;

/*
 * Completion Status field values
 * PCIe 4.0, Table 2-34.
 */
typedef enum {
    PCIECPL_SC          = 0x0,          /* Successful Completion */
    PCIECPL_UR          = 0x1,          /* Unsupported Request */
    PCIECPL_CRS         = 0x2,          /* Config Retry Status */
    PCIECPL_CA          = 0x4,          /* Completer Abort */
} pciecpl_t;

#define INDIRECT_TLPSZ          64

typedef struct indirect_entry_s {
    u_int32_t port;
    pciecpl_t cpl;                      /* PCIECPL_* completion type */
    u_int32_t completed:1;              /* completion has been delivered */
    u_int32_t data[4];
    u_int8_t rtlp[INDIRECT_TLPSZ];
    tlpauxinfo_t info;
} indirect_entry_t;

#endif /* __INDIRECT_ENTRY_H__ */
