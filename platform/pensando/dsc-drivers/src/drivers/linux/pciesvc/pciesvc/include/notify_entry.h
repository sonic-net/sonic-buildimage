/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#ifndef __NOTIFY_ENTRY_H__
#define __NOTIFY_ENTRY_H__

#include "tlpauxinfo.h"

#define NOTIFY_TLPSZ            48

typedef struct notify_entry_s {
    uint8_t rtlp[NOTIFY_TLPSZ];
    tlpauxinfo_t info;
} notify_entry_t;

#endif /* __NOTIFY_ENTRY_H__ */
