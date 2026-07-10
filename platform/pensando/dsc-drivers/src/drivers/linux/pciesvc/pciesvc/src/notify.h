/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#ifndef __NOTIFY_H__
#define __NOTIFY_H__

#include "notify_entry.h"

int pciehw_notify_poll_init(const int port);
int pciehw_notify_poll(const int port);
int pciehw_notify_intr_init(const int port,
                            u_int64_t msgaddr, u_int32_t msgdata);
int pciehw_notify_intr(const int port);

#endif /* __NOTIFY_H__ */
